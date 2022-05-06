// ------------------------------------------------------------------------------------------------
//  The MIT License
//  
//  Copyright (c) 2021 Tim Andersen
//  
//  Permission is hereby granted, free of charge, to any person obtaining a copy of this software
//  and associated documentation files (the "Software"), to deal in the Software without
//  restriction, including without limitation the rights to use, copy, modify, merge, publish,
//  distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
//  Software is furnished to do so, subject to the following conditions:
//  
//  The above copyright notice and this permission notice shall be included in all copies or
//  substantial portions of the Software.
//  
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
//  BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
//  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
//  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// ------------------------------------------------------------------------------------------------

// ====================================================================================================================
// TinRegistration.cpp
// ====================================================================================================================

// -- class include
#include "TinVariableEntry.h"

// -- lib includes
#include "stdio.h"

#include "TinScript.h"
#include "TinCompile.h"
#include "TinRegistration.h"
#include "TinHashtable.h"
#include "TinOpExecFunctions.h"

// == namespace TinScript =============================================================================================

namespace TinScript
{

// == class CVariableEntry ============================================================================================

// ====================================================================================================================
// Constructor:  Used for coded variables, where the address refers to the source of the registered variable.
// ====================================================================================================================
CVariableEntry::CVariableEntry(CScriptContext* script_context, const char* _name, eVarType _type,
                               int32 _array_size, void* _addr)
{
    mContextOwner = script_context;
	SafeStrcpy(mName, sizeof(mName), _name, kMaxNameLength);
	mType = _type;
    mArraySize = _array_size;
	mHash = Hash(_name);
    mOffset = 0;
    mAddr = _addr;
    mIsDynamic = false;
    mScriptVar = false;
    mStringValueHash = 0;
    mStringHashArray = NULL;
    mStackOffset = -1;
    mIsParameter = false;
    mDispatchConvertFromObject = 0;
    mFuncEntry = NULL;
	mBreakOnWrite = NULL;
    mWatchRequestID = 0;
    mDebuggerSession = 0;

    // -- validate the array size
    if (mArraySize == 0)
        mArraySize = 1;

    // -- a special case for arrays of strings - they have to have a matching array of hashes
    if (mArraySize > 1 && mType == TYPE_string)
    {
        mStringHashArray = (uint32*)TinAllocArray(ALLOC_VarStorage, char,
                                                    sizeof(const char*) * mArraySize);
    	memset(mStringHashArray, 0, sizeof(const char*) * mArraySize);
    }
}

// ====================================================================================================================
// Constructor:  Used for dynamic, script, parameter and other variable entries.  Also for object members.
// ====================================================================================================================
CVariableEntry::CVariableEntry(CScriptContext* script_context, const char* _name, uint32 _hash, eVarType _type,
                               int32 _array_size, bool8 isoffset, uint32 _offset, bool8 _isdynamic, bool8 is_param)
{
    mContextOwner = script_context;
	SafeStrcpy(mName, sizeof(mName), _name, kMaxNameLength);
	mType = _type;
    mArraySize = _array_size;
    mAddr = nullptr;
	mHash = _hash;
    mOffset = 0;
    mIsDynamic = _isdynamic;
    mScriptVar = false;
    mStringValueHash = 0;
    mStringHashArray = NULL;
    mStackOffset = -1;
    mIsParameter = is_param;
    mDispatchConvertFromObject = 0;
    mFuncEntry = NULL;
	mBreakOnWrite = NULL;
    mWatchRequestID = 0;
    mDebuggerSession = 0;

    // -- hashtables are tables of variable entries...
    // -- they can only be created from script
    if (mType == TYPE_hashtable)
    {
        mScriptVar = true;
		
		// -- no supporting arrays of hashtables
		mArraySize = 1;

        // -- in the context of hash tables, parameters are *passed* the hash table,
        // -- and do not actually own it
        if (!mIsParameter)
        {
            // -- setting allocation type as a VarTable, although this may be an exception:
            // -- since it's actually a script variable allocation...  it's size is not
            // -- consistent with the normal size of variable storage
            mAddr = (void*)TinAlloc(ALLOC_VarTable, tVarTable, kLocalVarTableSize);
        }
        else
        {
            mAddr = NULL;
        }
    }

    else if (isoffset)
    {
        mAddr = NULL;
        mOffset = _offset;
    }

    // -- not an offset (e.g not a class member)
    // -- registered variables are constructed above, so this is a script var, requiring us to allocate
    else
    {
		mScriptVar = true;
		
		// -- a negative array size means this had better be a parameter,
        // -- which has no size of its own, but refers to the array passed
		if (mArraySize < 0)
		{
            ScriptAssert_(GetScriptContext(), mIsParameter, "<internal>", -1,
                          "Error - creating an array reference\non a non-parameter (%s)\n", UnHash(GetHash()));
			mAddr = NULL;
		}
		else
		{
			if (mArraySize == 0)
				mArraySize = 1;
		
            // -- if we know the size of the array already, we can allocate it now
            if (mArraySize > 0)
            {
			    mAddr = (void*)TinAllocArray(ALLOC_VarStorage, char, gRegisteredTypeSize[_type] * mArraySize);
                if (mAddr != nullptr)
                {
			        memset(mAddr, 0, gRegisteredTypeSize[_type] * mArraySize);
                }
            }

            // -- otherwise the size is being determined dynamically - we'll allocate when we execute an OP_ArrayDecl
            else
            {
                mAddr = NULL;
            }
		}
    }

    // -- a special case for registered arrays of strings - they have to have a matching array of hashes
    if (mArraySize > 1 && mType == TYPE_string)
    {
        mStringHashArray = (uint32*)TinAllocArray(ALLOC_VarStorage, char, sizeof(const char*) * mArraySize);
    	memset(mStringHashArray, 0, sizeof(const char*) * mArraySize);
    }
}

// ====================================================================================================================
// Destructor
// ====================================================================================================================
CVariableEntry::~CVariableEntry()
{
    // -- if we have a debugger watch, delete it
    if (mBreakOnWrite)
        TinFree(mBreakOnWrite);

    // -- if the value is a string, update the string table
    if (mType == TYPE_string)
    {
        // -- keep the string table up to date
        GetScriptContext()->GetStringTable()->RefCountDecrement(mStringValueHash);
    }

    // -- we want to ensure any CHashtable instances that are "wrapping" this variable become "unwrapped"
    else if (mType == TYPE_hashtable)
    {
        CHashtable::NotifyHashtableDestroyed(this);
    }

    TryFreeAddrMem();
}

// ====================================================================================================================
// CVariableEntry
// ====================================================================================================================
void CVariableEntry::TryFreeAddrMem()
{
    // $$$TZA clean this up - we have too many variables used to figure out if the mAddr owns the memory or not
    if (mAddr == nullptr || mIsReference)
        return;

    if (mScriptVar)
    {
        // -- if this isn't a hashtable, and it isn't a parameter array
        // $$$TZA Array - *this* is why we require array parameters to be marked as a parameter!
        if (mType != TYPE_hashtable && (!mIsParameter || !IsArray()))
        {
            TinFreeArray((char*)mAddr);
            mAddr = nullptr;
        }

        // -- if this is a non-parameter hashtable, need to destroy all of its entries
        // note:  schedule() contexts that use hashtables, this is the one time where
        // a hashtable is copied, therefore, the parameter uses dynamic memory and must be freed
        else if (mType == TYPE_hashtable && (!mIsParameter || mIsDynamic))
        {
            tVarTable* ht = static_cast<tVarTable*>(mAddr);
            ht->DestroyAll();

            // -- now delete the hashtable itself
            TinFree(ht);

            // -- null the pointer
            mAddr = nullptr;
        }
    }

    // -- delete the hash array, if this happened to have been a registered const char*[]
    if (mStringHashArray)
    {
        TinFreeArray(mStringHashArray);
        mStringHashArray = nullptr;
    }
}

// ====================================================================================================================
// ConvertToArray():  Variables that are arrays, are allocated once the array size is actually known.
// ====================================================================================================================
bool8 CVariableEntry::ConvertToArray(int32 array_size)
{
    if (!mScriptVar || mIsParameter || mOffset != 0)
    {
        ScriptAssert_(GetScriptContext(), false, "<internal>", -1,
                        "Error - calling ConvertToArray() on an\ninvalid variable (%s)\n", UnHash(GetHash()));
        return (false);
    }

    // -- validate the array size
    if (array_size < 1 || array_size > kMaxVariableArraySize)
    {
        ScriptAssert_(GetScriptContext(), false, "<internal>", -1,
                      "Error - calling ConvertToArray() with an\ninvalid size %d, variable (%s)\n", array_size,
                      UnHash(GetHash()));
        return (false);
    }

    // -- see if the conversion has already happened - if we already have an allocated 
    if (mAddr)
    {
        if (mArraySize != array_size)
        {
            ScriptAssert_(GetScriptContext(), false, "<internal>", -1,
                          "Error - calling ConvertToArray() on a variable\nthat has already been allocated (%s)\n",
                          UnHash(GetHash()));
            return (false);
        }
    }

    // -- this only works with *certain* types of variables
    else
    {
        // -- set the size and allocate
        mArraySize = array_size;
    	mAddr = (void*)TinAllocArray(ALLOC_VarStorage, char, gRegisteredTypeSize[mType] * mArraySize);
		memset(mAddr, 0, gRegisteredTypeSize[mType] * mArraySize);
    }

    // -- success
    return (true);
}

// ====================================================================================================================
// ClearArrayParameter():  Array parameters are like references, clear the details upon function return.
// ====================================================================================================================
void CVariableEntry::ClearArrayParameter()
{
    // -- ensure we have an array parameter
    ScriptAssert_(GetScriptContext(), mIsParameter, "<internal>", -1,
                    "Error - calling ClearArrayParameter() on an invalid variable (%s)\n",
                    UnHash(GetHash()));

    // -- we're going to rely on InitializeArrayParameter to set these correctly...
    // -- if we call a function recursively, using an array param, we need the previous iteration's
    // param var, as it was initialized, to be used in this iteration...  arrays are by reference anyways,
    // so the mAddr will be the same value, regardless of which call (at any stack depth) we're using
    /*
    mArraySize = -1;
    mAddr = NULL;
    mStringHashArray = NULL;
    */
}

// ====================================================================================================================
// InitializeArrayParameter():  Array parameters are like references - initialize the details upon function call.
// ====================================================================================================================
void CVariableEntry::InitializeArrayParameter(CVariableEntry* assign_from_ve, CObjectEntry* assign_from_oe,
                                              const CExecStack& execstack, const CFunctionCallStack& funccallstack)
{
    // -- ensure we have an array parameter, and a valid source to initialize from
    // note:  watch expressions don't really use parameters, but in this case, arrays need to be
    // assigned as references (and parameters)
    if (!assign_from_ve)
    {
        ScriptAssert_(GetScriptContext(), 0, "<internal>", -1,
                      "Error - calling InitializeArrayParameter() on an invalid variable (%s)\n",
                      UnHash(GetHash()));
        return;
    }

    // $$$TZA SendArray
    // note:  if we're calling a function recursively, assign_from_ve could easily be "this"...
    // however, if assign_from_oe is different...  something to figure out and test??
    // for now, it should be harmless to this to this

    // -- we basically duplicate the internals of the variable entry, allowing the parameter to act as a reference
    // not dynamic, and arrays are always parameters - we never destroy the memory from a (reference) array
    mIsParameter = true;
	mOffset = assign_from_ve->mOffset;
    mIsDynamic = false;
    mScriptVar = assign_from_ve->mScriptVar;
    mArraySize = assign_from_ve->mArraySize;
    mStringHashArray = assign_from_ve->mStringHashArray;

    // -- the address is the usual complication, based on object member, dynamic var, global, registered, ...
    void* valueaddr = NULL;
    if (assign_from_ve->IsStackVariable(funccallstack, false))
    {
        valueaddr = GetStackVarAddr(GetScriptContext(), execstack, funccallstack, assign_from_ve->GetStackOffset());
        if (mStringHashArray != NULL)
        {
            mStringHashArray = (uint32*)valueaddr;
        }
    }
    else if (assign_from_oe && !assign_from_ve->mIsDynamic)
        valueaddr = (void*)((char*)assign_from_oe->GetAddr() + assign_from_ve->mOffset);
    else
        valueaddr = assign_from_ve->mAddr;
    mAddr = valueaddr;
}

// ====================================================================================================================
// GetStringArrayHashAddr():  Get the physical address for the string entry of an array
// -- this method is used only for registered arrays of const char*
// -- the address is actually a const char*[], and there's a parallel
// -- array of hash values, to keep the string table up to date
// ====================================================================================================================
void* CVariableEntry::GetStringArrayHashAddr(void* objaddr,int32 array_index) const
{
    // -- sanity check
    if(GetType() != TYPE_string || array_index < 0 || (array_index > 0 && !IsArray()) ||
        (mArraySize >= 0 && array_index > mArraySize))
    {
        ScriptAssert_(GetScriptContext(),false,"<internal>",-1,
                      "Error - GetStringAddr() called with an invalid array index: %s\n",UnHash(GetHash()));
        return (NULL);
    }

    // -- if this is an array with a size > 1, then mStringValueHash is actually an array of hashes
    if(mArraySize > 1)
        return (void*)(&mStringHashArray[array_index]);
    else
        return (void*)(&mStringValueHash);
}

// ====================================================================================================================
// GetValueAddr():  Get the physical address for where the variables stores its value
// -- this method is called by registered methods (dispatch templated implementation),
// -- and as it is used to cross into cpp, it returns a const char* for strings
// ====================================================================================================================
void* CVariableEntry::GetValueAddr(void* objaddr) const
{
    // -- strings are special
    if(mType == TYPE_string)
    {
        return (void*)mContextOwner->GetStringTable()->FindString(mStringValueHash);
    }

    // -- if we're providing an object address, this var is a member
    // -- if it's a dynamic var, it belongs to the object,
    // -- but lives in a local dynamic hashtable
    void* valueaddr = NULL;
    if(objaddr && !mIsDynamic)
        valueaddr = (void*)((char*)objaddr + mOffset);
    else
        valueaddr = mAddr;

    // -- return the value address
    return valueaddr;
}

// ====================================================================================================================
// GetAddr():  get the physical address for where a variable stores its value
// -- this method is used only on the script side.  The address it returns must *never*
// -- be written to - instead, the SetValue() method for variables is used.
// -- for strings, this returns the address of the hash value found in the string dictionary
// ====================================================================================================================
void* CVariableEntry::GetAddr(void* objaddr) const
{
    // -- strings are special...
    if(mType == TYPE_string)
    {
        return (GetStringArrayHashAddr(objaddr,0));
    }

    // -- find the value address
    void* valueaddr = NULL;

    // -- if we're providing an object address, this var is a member
    if(objaddr && !mIsDynamic)
        valueaddr = (void*)((char*)objaddr + mOffset);
    else
        valueaddr = mAddr;

    // -- return the address
    return (valueaddr);
}

// ====================================================================================================================
// GetArrayVarAddr():  Get the physical address for an array entry
// ====================================================================================================================
void* CVariableEntry::GetArrayVarAddr(void* objaddr,int32 array_index) const
{
    // -- strings are special
    if (mType == TYPE_string)
    {
        return  (GetStringArrayHashAddr(objaddr,array_index));
    }

    // -- can only call this if this actually is an array
    if(!IsArray())
    {
        ScriptAssert_(GetScriptContext(),false,"<internal>",-1,
                      "Error - GetArrayVarAddr() called on a non-array variable: %s\n",UnHash(GetHash()));
        return (NULL);
    }

    // -- if the array hasn't yet been allocated (e.g. during declaration), return NULL
    if(mArraySize < 0)
    {
        // -- this had better be a parameter, otherwise we're trying to access an uninitialized array
        ScriptAssert_(GetScriptContext(),mIsParameter,"<internal>",-1,
                      "Error - GetArrayVarAddr() called on an uninitialized array variable: %s\n",
                      UnHash(GetHash()));
        return (NULL);
    }

    // -- ensure we're within range
    if(array_index < 0 || array_index >= mArraySize)
    {
        ScriptAssert_(GetScriptContext(),false,"<internal>",-1,
                      "Error - GetArrayVarAddr() index %d out of range [%d],\nvariable: %s\n",array_index,
                      mArraySize,UnHash(GetHash()));
        return (NULL);
    }

    // -- get the base address for this variable
    void* addr = (void*)((char*)GetAddr(objaddr) + (gRegisteredTypeSize[mType] * array_index));
    return (addr);
}

// ====================================================================================================================
// GetHashtableAddr():  Get the address for a hashtable tVarTable...  allocate if needed - we're probably going
// to copy another hashtable to this one (e.g.  when calling schedule(), where one of the params is a hashtable
// note:  for now, not supporting this, if the hashtable is a member of an object
// ====================================================================================================================
void* CVariableEntry::GetOrAllocHashtableAddr()
{
    if (mAddr == nullptr)
    {
        // note: we rely on the scheduler, when it's executed its call, to know if it should
        // free this memory...  if the schedule is re-queued, then it won't...
        mAddr = (void*)TinAlloc(ALLOC_VarTable, tVarTable, kLocalVarTableSize);
        mIsDynamic = true;
    }

    return mAddr;
}

// ====================================================================================================================
// IsStackVariable():  Returns true if this variable is to use space on the stack as it's function is executing.
// ====================================================================================================================
bool8 CVariableEntry::IsStackVariable(const CFunctionCallStack& funccallstack, bool allow_indexed_var) const
{
    int32 stackoffset = 0;
    CObjectEntry* oe = NULL;
    uint32 oe_id = 0;
    CFunctionEntry* fe_executing = funccallstack.GetExecuting(oe_id, oe, stackoffset);
    CFunctionEntry* fe_top = funccallstack.GetTop(oe, stackoffset);

    bool belongs_to_executing_function = (fe_executing && fe_executing == GetFunctionEntry());
    bool is_top_function = fe_top && fe_top == GetFunctionEntry();
    bool var_allowed = allow_indexed_var || (GetType() != TYPE_hashtable && !IsArray());

    return (belongs_to_executing_function || (IsParameter() && is_top_function)) &&
            var_allowed && mStackOffset >= 0;
}

// ====================================================================================================================
// SetValue():  Sets the value of a variable, and notifies the debugger in support of data breakpoints.
// This method is called from the virtual machine executing a script.
// ====================================================================================================================
void CVariableEntry::SetValue(void* objaddr, void* value, CExecStack* execstack, CFunctionCallStack* funccallstack,
                              int32 array_index)
{
    // -- strings have their own implementation, as they have to manage both the hash value of the string
    // -- (essentially the script value), and the const char*, the actual string, only used by code
    if (mType == TYPE_string)
    {
        SetStringArrayHashValue(objaddr, value, execstack, funccallstack, array_index);
        return;
    }

	int32 size = gRegisteredTypeSize[mType];

    // -- if we're providing an objaddr, this variable is actually a member
    void* varaddr = IsArray() ? GetArrayVarAddr(objaddr, array_index) : GetAddr(objaddr);

    // -- if this variable is TYPE_hashtable, then we need to deliberately know we're
    // -- setting the entire HashTable and not just an entry
    if (mType == TYPE_hashtable)
    {
        if (!mIsParameter)
        {
            ScriptAssert_(GetScriptContext(), false, "<internal>", -1,
                            "Error - calling SetValue() on a non-parameter HashTable/Array variable (%s)\n",
                            UnHash(GetHash()));
        }

        // -- otherwise simply assign the new hash table
        else
        {
            mAddr = value;
        }
    }

    // -- otherwise simply copy the new value 
    else
    {
        // -- ensure we're not assigning to an uninitialized parameter array
        if (IsParameter() && IsArray() && GetArraySize() < 0)
        {
            ScriptAssert_(GetScriptContext(), false, "<internal>", -1,
                            "Error - calling SetValue() on an uninitialized array parameter (%s)\n",
                            UnHash(GetHash()));
        }

        // -- copy the new value
	    memcpy(varaddr, value, size);
    }

	// -- if we've been requested to break on write
    NotifyWrite(GetScriptContext(), execstack, funccallstack);
}

// ====================================================================================================================
// SetValue():  Sets the value of a variable.
// This method is called externally, e.g.  from code, as opposed to from the virtual machine.
// ====================================================================================================================
void CVariableEntry::SetValueAddr(void* objaddr, void* value, int32 array_index)
{
    // -- strings have their own implementation, as they have to manage both the hash value of the string
    // -- (essentially the script value), and the const char*, the actual string, only used by code
    if (mType == TYPE_string)
    {
        SetStringArrayLiteralValue(objaddr, value, array_index);
        return;
    }

    assert(value);
	int32 size = gRegisteredTypeSize[mType];
    void* varaddr = IsArray() ? GetArrayVarAddr(objaddr, array_index) : GetAddr(objaddr);

    // -- if this variable is TYPE_hashtable, then we're stomping the entire hash table - only permitted
    // -- if the variable is a parameter
    if (mType == TYPE_hashtable)
    {
        if (!mIsParameter)
        {
            ScriptAssert_(GetScriptContext(), false, "<internal>", -1,
                            "Error - calling SetValue() on a non-parameter HashTable variable (%s)\n",
                            UnHash(GetHash()));
        }

        // -- otherwise simply assign the new hash table
        else
            mAddr = value;
    }
    
    // -- otherwise simply copy the new value 
    else
        memcpy(varaddr, value, size);

	// -- if we've been requested to break on write
    // -- note:  SetValueAddr() is the external access (from code), and is never part 
    // -- of executing the VM... therefore, we have no stack
    NotifyWrite(GetScriptContext(), NULL, NULL);
}

// ====================================================================================================================
// SetReferenceAddr():  Used only on parameters, so type methods can still modify their own values
// ====================================================================================================================
bool CVariableEntry::SetReferenceAddr(void* ref_addr)
{
    // -- we have to have a value, and this can only be performed on parameters!
    if (ref_addr == nullptr || !mIsParameter)
    {
        ScriptAssert_(GetScriptContext(), false, "<internal>", -1,
                      "Error - failed SetReferenceAddr(): %s\n",
                      UnHash(GetHash()));
        return false;
    }

    // -- try to free the existing memory
    TryFreeAddrMem();

    // -- mark this as a reference, and set the addr
    mIsReference = true;
    mAddr = ref_addr;

    return (true);
}

// ====================================================================================================================
// SetStringArrayHashValue():  Sets the value of a TYPE_string variable, from the VM, where the void* is a hash value.
// ====================================================================================================================
 void CVariableEntry::SetStringArrayHashValue(void* objaddr, void* value, CExecStack* execstack,
                                              CFunctionCallStack* funccallstack, int32 array_index)
{
    // -- ensure we have type string, etc...
    if (!value || mType != TYPE_string)
    {
        ScriptAssert_(GetScriptContext(), false, "<internal>", -1,
                      "Error - call to SetStringArrayValue() is invalid for variable %s\n",
                      UnHash(GetHash()));
        return;
    }

    // -- if the value type is a TYPE_string, then the void* value contains a hash value
	int32 size = gRegisteredTypeSize[mType];
    void* hash_addr = GetStringArrayHashAddr(objaddr, array_index);

    // -- decrement the ref count for the current value
    uint32 current_hash_value = *(uint32*)hash_addr;
    GetScriptContext()->GetStringTable()->RefCountDecrement(current_hash_value);

    // -- if this is a script variable, we simply store the hash value at the address
    uint32 string_hash_value = *(uint32*)value;
    *(uint32*)hash_addr = string_hash_value;

    // -- if this is not a script variable, then in addition to setting the hash, the mAddr must be
    // -- set to the actual string
    if (!mScriptVar)
    {
        // -- get the current value of the string (which may have been changed in code
        const char* string_value = GetScriptContext()->GetStringTable()->FindString(string_hash_value);

        void* valueaddr = NULL;
        if (objaddr && !mIsDynamic)
            valueaddr = (void*)((char*)objaddr + mOffset);
        else
            valueaddr = mAddr;

        ((const char**)(valueaddr))[array_index] = string_value;
    } 

    // -- the act of assigning a string value means incrementing the reference in the string dictionary
    GetScriptContext()->GetStringTable()->RefCountIncrement(string_hash_value);

    // -- if we've been requested to break on write
    NotifyWrite(GetScriptContext(), execstack, funccallstack);
}

// ====================================================================================================================
// SetStringArrayLiteralValue():  Sets the value of a TYPE_string variable, called externally.
// The value of the void* is the actual const char* instead of a hash.
// ====================================================================================================================
 void CVariableEntry::SetStringArrayLiteralValue(void* objaddr, void* value, int32 array_index)
{
    // -- ensure we have type string, etc...
    if (!value || mType != TYPE_string)
    {
        ScriptAssert_(GetScriptContext(), false, "<internal>", -1,
                      "Error - call to SetStringArrayValue() is invalid for variable %s\n",
                      UnHash(GetHash()));
        return;
    }

    // -- if the value type is a TYPE_string, then the void* value contains a hash value
	int32 size = gRegisteredTypeSize[mType];
    void* hash_addr = GetStringArrayHashAddr(objaddr, array_index);

    // -- decrement the ref count for the current value
    uint32 current_hash_value = *(uint32*)hash_addr;
    GetScriptContext()->GetStringTable()->RefCountDecrement(current_hash_value);

    // -- hash the new value
    // note:  we're assigning a value, so include ref count increment
    uint32 string_hash_value = Hash((const char*)value, -1, true);
    *(uint32*)hash_addr = string_hash_value;

    // -- if this is not a script variable, then in addition to setting the hash, the mAddr must be
    // -- set to the actual string
    if (!mScriptVar)
    {
        // -- get the current value of the string (which may have been changed in code
        const char* string_value = GetScriptContext()->GetStringTable()->FindString(string_hash_value);

        void* valueaddr = NULL;
        if (objaddr && !mIsDynamic)
            valueaddr = (void*)((char*)objaddr + mOffset);
        else
            valueaddr = mAddr;

        ((const char**)(valueaddr))[array_index] = string_value;
    } 
}
 
 // ====================================================================================================================
// Clone():  used for, e.g., copying an entire hashtable
// ====================================================================================================================
 CVariableEntry* CVariableEntry::Clone() const
 {
     // -- ensure we're not trying to copy an array
    // $$$TZA paramemters?  if the offest != 0?  we only want to copy direct VE values atm
    // $$$TZA support arrays!
     if (IsArray())
     {
         TinPrint(TinScript::GetContext(), "Error - CVariableEntry::Clone(): arrays not yet supported\n");
         return nullptr;
     }

     uint32 ve_hash = GetHash();
     const char* ve_name = UnHash(ve_hash);
     CVariableEntry* copy_ve = TinAlloc(ALLOC_VarEntry, CVariableEntry, TinScript::GetContext(),
                                        ve_name, ve_hash, GetType(), 1, false, 0, true);

     // -- perform the assignment
     copy_ve->SetValueAddr(false, GetValueAddr(nullptr));

     // -- return the dup
     return copy_ve;
 }

} // namespace TinScript

// -- eof ------------------------------------------------------------------------------------------------------------
