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
// TinVariableEntry.h
// ====================================================================================================================

#pragma once

#include "integration.h"
#include "TinTypes.h"
#include "TinScript.h"
#include "TinExecStack.h"

// == namespace TinScript =============================================================================================

namespace TinScript
{

// -- forward declarations --------------------------------------------------------------------------------------------
class CObjectEntry;
class CExecStack;
class CFunctionCallStack;
class CFunctionEntry;
class CDebuggerWatchExpression;

// ====================================================================================================================
// class CVariableEntry:  Contains the information for any created or registered variable/member.
// ====================================================================================================================
class CVariableEntry
{
public:
    CVariableEntry(CScriptContext* script_context, const char* _name = NULL, eVarType _type = TYPE_NULL,
                   int32 array_size = 1, void* _addr = NULL);
    CVariableEntry(CScriptContext* script_context, const char* _name, uint32 _hash, eVarType _type,
                   int32 array_size, bool isoffset, uint32 _offset, bool _isdynamic = false, bool _isparam = false);

    virtual ~CVariableEntry();

    // $$$TZA Clean this up - too many variables determining if mAddr should be freed
    bool TryFreeAddrMem();

    CScriptContext* GetScriptContext() const
    {
        return (mContextOwner);
    }

    const char* GetName() const
    {
        return mName;
    }

    eVarType GetType() const
    {
        return mType;
    }

    void SetResolveType(eVarType resolve_type)
    {
        // -- we're *not* permitted to set the variable type of anything except a Type__resolve
        // (used for schedules, since the return type isn't known, until the schedule is executed)
        if (mType == TYPE__resolve)
        {
            // -- we're only permitted to set the resolve type to something "non-array" (non-hashtable)
            // $$$TZA Test returning arrays from schedules()...
            if (resolve_type >= FIRST_VALID_TYPE && resolve_type <= LAST_VALID_TYPE &&
                resolve_type != TYPE_hashtable)
            {
                mType = resolve_type;
            }
        }
    }

    uint32 GetHash() const
    {
        return mHash;
    }

    int GetStackOffset() const
    {
        return mStackOffset;
    }

    void SetStackOffset(int32 _stackoffset)
    {
        mStackOffset = _stackoffset;
    }

    int32 GetArraySize() const
    {
        // -- note:  -1 means the array is uninitialized,
        // 0 should never exist (uninitialized or non-zero)
        // 1 is technically just a value, but the var would have been declared as an array var
        return (mArraySize);
    }

    bool8 IsArray() const
    {
        return (mType != TYPE_hashtable && mIsArray);
    }

    // -- strings being special...
    uint32* GetStringHashArray() const
    {
        return (mStringHashArray);
    }

    bool8 ConvertToArray(int32 array_size);
    void ClearArrayParameter();
    void InitializeArrayParameter(CVariableEntry* assign_from_ve, CObjectEntry* assign_from_oe,
                                  const CExecStack& execstack, const CFunctionCallStack& funccallstack);

    // -- this method is used only for registered arrays of const char*
    // -- the address is actually a const char*[], and there's a parallel
    // -- array of hash values, to keep the string table up to date
    void* GetStringArrayHashAddr(void* objaddr, int32 array_index) const;

    // -- this method is called by registered methods (dispatch templated implementation),
    // -- and as it is used to cross into cpp, it returns a const char* for strings
    void* GetValueAddr(void* objaddr) const;

    // -- this method is used only on the script side.  The address it returns must *never*
    // -- be written to - instead, the SetValue() method for variables is used.
    // -- for strings, this returns the address of the hash value found in the string dictionary
    void* GetAddr(void* objaddr) const;
    void* GetArrayVarAddr(void* objaddr, int32 array_index) const;

    // -- reference addresses are unique - they're used for POD methods, and they
    // have already calculated the array offset (if needed)...
    void* GetRefAddr() const { return (mRefAddr); }

    // -- we have a separate addr for Hashtable type vars, as we may need to copy one ht to another
    // e.g. when a hashtable is a param to a schedule()
    // -- as such, we need to ensure that when the scheduled call is completed, the variable destroys hashtable mem
    void* GetOrAllocHashtableAddr();

    uint32 GetOffset() const
    {
        return mOffset;
    }

    bool8 IsParameter() const
    {
        return (mIsParameter);
    }

    bool8 IsReference() const
    {
        return (mIsReference);
    }

    bool IsScriptVar() const
    {
        return (mScriptVar);
    }

    bool IsDynamic() const
    {
        return (mIsDynamic);
    }

    bool8 IsStackVariable(const CFunctionCallStack& funccallstack, bool allow_indexed_var = false) const;

    void SetValue(void* objaddr, void* value, CExecStack* execstack = NULL, CFunctionCallStack* funccallstack = NULL,
                  int32 array_index = 0);
    void SetValueAddr(void* objaddr, void* value, int32 array_index = 0);
    bool SetReferenceAddr(CVariableEntry* ref_ve, void* ref_addr = nullptr);

    // -- the equivalent of SetValue and SetValueAddr, but for variable (arrays) of Type_string
    void SetStringArrayHashValue(void* objaddr, void* value, CExecStack* execstack = NULL,
                                 CFunctionCallStack* funccallstack = NULL, int32 array_index = 0);
    void SetStringArrayLiteralValue(void* objaddr, void* value, int32 array_index = 0);

    void ClearBreakOnWrite();
	void SetBreakOnWrite(int32 varWatchRequestID, int32 debugger_session, bool8 break_on_write, const char* condition,
                         const char* trace, bool8 trace_on_cond);

    void NotifyWrite(CScriptContext* script_context, CExecStack* execstack, CFunctionCallStack* funccallstack);

    void SetFunctionEntry(CFunctionEntry* _funcentry);
    CFunctionEntry* GetFunctionEntry() const;

    // -- if true, and this is the parameter of a registered function,
    // -- then instead of passing a uint32 to code, we'll
    // -- look up the object, verify it exists, verify it's namespace type matches
    // -- and convert to a pointer directly
    void SetDispatchConvertFromObject(uint32 convert_to_type_id);
    uint32 GetDispatchConvertFromObject();

    CVariableEntry* Clone() const;

private:

    CScriptContext* mContextOwner;

    char mName[kMaxNameLength];
    uint32 mHash = 0;
    eVarType mType = TYPE_void;
    int32 mArraySize = -1;
    bool mIsArray = false;
    void* mAddr = nullptr;
    uint32 mOffset = 0;
    int32 mStackOffset = -1;
    bool8 mIsParameter = false;
    bool8 mIsDynamic = false;
    bool8 mScriptVar = false;
    bool mIsReference = false;
    mutable uint32 mStringValueHash = 0;
    mutable uint32* mStringHashArray = nullptr; // used only for registered string *arrays*
    void* mRefAddr = nullptr;
    uint32 mDispatchConvertFromObject = 0;
    CFunctionEntry* mFuncEntry = nullptr;

	// -- a debugger hook to break if the variable changes
	CDebuggerWatchExpression* mBreakOnWrite = nullptr;
	int32 mWatchRequestID = -1;
	int32 mDebuggerSession = -1;
};

// ====================================================================================================================
// GetGlobalVar():  Provides access from code, to a registered or scripted global variable
// Must be used if the global is declared in script (not registered from code)
// Must be used, of the global is of type const char* (or in string, in script)
// ====================================================================================================================
template <typename T>
inline bool8 GetGlobalVar(CScriptContext* script_context, const char* varname, T& value)
{
    // -- sanity check
    if (!script_context->GetGlobalNamespace() || !varname ||!varname[0])
        return (false);

    CVariableEntry*
        ve = script_context->GetGlobalNamespace()->GetVarTable()->FindItem(Hash(varname));
    if (!ve)
        return (false);

    // -- see if we can recognize an appropriate type
    eVarType returntype = GetRegisteredType(GetTypeID<T>());
    if (returntype == TYPE_NULL)
        return (false);

    // -- because the return type is *not* a const char* (which is specialized below)
    // -- we want to call GetValue(), not GetValueAddr() - which allows us to properly
    // -- convert from a string (ste) to any other type
    void* convertvalue = TypeConvert(script_context, ve->GetType(), ve->GetAddr(NULL), returntype);
    if (!convertvalue)
        return (false);

    // -- set the return value
    value = *reinterpret_cast<T*>((uint32*)(convertvalue));

    return (true);
}

// ====================================================================================================================
// GetGlobalVar():  const char* specialization - since we want a const char*, we need to use GetValueAddr()
// which returns an actual string, not the ste hash value
// ====================================================================================================================
template <>
inline bool8 GetGlobalVar<const char*>(CScriptContext* script_context, const char* varname, const char*& value)
{
    // -- sanity check
    if (!script_context->GetGlobalNamespace() || !varname ||!varname[0])
        return (false);

    CVariableEntry*
        ve = script_context->GetGlobalNamespace()->GetVarTable()->FindItem(Hash(varname));
    if (!ve)
        return (false);

    // -- note we're using GetValueAddr() - which returns a const char*, not an STE, for TYPE_string
    void* convertvalue = TypeConvert(script_context, ve->GetType(), ve->GetValueAddr(NULL), TYPE_string);
    if (!convertvalue)
        return (false);

    // -- set the return value
    value = (const char*)(convertvalue);

    return (true);
}

// ====================================================================================================================
// SetGlobalVar():  Provides access for code to modify the value of a registered or scripted global variable
// Must be used if the global is declared in script (not registered from code)
// Must be used, of the global is of type const char* (or in string, in script)
// ====================================================================================================================
template <typename T>
bool8 SetGlobalVar(CScriptContext* script_context, const char* varname, T value)
{
    // -- sanity check
    if (!script_context->GetGlobalNamespace() || !varname ||!varname[0])
        return (false);

    CVariableEntry*
        ve = script_context->GetGlobalNamespace()->GetVarTable()->FindItem(Hash(varname));
    if (!ve)
        return (false);

    // -- see if we can recognize an appropriate type
    eVarType input_type = GetRegisteredType(GetTypeID<T>());
    if (input_type == TYPE_NULL)
        return (false);

    void* convertvalue = TypeConvert(script_context, ve->GetType(), convert_to_void_ptr<T>::Convert(value), input_type);
    if (!convertvalue)
        return (false);

    // -- set the value - note, we're using SetValueAddr(), not SetValue(), which uses a const char*,
    // -- not an STE, for TYPE_string
    ve->SetValueAddr(NULL, convertvalue);
    return (true);
}

// ====================================================================================================================
// ConvertVariableForDispatch():  Converts a variable to the actual argument type, to pass to a registerd function.
// ====================================================================================================================
// A special case, where a registered parameter is an actual class pointer, not a uint32
// -- we'll look up the object, and if it exists, ensure it's namespace hierarchy
// -- contains the pointer type we're converting to... then we'll do the conversion
template <typename T>
T ConvertVariableForDispatch(CVariableEntry* ve)
{
    T return_value;
    uint32 conversion_type_id = ve->GetDispatchConvertFromObject();
    if (conversion_type_id != 0)
    {
        uint32 obj_id = convert_from_void_ptr<uint32>::Convert(ve->GetValueAddr(NULL));
        CObjectEntry* oe = GetContext()->FindObjectEntry(obj_id);
        if (oe)
        {
            // -- validate that the object is actually derived from the parameter expected
            bool ns_type_found = false;
            CNamespace* ns_entry = oe->GetNamespace();
            while (ns_entry)
            {
                if (ns_entry->GetTypeID() == conversion_type_id)
                {
                    ns_type_found = true;
                    break;
                }
                else
                {
                    ns_entry = ns_entry->GetNext();
                }
            }

            if (!ns_type_found)
            {
                ScriptAssert_(::TinScript::GetContext(), false, "<internal>", -1,
                              "Error - object %d cannot be passed - invalid type\n", oe->GetID());
            }

            return_value = convert_from_void_ptr<T>::Convert(oe->GetAddr());
            return (return_value);
        }

        // -- invalid or not found - return NULL
        return_value = convert_from_void_ptr<T>::Convert((void*)NULL);
    }
    else
    {
        return_value = convert_from_void_ptr<T>::Convert(ve->GetValueAddr(NULL));
    }

    // -- return the value
    return (return_value);
}

// ====================================================================================================================
// GetPODStackVarAddr():  Templated helper for getting address of a value (by type) for a variable entry
// ====================================================================================================================
template<typename T>
T* GetPODStackVarAddr(CVariableEntry* ve_src, int32 stack_depth)
{
    // -- sanity check
    if (ve_src == nullptr || ve_src->GetFunctionEntry() == nullptr)
        return nullptr;

    // -- this is a stack variable, if it's owned by a function
    // -- by definition, we're executing a function call for this method, so we want the
    // calling function's stack offset, which will likely be at 1 (stack_depth) below us on the stack
    int32 stack_var_offset = 0;
    CExecStack* execstack = nullptr;
    CFunctionCallStack* funccallstack = CFunctionCallStack::GetExecutionStackAtDepth(stack_depth, execstack,
                                                                                        stack_var_offset);
    T* value = funccallstack != nullptr
               ? (T*)execstack->GetStackVarAddr(stack_var_offset, ve_src->GetStackOffset())
               : nullptr;

    // -- if we were not able to retrieve the value by now, we failed
    if (value == nullptr)
    {
        TinPrint(TinScript::GetContext(), "Error - unable to get vector3f stack var addr for %s\n",
                                            UnHash(ve_src->GetHash()));
    }

    return (value);
}


} // TinScript

// ------------------------------------------------------------------------------------------------
// eof
// ------------------------------------------------------------------------------------------------

