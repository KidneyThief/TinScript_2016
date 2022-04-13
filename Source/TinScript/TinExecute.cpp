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

#include "integration.h"

// -- lib includes
#include <assert.h>
#include <string.h>
#include <stdio.h>

// -- external includes
#include "socket.h"
#include <thread>
#include <chrono>

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
// -- constructor for the internal tFunctionCallEntry
// ====================================================================================================================
CFunctionCallStack::tFunctionCallEntry::tFunctionCallEntry(CFunctionEntry* _funcentry, CObjectEntry* _objentry,
                                                           int32 _varoffset)
{
    funcentry = _funcentry;
    objentry = _objentry;
    fe_hash = funcentry != nullptr ? funcentry->GetHash() : 0;
    fe_ns_hash = funcentry != nullptr ? funcentry->GetNamespaceHash() : 0;
    fe_cb_hash = funcentry != nullptr && funcentry->GetCodeBlock() != nullptr
                 ? funcentry->GetCodeBlock()->GetFilenameHash() : 0;
    oe_id = objentry != nullptr ? objentry->GetID() : 0;
    stackvaroffset = _varoffset;
    linenumberfunccall = 0;
    isexecuting = false;
    mLocalObjectCount = 0;
}

// ====================================================================================================================
// -- constructor
// ====================================================================================================================
_declspec(thread) CFunctionCallStack* g_ExecutionHead = nullptr;

_declspec(thread) bool8 g_DebuggerBreakStep = false;

_declspec(thread) CFunctionCallStack* g_DebuggerBreakLastCallstack = nullptr;
_declspec(thread) int32 g_DebuggerBreakLastLineNumber = -1;
_declspec(thread) int32 g_DebuggerBreakLastStackDepth = -1;

CFunctionCallStack::CFunctionCallStack(CExecStack* var_execstack)
{
    m_varExecStack = var_execstack;
	m_functionEntryStack = (tFunctionCallEntry*)m_functionStackStorage;
	m_size = kExecFuncCallDepth;
	m_stacktop = 0;

	// -- we need to know if the function currently being stepped through has been reloaded
	mDebuggerFunctionReload = 0;

	// -- add this to the execution linked list
	// lets limit this to the main thread (we don't want, the IDE's execution on a separate thread clouding things)
	m_ExecutionPrev = nullptr;
	m_ExecutionNext = nullptr;
	m_ExecutionNext = g_ExecutionHead;
	if (g_ExecutionHead != nullptr)
		g_ExecutionHead->m_ExecutionPrev = this;
	g_ExecutionHead = this;
}

// ====================================================================================================================
// -- destructor
// ====================================================================================================================
CFunctionCallStack::~CFunctionCallStack()
{
	// -- remove this from the execution linked list
	CFunctionCallStack* found = g_ExecutionHead;
	while (found != nullptr && found != this)
	{
		found = found->m_ExecutionNext;
	}

	// -- arguably should assert here, if not found...
	if (found != nullptr)
	{
		if (found->m_ExecutionNext != nullptr)
			found->m_ExecutionNext->m_ExecutionPrev = found->m_ExecutionPrev;
		if (found->m_ExecutionPrev != nullptr)
			found->m_ExecutionPrev->m_ExecutionNext = found->m_ExecutionNext;
		else
			g_ExecutionHead = found->m_ExecutionNext;
	}

    // -- see if we still have anything executing...
    // (watch expressions hang around as long as the breakpoint exists, but aren't actively executing)
    bool finished_executing = true;
    CFunctionCallStack* walk = g_ExecutionHead;
    while (walk != nullptr)
    {
        uint32 obj_id = 0;
        CObjectEntry* dummy_obj = nullptr;
        int32 dummy_offset = -1;
        if (walk->GetExecuting(obj_id, dummy_obj, dummy_offset) != nullptr)
        {
            finished_executing = false;
            break;
        }

        walk = walk->m_ExecutionNext;
    }

    // -- if the execution head is empty - the stack is completely unwound, and this is a good time
    // to remove all unreferenced strings from the string table
    if (finished_executing)
    {
        // -- clean the function defining list, as we've completed defining our functions
        TinScript::GetContext()->ClearDefiningFunctionsList();
        
        // -- clean the string table of unused strings
        if (TinScript::GetContext()->GetStringTable())
        {
            TinScript::GetContext()->GetStringTable()->RemoveUnreferencedStrings();
        }

        // -- as we've also completed the entire execution stack, now is a good time to reset all
        // debugger variables (especially related to StepOut)
        g_DebuggerBreakStep = false;
        g_DebuggerBreakLastCallstack = nullptr;
        g_DebuggerBreakLastLineNumber = -1;
        g_DebuggerBreakLastStackDepth = -1;
    }
}

// ====================================================================================================================
// Push():  pushes a function entry (and obj, if this is a method) onto the call stack - still needs to be 
// "prepared" (e.g. assign arg values to the function context parameter vars) before BeginExecution()
// ====================================================================================================================
void CFunctionCallStack::Push(CFunctionEntry* functionentry, CObjectEntry* objentry, int32 varoffset, bool in_is_watch)
{
	assert(functionentry != NULL);
	assert(m_stacktop < m_size);
	m_functionEntryStack[m_stacktop].objentry = objentry;
	m_functionEntryStack[m_stacktop].funcentry = functionentry;

    m_functionEntryStack[m_stacktop].fe_hash = functionentry != nullptr ? functionentry->GetHash() : 0;
    m_functionEntryStack[m_stacktop].fe_ns_hash = functionentry != nullptr ? functionentry->GetNamespaceHash() : 0;
    m_functionEntryStack[m_stacktop].fe_cb_hash = functionentry != nullptr && functionentry->GetCodeBlock() != nullptr
                                                  ? functionentry->GetCodeBlock()->GetFilenameHash() : 0;
    m_functionEntryStack[m_stacktop].oe_id = objentry != nullptr ? objentry->GetID() : 0;

	m_functionEntryStack[m_stacktop].stackvaroffset = varoffset;
	m_functionEntryStack[m_stacktop].isexecuting = false;
    m_functionEntryStack[m_stacktop].is_watch_expression = in_is_watch;
    m_functionEntryStack[m_stacktop].mLocalObjectCount = 0;
	++m_stacktop;
}

// ====================================================================================================================
// Pop():  Execution of the given function has completed
// ====================================================================================================================
CFunctionEntry* CFunctionCallStack::Pop(CObjectEntry*& objentry, int32& var_offset)
{
	assert(m_stacktop > 0);

#if LOG_FUNCTION_EXEC
	if (TinScript::GetContext()->IsMainThread())
	{
		// -- function declarations will push and pop to the stack, so it's legitimate for
		// an empty function_call_str, as declaring isn't executing
		bool is_script_function = false;
		const char* function_call_str = GetExecutingFunctionCallString(is_script_function);
		if (function_call_str && function_call_str[0])
		{
			TinPrint(TinScript::GetContext(), "### [%s] Pop Function: %s\n",
											  is_script_function ? "TS" : "C++", function_call_str);
		}
	}
#endif

	objentry = m_functionEntryStack[m_stacktop - 1].objentry;
	var_offset = m_functionEntryStack[m_stacktop - 1].stackvaroffset;

	// -- any time we pop a function call, we auto-destroy the local objects
    // $$$TZA we need to do this when a function is reloaded during execution as well,
    // --atm, this is just an object leak during a debugging workflow
	CFunctionEntry* function_entry = m_functionEntryStack[m_stacktop - 1].funcentry;
	uint32* local_object_id_ptr = m_functionEntryStack[m_stacktop - 1].mLocalObjectIDList;
	for (int32 i = 0; i < m_functionEntryStack[m_stacktop - 1].mLocalObjectCount; ++i)
	{
		// -- if the object still exists, destroy it
		CObjectEntry* local_object =
			TinScript::GetContext()->FindObjectEntry(*local_object_id_ptr++);
		if (local_object != nullptr)
		{
			TinScript::GetContext()->DestroyObject(local_object->GetID());
		}
	}

	return (m_functionEntryStack[--m_stacktop].funcentry);
}

// ====================================================================================================================
// GetBreakExecutionFunctionCallEntry():  Get the function call entry from callstack, at the internal stack offset
// ====================================================================================================================
const CFunctionCallStack*
    CFunctionCallStack::GetBreakExecutionFunctionCallEntry(int32 execution_depth, int32& stack_offset,
                                                           int32& stack_offset_from_bottom)
{
    // -- sanity check
    if (execution_depth < 0)
        return false;

    stack_offset_from_bottom = 0;
    int32 current_stack_index = 0;
    CFunctionCallStack* found = nullptr;
    CFunctionCallStack* walk = this;
    while (walk != nullptr)
    {
        int32 walk_depth = walk->GetStackDepth();
        for (int32 walk_index = 0; walk_index < walk_depth; ++walk_index)
        {
            uint32 oe_id = 0;
            uint32 fe_hash = 0;
            const tFunctionCallEntry* func_call_entry = walk->GetExecutingCallByIndex(walk_index);
            if (func_call_entry != nullptr)
            {
                // -- if we've found an executing fe, see if we've counted up to the requested execution depth
                if (current_stack_index++ == execution_depth)
                {
                    // -- the output is the index into the function callstack,
                    // and the actual function callstack that contains the execution function at the global depth
                    stack_offset = walk_index;
                    found = walk;
                    stack_offset_from_bottom = 0;
                }
                else
                {
                    stack_offset_from_bottom++;
                }
            }
        }

        walk = walk->m_ExecutionNext;
    }

    // -- return the function call stack at the requested depth
    // stack_offset and stack_offset_from_bottom will also be valid, if found != nullptr
    return found;
}

// ====================================================================================================================
// GetCompleteExecutionStack():  walks through all callstacks, and returns the executing function calls for each
// -- the populated object/function lists should contain a complete callstack of current script calls
// ====================================================================================================================
int32 CFunctionCallStack::GetCompleteExecutionStack(CObjectEntry** _objentry_list, CFunctionEntry** _funcentry_list,
													uint32* _ns_hash_list, uint32* _cb_hash_list,
													int32* _linenumber_list, int32 max_count)
{
	// -- sanity check
	if (_objentry_list == nullptr || _funcentry_list == nullptr ||
		_cb_hash_list == nullptr || _linenumber_list == nullptr || max_count <= 0)
	{
		return 0;
	}

	int32 current_stack_index = 0;
	CFunctionCallStack* walk = g_ExecutionHead;
	while (walk != nullptr)
	{
		int32 walk_depth = walk->GetStackDepth();
		for (int32 walk_index = 0; walk_index < walk_depth; ++walk_index)
		{
            uint32 oe_id = 0;
            uint32 fe_hash = 0;
			if (walk->GetExecutingByIndex(oe_id, _objentry_list[current_stack_index], fe_hash,
                                          _funcentry_list[current_stack_index],  _ns_hash_list[current_stack_index],
                                          _cb_hash_list[current_stack_index], _linenumber_list[current_stack_index],
                                          walk_index))
			{
				if (++current_stack_index >= max_count)
					return current_stack_index;
			}
		}

		walk = walk->m_ExecutionNext;
	}

	return current_stack_index;
}

// ====================================================================================================================
// GetExecutionStackDepth():  Traverses all executionstacks, and returns the total execution stack depth
// ====================================================================================================================
int32 CFunctionCallStack::GetExecutionStackDepth()
{
    int32 total_stack_depth = 0;
	CFunctionCallStack* walk = g_ExecutionHead;
	while (walk != nullptr)
	{
		int32 walk_depth = walk->GetStackDepth();
		for (int32 walk_index = 0; walk_index < walk_depth; ++walk_index)
		{
            bool dummy = false;
			if (walk->IsExecutingByIndex(walk_index, dummy))
			{
                ++total_stack_depth;
			}
		}

		walk = walk->m_ExecutionNext;
	}

	return total_stack_depth;
}

// -- find the execution depth of a specific function callstack
// e.g.  the keyword execute() will spin up its own VM, and we still want to step in/over/out
// note:  we return the *actual* depth of executing call stacks...
int32 CFunctionCallStack::GetDepthOfFunctionCallStack(CFunctionCallStack* in_func_callstack)
{
    // -- sanity check
    if (in_func_callstack == nullptr)
        return -1;

    int32 found_stack_depth = 0;
    CFunctionCallStack* walk = g_ExecutionHead;
    while (walk != nullptr)
    {
        if (walk == in_func_callstack)
            return found_stack_depth;

        // -- we count this execution callstack, if the function is actually executing
        bool dummy = false;
        if (walk->IsExecutingByIndex(0, dummy))
            ++found_stack_depth;
        walk = walk->m_ExecutionNext;
    }

    // -- not found
    return -1;
}

// ====================================================================================================================
// NotifyFunctionDeleted():  if during execution, a function is redefined - any existing execution callstack
// containing that function has to be aborted - this is legitimate when reloading a script
// ====================================================================================================================
void CFunctionCallStack::NotifyFunctionDeleted(CFunctionEntry* deleted_fe)
{
    // -- note:  this will have to be re-thought, if we have *active* multi-threading for TinScript
    // -- at present, the only use cases are from the MainThread... but we could!
    CFunctionCallStack* walk = g_ExecutionHead;
    while (walk != nullptr)
    {
        int32 walk_depth = walk->GetStackDepth();
        for (int32 walk_index = 0; walk_index < walk_depth; ++walk_index)
        {
            uint32 oe_id = 0;
            uint32 fe_hash = 0;
            CObjectEntry* fc_oe = nullptr;
            CFunctionEntry* fc_fe = nullptr;
            uint32 fc_ns = 0;
            uint32 fc_fn = 0;
            int32 fc_ln = -1;
            if (walk->GetExecutingByIndex(oe_id, fc_oe, fe_hash, fc_fe, fc_ns, fc_fn, fc_ln, walk_index))
            {
                // -- if we've found our deleted function in the execution stack, mark the callstack
                if (walk->mDebuggerFunctionReload == 0 && deleted_fe != nullptr && deleted_fe->GetHash() == fe_hash)
                {
                    walk->mDebuggerFunctionReload = fe_hash;
                    break;
                }
            }
        }

        walk = walk->m_ExecutionNext;
    }
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
        m_functionEntryStack[m_stacktop - 1].fe_cb_hash != cur_codeblock)
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
                                               int32* linenumber_array, int32 max_array_size) const
{
    int32 entry_count = 0;
    int32 temp = m_stacktop - 1;
    while(temp >= 0 && entry_count < max_array_size)
    {
        if (m_functionEntryStack[temp].isexecuting)
        {
            uint32 codeblock_hash = m_functionEntryStack[temp].fe_cb_hash;
            uint32 objid = m_functionEntryStack[temp].oe_id;
            uint32 namespace_hash = m_functionEntryStack[temp].fe_ns_hash;
            uint32 func_hash = m_functionEntryStack[temp].fe_hash;
            int32 linenumber = m_functionEntryStack[temp].linenumberfunccall;

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
// GetCompleteExecutionStackVarEntries():  walks the entire execution stack, not just the current function call stack
// ====================================================================================================================
int32 CFunctionCallStack::GetCompleteExecutionStackVarEntries(CScriptContext* script_context,
                                                              CDebuggerWatchVarEntry* entry_array,
                                                              int32 max_array_size)
{

    // -- note:  this will have to be re-thought, if we have *active* multi-threading for TinScript
    // -- at present, the only use cases are from the MainThread... but we could!
    int32 ref_execution_offset_from_bottom = GetExecutionStackDepth();
    int32 total_entry_count = 0;
    int32 array_size_remaining = max_array_size;
    CFunctionCallStack* walk = g_ExecutionHead;
    while (walk != nullptr)
    {
        CExecStack* cur_exec_stack = walk->GetVariableExecStack();
        if (cur_exec_stack != nullptr)
        {
            // -- we're passing in a ref param - the execution depth, so the watch var entries
            // have their stack level set based on the overall complete execution stack
            int32 cur_entry_count = walk->DebuggerGetStackVarEntries(script_context, *cur_exec_stack,
                                                                     &entry_array[total_entry_count],
                                                                     array_size_remaining, ref_execution_offset_from_bottom);
            total_entry_count += cur_entry_count;
            array_size_remaining -= cur_entry_count;
            if (array_size_remaining <= 0)
                break;
        }
        walk = walk->m_ExecutionNext;
    }

    return total_entry_count;
}

// ====================================================================================================================
// DebuggerGetStackVarEntries():  Fills in the array of variables for a given stack frame, to send to the debugger.
// ====================================================================================================================
int32 CFunctionCallStack::DebuggerGetStackVarEntries(CScriptContext* script_context, CExecStack& execstack,
                                                     CDebuggerWatchVarEntry* entry_array, int32 max_array_size,
                                                     int32& ref_execution_offset_from_bottom)
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
    cur_entry->mStackOffsetFromBottom = -1;

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
	snprintf(cur_entry->mVarName, sizeof(cur_entry->mVarName), "%s", "__return");

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

    // -- iterating backwards *in case* we run out of space - more important to have stack entries at the stack top...
    int32 stack_index = m_stacktop - 1;
    while (stack_index >= 0 && entry_count < max_array_size)
    {
        if (m_functionEntryStack[stack_index].isexecuting)
        {
            // -- update the execution depth - this is the depth from the bottom across multiple VMs
            // -- the further we go down this function entry stack, the closer to the bottom (hence --);
            --ref_execution_offset_from_bottom;

            // -- if this function call is a method, send the "self" variable
            if (m_functionEntryStack[stack_index].oe_id != 0)
            {
                // -- limit of kDebuggerWatchWindowSize
                if (entry_count >= max_array_size)
                    return (entry_count);

                cur_entry = &entry_array[entry_count++];

				// -- debugger stack dumps are well defined and aren't a response to a dynamic request
				cur_entry->mWatchRequestID = 0;

                // -- set the stack level
                cur_entry->mStackOffsetFromBottom = ref_execution_offset_from_bottom;

                // -- copy the calling function info
                cur_entry->mFuncNamespaceHash = m_functionEntryStack[stack_index].fe_ns_hash;
                cur_entry->mFunctionHash = m_functionEntryStack[stack_index].fe_hash;
                cur_entry->mFunctionObjectID = m_functionEntryStack[stack_index].oe_id;

                // -- this isn't a member of an object
                cur_entry->mObjectID = 0;
                cur_entry->mNamespaceHash = 0;

                // -- copy the var type, name and value
                cur_entry->mType = TYPE_object;
                cur_entry->mArraySize = 1;
                snprintf(cur_entry->mVarName, sizeof(cur_entry->mVarName), "%s", "self");
                snprintf(cur_entry->mValue, sizeof(cur_entry->mValue), "%d", cur_entry->mFunctionObjectID);

                // -- fill in the cached members
                cur_entry->mVarHash = Hash("self");
                cur_entry->mVarObjectID = cur_entry->mFunctionObjectID;
            }

            // -- get the variable table - note - if the function has been deleted during a reload, we need to find
            // the *new* function entry
            CNamespace* verify_fe_ns = script_context->FindNamespace(m_functionEntryStack[stack_index].fe_ns_hash);
            CFunctionEntry* verify_fe = verify_fe_ns != nullptr
                                        ? verify_fe_ns->GetFuncTable()->FindItem(m_functionEntryStack[stack_index].fe_hash)
                                        : nullptr;

            tVarTable* func_vt = verify_fe != nullptr ? verify_fe->GetLocalVarTable() : nullptr;
            CVariableEntry* ve = func_vt != nullptr ? func_vt->First() : nullptr;
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
                cur_entry = &entry_array[entry_count++];
                if (entry_count >= max_array_size)
                    return max_array_size;

				// -- clear the dynamic watch request ID
				cur_entry->mWatchRequestID = 0;

                // -- set the stack level
                cur_entry->mStackOffsetFromBottom = ref_execution_offset_from_bottom;

                // -- copy the calling function info
                cur_entry->mFuncNamespaceHash = m_functionEntryStack[stack_index].fe_ns_hash;
                cur_entry->mFunctionHash = m_functionEntryStack[stack_index].fe_hash;
                cur_entry->mFunctionObjectID = m_functionEntryStack[stack_index].oe_id;

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
// FindExecutionStackVar():  Find the variable entry by hash, at the execution stack offset
// ====================================================================================================================
bool CFunctionCallStack::FindExecutionStackVar(uint32 var_hash, CDebuggerWatchVarEntry& watch_entry,
											   CVariableEntry*& found_ve)
{
    // -- initialize the output param
    found_ve = NULL;

    CScriptContext* script_context = TinScript::GetContext();
    if (script_context == nullptr || script_context->mDebuggerBreakFuncCallStack == nullptr)
        return false;

    // -- looking for a stack variable, we need to consider the entire execution stack, not just the current VMs
    // -- the depth we're looking for is the debugger execution stack offset
    int32 execution_offset = TinScript::GetContext()->mDebuggerWatchStackOffset;
    int32 stack_offset = -1;
    int32 stack_offset_from_bottom = -1;
    const CFunctionCallStack* debug_callstack =
        script_context->mDebuggerBreakFuncCallStack->
                        GetBreakExecutionFunctionCallEntry(execution_offset, stack_offset, stack_offset_from_bottom);
    if (debug_callstack == nullptr)
        return false;

    // -- all function call stacks are associated with their storage for local variables...
    CExecStack* execstack = debug_callstack->GetVariableExecStack();

    // -- this isn't safe - do not cache/use this address beyond the above function call...!!
    const tFunctionCallEntry* func_call_entry = debug_callstack->GetExecutingCallByIndex(stack_offset);
    if (func_call_entry == nullptr || execstack == nullptr)
        return false;

    // -- if this function call is a method, and we're requesting the "self" variable
    if (func_call_entry->objentry != NULL && var_hash == Hash("self"))
    {
		// -- clear the dynamic watch request ID
		watch_entry.mWatchRequestID = 0;

        // -- set the stack level
        watch_entry.mStackOffsetFromBottom = stack_offset;

		// -- copy the calling function info
		watch_entry.mFuncNamespaceHash = func_call_entry->fe_ns_hash;
		watch_entry.mFunctionHash = func_call_entry->fe_hash;
		watch_entry.mFunctionObjectID = func_call_entry->oe_id;

		// -- this isn't a member of an object
		watch_entry.mObjectID = 0;
		watch_entry.mNamespaceHash = 0;

		// -- copy the var type, name and value
		watch_entry.mType = TYPE_object;
		snprintf(watch_entry.mVarName, sizeof(watch_entry.mVarName), "%s", "self");
		snprintf(watch_entry.mValue, sizeof(watch_entry.mValue), "%d", func_call_entry->oe_id);

		// -- copy the var type
		watch_entry.mVarHash = var_hash;
		watch_entry.mVarObjectID = func_call_entry->oe_id;

		return (true);
	}

	// -- else see if it's a valid stack var
	else
	{
		// -- get the variable table
		tVarTable* func_vt = func_call_entry->funcentry->GetLocalVarTable();
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
                watch_entry.mStackOffsetFromBottom = stack_offset_from_bottom;

				// -- copy the calling function info
				watch_entry.mFuncNamespaceHash = func_call_entry->fe_ns_hash;
				watch_entry.mFunctionHash = func_call_entry->fe_hash;
				watch_entry.mFunctionObjectID = func_call_entry->oe_id;

				// -- this isn't a member of an object
				watch_entry.mObjectID = 0;
				watch_entry.mNamespaceHash = 0;

				// -- copy the var type
				watch_entry.mType = ve->GetType();

				// -- copy the var name
				SafeStrcpy(watch_entry.mVarName, sizeof(watch_entry.mVarName), UnHash(ve->GetHash()), kMaxNameLength);

				// -- get the address on the stack, where this local var is stored
				int32 func_stacktop = func_call_entry->stackvaroffset;
				int32 var_stackoffset = ve->GetStackOffset();
				void* stack_var_addr = execstack->GetStackVarAddr(func_stacktop, var_stackoffset);

				// -- copy the value, as a string (to a max length)
              	gRegisteredTypeToString[ve->GetType()](TinScript::GetContext(), stack_var_addr, watch_entry.mValue, kMaxNameLength);

				// -- fill in the hash of the var name, and if applicable, the var object ID
				watch_entry.mVarHash = ve->GetHash();
				watch_entry.mVarObjectID = 0;
				if (ve->GetType() == TYPE_object)
				{
					watch_entry.mVarObjectID = stack_var_addr ? *(uint32*)stack_var_addr : 0;

					// -- ensure the object actually exists
					if (TinScript::GetContext()->FindObjectEntry(watch_entry.mVarObjectID) == NULL)
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
	}

	// -- not found
	return (false);
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
// GetExecuting():  Get the function entry at the top of the stack, that is currently executing
// ====================================================================================================================
CFunctionEntry* CFunctionCallStack::GetExecuting(uint32& obj_id, CObjectEntry*& objentry, int32& varoffset)
{
    int32 temp = m_stacktop - 1;
    while (temp >= 0)
    {
        if (m_functionEntryStack[temp].isexecuting)
        {
            // -- note  we *could* add additional verification that the oe_id still exists - but
            // that should only be necessary when we *begin* a self.XXX instruction, not in the middle of all the ops
            obj_id = m_functionEntryStack[temp].oe_id;
            objentry = m_functionEntryStack[temp].objentry;
            varoffset = m_functionEntryStack[temp].stackvaroffset;
            return m_functionEntryStack[temp].funcentry;
        }
        --temp;
    }
    return (NULL);
}

// ====================================================================================================================
// GetExecutingFunctionCallString():  Get a loggable string representing the executing function call
// ====================================================================================================================
void CFunctionCallStack::FormatFunctionCallString(char* bufferptr, int32 buffer_len, CObjectEntry* fc_oe,
                                                  CFunctionEntry* fc_fe, uint32 fc_ns, uint32 fc_fn, int32 fc_ln)
{
    // -- sanity check
    if (bufferptr == nullptr || buffer_len <= 0 || fc_fe == nullptr)
    {
        return;
    }
    bufferptr[0] = '\0';
    if (fc_oe != nullptr)
    {
        snprintf(bufferptr, buffer_len, "%s%s%s(), obj: [%d] %s, src: %s @ %d",
            // -- function call
            fc_ns != 0 ? TinScript::UnHash(fc_ns) : "",
            fc_ns != 0 ? "::" : "",
            TinScript::UnHash(fc_fe->GetHash()),

            // -- object
            fc_oe->GetID(),
            fc_oe->GetNameHash() != 0 ? TinScript::UnHash(fc_oe->GetNameHash()) : "",

            // -- source
            fc_fn != 0 ? TinScript::UnHash(fc_fn) : "C++",
            fc_fn != 0 ? fc_ln : -1);
    }
    else
    {
        snprintf(bufferptr, buffer_len, "%s%s%s(), src: %s @ %d",
            // -- function call
            fc_ns != 0 ? TinScript::UnHash(fc_ns) : "",
            fc_ns != 0 ? "::" : "",
            TinScript::UnHash(fc_fe->GetHash()),

            // -- source
            fc_fn != 0 ? TinScript::UnHash(fc_fn) : "C++",
            fc_fn != 0 ? fc_ln : -1);
    }
}

// ====================================================================================================================
// GetExecutingFunctionCallString():  Get a loggable string representing the executing function call
// ====================================================================================================================
const char* CFunctionCallStack::GetExecutingFunctionCallString(bool& isScriptFunction)
{
	isScriptFunction = false;
	CObjectEntry* fc_oe = nullptr;
	CFunctionEntry* fc_fe = nullptr;
    uint32 fc_oe_id = 0;
    uint32 fc_fe_hash = 0;
	uint32 fc_ns = 0;
	uint32 fc_fn = 0;
	int32 fc_ln = -1;
	if (GetExecutingByIndex(fc_oe_id, fc_oe, fc_fe_hash, fc_fe, fc_ns, fc_fn, fc_ln, 0))
	{
		isScriptFunction = fc_fe->GetType() == eFuncTypeScript;
		char* bufferptr = TinScript::GetContext()->GetScratchBuffer();
        FormatFunctionCallString(bufferptr, kMaxTokenLength, fc_oe, fc_fe, fc_ns, fc_fn, fc_ln);
		return bufferptr;
	}
	else
	{
		return "";
	}
}

// ====================================================================================================================
// IsExecutingByIndex():  return bool if the index is a valid executing function call
// ====================================================================================================================
bool CFunctionCallStack::IsExecutingByIndex(int32 stack_top_offset, bool& is_watch_expression) const
{
    // -- sanity check
    if (stack_top_offset < 0)
        return false;

    // -- init the return value
    is_watch_expression = false;

    // note:  we count from the top down
    int32 stack_top_index = m_stacktop - 1;
    if (stack_top_offset > stack_top_index)
        return false;

    int32 stack_index = stack_top_index - stack_top_offset;
    if (!m_functionEntryStack[stack_index].isexecuting)
        return false;

    // -- set the output param value
    is_watch_expression = m_functionEntryStack[stack_index].is_watch_expression;

    // -- valid index for an executing function
    return true;
}

// ====================================================================================================================
// GetExecutingCallByIndex():  Get the function entry call at the stack index (from the top), if executing
// ====================================================================================================================
const CFunctionCallStack::tFunctionCallEntry* CFunctionCallStack::GetExecutingCallByIndex(int32 stack_top_offset) const
{
    bool dummy;
    if (!IsExecutingByIndex(stack_top_offset, dummy))
        return (false);

	// note:  we count from the top down
    // note: this is unsafe!!  returning an internal stack pointer - it had better not be cached or used beyond
    // the scope of the calling function!
	int32 stack_top_index = m_stacktop - 1;
	int32 stack_index = stack_top_index - stack_top_offset;
    if (m_functionEntryStack[stack_index].isexecuting)
    {
        return &m_functionEntryStack[stack_index];
    }

    // -- not executing
    return nullptr;
}

// ====================================================================================================================
// GetExecutingByIndex():  Get the function entry at the stack index (from the top), if executing
// ====================================================================================================================
bool CFunctionCallStack::GetExecutingByIndex(uint32& oe_id, CObjectEntry*& objentry, uint32& fe_hash,
                                             CFunctionEntry*& funcentry, uint32& _ns_hash,
											 uint32& _cb_hash, int32& _linenumber, int32 stack_top_offset)
{
    bool dummy;
    if (!IsExecutingByIndex(stack_top_offset, dummy))
        return (false);

	// note:  we count from the top down
	int32 stack_top_index = m_stacktop - 1;
	int32 stack_index = stack_top_index - stack_top_offset;

    // -- note:  this is used by asserts, possibly when a function or codeblock has been deleted -
    // do not dereference or return any unvalidated pointers here (e.g.  oe or fe)
    CScriptContext* script_context = TinScript::GetContext();
    oe_id = m_functionEntryStack[stack_index].oe_id;
    objentry = oe_id != 0 ? script_context->FindObjectEntry(oe_id) : nullptr;

    fe_hash = m_functionEntryStack[stack_index].fe_hash;
    CNamespace* ns = script_context->FindNamespace(m_functionEntryStack[stack_index].fe_ns_hash);
    funcentry = ns != nullptr ? ns->GetFuncTable()->FindItem(fe_hash) : nullptr;

	_ns_hash = m_functionEntryStack[stack_index].fe_ns_hash;
	_cb_hash = m_functionEntryStack[stack_index].fe_cb_hash;
	_linenumber = m_functionEntryStack[stack_index].linenumberfunccall;

	return (true);
}

// ====================================================================================================================
// GetTopMethod():  Get the top function entry, actively executing or not...
// ====================================================================================================================
CFunctionEntry* CFunctionCallStack::GetTopMethod(CObjectEntry*& objentry)
{
    int32 depth = 0;
    while (m_stacktop - depth > 0)
    {
        ++depth;
        if (m_functionEntryStack[m_stacktop - depth].objentry)
        {
            objentry = m_functionEntryStack[m_stacktop - depth].objentry;
            return m_functionEntryStack[m_stacktop - depth].funcentry;
        }
    }

    // -- no methods
    objentry = NULL;
    return (NULL);
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

#if LOG_FUNCTION_EXEC
	if (TinScript::GetContext()->IsMainThread())
	{
		// -- it *should* be impossible to get an empty function_call_str here, unlike Pop(), since we're actually
		// executing, but... safety...
		bool is_script_function = false;
		const char* function_call_str = funccallstack.GetExecutingFunctionCallString(is_script_function);
		if (function_call_str && function_call_str[0])
		{
			TinPrint(TinScript::GetContext(), "### [%s] Push Function: %s\n",
											  is_script_function ? "TS" : "C++", function_call_str);
		}
	}
#endif

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
            // -- we only assert if the failure was not because the function was reloaded
            if (funccallstack.mDebuggerFunctionReload == 0)
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
    CFunctionCallStack funccallstack(&execstack);

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
        TinPrint(script_context, "Error - ExecuteScheduledFunction(): invalid funchash/parameters\n");
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
            TinPrint(script_context, "Error - ExecuteScheduledFunction(): unable to find object: %d\n", objectid);
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
        TinPrint(script_context, "Error - ExecuteScheduledFunction(): unable to find function: %s\n", UnHash(funchash));
        return false;
    }

	// -- create the stack to use for the execution
	CExecStack execstack;
    CFunctionCallStack funccallstack(&execstack);

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
            TinPrint(script_context,
                     "Error - ExecuteScheduledFunction(): unable to assign parameter %d, calling function %s()\n",
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
            TinPrint(script_context,
                     "Error - ExecuteScheduledFunction(): unable to assign parameter %d, calling function %s()\n",
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
        // -- we only assert if the failure was not because the function was reloaded
        if (funccallstack.mDebuggerFunctionReload == 0)
        {
            TinPrint(script_context,
                     "Error - ExecuteScheduledFunction(): Unable to call function: %s()\n",
                     UnHash(fe->GetHash()));
        }

        // -- at this point, our execution stack is fully "unwound", we can reset asserts
        script_context->ResetAssertStack();

        return false;
    }

    // -- because every function is required to push a value onto the stack, pop the stack and
    // -- copy it to the _return parameter of this scheduled function
    eVarType contenttype;
    void* contentptr = execstack.Pop(contenttype);
    if (!contentptr)
    {
        TinPrint(script_context, "Error - ExecuteScheduledFunction(): no return value for scheduled func: %s()\n",
                                  UnHash(fe->GetHash()));

        // -- at this point, our execution stack is fully "unwound", we can reset asserts
        script_context->ResetAssertStack();

        return (false);
    }

    // -- get the return variable
    CVariableEntry* return_ve = parameters->GetParameter(0);
    if (!return_ve && return_ve->GetType() != TYPE_void)
    {
        TinPrint(script_context,
                 "Error - ExecuteScheduledFunction(): invalid return parameter for scheduled func: %s()\n",
                 UnHash(fe->GetHash()));

        // -- at this point, our execution stack is fully "unwound", we can reset asserts
        script_context->ResetAssertStack();

        return (false);
    }

    // -- if we do have a return value, try to convert the stack content to the right value
    if (return_ve && (return_ve->GetType() >= FIRST_VALID_TYPE || return_ve->GetType() == TYPE__resolve))
    {
        // -- if the return type is "resolve", it could be anything - so don't change it from what we have
        eVarType result_type = return_ve->GetType() != TYPE__resolve ? return_ve->GetType() : contenttype;
        void* converted_addr = TypeConvert(script_context, contenttype, contentptr, result_type);
        if (!converted_addr)
        {
            TinPrint(script_context,
                      "Error - ExecuteScheduledFunction(): invalid return parameter for func: %s()\n", UnHash(fe->GetHash()));

            // -- at this point, our execution stack is fully "unwound", we can reset asserts
            script_context->ResetAssertStack();

            return (false);
        }

        // -- if the return type is "resolve", then set the variable type to whatever was being assigned
        // $$$TZA note:  this doesn't support hashtables or arrays, limited to sizeof(Type__resolve), 16 bytes
        if (return_ve->GetType() == TYPE__resolve)
        {
            return_ve->SetResolveType(result_type);
        }

        // -- set the value - note:  a return value is a function context param, and never an object member
        return_ve->SetValue(nullptr, converted_addr, NULL, NULL);
    }

    // -- also copy them into the script context's return value
    script_context->SetFunctionReturnValue(contentptr, contenttype);

    // -- there was no assert, but still...
    script_context->ResetAssertStack();

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
    snprintf(cond_buf, sizeof(cond_buf), "Assert(%s) file: %s, line %d:", condition, filename, line_number + 1);

    // -- compose the assert message
    va_list args;
    va_start(args, fmt);
    char msg_buf[512];
    vsprintf_s(msg_buf, 512, fmt, args);
    va_end(args);

    // -- put both messages together, and send that to the DebuggerBreakLoop
    char assert_msg[2048];
    snprintf(assert_msg, sizeof(assert_msg), "%s\n%s", cond_buf, msg_buf);

    return (DebuggerBreakLoop(cb, instrptr, execstack, funccallstack, assert_msg));
}

// ====================================================================================================================
// DebuggerWaitForConnection():  If we're listening for a remote debugger, on assert, wait for connection
// ====================================================================================================================
bool8 DebuggerWaitForConnection(CScriptContext* script_context, const char* assert_msg)
{
    // -- we'll only wait if we're on the main thread, and not already breaking or skipping
    if (script_context == nullptr || script_context->mDebuggerBreakLoopGuard || script_context->IsAssertStackSkipped())
    {
        return false;
    }

    // -- if we're already connected, we're done
    int32 debugger_session = 0;
    if (script_context->IsDebuggerConnected(debugger_session))
    {
        return true;
    }

    // -- if we're not actively listening for a connection, we're done waiting
    if (!SocketManager::IsListening())
    {
        return false;
    }

    // -- make sure we have a valid time to wait
    float timeout_seconds = script_context->GetAssertConnectTime();
    if (timeout_seconds <= 0.0f)
    {
        return false;
    }

    // -- print the assert message, and the "waiting" message
    // note:  we need a special notification, since we won't have any kind of an "engine update",
    // affecting platforms that don't make any visual changes in their UI until the next Tick()
    TinAssert(script_context, assert_msg);

    // -- now set the timeout to 0.0...
    // --we only get one shot at this, or waiting *every* assert
    // hereafter would be tedious
    script_context->SetAssertConnectTime(0.0f);

    // -- loop, processing thread commands, until we receive a connected notification, or we time out
    auto time_start = std::chrono::high_resolution_clock::now();
    while (true)
    {
        // -- process the thread commands (e.g. notification of a debugger connection)
        script_context->ProcessThreadCommands();
        if (script_context->IsDebuggerConnected(debugger_session))
        {
            return true;
        }

        // -- ensure we only wait for the given duration
        auto current_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float, std::milli> elapsed_milli = current_time - time_start;
        if (elapsed_milli.count() > timeout_seconds * 1000.0f)
        {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(33));        
    }

    return false;
}

// ====================================================================================================================
// DebuggerBreakLoop():  If a remote debugger is connected, we'll halt the VM until released by the debugger
// ====================================================================================================================
bool8 DebuggerBreakLoop(CCodeBlock* cb, const uint32* instrptr, CExecStack& execstack,
                        CFunctionCallStack& funccallstack, const char* assert_msg)
{
    // -- sanity check
    if (cb == nullptr || cb->GetScriptContext() == nullptr)
    {
        return false;
    }

    // -- asserts, and breakpoints both need to do the same thing - notify the debugger of the file/line,
    // -- loop until the user makes choice.  Asserts have an additional message
    // -- if we don't have a connection, and we're done waiting for one, we don't loop
    CScriptContext* script_context = cb->GetScriptContext();
    if (!DebuggerWaitForConnection(script_context, assert_msg))
    {
        return false;
    }

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
    script_context->mDebuggerWatchStackOffset = 0;
    script_context->mDebuggerForceExecLineNumber = -1;

    // -- set the current callstack we're breaking in
    g_DebuggerBreakLastCallstack = &funccallstack;
    g_DebuggerBreakLastStackDepth = funccallstack.GetStackDepth();
    g_DebuggerBreakLastLineNumber = cur_line;

    // -- we're also going to plug the current line into the top entry of the funccallstack, which
    // is normally unused/unset
    funccallstack.DebuggerUpdateStackTopCurrentLine(codeblock_hash, cur_line);

    // -- grab the complete execution stack, and send it to the debugger
    CObjectEntry* oeList[kDebuggerCallstackSize];
    CFunctionEntry* feList[kDebuggerCallstackSize];
    uint32 nsHashList[kDebuggerCallstackSize];
    uint32 cbHashList[kDebuggerCallstackSize];
    int32 lineNumberList[kDebuggerCallstackSize];
    int32 stack_depth = CFunctionCallStack::GetCompleteExecutionStack(oeList, feList, nsHashList, cbHashList,
                                                                      lineNumberList, kDebuggerCallstackSize);
    CFunctionCallStack::GetCompleteExecutionStack(oeList, feList, nsHashList, cbHashList, lineNumberList, kDebuggerCallstackSize);
    script_context->DebuggerSendCallstack(oeList, feList, nsHashList, cbHashList, lineNumberList, stack_depth, 0);


    // -- get the entire list of variables, at every level for the current call stack
    CDebuggerWatchVarEntry watch_var_stack[kDebuggerWatchWindowSize];
    int32 watch_entry_size =
        CFunctionCallStack::GetCompleteExecutionStackVarEntries(script_context, watch_var_stack,
                                                                kDebuggerWatchWindowSize);

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
    while (true)
    {
        // -- disable breaking on any asserts while we're waiting for the original loop to exit
        if (is_assert)
        {
            script_context->SetAssertStackSkipped(true);
        }

        // -- we spin forever in this loop, until either the debugger disconnects,
        // -- or sends a message to step or run
        script_context->ProcessThreadCommands();

        // -- if either mDebuggerBreakStep or mDebuggerBreakRun was set, exit the loop
        if (script_context->mDebuggerActionStep || script_context->mDebuggerActionRun)
        {
            // -- set the bool to continue to break, based on which action is true
            // -- (unless it's an assert)
            g_DebuggerBreakStep = !is_assert && script_context->mDebuggerActionStep;
            break;
        }

        // -- if the function call stack is no longer valid because a function was reloaded, break
        if (funccallstack.mDebuggerFunctionReload != 0)
        {
            break;
        }

        // -- if we're trying to force execution at a different line number (buyer beware!)...
        if (script_context->mDebuggerForceExecLineNumber >= 0)
        {
            break;
        }

        // -- otherwise, sleep
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }

    // -- disable further asserts until the stack is unwound.
    if (is_assert)
    {
        script_context->SetAssertStackSkipped(true);
    }

    // -- release the guard
    script_context->mDebuggerBreakLoopGuard = false;
	script_context->mDebuggerBreakFuncCallStack = nullptr;
	script_context->mDebuggerBreakExecStack = nullptr;
    script_context->mDebuggerWatchStackOffset = 0;

    // -- we successfully handled the breakpoint with the loop
    return (true);
}

// ====================================================================================================================
// DebuggerFindStackVar():  Interface to retrieve the variable entry for a currently executing function.
// ====================================================================================================================
bool8 DebuggerFindStackVar(CScriptContext* script_context, uint32 var_hash, CDebuggerWatchVarEntry& watch_entry,
							  CVariableEntry*& ve)
{
	// -- this is only valid while we're broken in the debugger
	if (!script_context || !script_context->mDebuggerConnected || !script_context->mDebuggerBreakLoopGuard)
		return (false);

	// -- ensure we have a function call stack, and an exec stack
	if (!script_context->mDebuggerBreakFuncCallStack || !script_context->mDebuggerBreakExecStack)
		return (false);

    // -- find the stack variable - it'll use the mDebuggerWatchStackOffset, so we can find
    // locals from functions not at the stack top
	return (CFunctionCallStack::FindExecutionStackVar(var_hash, watch_entry, ve));
}

// ====================================================================================================================
// Execute():  Execute a code block
// ====================================================================================================================
bool8 CCodeBlock::Execute(uint32 offset, CExecStack& execstack, CFunctionCallStack& funccallstack)
{

#if DEBUG_CODEBLOCK
    if (GetDebugCodeBlock())
    {
        TinPrint(TinScript::GetContext(), "\n*** EXECUTING: %s\n\n", mFileName && mFileName[0] ? mFileName : "<stdin>");
    }
#endif

    // -- we'll track which line is we're on, so breakpoints only trigger
    // for the instrution, the *first* time the requested line number is being executed
    mLineNumberCurrent = -1;

    // -- initialize the function return value
    GetScriptContext()->SetFunctionReturnValue(NULL, TYPE_NULL);

    const uint32* instrptr = GetInstructionPtr();
    instrptr += offset;

	while (instrptr != NULL)
    {

// -- Debugging is done through a remote connection, which right now is only supported through WIN32
#if TIN_DEBUGGER

        // -- see if there's a breakpoint set for this line
        // -- or if it's being forced, or if we're stepping from the last break
        // -- note:  it's possible to be *in* the infinite loop, and then try to connect the debugger
        // -- followed by forcing a break.  The break comes in on the socket thread, so processing the
        // -- debugger connection might not have happened yet...
        CScriptContext* script_context = GetScriptContext();

        // -- get our current callstack depth
        int32 cur_stack_depth = funccallstack.GetStackDepth();

        // -- we only "force break" from executing a statement within a function....
        // -- by definition, "immediate execution" code can't be "broken" (use a breakpoint...!)
        // -- and we don't want to "force break" on the actual call to DebuggerActionStep(), but on the
        // current schedule/loop/w/e....
        bool force_break = script_context->mDebuggerActionForceBreak && cur_stack_depth > 0;

        if (force_break ||
            script_context->mDebuggerConnected && (g_DebuggerBreakStep || HasBreakpoints()))
        {
            // -- get the current line number - see if we should break
            int32 cur_line = CalcLineNumber(instrptr);

            // -- see if we're still on the line we last broke
            // (if we're returning from a function call, it's a "new line", but will match
            // the last break line when we stepped in)

            // -- see if our last function callstack is still being executed
            bool found_last_callstack = false;
            int32 found_last_depth =
                CFunctionCallStack::GetDepthOfFunctionCallStack(g_DebuggerBreakLastCallstack);

            // -- by definition, this is a new line, if we're in a different VM
            // (different funccallstack that *isn't* a watch expression)
            // -- note:  if the funccallstack has a 0 stack depth, we're executing
            // "immediate" code within a code block (not in a function)
            bool is_executing_watch = false;
            bool is_executing = funccallstack.GetStackDepth() == 0 ||
                                funccallstack.IsExecutingByIndex(0, is_executing_watch);
            bool is_new_line = is_executing && !is_executing_watch &&
                               (mLineNumberCurrent != cur_line ||
                               (g_DebuggerBreakLastCallstack && g_DebuggerBreakLastCallstack != &funccallstack));
            mLineNumberCurrent = cur_line;

            // -- unless we're forcing a break, see if we should break via stepping (in, over, out)
            bool should_break = force_break;
            if (!should_break && is_new_line && g_DebuggerBreakStep)
            {
                // -- if we're stepping in, then we break if this is a new line, wherever it is...
                if (!script_context->mDebuggerActionStepOver && !script_context->mDebuggerActionStepOut)
                {
                    should_break = true;
                }

                else
                {
                    // -- if we step out, then either we're in the same callstack, at a lower depth,
                    // or our last callstack is no longer being executed
                    if (script_context->mDebuggerActionStepOut)
                    {
                        if (found_last_depth == -1 ||
                            (found_last_depth == 0 && cur_stack_depth < g_DebuggerBreakLastStackDepth))
                        {
                            should_break = true;
                        }
                    }

                    // -- else we're stepping over - break, if we're no longer executing the same function callstack
                    // (e.g.  an execute() statement, which uses its own VM, has completed)
                    else if (found_last_depth == -1 ||
                             (found_last_depth == 0 &&
                             (cur_stack_depth < g_DebuggerBreakLastStackDepth ||
                              cur_stack_depth == g_DebuggerBreakLastStackDepth)))
                    {
                        should_break = true;
                    }
                }
            }

            // -- whether we break or not, we need to determine if we have a breakpoint
            // that might have a trace point (which might be executed independent of actually breaking)
            bool execute_trace = false;
            bool execute_condition = false;
            bool break_on_condition = false;
            CDebuggerWatchExpression* break_condition = mBreakpoints->FindItem(cur_line);
            if (break_condition != nullptr)
            {
                execute_trace = script_context->HasTraceExpression(*break_condition) &&
                                (is_new_line || break_condition->mTraceIsUpdated);
                execute_condition = is_new_line ||
                                    (break_condition->mTraceIsUpdated && break_condition->mTraceOnCondition);
                break_on_condition = break_condition->mIsEnabled && is_new_line;

                // -- either way, we only get one chance to execute a trace that's just been added to the current line
                break_condition->mTraceIsUpdated = false;
            }

            // -- if we should execute one or both for the breakpoint
            if (execute_trace || execute_condition)
            {
                // -- evaluate the conditional for the line - it's also used if to see if we execute the trace point
                bool condition_result = true;

                // -- note:  if we do have an expression, that can't be evaluated, assume true
                if (execute_condition && script_context->HasWatchExpression(*break_condition) &&
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

                // -- see if we should execute the trace expression
                if (execute_trace)
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

                // -- at this point, we check to see if we should break because of a breakpoint
                // (including a successful conditional)
                if (!should_break)
                {
                    should_break = break_on_condition && condition_result;
                }
            }

            // -- now actually break
            if (should_break)
            {
                DebuggerBreakLoop(this, instrptr, execstack, funccallstack);

                // -- on emerging from the break loop, see if we should force the vm to start executing from a different line number
                if (script_context->mDebuggerForceExecLineNumber >= 0)
                {
                    int32 force_line_number = TinScript::GetContext()->mDebuggerForceExecLineNumber;
                    script_context->mDebuggerForceExecLineNumber = -1;

                    // -- this should *only* be permitted if the line number is within the same currently executing function
                    // $$$TZA need to verify/enforce this
					
                    // -- after forcing the next line to execute, but before we actually do execute, we'll
                    // break into the debugger, for confirmation
                    CObjectEntry* cur_oe = nullptr;
                    int32 cur_var_offset = 0;
                    CFunctionEntry* cur_fe = funccallstack.GetTop(cur_oe, cur_var_offset);
                    if (cur_fe != nullptr && cur_fe->GetCodeBlock() != nullptr)
                    {
                        int32 adjusted_line_number = -1;
                        const uint32* updated_instrptr =
                            cur_fe->GetCodeBlock()->GetPCForFunctionLineNumber(force_line_number, adjusted_line_number);
                        if (updated_instrptr != nullptr)
                        {
                            instrptr = updated_instrptr;

                            TinPrint(TinScript::GetContext(), "### WARNING:  forcing execution to line: %d\n"
                                     "this will crash, if outside the definition of current function: %s()\n",
                                adjusted_line_number + 1, UnHash(cur_fe->GetHash()));

                            script_context->mDebuggerActionForceBreak = true;
                            mLineNumberCurrent = -1;
                        }
                    }
                }
            }
        }

        // -- if at any point during execution, we deleted a currently executing object, or reloaded a function
        // -- we need to break from this VM so we don't dereference an IP that no longer exists.
        if (funccallstack.mDebuggerFunctionReload != 0)
        {
            TinPrint(TinScript::GetContext(), "### Exiting current execution stack:\n    function %s() was reloaded during execution\n",
                                              UnHash(funccallstack.mDebuggerFunctionReload));
            return false;
        }

#endif // TIN_DEBUGGER

		// -- get the operation and process it
		eOpCode curoperation = (eOpCode)(*instrptr++);

        // -- execute the op - check the return value to ensure all operations are successful
        bool8 success = GetOpExecFunction(curoperation)(this, curoperation, instrptr, execstack, funccallstack);
        if (! success)
        {
            // -- we only assert if the failure was not because the function was reloaded
            if (funccallstack.mDebuggerFunctionReload == 0)
            {
                DebuggerAssert_(false, this, instrptr - 1, execstack, funccallstack,
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
