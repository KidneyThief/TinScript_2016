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
// TinExecute.cpp : Implementation of the virtual machine
// ====================================================================================================================

// -- lib includes
#include "assert.h"
#include "string.h"
#include "stdio.h"

// -- external includes
#include "socket.h"

// -- TinScript includes
#include "TinScript.h"
#include "TinCompile.h"
#include "TinNamespace.h"
#include "TinScheduler.h"
#include "TinExecute.h"
#include "TinOpExecFunctions.h"

// == namespace TinScript =============================================================================================

namespace TinScript
{

// ====================================================================================================================
// -- this is the table of function pointers, one to execute each operation
OpExecuteFunction gOpExecFunctions[OP_COUNT] =
{
    #define OperationEntry(a) OpExec##a,
    OperationTuple
    #undef OperationEntry
};

// ====================================================================================================================
// CopyStackParameters():  At the start of a function call, copy the argument values onto the stack.
// ====================================================================================================================
bool8 CopyStackParameters(CFunctionEntry* fe, CExecStack& execstack, CFunctionCallStack& funccallstack)
{
    // -- sanity check
    if (fe == NULL || !fe->GetContext())
    {
        ScriptAssert_(TinScript::GetContext(), 0, "<internal>", -1, "Error - invalid function entry\n");
        return false;
    }

    // -- initialize the parameters of our fe with the function context
    CFunctionContext* parameters = fe->GetContext();
    int32 srcparamcount = parameters->GetParameterCount();
    for (int32 i = 0; i < srcparamcount; ++i)
    {
        CVariableEntry* src = parameters->GetParameter(i);
        void* dst = GetStackVarAddr(TinScript::GetContext(), execstack, funccallstack,
                                    src->GetStackOffset());
        if (!dst)
        {
            ScriptAssert_(TinScript::GetContext(), 0, "<internal>", -1,
                          "Error - unable to assign parameter %d, calling function %s()\n",
                          i, UnHash(fe->GetHash()));
            return false;
        }

        // -- set the value - note, parameters in a function context are never full arrays, just references
        // -- CVariableEntry's with object offsets
        if (src)
        {
            void* src_addr = src->GetAddr(NULL);
            if (src_addr)
                memcpy(dst, src->GetAddr(NULL), MAX_TYPE_SIZE * sizeof(uint32));
        }
        else
            memset(dst, 0, MAX_TYPE_SIZE * sizeof(uint32));
    }

    return (true);
}

// ====================================================================================================================
// DebuggerUpdateStackTopCurrentLine():  the top of the function call stack is the currently executing function,
// -- but since it hasn't executed a function call (as it's the top), it's func call line number is unused/unset
// ====================================================================================================================
void CFunctionCallStack::DebuggerUpdateStackTopCurrentLine(uint32 cur_codeblock,int32 cur_line)
{
    // -- sanity check
    // -- make sure the stack actually has a function call entry, and that it's executing,
    // and that it's executing from the code block given
    if (m_stacktop < 1 || !m_functionEntryStack[m_stacktop - 1].isexecuting ||
        m_functionEntryStack[m_stacktop - 1].funcentry == nullptr ||
        m_functionEntryStack[m_stacktop - 1].funcentry->GetCodeBlock() == nullptr ||
        m_functionEntryStack[m_stacktop - 1].funcentry->GetCodeBlock()->GetFilenameHash() != cur_codeblock)
    {
        return;
    }

    // -- set the function call line number
    m_functionEntryStack[m_stacktop - 1].linenumberfunccall = cur_line;
}

// ====================================================================================================================
// DebuggerGetCallstack():  Fill in the arrays storing the current call stack information, to send to the debugger.
// ====================================================================================================================
int32 CFunctionCallStack::DebuggerGetCallstack(uint32* codeblock_array, uint32* objid_array,
                                               uint32* namespace_array, uint32* func_array,
                                               uint32* linenumber_array, int32 max_array_size) const
{
    int32 entry_count = 0;
    int32 temp = m_stacktop - 1;
    while(temp >= 0 && entry_count < max_array_size)
    {
        if (m_functionEntryStack[temp].isexecuting)
        {
            CCodeBlock* codeblock = NULL;
            m_functionEntryStack[temp].funcentry->GetCodeBlockOffset(codeblock);
            uint32 codeblock_hash = codeblock->GetFilenameHash();
            uint32 objid = m_functionEntryStack[temp].objentry ? m_functionEntryStack[temp].objentry->GetID() : 0;
            uint32 namespace_hash = m_functionEntryStack[temp].funcentry->GetNamespaceHash();
            uint32 func_hash = m_functionEntryStack[temp].funcentry->GetHash();
            uint32 linenumber = m_functionEntryStack[temp].linenumberfunccall;

            codeblock_array[entry_count] = codeblock_hash;
            objid_array[entry_count] = objid;
            namespace_array[entry_count] = namespace_hash;
            func_array[entry_count] = func_hash;
            linenumber_array[entry_count] = linenumber;
            ++entry_count;
        }
        --temp;
    }
    return (entry_count);
}

// ====================================================================================================================
// DebuggerGetStackVarEntries():  Fills in the array of variables for a given stack frame, to send to the debugger.
// ====================================================================================================================
int32 CFunctionCallStack::DebuggerGetStackVarEntries(CScriptContext* script_context, CExecStack& execstack,
                                                     CDebuggerWatchVarEntry* entry_array, int32 max_array_size)
{
    int32 entry_count = 0;

	// -- the first entry is always whatever is in the return buffer for the stack
	void* funcReturnValue = NULL;
	eVarType funcReturnType = TYPE_void;
	script_context->GetFunctionReturnValue(funcReturnValue, funcReturnType);

	// -- fill in the return value (which could be void)
	CDebuggerWatchVarEntry* cur_entry = &entry_array[entry_count++];

	// -- debugger stack dumps are well defined and aren't a response to a dynamic request
	cur_entry->mWatchRequestID = 0;

    // -- set the stack level
    cur_entry->mStackLevel = -1;

	// -- copy the calling function info
	// -- use the top level function being called, since it's the only function that can use
	// -- the return value from the last function executed
	cur_entry->mFuncNamespaceHash = 0;
	cur_entry->mFunctionHash = 0;
	cur_entry->mFunctionObjectID = 0;

	// -- this isn't a member of an object
	cur_entry->mObjectID = 0;
	cur_entry->mNamespaceHash = 0;

	// -- copy the var type, name and value
	cur_entry->mType = funcReturnType;
    cur_entry->mArraySize = 1;
	strcpy_s(cur_entry->mVarName, "__return");

    // -- copy the value, as a string (to a max length)
	if (funcReturnType >= FIRST_VALID_TYPE)
        gRegisteredTypeToString[funcReturnType](script_context, funcReturnValue, cur_entry->mValue, kMaxNameLength);
	else
		cur_entry->mValue[0] = '\0';

	// -- fill in the cached members
	cur_entry->mVarHash = Hash("__return");
	cur_entry->mVarObjectID = 0;
    if (funcReturnType == TYPE_object)
    {
        cur_entry->mVarObjectID = funcReturnValue ? *(uint32*)funcReturnValue : 0;

        // -- ensure the object actually exists
        if (script_context->FindObjectEntry(cur_entry->mVarObjectID) == NULL)
        {
            cur_entry->mVarObjectID = 0;
			cur_entry->mValue[0] = '\0';
        }
    }

    int32 stack_index = m_stacktop - 1;
    while (stack_index >= 0 && entry_count < max_array_size)
    {
        if (m_functionEntryStack[stack_index].isexecuting)
        {
            // -- if this function call is a method, send the "self" variable
            if (m_functionEntryStack[stack_index].objentry != NULL)
            {
                // -- limit of kDebuggerWatchWindowSize
                if (entry_count >= max_array_size)
                    return (entry_count);

                CDebuggerWatchVarEntry* cur_entry = &entry_array[entry_count++];

				// -- debugger stack dumps are well defined and aren't a response to a dynamic request
				cur_entry->mWatchRequestID = 0;

                // -- set the stack level
                cur_entry->mStackLevel = stack_index;

                // -- copy the calling function info
                cur_entry->mFuncNamespaceHash = m_functionEntryStack[stack_index].funcentry->GetNamespaceHash();
                cur_entry->mFunctionHash = m_functionEntryStack[stack_index].funcentry->GetHash();
                cur_entry->mFunctionObjectID = m_functionEntryStack[stack_index].objentry->GetID();

                // -- this isn't a member of an object
                cur_entry->mObjectID = 0;
                cur_entry->mNamespaceHash = 0;

                // -- copy the var type, name and value
                cur_entry->mType = TYPE_object;
                cur_entry->mArraySize = 1;
                strcpy_s(cur_entry->mVarName, "self");
                sprintf_s(cur_entry->mValue, "%d", cur_entry->mFunctionObjectID);

                // -- fill in the cached members
                cur_entry->mVarHash = Hash("self");
                cur_entry->mVarObjectID = cur_entry->mFunctionObjectID;
            }

            // -- get the variable table
            tVarTable* func_vt = m_functionEntryStack[stack_index].funcentry->GetLocalVarTable();
            CVariableEntry* ve = func_vt->First();
            while (ve)
            {
                // -- the first variable is usually the "__return", which we handle separately
                if (ve->GetHash() == Hash("__return"))
                {
                    ve = func_vt->Next();
                    continue;
                }

                // -- limit of kDebuggerWatchWindowSize
                if (entry_count >= max_array_size)
                    return (entry_count);

                // -- fill in the current entry
                CDebuggerWatchVarEntry* cur_entry = &entry_array[entry_count++];
                if (entry_count >= max_array_size)
                    return max_array_size;

				// -- clear the dynamic watch request ID
				cur_entry->mWatchRequestID = 0;

                // -- set the stack level
                cur_entry->mStackLevel = stack_index;

                // -- copy the calling function info
                cur_entry->mFuncNamespaceHash = m_functionEntryStack[stack_index].funcentry->GetNamespaceHash();
                cur_entry->mFunctionHash = m_functionEntryStack[stack_index].funcentry->GetHash();
                cur_entry->mFunctionObjectID = m_functionEntryStack[stack_index].objentry
                                               ? m_functionEntryStack[stack_index].objentry->GetID()
                                               : 0;

                // -- this isn't a member of an object
                cur_entry->mObjectID = 0;
                cur_entry->mNamespaceHash = 0;

                // -- copy the var type
                cur_entry->mType = ve->GetType();
                cur_entry->mArraySize = ve->GetArraySize();

                // -- copy the var name
                SafeStrcpy(cur_entry->mVarName, sizeof(cur_entry->mVarName), UnHash(ve->GetHash()), kMaxNameLength);

                // -- get the address on the stack, where this local var is stored
                int32 func_stacktop = m_functionEntryStack[stack_index].stackvaroffset;
                int32 var_stackoffset = ve->GetStackOffset();
                void* stack_var_addr = execstack.GetStackVarAddr(func_stacktop, var_stackoffset);

                // -- copy the value, as a string (to a max length)
              	gRegisteredTypeToString[ve->GetType()](script_context, stack_var_addr, cur_entry->mValue, kMaxNameLength);

                // -- fill in the hash of the var name, and if applicable, the var object ID
                cur_entry->mVarHash = ve->GetHash();
                cur_entry->mVarObjectID = 0;
                if (ve->GetType() == TYPE_object)
                {
                    cur_entry->mVarObjectID = stack_var_addr ? *(uint32*)stack_var_addr : 0;

                    // -- ensure the object actually exists
                    if (script_context->FindObjectEntry(cur_entry->mVarObjectID) == NULL)
                    {
                        cur_entry->mVarObjectID = 0;
						cur_entry->mValue[0] = '\0';
                    }
                }

                // -- get the next
                ve = func_vt->Next();
            }
        }
        --stack_index;
    }

    return (entry_count);
}

// ====================================================================================================================
// DebuggerFindStackTopVar():  Find the variable entry by hash, existing in the top stack frame.
// ====================================================================================================================
bool CFunctionCallStack::DebuggerFindStackTopVar(CScriptContext* script_context, CExecStack& execstack,
                                                 uint32 var_hash, CDebuggerWatchVarEntry& watch_entry,
												 CVariableEntry*& found_ve)
{
	found_ve = NULL;
    int32 stack_index = m_stacktop - 1;
    while (stack_index >= 0)
    {
        if (m_functionEntryStack[stack_index].isexecuting)
        {
            // -- if this function call is a method, and we're requesting the "self" variable
            if (m_functionEntryStack[stack_index].objentry != NULL && var_hash == Hash("self"))
            {
				// -- clear the dynamic watch request ID
				watch_entry.mWatchRequestID = 0;

                // -- set the stack level
                watch_entry.mStackLevel = stack_index;

				// -- copy the calling function info
				watch_entry.mFuncNamespaceHash = m_functionEntryStack[stack_index].funcentry->GetNamespaceHash();
				watch_entry.mFunctionHash = m_functionEntryStack[stack_index].funcentry->GetHash();
				watch_entry.mFunctionObjectID = m_functionEntryStack[stack_index].objentry
												? m_functionEntryStack[stack_index].objentry->GetID()
												: 0;

				// -- this isn't a member of an object
				watch_entry.mObjectID = 0;
				watch_entry.mNamespaceHash = 0;

				// -- copy the var type, name and value
				watch_entry.mType = TYPE_object;
				strcpy_s(watch_entry.mVarName, "self");
				sprintf_s(watch_entry.mValue, "%d", m_functionEntryStack[stack_index].objentry->GetID());

				// -- copy the var type
				watch_entry.mVarHash = var_hash;
				watch_entry.mVarObjectID = m_functionEntryStack[stack_index].objentry->GetID();

				return (true);
			}

			// -- else see if it's a valid stack var
			else
			{
				// -- get the variable table
				tVarTable* func_vt = m_functionEntryStack[stack_index].funcentry->GetLocalVarTable();
				CVariableEntry* ve = func_vt->First();
				while (ve)
				{
					if (ve->GetHash() == var_hash)
					{
						// -- set the ve result
						found_ve = ve;

						// -- clear the dynamic watch request ID
						watch_entry.mWatchRequestID = 0;

                        // -- set the stack level
                        watch_entry.mStackLevel = stack_index;

						// -- copy the calling function info
						watch_entry.mFuncNamespaceHash = m_functionEntryStack[stack_index].funcentry->GetNamespaceHash();
						watch_entry.mFunctionHash = m_functionEntryStack[stack_index].funcentry->GetHash();
						watch_entry.mFunctionObjectID = m_functionEntryStack[stack_index].objentry
														? m_functionEntryStack[stack_index].objentry->GetID()
														: 0;

						// -- this isn't a member of an object
						watch_entry.mObjectID = 0;
						watch_entry.mNamespaceHash = 0;

						// -- copy the var type
						watch_entry.mType = ve->GetType();

						// -- copy the var name
						SafeStrcpy(watch_entry.mVarName, sizeof(watch_entry.mVarName), UnHash(ve->GetHash()), kMaxNameLength);

						// -- get the address on the stack, where this local var is stored
						int32 func_stacktop = m_functionEntryStack[stack_index].stackvaroffset;
						int32 var_stackoffset = ve->GetStackOffset();
						void* stack_var_addr = execstack.GetStackVarAddr(func_stacktop, var_stackoffset);

						// -- copy the value, as a string (to a max length)
              			gRegisteredTypeToString[ve->GetType()](script_context, stack_var_addr, watch_entry.mValue, kMaxNameLength);

						// -- fill in the hash of the var name, and if applicable, the var object ID
						watch_entry.mVarHash = ve->GetHash();
						watch_entry.mVarObjectID = 0;
						if (ve->GetType() == TYPE_object)
						{
							watch_entry.mVarObjectID = stack_var_addr ? *(uint32*)stack_var_addr : 0;

							// -- ensure the object actually exists
							if (script_context->FindObjectEntry(watch_entry.mVarObjectID) == NULL)
							{
								watch_entry.mVarObjectID = 0;
								watch_entry.mValue[0] = '\0';
							}
						}

						// -- success
						return (true);
					}

					// -- try the next var
					ve = func_vt->Next();
				}

				// -- not found - we're done
				return (false);
			}

			// -- we stop at the the first executing stack entry
			break;
		}

		// -- next layer
		--stack_index;
	}

	// -- not found
	return (false);
}

// ====================================================================================================================
// DebuggerNotifyFunctionDeleted():  Called from the CFunctionEntry deconstructor, ensuring we abort the VM if needed.
// ====================================================================================================================
void CFunctionCallStack::DebuggerNotifyFunctionDeleted(CObjectEntry* oe, CFunctionEntry* fe)
{
    // -- if this function is anywhere on the current execution stack, we need to abort the debugger break loop
    int32 stack_index = m_stacktop - 1;
    while (stack_index >= 0)
    {
        if ((oe && m_functionEntryStack[stack_index].objentry == oe) || m_functionEntryStack[stack_index].funcentry == fe)
        {
            mDebuggerObjectDeleted = oe ? oe->GetID() : 0;
            mDebuggerFunctionReload = fe->GetHash();
            break;
        }

        // -- next
        --stack_index;
    }
}

// ====================================================================================================================
// BeginExecution():  Begin execution of the function we've prepared to call (assigned args, etc...)
// ====================================================================================================================
void CFunctionCallStack::BeginExecution(const uint32* instrptr)
{
    // -- the top entry on the function stack is what we're about to call...
    // -- the m_stacktop - 2, therefore, is the calling function (if it exists)...
    // -- tag it with the offset into the codeblock, for a debugger callstack
    if (m_stacktop >= 2 && m_functionEntryStack[m_stacktop - 2].funcentry->GetType() == eFuncTypeScript)
    {
        CCodeBlock* callingfunc_cb = NULL;
        m_functionEntryStack[m_stacktop - 2].funcentry->GetCodeBlockOffset(callingfunc_cb);
        m_functionEntryStack[m_stacktop - 2].linenumberfunccall = callingfunc_cb->CalcLineNumber(instrptr);
    }

    BeginExecution();
}

// ====================================================================================================================
// BeginExecution():  Notify execution has begun for the stack top function entry.
// ====================================================================================================================
void CFunctionCallStack::BeginExecution()
{
	assert(m_stacktop > 0);
    assert(m_functionEntryStack[m_stacktop - 1].isexecuting == false);
    m_functionEntryStack[m_stacktop - 1].isexecuting = true;
}

// ====================================================================================================================
// CodeBlockCallFunction():  Begin execution of a function, given the function entry and execution stacks.
// ====================================================================================================================
bool8 CodeBlockCallFunction(CFunctionEntry* fe, CObjectEntry* oe, CExecStack& execstack,
                            CFunctionCallStack& funccallstack, bool copy_stack_parameters)
{
    // -- at this point, the funccallstack has the CFunctionEntry pushed
    // -- and all parameters have been copied - either to the function's local var table
    // -- for registered 'C' functions, or to the execstack for scripted functions

    // -- scripted function
    if (fe->GetType() == eFuncTypeScript)
    {
        // -- for scheduled function calls, the stack parameters are still stored in the context
        // -- for regular function calls, GetStackVarAddr() will already have used the stack
        if (copy_stack_parameters)
            CopyStackParameters(fe, execstack, funccallstack);

        CCodeBlock* funccb = NULL;
        uint32 funcoffset = fe->GetCodeBlockOffset(funccb);
        if (!funccb)
        {
            ScriptAssert_(TinScript::GetContext(), 0, "<internal>", -1,
                          "Error - Undefined function: %s()\n", UnHash(fe->GetHash()));
            return false;
        }

        // -- execute the function via codeblock/offset
        bool8 success = funccb->Execute(funcoffset, execstack, funccallstack);

        if (!success)
        {
            if (funccallstack.mDebuggerObjectDeleted == 0 && funccallstack.mDebuggerFunctionReload == 0)
            {
                ScriptAssert_(TinScript::GetContext(), 0, "<internal>", -1,
                              "Error - error executing function: %s()\n",
                              UnHash(fe->GetHash()));
            }
            return false;
        }
    }

    // -- registered 'C' function
    else if (fe->GetType() == eFuncTypeRegistered)
    {
        fe->GetRegObject()->DispatchFunction(oe ? oe->GetAddr() : NULL);

        // -- if the function has a return type, push it on the stack
        if (fe->GetReturnType() > TYPE_void)
        {
            assert(fe->GetContext() && fe->GetContext()->GetParameterCount() > 0);
            CVariableEntry* returnval = fe->GetContext()->GetParameter(0);
            assert(returnval);
            execstack.Push(returnval->GetAddr(NULL), returnval->GetType());
        }

        // -- all functions must push a return value
        else
        {
            int32 empty = 0;
            execstack.Push(&empty, TYPE_int);
        }

        // -- clear all parameters for the function - this will ensure all
        // -- strings are decremented, keeping the string table clear of unassigned values
        fe->GetContext()->ClearParameters();
        TinScript::GetContext()->GetStringTable()->RemoveUnreferencedStrings();
        
        // -- since we called a 'C' function, there's no OP_FuncReturn - pop the function call stack
        int32 var_offset = 0;
        funccallstack.Pop(oe, var_offset);
    }

    return true;
}

// ====================================================================================================================
// ExecuteCodeBlock():  Execute a code block, including immediate instructions and defining functions.
// ====================================================================================================================
bool8 ExecuteCodeBlock(CCodeBlock& codeblock)
{
	// -- create the stack to use for the execution
	CExecStack execstack;
    CFunctionCallStack funccallstack;

    return (codeblock.Execute(0, execstack, funccallstack));
}

// ====================================================================================================================
// ExecuteScheduledFunction():  Execute a scheduled function.
// ====================================================================================================================
bool8 ExecuteScheduledFunction(CScriptContext* script_context, uint32 objectid, uint32 ns_hash, uint32 funchash,
                               CFunctionContext* parameters)
{
    // -- sanity check
    if (funchash == 0 && parameters == NULL)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1,
                      "Error - invalid funchash/parameters\n");
        return false;
    }

    // -- see if this is a method or a function
    CObjectEntry* oe = NULL;
    CFunctionEntry* fe = NULL;
    if (objectid != 0)
    {
        // -- find the object
        oe = script_context->FindObjectEntry(objectid);
        if (!oe)
        {
            ScriptAssert_(script_context, 0, "<internal>", -1,
                          "Error - unable to find object: %d\n", objectid);
            return false;
        }

        // -- get the namespace, then the function
        fe = oe->GetFunctionEntry(ns_hash, funchash);
    }
    else
    {
        fe = script_context->GetGlobalNamespace()->GetFuncTable()->FindItem(funchash);
    }

    // -- ensure we found our function
    if (!fe)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1,
                      "Error - unable to find function: %s\n", UnHash(funchash));
        return false;
    }

	// -- create the stack to use for the execution
	CExecStack execstack;
    CFunctionCallStack funccallstack;

    // -- nullvalue used to clear parameter values
    char nullvalue[MAX_TYPE_SIZE];
    memset(nullvalue, 0, MAX_TYPE_SIZE);

    // -- initialize the parameters of our fe with the function context
    int32 srcparamcount = parameters->GetParameterCount();
    for (int32 i = 0; i < srcparamcount; ++i)
    {
        CVariableEntry* src = parameters->GetParameter(i);
        CVariableEntry* dst = fe->GetContext()->GetParameter(i);
        if (!dst)
        {
            ScriptAssert_(script_context, 0, "<internal>", -1,
                          "Error - unable to assign parameter %d, calling function %s()\n",
                          i, UnHash(funchash));
            return false;
        }

        // -- ensure the type of the parameter value is converted to the type required
        // note:  the first parameter is *always* the return value, which we initialize to null
        void* srcaddr = NULL;
        if (i > 0 && src && dst->GetType() >= FIRST_VALID_TYPE)
            srcaddr = TypeConvert(script_context, src->GetType(), src->GetAddr(NULL), dst->GetType());
        else
            srcaddr = nullvalue;

        // -- if we were unable to convert, we're  done
        if (srcaddr == nullptr)
        {
            ScriptAssert_(script_context, 0, "<internal>", -1,
                          "Error - unable to assign parameter %d, calling function %s()\n",
                          i, UnHash(funchash));
            return false;
        }

        // -- set the value - note stack parameters are always local variables, never members
        dst->SetValue(NULL, srcaddr);
    }

    // -- initialize any remaining parameters
    int32 dstparamcount = fe->GetContext()->GetParameterCount();
    for (int32 i = srcparamcount; i < dstparamcount; ++i)
    {
        CVariableEntry* dst = fe->GetContext()->GetParameter(i);
        dst->SetValue(NULL, nullvalue);
    }

    // -- push the function entry onto the call stack (same as if OP_FuncCallArgs had been used)
    funccallstack.Push(fe, oe, 0);
    
    // -- create space on the execstack, if this is a script function
    if (fe->GetType() != eFuncTypeRegistered)
    {
        int32 localvarcount = fe->GetContext()->CalculateLocalVarStackSize();
        execstack.Reserve(localvarcount * MAX_TYPE_SIZE);
    }

    // -- scheduled functions are never nested, so it's ok to tag this function as having started
    // -- execution
    funccallstack.BeginExecution();

    // -- call the function
    bool8 result = CodeBlockCallFunction(fe, oe, execstack, funccallstack, true);
    if (!result)
    {
        if (funccallstack.mDebuggerObjectDeleted == 0 && funccallstack.mDebuggerFunctionReload == 0)
        {
            ScriptAssert_(script_context, 0, "<internal>", -1,
                          "Error - Unable to call function: %s()\n",
                          UnHash(fe->GetHash()));
        }
        return false;
    }

    // -- because every function is required to push a value onto the stack, pop the stack and
    // -- copy it to the _return parameter of this scheduled function
    eVarType contenttype;
    void* contentptr = execstack.Pop(contenttype);
    if (!contentptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1,
                                         "Error - no return value for scheduled func: %s()\n",
                                          UnHash(fe->GetHash()));
        return (false);
    }

    // -- get the return variable
    CVariableEntry* return_ve = parameters->GetParameter(0);
    if (!return_ve && return_ve->GetType() != TYPE_void)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1,
                     "Error - invalid return parameter for scheduled func: %s()\n",
                     UnHash(fe->GetHash()));
        return (false);
    }

    // -- if we do have a return value, try to convert the stack content to the right value
    if (return_ve && return_ve->GetType() >= FIRST_VALID_TYPE)
    {
        void* converted_addr = TypeConvert(script_context, contenttype, contentptr, return_ve->GetType());
        if (!converted_addr)
        {
            ScriptAssert_(script_context, 0, "<internal>", -1,
                          "Error - invalid return parameter for func: %s()\n", UnHash(fe->GetHash()));
            return (false);
        }
        return_ve->SetValue(oe ? oe->GetAddr() : NULL, converted_addr, NULL, NULL);
    }

    // -- also copy them into the script context's return value
    script_context->SetFunctionReturnValue(contentptr, contenttype);

    return (true);
}

// ====================================================================================================================
// GetOpExecFunction():  Get the function pointer from the table tied to the enum of operations.
// ====================================================================================================================
OpExecuteFunction GetOpExecFunction(eOpCode curoperation)
{
    return (gOpExecFunctions[curoperation]);
}

// ====================================================================================================================
// DebuggerAssertLoop():  Handles a failed assert condition, and either breaks in the remote debugger, or asserts
// ====================================================================================================================
bool8 DebuggerAssertLoop(const char* condition, CCodeBlock* cb, const uint32* instrptr, CExecStack& execstack,
                         CFunctionCallStack& funccallstack, const char* fmt, ...)
{
    // -- pull the file and line number, and construct the full condition message
    const char* filename = cb->GetFileName();
    int32 line_number = cb->CalcLineNumber(instrptr);
    char cond_buf[512];
    sprintf_s(cond_buf, "Assert(%s) file: %s, line %d:", condition, filename, line_number + 1);

    // -- compose the assert message
    va_list args;
    va_start(args, fmt);
    char msg_buf[512];
    vsprintf_s(msg_buf, 512, fmt, args);
    va_end(args);

    // -- put both messages together, and send that to the DebuggerBreakLoop
    char assert_msg[2048];
    sprintf_s(assert_msg, "%s\n%s", cond_buf, msg_buf);

    return (DebuggerBreakLoop(cb, instrptr, execstack, funccallstack, assert_msg));
}

// ====================================================================================================================
// DebuggerBreakLoop():  If a remote debugger is connected, we'll halt the VM until released by the debugger
// ====================================================================================================================
bool8 DebuggerBreakLoop(CCodeBlock* cb, const uint32* instrptr, CExecStack& execstack,
                        CFunctionCallStack& funccallstack, const char* assert_msg)
{
    // -- asserts, and breakpoints both need to do the same thing - notify the debugger of the file/line,
    // -- loop until the user makes choice.  Asserts have an additional message

    // -- if we're not able to handle the break loop, return false (possibly so an assert can trigger instead)
	int32 debugger_session = 0;
    if (!cb || !cb->GetScriptContext() || !cb->GetScriptContext()->IsDebuggerConnected(debugger_session))
        return (false);

    CScriptContext* script_context = cb->GetScriptContext();
    int32 cur_line = cb->CalcLineNumber(instrptr);
    uint32 codeblock_hash = cb->GetFilenameHash();

    // -- if we're already broken in an assert of some sort, we need to protect this loop from being re-entrant
    if (script_context->mDebuggerBreakLoopGuard || script_context->IsAssertStackSkipped())
    {
        // -- print the assert message, if we have one, but simply return (true)
        if (assert_msg && assert_msg[0])
        {
            TinPrint(script_context, assert_msg);
        }
        return (true);
    }

    // -- set the guard and cache the current exec stack and function call stack
    script_context->mDebuggerBreakLoopGuard = true;
	script_context->mDebuggerBreakFuncCallStack = &funccallstack;
	script_context->mDebuggerBreakExecStack = &execstack;

    // -- set the current line we're broken on
    funccallstack.mDebuggerLastBreak = cur_line;

    // -- we're also going to plug the current line into the top entry of the funccallstack, which
    // is normally unused/unset
    funccallstack.DebuggerUpdateStackTopCurrentLine(codeblock_hash, cur_line);

    // -- build the callstack arrays, in preparation to send them to the debugger
    uint32 codeblock_array[kDebuggerCallstackSize];
    uint32 objid_array[kDebuggerCallstackSize];
    uint32 namespace_array[kDebuggerCallstackSize];
    uint32 func_array[kDebuggerCallstackSize];
    uint32 linenumber_array[kDebuggerCallstackSize];
    int32 stack_size =
        funccallstack.DebuggerGetCallstack(codeblock_array, objid_array,
                                            namespace_array, func_array,
                                            linenumber_array, kDebuggerCallstackSize);

    script_context->DebuggerSendCallstack(codeblock_array, objid_array,
                                            namespace_array, func_array,
                                            linenumber_array, stack_size);

    // -- get the entire list of variables, at every level for the current call stack
    CDebuggerWatchVarEntry watch_var_stack[kDebuggerWatchWindowSize];
    int32 watch_entry_size =
        funccallstack.DebuggerGetStackVarEntries(script_context, execstack,
                                                    watch_var_stack, kDebuggerWatchWindowSize);

    // -- now loop through all stack variables, and any that are objects, send their
    // -- member dictionaries as well
    for (int32 i = 0; i < watch_entry_size; ++i)
    {
        script_context->DebuggerSendWatchVariable(&watch_var_stack[i]);

        // -- if the watch var is of type object, send the object members over as well
        if (watch_var_stack[i].mType == TYPE_object)
        {
            script_context->DebuggerSendObjectMembers(&watch_var_stack[i],
                                                        watch_var_stack[i].mVarObjectID);
        }
    }

    // -- send a message to the debugger - either this is an assert, or a breakpoint
    bool is_assert = (assert_msg && assert_msg[0]);
    if (is_assert)
    {
        script_context->DebuggerSendAssert(assert_msg, codeblock_hash, cur_line);
    }
    else
    {
        script_context->DebuggerBreakpointHit(script_context->mDebuggerVarWatchRequestID, codeblock_hash, cur_line);
    }

    // -- wait for the debugger to either continue to step or run
    script_context->SetBreakActionStep(false);
    script_context->SetBreakActionRun(false);
    funccallstack.mDebuggerBreakOnStackDepth = -1;
    while (true)
    {
        // -- disable breaking on any asserts while we're waiting for the original loop to exit
        if (is_assert)
        {
            script_context->SetAssertStackSkipped(true);
            script_context->SetAssertEnableTrace(true);
        }

        // -- we spin forever in this loop, until either the debugger disconnects,
        // -- or sends a message to step or run
        script_context->ProcessThreadCommands();

        // -- if either mDebuggerBreakStep or mDebuggerBreakRun was set, exit the loop
        if (script_context->mDebuggerActionStep || script_context->mDebuggerActionRun)
        {
            // -- set the bool to continue to break, based on which action is true
            // -- (unless it's an assert)
            funccallstack.mDebuggerBreakStep = !is_assert && script_context->mDebuggerActionStep;

            // -- if the break step action is either step over, or step out, we need to track the stack
            if (script_context->mDebuggerActionStepOver)
            {
                // -- only break at the same depth (or lower) - e.g.  don't step *into* functions
                funccallstack.mDebuggerBreakOnStackDepth = funccallstack.GetStackDepth();
            }
            else if (script_context->mDebuggerActionStepOut)
            {
                // -- only break at below the current depth
                funccallstack.mDebuggerBreakOnStackDepth = funccallstack.GetStackDepth() - 1;
            }
            break;
        }

        // -- if the function call stack is no longer valid because a function was reloaded, break
        if (funccallstack.mDebuggerObjectDeleted != 0 || funccallstack.mDebuggerFunctionReload != 0)
        {
            break;
        }

        // -- otherwise, sleep
        Sleep(1);
    }

    // -- disable further asserts until the stack is unwound.
    if (is_assert)
    {
        script_context->SetAssertStackSkipped(true);
        script_context->SetAssertEnableTrace(true);
    }

    // -- release the guard
    script_context->mDebuggerBreakLoopGuard = false;
	script_context->mDebuggerBreakFuncCallStack = NULL;
	script_context->mDebuggerBreakExecStack = NULL;

    // -- we successfully handled the breakpoint with the loop
    return (true);
}

// ====================================================================================================================
// DebuggerFindStackTopVar():  Interface to retrieve the variable entry for a currently executing function.
// ====================================================================================================================
bool8 DebuggerFindStackTopVar(CScriptContext* script_context, uint32 var_hash, CDebuggerWatchVarEntry& watch_entry,
							  CVariableEntry*& ve)
{
	// -- this is only valid while we're broken in the debugger
	if (!script_context || !script_context->mDebuggerConnected || !script_context->mDebuggerBreakLoopGuard)
		return (false);

	// -- ensure we have a function call stac, and an exec stack
	if (!script_context->mDebuggerBreakFuncCallStack || !script_context->mDebuggerBreakExecStack)
		return (false);

	return (script_context->mDebuggerBreakFuncCallStack->
							DebuggerFindStackTopVar(script_context, *script_context->mDebuggerBreakExecStack,
													var_hash, watch_entry, ve));
}

// ====================================================================================================================
// Execute():  Execute a code block
// ====================================================================================================================
bool8 CCodeBlock::Execute(uint32 offset, CExecStack& execstack, CFunctionCallStack& funccallstack)
{

#if DEBUG_CODEBLOCK
    if (GetDebugCodeBlock())
    {
        printf("\n*** EXECUTING: %s\n\n", mFileName && mFileName[0] ? mFileName : "<stdin>");
    }
#endif

    // -- initialize the function return value
    GetScriptContext()->SetFunctionReturnValue(NULL, TYPE_NULL);

    const uint32* instrptr = GetInstructionPtr();
    instrptr += offset;

	while (instrptr != NULL)
    {

// -- Debugging is done through a remote connection, which right now is only supported through WIN32
#ifdef WIN32
#if TIN_DEBUGGER

        // -- see if there's a breakpoint set for this line
        // -- or if it's being forced, or if we're stepping from the last break
        // -- note:  it's possible to be *in* the infinite loop, and then try to connect the debugger
        // -- followed by forcing a break.  The break comes in on the socket thread, so processing the
        // -- debugger connection might not have happened yet...
        CScriptContext* script_context = GetScriptContext();
        if (script_context->mDebuggerActionForceBreak ||
            script_context->mDebuggerConnected && (funccallstack.mDebuggerBreakStep || HasBreakpoints()))
        {
            // -- get the current line number - see if we should break
            bool isNewLine = false;
            int32 cur_line = CalcLineNumber(instrptr, &isNewLine);

            // -- break if we're stepping, and on a new line
            // -- if we're stepping out or over, then there's a stack depth we want to be at or below
            int32 cur_stack_depth = funccallstack.GetStackDepth();
            bool break_at_stack_depth = (funccallstack.mDebuggerBreakOnStackDepth < 0 ||
                                         cur_stack_depth <= funccallstack.mDebuggerBreakOnStackDepth);

            // -- if we're forcing a debugger break
            // -- if we're stepping, and we're on a different line, or if
            // -- we're not stepping, and on a different line, and this new line has a breakpoint
            CDebuggerWatchExpression* break_condition = mBreakpoints->FindItem(cur_line);
            bool force_break = script_context->mDebuggerActionForceBreak;
            bool step_new_line = funccallstack.mDebuggerBreakStep && funccallstack.mDebuggerLastBreak != cur_line &&
                                 break_at_stack_depth;
            bool found_break = (!funccallstack.mDebuggerBreakStep &&
                               (isNewLine || cur_line != funccallstack.mDebuggerLastBreak) && break_condition);

            // -- if we aren't forcing a break, and not stepping to a new line, and we found a break,
            // -- then evaluate the break conditional
            if (!force_break && !step_new_line && found_break)
            {
                // -- when looking to see if we have a breakpoint on this line,
                // -- we may have a condition and/or a trace expression
                bool condition_result = true;

                // -- note:  if we do have an expression, that can't be evaluated, assume true
                if (script_context->HasWatchExpression(*break_condition) &&
                    script_context->InitWatchExpression(*break_condition, false, funccallstack) &&
                    script_context->EvalWatchExpression(*break_condition, false, funccallstack, execstack))
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
                if (isNewLine && break_condition && script_context->HasTraceExpression(*break_condition))
                {
                    if (!break_condition->mTraceOnCondition || condition_result)
                    {
                        if (script_context->InitWatchExpression(*break_condition, true, funccallstack))
                        {
                            // -- the trace expression has no result
                            script_context->EvalWatchExpression(*break_condition, true, funccallstack, execstack);
                        }
                    }
                }

                // -- we want to break only if the break is enabled, and the condition is true
                found_break = break_condition->mIsEnabled && condition_result;
            }

            // -- now see if we should break
            if (force_break || step_new_line || found_break)
            {
                DebuggerBreakLoop(this, instrptr, execstack, funccallstack);
            }
        }

        // -- if at any point during execution, we deleted a currently executing object, or reloaded a function
        // -- we need to break from this VM so we don't dereference an IP that no longer exists.
        if (funccallstack.mDebuggerObjectDeleted != 0 || funccallstack.mDebuggerFunctionReload != 0)
        {
            char msg_buf[kMaxTokenLength];
            if (funccallstack.mDebuggerFunctionReload != 0)
                sprintf_s(msg_buf, "Break suspended - function %s() has been redefined.\n",
                          UnHash(funccallstack.mDebuggerFunctionReload));
            else
                sprintf_s(msg_buf, "Break suspended - Object [%d] no longer exists.\n",
                          funccallstack.mDebuggerObjectDeleted);
            script_context->DebuggerSendAssert(msg_buf, 0, 0);
            return (false);
        }

#endif // TIN_DEBUGGER
#endif // WIN32

		// -- get the operation and process it
		eOpCode curoperation = (eOpCode)(*instrptr++);

        // -- execute the op - check the return value to ensure all operations are successful
        bool8 success = GetOpExecFunction(curoperation)(this, curoperation, instrptr, execstack, funccallstack);
        if (! success)
        {
            if (funccallstack.mDebuggerObjectDeleted == 0 && funccallstack.mDebuggerFunctionReload == 0)
            {
                ScriptAssert_(GetScriptContext(), false, GetFileName(), CalcLineNumber(instrptr - 1),
                              "Error - Unable to execute OP:  %s\n", GetOperationString(curoperation));
            }
            return (false);
        }

        // -- two notable exceptions - if the curoperation was either OP_FuncReturn or OP_EOF,
        // -- we're finished executing this codeblock
        if (curoperation == OP_FuncReturn || curoperation == OP_EOF)
        {
            return (true);
        }
	}

	// -- ran out of instructions, without a legitimate OP_EOF
	return false;
}

}  // TinScript

// ====================================================================================================================
// EOF
// ====================================================================================================================
