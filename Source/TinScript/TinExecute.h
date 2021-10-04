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

        CScriptContext* GetContextOwner() { return (mContextOwner); }

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
			    uint32* contentptr = (uint32*)content;
                *mStackTop++ = (uint32)contentptr;
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
                return ((void*)(*mStackTop));
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

        void* GetStackVarAddr(int32 varstacktop, int32 varoffset)
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
		CFunctionCallStack()
        {
            m_functionEntryStack = (tFunctionCallEntry*)m_functionStackStorage;
            m_size = kExecFuncCallDepth;
			m_stacktop = 0;

            // -- debugger members
            mDebuggerBreakStep = false;
            mDebuggerLastBreak = -1;
            mDebuggerObjectDeleted = 0;
            mDebuggerFunctionReload = 0;
            mDebuggerBreakOnStackDepth = -1;
		}

		virtual ~CFunctionCallStack()
        {
		}

		void Push(CFunctionEntry* functionentry, CObjectEntry* objentry, int32 varoffset)
		{
			assert(functionentry != NULL);
            assert(m_stacktop < m_size);
            m_functionEntryStack[m_stacktop].objentry = objentry;
            m_functionEntryStack[m_stacktop].funcentry = functionentry;
            m_functionEntryStack[m_stacktop].stackvaroffset = varoffset;
            m_functionEntryStack[m_stacktop].isexecuting = false;
            m_functionEntryStack[m_stacktop].mLocalObjectCount = 0;
            ++m_stacktop;
		}

		CFunctionEntry* Pop(CObjectEntry*& objentry, int32& var_offset)
        {
            assert(m_stacktop > 0);
            objentry = m_functionEntryStack[m_stacktop - 1].objentry;
            var_offset = m_functionEntryStack[m_stacktop - 1].stackvaroffset;

			// -- any time we pop a function call, we auto-destroy the local objects
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

   		CFunctionEntry* GetTop(CObjectEntry*& objentry, int32& varoffset)
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
                                   uint32* linenumber_array, int32 max_array_size) const;

        int32 DebuggerGetStackVarEntries(CScriptContext* script_context, CExecStack& execstack,
                                         CDebuggerWatchVarEntry* entry_array, int32 max_array_size);

		bool DebuggerFindStackTopVar(CScriptContext* script_context, CExecStack& execstack, uint32 var_hash,
								     CDebuggerWatchVarEntry& watch_entry, CVariableEntry*& ve);

        void DebuggerNotifyFunctionDeleted(CObjectEntry* oe, CFunctionEntry* fe);

        void BeginExecution(const uint32* instrptr);
        void BeginExecution();

        CFunctionEntry* GetExecuting(CObjectEntry*& objentry, int32& varoffset)
        {
            int32 temp = m_stacktop - 1;
            while (temp >= 0)
            {
                if (m_functionEntryStack[temp].isexecuting)
                {
                    objentry = m_functionEntryStack[temp].objentry;
                    varoffset = m_functionEntryStack[temp].stackvaroffset;
                    return m_functionEntryStack[temp].funcentry;
                }
                --temp;
            }
            return (NULL);
        }

   		CFunctionEntry* GetTopMethod(CObjectEntry*& objentry)
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

        struct tFunctionCallEntry
        {
            tFunctionCallEntry(CFunctionEntry* _funcentry = NULL, CObjectEntry* _objentry = NULL,
                               int32 _varoffset = -1)
            {
                funcentry = _funcentry;
                objentry = _objentry;
                stackvaroffset = _varoffset;
                linenumberfunccall = 0;
                isexecuting = false;
				mLocalObjectCount = 0;
            }

            CFunctionEntry* funcentry;
            CObjectEntry* objentry;
            int32 stackvaroffset;
            uint32 linenumberfunccall;
            bool8 isexecuting;
			int32 mLocalObjectCount;
			uint32 mLocalObjectIDList[kExecFuncCallMaxLocalObjects];
        };

        // -- because we can have multiple virtual machines running,
        // -- the debugger (break, line) members must be stored per execution stack
        bool8 mDebuggerBreakStep;
        int32 mDebuggerLastBreak;
        uint32 mDebuggerObjectDeleted;
        uint32 mDebuggerFunctionReload;

        // -- to manage stepping over/out, we might need to track which stack depth is appropriate to break on
        int32 mDebuggerBreakOnStackDepth;

	private:
        char m_functionStackStorage[sizeof(tFunctionCallEntry) * kExecFuncCallDepth];
        tFunctionCallEntry* m_functionEntryStack;
		int32 m_size;
		int32 m_stacktop;
};

bool8 ExecuteCodeBlock(CCodeBlock& codeblock);
bool8 ExecuteScheduledFunction(CScriptContext* script_context, uint32 objectid, uint32 ns_hash, uint32 funchash,
                               CFunctionContext* parameters);
bool8 CodeBlockCallFunction(CFunctionEntry* fe, CObjectEntry* oe, CExecStack& execstack,
                            CFunctionCallStack& funccallstack, bool copy_stack_parameters);

bool8 DebuggerBreakLoop(CCodeBlock* cb, const uint32* instrptr, CExecStack& execstack,
                        CFunctionCallStack& funccallstack, const char* assert_msg = NULL);

bool8 DebuggerAssertLoop(const char* condition, CCodeBlock* cb, const uint32* instrptr, CExecStack& execstack,
                         CFunctionCallStack& funccallstack, const char* fmt, ...);

bool8 DebuggerFindStackTopVar(CScriptContext* script_context, uint32 var_hash, CDebuggerWatchVarEntry& watcn_entry,
							  CVariableEntry*& ve);

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
