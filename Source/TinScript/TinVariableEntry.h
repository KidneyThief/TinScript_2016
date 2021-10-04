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
        // -- note:  0 or 1 means this is not an array...
        // -- postive value < kMaxVariableArraySize is the size, and -1 is an array, but an undetermined size
        return (mArraySize);
    }

    bool8 IsArray() const
    {
        return ((mArraySize > 1) || (mArraySize == -1 && mIsParameter && mType != TYPE_hashtable));
    }

    // -- strings being special...
    uint32* GetStringHashArray() const
    {
        return (mStringHashArray);
    }

    bool8 ConvertToArray(int32 array_size);
    void ClearArrayParameter();
    void InitializeArrayParameter(CVariableEntry* assign_from_ve, CObjectEntry* assign_from_oe,
                                  CExecStack& execstack, CFunctionCallStack& funccallstack);

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

    uint32 GetOffset() const
    {
        return mOffset;
    }

    bool8 IsParameter() const
    {
        return (mIsParameter);
    }

    bool8 IsStackVariable(CFunctionCallStack& funccallstack, bool allow_indexed_var = false) const;

    void SetValue(void* objaddr, void* value, CExecStack* execstack = NULL, CFunctionCallStack* funccallstack = NULL,
                  int32 array_index = 0);
    void SetValueAddr(void* objaddr, void* value, int32 array_index = 0);

    // -- the equivalent of SetValue and SetValueAddr, but for variable (arrays) of Type_string
    void SetStringArrayHashValue(void* objaddr, void* value, CExecStack* execstack = NULL,
                                 CFunctionCallStack* funccallstack = NULL, int32 array_index = 0);
    void SetStringArrayLiteralValue(void* objaddr, void* value, int32 array_index = 0);

	void SetBreakOnWrite(int32 varWatchRequestID, int32 debugger_session, bool8 break_on_write, const char* condition,
                         const char* trace, bool8 trace_on_cond)
	{
        // -- see if we need to remove an existing break
        if (mBreakOnWrite != NULL && !break_on_write && (!trace || !trace[0]))
        {
            TinFree(mBreakOnWrite);
            mBreakOnWrite = NULL;
		    mWatchRequestID = 0;
		    mDebuggerSession = 0;
        }

        else if (!mBreakOnWrite)
        {
            mBreakOnWrite = TinAlloc(ALLOC_Debugger, CDebuggerWatchExpression, true, break_on_write, condition,
                                     trace, trace_on_cond);
        }
        else
        {
            mBreakOnWrite->SetAttributes(break_on_write, condition, trace, trace_on_cond);
        }

		mWatchRequestID = varWatchRequestID;
		mDebuggerSession = debugger_session;
	}

    void NotifyWrite(CScriptContext* script_context, CExecStack* execstack, CFunctionCallStack* funccallstack)
    {
	    if (mBreakOnWrite)
	    {
		    int32 cur_debugger_session = 0;
		    bool is_debugger_connected = script_context->IsDebuggerConnected(cur_debugger_session);
		    if (!is_debugger_connected || mDebuggerSession < cur_debugger_session)
                return;

            // -- evaluate any condition we might have (by default, the condition is true)
            bool condition_result = true;

            // -- we can only evaluate conditions and trace points, if the variable is modified while
            // -- we have access to the stack
            if (execstack && funccallstack)
            {
                // -- note:  if we do have an expression, that can't be evaluated, assume true
                if (script_context->HasWatchExpression(*mBreakOnWrite) &&
                    script_context->InitWatchExpression(*mBreakOnWrite, false, *funccallstack) &&
                    script_context->EvalWatchExpression(*mBreakOnWrite, false, *funccallstack, *execstack))
                {
                    // -- if we're unable to retrieve the result, then found_break
                    eVarType return_type = TYPE_void;
                    void* return_value = NULL;
                    if (script_context->GetFunctionReturnValue(return_value, return_type))
                    {
                        // -- if this is false, then we *do not* break
                        void* bool_result = TypeConvert(script_context, return_type, return_value, TYPE_bool);
                        if (!(*(bool8*)bool_result))
                        {
                            condition_result = false;
                        }
                    }
                }

                // -- regardless of whether we break, we execute the trace expression, but only at the start of the line
                if (script_context->HasTraceExpression(*mBreakOnWrite))
                {
                    if (!mBreakOnWrite->mTraceOnCondition || condition_result)
                    {
                        if (script_context->InitWatchExpression(*mBreakOnWrite, true, *funccallstack))
                        {
                            // -- the trace expression has no result
                            script_context->EvalWatchExpression(*mBreakOnWrite, true, *funccallstack, *execstack);
                        }
                    }
                }
            }

            // -- we want to break only if the break is enabled, and the condition is true
            if (mBreakOnWrite->mIsEnabled && condition_result)
            {
			    script_context->SetForceBreak(mWatchRequestID);
            }
	    }
    }

    void SetFunctionEntry(CFunctionEntry* _funcentry)
    {
        mFuncEntry = _funcentry;
    }

    CFunctionEntry* GetFunctionEntry() const
    {
        return (mFuncEntry);
    }

    // -- if true, and this is the parameter of a registered function,
    // -- then instead of passing a uint32 to code, we'll
    // -- look up the object, verify it exists, verify it's namespace type matches
    // -- and convert to a pointer directly
    void SetDispatchConvertFromObject(uint32 convert_to_type_id)
    {
        mDispatchConvertFromObject = convert_to_type_id;
    }
    uint32 GetDispatchConvertFromObject()
    {
        return (mDispatchConvertFromObject);
    }

private:

    CScriptContext* mContextOwner;

    char mName[kMaxNameLength];
    uint32 mHash;
    eVarType mType;
    int32 mArraySize;
    void* mAddr;
    uint32 mOffset;
    int32 mStackOffset;
    bool8 mIsParameter;
    bool8 mIsDynamic;
    bool8 mScriptVar;
    mutable uint32 mStringValueHash;
    mutable uint32* mStringHashArray; // used only for registered string *arrays*
    uint32 mDispatchConvertFromObject;
    CFunctionEntry* mFuncEntry;

	// -- a debugger hook to break if the variable changes
	CDebuggerWatchExpression* mBreakOnWrite;
	int32 mWatchRequestID;
	int32 mDebuggerSession;
};

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

} // TinScript

// ------------------------------------------------------------------------------------------------
// eof
// ------------------------------------------------------------------------------------------------

