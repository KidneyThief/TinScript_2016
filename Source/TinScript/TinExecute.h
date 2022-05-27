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
// TinExecute.h
// ====================================================================================================================

#ifndef __TINEXECUTE_H
#define __TINEXECUTE_H

// -- includes
#include "integration.h"
#include "TinTypes.h"
#include "TinScript.h"

// == namespace TinScript =============================================================================================

namespace TinScript
{

//  DebugPrintVar(): helper function for dumping variables/values during execution
const char* DebugPrintVar(void* addr, eVarType vartype, bool dump_stack = false);

// ====================================================================================================================
// class CFunctionCallStack:  The class used to push/pop function entries as they are called and exited.
// ====================================================================================================================
class CFunctionCallStack
{
	public:
        struct tFunctionCallEntry;
		CFunctionCallStack(CExecStack* var_execstack = nullptr);
		virtual ~CFunctionCallStack();

        CExecStack* GetVariableExecStack() const { return m_varExecStack; }

		void Push(CFunctionEntry* functionentry, CObjectEntry* objentry, int32 varoffset, bool is_watch = false);
		CFunctionEntry* Pop(CObjectEntry*& objentry, int32& var_offset);

		void NotifyLocalObjectID(uint32 local_object_id)
		{
			// -- ensure we're actually in a function call
            if (m_stacktop <= 0)
			{
				ScriptAssert_(TinScript::GetContext(), 0, "<internal>", -1,
		                      "Error - create_local called outside a function definition\n");
				return;
			}

            if (m_functionEntryStack[m_stacktop - 1].mLocalObjectCount >= kExecFuncCallMaxLocalObjects)
			{
				ScriptAssert_(TinScript::GetContext(), 0, "<internal>", -1,
						      "Error - max local vars exceeded (size: %d)\n", kExecFuncCallMaxLocalObjects);
				return;
			}

			// -- push the ID into the list of local objects
            m_functionEntryStack[m_stacktop - 1].mLocalObjectIDList[m_functionEntryStack[m_stacktop - 1].mLocalObjectCount++] =
                local_object_id;
		}

   		CFunctionEntry* GetTop(CObjectEntry*& objentry, int32& varoffset) const
        {
            if (m_stacktop > 0)
            {
                objentry = m_functionEntryStack[m_stacktop - 1].objentry;
                varoffset = m_functionEntryStack[m_stacktop - 1].stackvaroffset;
                return (m_functionEntryStack[m_stacktop - 1].funcentry);
            }
            else
            {
                objentry = NULL;
                varoffset = -1;
                return (NULL);
            }
        }

        int32 GetStackDepth() const
        {
            return (m_stacktop);
        }

        void DebuggerUpdateStackTopCurrentLine(uint32 cur_codeblock, int32 cur_line);

        int32 DebuggerGetCallstack(uint32* codeblock_array, uint32* objid_array,
                                   uint32* namespace_array, uint32* func_array,
                                   int32* linenumber_array, int32 max_array_size) const;


        static int32 GetCompleteExecutionStackVarEntries(CScriptContext* script_context,
                                                         CDebuggerWatchVarEntry* entry_array, int32 max_array_size);
        int32 DebuggerGetStackVarEntries(CScriptContext* script_context, CExecStack& execstack,
                                         CDebuggerWatchVarEntry* entry_array, int32 max_array_size,
                                         int32& ref_execution_depth);

        void BeginExecution(const uint32* instrptr);
        void BeginExecution();

        CFunctionEntry* GetExecuting(uint32& obj_id, CObjectEntry*& objentry, int32& varoffset) const;
        bool IsExecutingByIndex(int32 stack_top_offset, bool& is_watch_expression) const;
		bool GetExecutingByIndex(uint32& oe_id, CObjectEntry*& objentry, uint32& fe_hash, CFunctionEntry*& funcentry,
                                 uint32& _ns_hash, uint32& _cb_hash, int32& _linenumber, int32 stack_top_offset);
        const tFunctionCallEntry* GetExecutingCallByIndex(int32 stack_top_offset) const;
   		CFunctionEntry* GetTopMethod(CObjectEntry*& objentry);
        static void FormatFunctionCallString(char* bufferptr, int32 buffer_len, CObjectEntry* fc_oe,
                                                          CFunctionEntry* fc_fe, uint32 fc_ns, uint32 fc_fn,
                                                          int32 fc_ln);
		const char* GetExecutingFunctionCallString(bool& isScriptFunction);

        struct tFunctionCallEntry
        {
            tFunctionCallEntry(CFunctionEntry* _funcentry = NULL, CObjectEntry* _objentry = NULL,
                               int32 _varoffset = -1);

            CFunctionEntry* funcentry = nullptr;
            CObjectEntry* objentry = nullptr;
            uint32 fe_hash = 0;
            uint32 fe_ns_hash = 0;
            uint32 fe_cb_hash = 0;
            uint32 oe_id = 0;
            int32 stackvaroffset = 0;
            uint32 linenumberfunccall = 0;
            bool8 isexecuting = false;
            bool is_watch_expression = false;
            int32 mLocalObjectCount = 0;
            uint32 mLocalObjectIDList[kExecFuncCallMaxLocalObjects];
        };

        // $$$TZA this may affect the current function we're stepping through - for now, it's the one debugger
        // member that is not a global (thread) var
        uint32 mDebuggerFunctionReload;

        static bool FindExecutionStackVar(uint32 var_hash, CDebuggerWatchVarEntry& watch_entry);

        // -- we're looking for the function call at the internal mDebuggerWatchStackOffset, from the break callstack
        const CFunctionCallStack* GetBreakExecutionFunctionCallEntry(int32 execution_depth, int32& stack_offset,
                                                                     int32& stack_offset_from_bottom);

		// -- for the crash reporter, we may need to get the complete script callstack being executed
		static int32 GetCompleteExecutionStack(CObjectEntry** _objentry_list, CFunctionEntry** _funcentry_list,
											   uint32* _ns_hash_list, uint32* _cb_hash_list,
											   int32* _linenumber_list, int32 max_count);
        static int32 GetExecutionStackDepth();
        static int32 GetDepthOfFunctionCallStack(CFunctionCallStack* in_func_callstack);
        static void NotifyFunctionDeleted(CFunctionEntry* deleted_fe);

        static CFunctionCallStack* GetExecutionStackAtDepth(int32 depth, CExecStack*& out_execstack,
                                                            int32& out_var_stack_offset);

        // -- API for tracking branch instructions, to detect infinite loops
        static bool NotifyBranchInstruction(const uint32* from_instr);
        static void ClearBranchTracking();

	private:
        CExecStack* m_varExecStack = nullptr;
        char m_functionStackStorage[sizeof(tFunctionCallEntry) * kExecFuncCallDepth];
        tFunctionCallEntry* m_functionEntryStack;
		int32 m_size;
		int32 m_stacktop;

		// -- we need to keep track of the full script callstack
		// note:  lots of things execute functions with their own independent CFunctionCallStack
		// (e.g. schedules, conditionals, watches, etc...)...
		// -- the "power" of this system is in part
		// that you can declare a Function and Execution stacks, and spin off the VM independently...
		// hence a function call stack could be "completed" even though it's not at the top of the stack
		// -- If the crash reporter needs to gather the full script stack of everything that is executing,
		// we need to gather from all the "active" call stacks
		static CFunctionCallStack* m_ExecutionHead;
		CFunctionCallStack* m_ExecutionPrev;
		CFunctionCallStack* m_ExecutionNext;
};

bool8 ExecuteCodeBlock(CCodeBlock& codeblock);
bool8 ExecuteScheduledFunction(CScriptContext* script_context, uint32 objectid, uint32 ns_hash, uint32 funchash,
                               CFunctionContext* parameters);
bool8 CodeBlockCallFunction(CFunctionEntry* fe, CObjectEntry* oe, CExecStack& execstack,
                            CFunctionCallStack& funccallstack, bool copy_stack_parameters);

bool8 DebuggerWaitForConnection(CScriptContext* script_context, const char* assert_msg);

bool8 DebuggerBreakLoop(CCodeBlock* cb, const uint32* instrptr, CExecStack& execstack,
                        CFunctionCallStack& funccallstack, const char* assert_msg = NULL);

bool8 DebuggerAssertLoop(const char* condition, CCodeBlock* cb, const uint32* instrptr, CExecStack& execstack,
                         CFunctionCallStack& funccallstack, const char* fmt, ...);

bool8 DebuggerFindStackVar(CScriptContext* script_context, uint32 var_hash, CDebuggerWatchVarEntry& watch_entry);

// --  a debugger assert is special, in that it happens while we have a callstack and use a remote
// -- debugger to provide insight into the issue (callstack variables can be examined for a bad value/object/etc...)
#define DebuggerAssert_(condition, cb, intstrptr, execstack, funccallstack, fmt, ...) \
    {                                                                                                               \
        if (!(condition) && (!cb->GetScriptContext()->mDebuggerConnected ||		      								\
							!cb->GetScriptContext()->mDebuggerBreakLoopGuard))				                        \
        {                                                                                                           \
            if (!DebuggerAssertLoop(#condition, cb, instrptr, execstack, funccallstack, fmt, ##__VA_ARGS__))        \
            {                                                                                                       \
                ScriptAssert_(cb->GetScriptContext(), condition, cb->GetFileName(), cb->CalcLineNumber(instrptr),   \
                              fmt, ##__VA_ARGS__);                                                                  \
            }                                                                                                       \
        }                                                                                                           \
    }                                                                                                               \


}  // TinScript

#endif // __TINEXECUTE_H

// ====================================================================================================================
// EOF
// ====================================================================================================================
