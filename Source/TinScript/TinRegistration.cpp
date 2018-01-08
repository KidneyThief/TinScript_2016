// ------------------------------------------------------------------------------------------------
//  The MIT License
//  
//  Copyright (c) 2013 Tim Andersen
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

// -- lib includes
#include "stdio.h"

#include "TinScript.h"
#include "TinCompile.h"
#include "TinRegistration.h"
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
	SafeStrcpy(mName, _name, kMaxNameLength);
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
	SafeStrcpy(mName, _name, kMaxNameLength);
	mType = _type;
    mArraySize = _array_size;
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
		
		// -- a negative array size means this had better be a paramter,
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
			    memset(mAddr, 0, gRegisteredTypeSize[_type] * mArraySize);

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

	if (mScriptVar)
    {
        // -- if this isn't a hashtable, and it isn't a parameter array
        if (mType != TYPE_hashtable && (!mIsParameter || !IsArray()))
        {
		    TinFreeArray((char*)mAddr);
        }

        // -- if this is a non-paramter hashtable, need to destroy all of its entries
        else if (mType == TYPE_hashtable && !mIsParameter)
        {
            tVarTable* ht = static_cast<tVarTable*>(mAddr);
            ht->DestroyAll();

            // -- now delete the hashtable itself
            TinFree(ht);
        }
	}

    // -- delete the hash array, if this happened to have been a registered const char*[]
    if (mStringHashArray)
    {
        TinFreeArray(mStringHashArray);
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
    mArraySize = -1;
    mAddr = NULL;
    mStringHashArray = NULL;
}

// ====================================================================================================================
// InitializeArrayParameter():  Array parameters are like references - initialize the details upon function call.
// ====================================================================================================================
void CVariableEntry::InitializeArrayParameter(CVariableEntry* assign_from_ve, CObjectEntry* assign_from_oe,
                                              CExecStack& execstack, CFunctionCallStack& funccallstack)
{
    // -- ensure we have an array parameter
    if (!mIsParameter || mArraySize != -1 || !assign_from_ve)
    {
        ScriptAssert_(GetScriptContext(), 0, "<internal>", -1,
                      "Error - calling InitializeArrayParameter() on an invalid variable (%s)\n",
                      UnHash(GetHash()));
        return;
    }

    // -- we basically duplicate the internals of the variable entry, allowing the parameter to act as a reference
	mOffset = assign_from_ve->mOffset;
    mIsDynamic = assign_from_ve->mIsDynamic;
    mScriptVar = assign_from_ve->mScriptVar;
    mArraySize = assign_from_ve->mArraySize;
    mStringHashArray = assign_from_ve->mStringHashArray;

    // -- the address is the usual complication, based on object member, dynamic var, global, registered, ...
    void* valueaddr = NULL;
    if (assign_from_ve->IsStackVariable(funccallstack))
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
// IsStackVariable():  Returns true if this variable is to use space on the stack as it's function is executing.
// ====================================================================================================================
bool8 CVariableEntry::IsStackVariable(CFunctionCallStack& funccallstack) const
{
    int32 stackoffset = 0;
    CObjectEntry* oe = NULL;
    CFunctionEntry* fe_executing = funccallstack.GetExecuting(oe, stackoffset);
    CFunctionEntry* fe_top = funccallstack.GetTop(oe, stackoffset);
    return (((fe_executing && fe_executing == GetFunctionEntry()) || (IsParameter() && fe_top &&
                                                                      fe_top == GetFunctionEntry())) &&
            GetType() != TYPE_hashtable && mStackOffset >= 0 && (!IsArray() || !IsParameter()));
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
        // -- ensure we're not assigning to an uninitialized paramter array
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

// == class CFunctionContext ==========================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CFunctionContext::CFunctionContext(CScriptContext* script_context)
{
    mContextOwner = script_context;
    localvartable = TinAlloc(ALLOC_VarTable, tVarTable, eMaxLocalVarCount);
    paramcount = 0;
    for (int32 i = 0; i < eMaxParameterCount; ++i)
        parameterlist[i] = NULL;
}

// ====================================================================================================================
// Destructor
// ====================================================================================================================
CFunctionContext::~CFunctionContext()
{
    // -- delete all the variable entries
    localvartable->DestroyAll();

    // -- delete the actual table
    TinFree(localvartable);
}

// ====================================================================================================================
// AddParameter():  Parameter declaration for a function definition, for a specific index.
// ====================================================================================================================
bool8 CFunctionContext::AddParameter(const char* varname, uint32 varhash, eVarType type, int32 array_size,
                                     int32 paramindex, uint32 actual_type_id)
{
    // add the entry to the parameter list as well
    assert(paramindex >= 0 && paramindex < eMaxParameterCount);
    if (paramindex >= eMaxParameterCount)
    {
        printf("Error - Max parameter count %d exceeded, parameter: %s\n",
                eMaxParameterCount, varname);
        return (false);
    }

    if (parameterlist[paramindex] != NULL)
    {
        printf("Error - parameter %d has already been added\n", paramindex);
        return (false);
    }

    // -- create the Variable entry
    CVariableEntry* ve = AddLocalVar(varname, varhash, type, array_size, true);
    if (!ve)
        return (false);

    // -- parameters that are registered as TYPE_object, but are actually
    // -- pointers to registered classes, can be automatically
    // -- converted
    if (type == TYPE_object && actual_type_id != 0 && actual_type_id != GetTypeID<uint32>())
    {
        ve->SetDispatchConvertFromObject(actual_type_id);
    }

    // -- bump the count if we need to
    if (paramindex >= paramcount)
        paramcount = paramindex + 1;
    parameterlist[paramindex] = ve;

    return (true);
}

// ====================================================================================================================
// AddParameter():  Parameter declaration for a function definition.
// ====================================================================================================================
bool8 CFunctionContext::AddParameter(const char* varname, uint32 varhash, eVarType type, int32 array_size,
                                     uint32 actual_type_id)
{
    // -- adding automatically increments the paramcount if needed
    AddParameter(varname, varhash, type, array_size, paramcount, actual_type_id);
    return (true);
}

// ====================================================================================================================
// AddLocalVar():  Local variable declaration for a function definition.
// ====================================================================================================================
CVariableEntry* CFunctionContext::AddLocalVar(const char* varname, uint32 varhash, eVarType type, int32 array_size,
                                              bool8 is_param)
{

    // -- ensure the variable doesn't already exist
    CVariableEntry* exists = localvartable->FindItem(varhash);
    if (exists != NULL)
    {
        printf("Error - variable already exists: %s\n", varname);
        return (NULL);
    }

    // -- create the Variable entry
    CVariableEntry* ve = TinAlloc(ALLOC_VarEntry, CVariableEntry, GetScriptContext(), varname, varhash, type,
                                  array_size, false, 0, false, is_param);
	uint32 hash = ve->GetHash();
	localvartable->AddItem(*ve, hash);

    return (ve);
}

// ====================================================================================================================
// GetParameterCount():  Returns the number of parameters for a function definition.
// ====================================================================================================================
int32 CFunctionContext::GetParameterCount()
{
    return (paramcount);
}

// ====================================================================================================================
// GetParameter():  Returns the variable entry for a paremeter, by index.
// ====================================================================================================================
CVariableEntry* CFunctionContext::GetParameter(int32 index)
{
    assert(index >= 0 && index < paramcount);
    return (parameterlist[index]);
}

// ====================================================================================================================
// GetLocalVar():  Get the variable entry for a local variable.
// ====================================================================================================================
CVariableEntry* CFunctionContext::GetLocalVar(uint32 varhash)
{
    return (localvartable->FindItem(varhash));
}

// ====================================================================================================================
// GetLocalVarTable():  Get the variable table for all local variables and parameters.
// ====================================================================================================================
tVarTable* CFunctionContext::GetLocalVarTable()
{
    return (localvartable);
}

// ====================================================================================================================
// CalculateLocalVarStackSize():  Calculate the space needed to be reserved on the stack, for a function call.
// ====================================================================================================================
int32 CFunctionContext::CalculateLocalVarStackSize()
{
    int32 count = 0;
    CVariableEntry* ve = localvartable->First();
    while (ve)
    {
        if (ve->IsArray() && !ve->IsParameter())
            count += ve->GetArraySize();
        else
            ++count;
        ve = localvartable->Next();
    }

    // -- return the result
    return (count);
}

// ====================================================================================================================
// IsParameter():  Returns true if the given variable is a parameter, and not just a local variable.
// ====================================================================================================================
bool8 CFunctionContext::IsParameter(CVariableEntry* ve)
{
    if (!ve)
        return (false);
    for (int32 i = 0; i < paramcount; ++i)
    {
        if (parameterlist[i]->GetHash() == ve->GetHash())
            return (true);
    }

    return (false);
}

// ====================================================================================================================
// ClearParameters():  Reset the value of all parameters so ensure a "clean" function call.
// ====================================================================================================================
void CFunctionContext::ClearParameters()
{
    // -- first, clear the parameters
    const int32 max_size = MAX_TYPE_SIZE * (int32)sizeof(uint32);
    char buf[max_size];
    memset(buf, 0, max_size);
    for (int32 i = 0; i < paramcount; ++i)
    {
        CVariableEntry* ve = parameterlist[i];
        if (ve->GetType() == TYPE_hashtable || ve->IsArray())
            ve->ClearArrayParameter();
        else
            ve->SetValue(NULL, (void*)&buf);
    }

    // -- next, we want to ensure any local variables (not parameters) belonging to this function
    // -- which are of TYPE_hashtable, are *empty* tables, to ensure clean execution
    // -- as well as no memory leaks
    tVarTable* local_vars = GetLocalVarTable();
    CVariableEntry* ve = local_vars->First();
    while (ve)
    {
        if (!ve->IsParameter() && ve->GetType() == TYPE_hashtable)
        {
            tVarTable* hashtable = (tVarTable*)ve->GetAddr(NULL);
            hashtable->DestroyAll();
        }
        ve = local_vars->Next();
    }
}

// ====================================================================================================================
// InitStackVarOffsets():  Initialize the offset where the memory for a local variable can be found.
// ====================================================================================================================
void CFunctionContext::InitStackVarOffsets(CFunctionEntry* fe)
{
    int32 stackoffset = 0;

    // -- loop the parameters
    int32 paramcount = GetParameterCount();
    for (int32 i = 0; i < paramcount; ++i)
    {
        CVariableEntry* ve = GetParameter(i);
        assert(ve);

        // -- set the stackoffset (note:  parameters are never entire arrays, just references to them)
        // -- so the stack offset can simply be incremented
        if (ve->GetStackOffset() < 0)
        {
            ve->SetStackOffset(stackoffset);
            ve->SetFunctionEntry(fe);
        }
        ++stackoffset;
    }

    // -- now declare the rest of the local vars
    tVarTable* vartable = GetLocalVarTable();
    assert(vartable);
	if (vartable)
    {
		CVariableEntry* ve = vartable->First();
        while (ve)
        {
            if (!ve->IsParameter())
            {
                // -- set the stackoffset
                if (ve->GetStackOffset() < 0)
                {
                    ve->SetStackOffset(stackoffset);
                    ve->SetFunctionEntry(fe);
                }

                // -- if the variable is an array, we need to account for the size
                if (ve->IsArray())
                    stackoffset += ve->GetArraySize();
                else
                    ++stackoffset;
            }
		    ve = vartable->Next();
		}
	}
}

// ====================================================================================================================
// CalcHash():  Calculate the hash based on the parameter types, to support overloading 
// ====================================================================================================================
uint32 CFunctionContext::CalcHash()
{
    // -- the strategy here is, each paremeter causes the current hash to be multiplied
    // -- by 3x the number of valid types...  then we add the current type, multiplied by 2 if
    // -- it's an array...  there should be no numerical collisions based on this
    const uint32 next_hash_multiplier = 3 * (uint32)LAST_VALID_TYPE;
    uint32 param_hash = 0;

    // -- note:  we don't inclue the return value
    int param_count = GetParameterCount();
    for (int i = 1; i < param_count; ++i)
    {
        // -- get the type for the parameter
        CVariableEntry* param = GetParameter(i);
        eVarType param_type = param->GetType();

        // -- first multiply the current hash by the number of parameter types, times 2
        param_hash *= next_hash_multiplier;

        // -- now add the current type
        param_hash += (uint32)param_type;
    }

    // -- return the result
    return param_hash;
}

// == class CFunctionEntry ============================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CFunctionEntry::CFunctionEntry(CScriptContext* script_context, uint32 _nshash, const char* _name, uint32 _hash,
                               eFunctionType _type, void* _addr, uint32 sig_hash)
{
    mContextOwner = script_context;
	SafeStrcpy(mName, _name, kMaxNameLength);
	mHash = _hash;
    mNamespaceHash = _nshash;

    // -- initially we have one overload, with an unspecified signature (not yet known)
    CFunctionOverload* new_overload = TinAlloc(ALLOC_FuncOverload, CFunctionOverload, this, sig_hash,
                                                                                      _type, nullptr);
    AddOverload(new_overload);
}

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CFunctionEntry::CFunctionEntry(CScriptContext* script_context, uint32 _nshash, const char* _name, uint32 _hash,
                               eFunctionType _type, CRegFunctionBase* _func, uint32 sig_hash)
{
    mContextOwner = script_context;
	SafeStrcpy(mName, _name, kMaxNameLength);
	mHash = _hash;
    mNamespaceHash = _nshash;

    // -- initially we have one overload, with an unspecified signature (not yet known)
    CFunctionOverload* new_overload = TinAlloc(ALLOC_FuncOverload, CFunctionOverload, this, sig_hash,
                                                                                      _type, nullptr, _func);
    AddOverload(new_overload);
}

// ====================================================================================================================
// Destructor
// ====================================================================================================================
CFunctionEntry::~CFunctionEntry()
{
    // $$$TZA overload - will the overload table already have removed its entries from codeblocks?
    // -- notify the codeblock that this entry no longer exists
    /*
    if (mCodeBlock)
    {
#if TIN_DEBUGGER
        // -- if we're currently broken in the debugger, on this function, we need to exit the VM cleanly
        if (GetScriptContext()->mDebuggerBreakFuncCallStack)
            GetScriptContext()->mDebuggerBreakFuncCallStack->DebuggerNotifyFunctionDeleted(0, this);
#endif
        mCodeBlock->RemoveFunction(this);
    }
*/
}

// ====================================================================================================================
// AddOverload():  Add an overload to a function entry - replacing any that exist with the same signature
// ====================================================================================================================
void CFunctionEntry::AddOverload(CFunctionOverload* _overload)
{
    CFunctionOverload* exists = mOverloadTable.FindItem(_overload->GetHash());
    if (exists != nullptr)
    {
        // -- remove the existing overload, but don't remove the function
        RemoveOverload(exists, false);
    }

    // -- add the new overload
    mOverloadTable.AddItem(*_overload, _overload->GetHash());

    // -- also add the overload to the codeblock
    if (_overload->GetCodeBlock() != nullptr)
    {
        _overload->GetCodeBlock()->AddOverload(_overload);
    }

    // -- tag this overload as the one we (might be) defining
    SetActiveOverload(_overload->GetHash());
}

// ====================================================================================================================
// RemoveOverload():  Remove an overload from a function entry
// ====================================================================================================================
void CFunctionEntry::RemoveOverload(CFunctionOverload* _overload, bool remove_function)
{
    // -- ensure the overload exists
    CFunctionOverload* exists = FindOverload(_overload->GetHash());
    if (exists != nullptr)
    {
        // -- also remove the overload from the code block defining it
        if (exists->GetCodeBlock() != nullptr)
            exists->GetCodeBlock()->RemoveOverload(exists);

        mOverloadTable.RemoveItem(exists->GetHash());
    }

    // -- see if we're supposed to deleted the function entry
    if (remove_function && mOverloadTable.Used() == 0)
    {
        delete this;
    }
}

// ====================================================================================================================
// FindMatchingSignature():  Given a set of arguments, see if there's a non-ambiguous overload that is appropriate
// ====================================================================================================================
uint32 CFunctionEntry::FindMatchingSignature(CFunctionContext* caller) const
{
    // -- if there's only one overload, then whether it matches or not, it's not ambiguous
    if (mOverloadTable.Used() == 1)
    {
        // -- no need to even compute the signature hash - it'll always return the first, unless "undefined" is given
        CFunctionOverload* overload = FindOverload(0);
        return (overload->GetHash());
    }

    // $$$TZA overload - implement me!
    assert(0);
    return kFunctionSignatureUndefined;
}

// ====================================================================================================================
// GetType():  Return the type of the return value of a function overload.
// ====================================================================================================================
eFunctionType CFunctionEntry::GetType(uint32 signature_hash) const
{
    const CFunctionOverload* overload = FindOverload(signature_hash);
    assert(overload != nullptr);
    return (overload->GetType());
}

// ====================================================================================================================
// GetAddr():  Return the address of a registered (non scripted) function.
// ====================================================================================================================
void* CFunctionEntry::GetAddr(uint32 signature_hash) const
{
    // $$$TZA overload - support failure
    // -- find the overload
    CFunctionOverload* overload = FindOverload(signature_hash);
    assert(overload != nullptr);

    assert(overload->GetType() != eFunctionType::Script);
	return (overload->GetAddr());
}

// ====================================================================================================================
// SetCodeBlockOffset():  Set the offset where the byte code begins for a scripted function.
// ====================================================================================================================
void CFunctionEntry::SetCodeBlockOffset(uint32 signature_hash, CCodeBlock* _codeblock, uint32 _offset)
{
    // $$$TZA overload
    CFunctionOverload* overload = FindOverload(signature_hash);
    assert(overload != nullptr);

    // -- if we're switching codeblocks (recompiling...) change owners
    if (overload->GetCodeBlock() != nullptr)
        overload->GetCodeBlock()->RemoveOverload(overload);
    overload->SetCodeBlockOffset(_codeblock, _offset);
    if (overload->GetCodeBlock())
        overload->GetCodeBlock()->AddOverload(overload);
}

// ====================================================================================================================
// GetCodeBlockOffset():  Get the offset from within a codeblock for the start of the byte code.
// ====================================================================================================================
int32 CFunctionEntry::GetCodeBlockOffset(uint32 signature_hash, CCodeBlock*& _codeblock) const
{
    CFunctionOverload* overload = FindOverload(signature_hash);
    assert(overload != nullptr);
    return (overload->GetCodeBlockOffset(_codeblock));
}

// ====================================================================================================================
// GetContext():  Return the function context.
// ====================================================================================================================
CFunctionContext* CFunctionEntry::GetContext(uint32 signature_hash)
{
    CFunctionOverload* overload = FindOverload(signature_hash);
    assert(overload != nullptr);
    return (overload->GetContext());
}

// ====================================================================================================================
// GetReturnType():  Get the value type returned by the function.
// ====================================================================================================================
eVarType CFunctionEntry::GetReturnType(uint32 signature_hash)
{
    CFunctionOverload* overload = FindOverload(signature_hash);
    assert(overload != nullptr);
    CFunctionContext* context = overload->GetContext();

    // -- return value is always the first var entry in the array
    assert(context->GetParameterCount() > 0);
    return (context->GetParameter(0)->GetType());
}

// ====================================================================================================================
// GetLocalVarTable():  Get the variable table for all local variables and parameters for the function.
// ====================================================================================================================
tVarTable* CFunctionEntry::GetLocalVarTable(uint32 signature_hash)
{
    CFunctionOverload* overload = FindOverload(signature_hash);
    assert(overload != nullptr);
    CFunctionContext* context = overload->GetContext();
    return context->GetLocalVarTable();
}

// ====================================================================================================================
// GetRegObject():  Get the registration 
// ====================================================================================================================
CRegFunctionBase* CFunctionEntry::GetRegObject(uint32 signature_hash)
{
    CFunctionOverload* overload = FindOverload(signature_hash);
    assert(overload != nullptr);
    return (overload->GetRegObject());
}

// -- class CFunctionOverload -----------------------------------------------------------------------------------------

// ====================================================================================================================
// CFunctionOverload constructor
// ====================================================================================================================
CFunctionOverload::CFunctionOverload(CFunctionEntry* fe, uint32 _signature_hash, eFunctionType _type, void* _addr,
                                     CRegFunctionBase* reg_object)
    : mContext(fe->GetScriptContext())
{
    // -- store the owner
    mFunctionEntry = fe;

    // -- set the signature hash - initially this will be "unassigned", until the
    // -- signature has actually been parsed
    mSignatureHash = _signature_hash;

    // -- the type is either Script, or if it's registered code, Global or Method
    mType = _type;

    // -- either the overload is a code function (with and address), or the codeblock
    // -- and instruction offset will be set once the script is compiled
    assert(_addr == nullptr || reg_object == nullptr);
	mAddr = _addr;
    mRegObject = reg_object;

    mCodeBlock = NULL;
    mInstrOffset = 0;
}

// ====================================================================================================================
// CFunctionOverload destructor
// ====================================================================================================================
CFunctionOverload::~CFunctionOverload()
{
}

// ====================================================================================================================
// FinalizeSignature():  Calculates the signature hash, once the FunctionContext has been defined
// ====================================================================================================================
bool8 CFunctionOverload::FinalizeSignature()
{
    // -- remove ourself from the function entry - to be re-addede
    // -- to be re-added once the signature has been calculated
    // -- 
    mFunctionEntry->RemoveOverload(this, false);
    mSignatureHash = GetContext()->CalcHash();
    mFunctionEntry->AddOverload(this);

    // -- ensure the active overload is the this one
    mFunctionEntry->SetActiveOverload(mSignatureHash);

    // -- success
    return (true);
}

// ====================================================================================================================
// SetCodeBlockOffset():  Sets where the instructions for the overload are stored
// ====================================================================================================================
void CFunctionOverload::SetCodeBlockOffset(CCodeBlock* _codeblock, uint32 _offset)
{
    assert(_codeblock != nullptr);
    assert(_codeblock->GetScriptContext() != nullptr);
    mCodeBlock = _codeblock;
    mInstrOffset = _offset;
}

}  // TinScript

// ====================================================================================================================
// EOF
// ====================================================================================================================
