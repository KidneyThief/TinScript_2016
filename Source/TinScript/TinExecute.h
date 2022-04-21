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
#include "TinCompile.h"

// == namespace TinScript =============================================================================================

namespace TinScript
{

// ====================================================================================================================
// class CExecStack: The class used to push and pop entries (values, variables, etc...) during VM execution.
// ====================================================================================================================
class CExecStack
{
	public:
		CExecStack()
        {
            mContextOwner = TinScript::GetContext();
            mSize = kExecStackSize;
			mStack = mStackStorage;
			mStackTop = mStack;
		}

		virtual ~CExecStack()
        {
		}

        CScriptContext* GetContextOwner() const { return (mContextOwner); }

		void Push(void* content, eVarType contenttype)
		{
			Assert_(content != NULL);
			uint32 contentsize = kBytesToWordCount(gRegisteredTypeSize[contenttype]);

            uint32 stacksize = kPointerDiffUInt32(mStackTop, mStack) / sizeof(uint32);
            if (stacksize + contentsize > kExecStackSize)
            {
                ScriptAssert_(TinScript::GetContext(), 0, "<internal>", -1,
                              "Error - stack overflow (size: %d) - unrecoverable\n", kExecStackSize);
                return;
            }

            // -- if we're pushing a hash table, we don't want to dereference the pointer
            if (contenttype == TYPE_hashtable)
            {
// -- in 64-bit, we push the actual HashTable address, and TinTypes.h
// defines TYPE_hashtable as needing an extra 4 bytes
#if BUILD_64								
                *mStackTop++ = kPointer64UpperUInt32(content);
				*mStackTop++ = kPointer64LowerUInt32(content);
#else
			    uint32* contentptr = (uint32*)content;
                *mStackTop++ = (uint32)contentptr;
#endif				
            }
            else
            {
			    uint32* contentptr = (uint32*)content;
			    for(uint32 i = 0; i < contentsize; ++i)
				    *mStackTop++ = *contentptr++;
            }

			// -- push the type of the content as well, so we know what to pull
			*mStackTop++ = (uint32)contenttype;

            // -- pushing and popping strings onto the execstack need to be refcounted
            if (contenttype == TYPE_string)
            {
                uint32 string_hash = *(uint32*)content;
                mContextOwner->GetStringTable()->RefCountIncrement(string_hash);
            }
		}

		void* Pop(eVarType& contenttype)
        {
			uint32 stacksize = kPointerDiffUInt32(mStackTop, mStack) / sizeof(uint32);
            Unused_(stacksize);
            if (stacksize == 0)
            {
                ScriptAssert_(TinScript::GetContext(), 0, "<internal>", -1,
                              "Error - attempting to pop a value off an empty stack\n");
                return (NULL);
            }

            contenttype = (eVarType)(*(--mStackTop));

            // -- if what's on the stack isn't a valid content type, leave the stack alone, but
            // -- return NULL - the calling operation should catch the NULL and assert
            if (contenttype < 0 || contenttype >= TYPE_COUNT)
            {
                ++mStackTop;
                return (NULL);
            }

			// -- ensure we have enough data on the stack, both the content, and the type
            uint32 contentsize = kBytesToWordCount(gRegisteredTypeSize[contenttype]);
            Assert_(stacksize >= contentsize + 1);
			mStackTop -= contentsize;

            // -- pushing and popping strings onto the execstack need to be refcounted
            if (contenttype == TYPE_string)
            {
                uint32 string_hash = *(uint32*)mStackTop;
                mContextOwner->GetStringTable()->RefCountDecrement(string_hash);
            }

            // -- for hashtables, match the Push(), where the address was assigned directly to the contents
            else if (contenttype == TYPE_hashtable)
            {
// -- in 64-bit, we push the actual HashTable address, and TinTypes.h
// defines TYPE_hashtable as needing an extra 4 byte
// -- note:  the upper and lower are in reverse order on the stack
#if BUILD_64												
				uint32 ht_addr_upper = mStackTop[0];
				uint32 ht_addr_lower = mStackTop[1];
                uint64* void_ptr_64 = kPointer64FromUInt32(ht_addr_upper, ht_addr_lower);
                return void_ptr_64;
#else
                return ((void*)(*mStackTop));
#endif				
            }

			return ((void*)mStackTop);
		}

        // -- doesn't remove the top of the stack, and doesn't assert if the stack is empty
		void* Peek(eVarType& contenttype, int depth = 0)
        {
            uint32* cur_stack_top = mStackTop;
            while (depth >= 0)
            {
			    uint32 stacksize = kPointerDiffUInt32(mStackTop, mStack) / sizeof(uint32);
                if (stacksize == 0)
                    return (NULL);

			    contenttype = (eVarType)(*(--cur_stack_top));

                // -- if what's on the stack isn't a valid content type, leave the stack alone, but
                // -- return NULL - the calling operation should catch the NULL and assert
                if (contenttype < 0 || contenttype >= TYPE_COUNT)
                    return (NULL);

			    // -- ensure we have enough data on the stack, both the content, and the type
                uint32 contentsize = kBytesToWordCount(gRegisteredTypeSize[contenttype]);
                Assert_(stacksize >= contentsize + 1);
			    cur_stack_top -= contentsize;

                // -- pushing and popping strings onto the execstack need to be refcounted
                // -- peeking, however, does not alter either the stack, or the string table

                // -- if we're digging past the top of the stack, keep loopoing
                --depth;
            }

			return ((void*)cur_stack_top);
		}

        void Reserve(int32 wordcount)
        {
            uint32* cur_stack_top = mStackTop;
            mStackTop += wordcount;
            if (wordcount > 0)
            {
                memset(cur_stack_top, 0, sizeof(uint32) * wordcount);
            }
        }

        void UnReserve(int32 wordcount)
        {
            mStackTop -= wordcount;
        }

        // -- this method is for recovery only...  the VM will probably continue correctly,
        // -- and this will prevent the stack from leaking, assuming the leak is within a
        // -- function call
        void ForceStackTop(int32 new_stack_top)
        {
            int32 cur_stack_top = (kPointerDiffUInt32(mStackTop, mStack) / sizeof(uint32));
            if (new_stack_top < cur_stack_top)
            {
                ScriptAssert_(TinScript::GetContext(), 0, "<internal>", -1,
                              "Error - attempting to *increase* the stack top, which creates garbage.\n");
                return;
            }
            mStackTop = mStack + new_stack_top;
        }

        int32 GetStackTop()
        {
            return (kPointerDiffUInt32(mStackTop, mStack) / sizeof(uint32));
        }

        void* GetStackVarAddr(int32 varstacktop, int32 varoffset) const
        {
            uint32* varaddr = &mStack[varstacktop];

            // -- increment by (varoffset * MAX_TYPE_SIZE), so we're pointing
            // -- at the start of the memory block for the variable.
            varaddr += varoffset * MAX_TYPE_SIZE;

            // -- validate and return the addr
            if (varaddr < mStack || varaddr >= mStackTop)
            {
                ScriptAssert_(GetContextOwner(), 0, "<internal>", -1,
                              "Error - GetStackVarAddr() out of range\n");
                return NULL;
            }
            return (varaddr);
        }

        void DebugDump(CScriptContext* script_context)
        {
            uint32* stacktop_ptr = mStackTop;
            while (stacktop_ptr > mStack)
            {
                eVarType contenttype = (eVarType)(*(--stacktop_ptr));
                assert(contenttype >= 0 && contenttype < TYPE_COUNT);
                uint32 contentsize = kBytesToWordCount(gRegisteredTypeSize[contenttype]);

                // -- ensure we have enough data on the stack, both the content, and the type
                stacktop_ptr -= contentsize;

                // -- Print out whatever it was we found
                TinPrint(script_context, "STACK: %s\n", DebugPrintVar(stacktop_ptr, contenttype));
            }
        }

	private:
        CScriptContext* mContextOwner;

        uint32 mStackStorage[kExecStackSize];
		uint32* mStack;
		uint32 mSize;

		uint32* mStackTop;
};

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

#include "registrationexecs.h"

// ====================================================================================================================
// EOF
// ====================================================================================================================
