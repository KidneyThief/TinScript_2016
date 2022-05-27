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

// -- class include
#include "TinExecStack.h"

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


// class CExecStack ---------------------------------------------------------------------------------------------------

// ====================================================================================================================
//  constructor
// ====================================================================================================================
CExecStack::CExecStack()
{
    mContextOwner = TinScript::GetContext();
    mSize = kExecStackSize;
    mStack = mStackStorage;
    mStackTop = mStack;
}

// ====================================================================================================================
// Push():  Pushes an entry onto the execution stack by contenttype (which will determine the byte count pushed)
// ====================================================================================================================
bool CExecStack::Push(void* content, eVarType contenttype)
{
    if (CScriptContext::gDebugExecStack)
    {
        // -- get the depth only
        int depth = DebugPrintStack(true);

        // -- Print out whatever it was we found
        TinPrint(mContextOwner, "    >>> [%d] Stack PUSH: %s\n", depth + 1, DebugPrintVar(content, contenttype, true));
    }

    // -- note:  this can happen if, e.g., you try to push an array value, where the index is
    // out of scope...  asserts/recovery needs to happen in the VM
    if (content == NULL)
    {
        TinPrint(TinScript::GetContext(), "Error - CExecStack::Push() null content\n");
        return (false);
    }

	uint32 contentsize = kBytesToWordCount(gRegisteredTypeSize[contenttype]);

    uint32 stacksize = kPointerDiffUInt32(mStackTop, mStack) / sizeof(uint32);
    if (stacksize + contentsize > kExecStackSize)
    {
        TinPrint(TinScript::GetContext(), "Error - stack overflow (size: %d) - unrecoverable\n", kExecStackSize);
        assert(0);
        return (false);
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
	*mStackTop = (uint32)contenttype;
    mStackTop++;

    // -- pushing and popping strings onto the execstack need to be refcounted
    if (contenttype == TYPE_string)
    {
        uint32 string_hash = *(uint32*)content;
        mContextOwner->GetStringTable()->RefCountIncrement(string_hash);
    }

    // -- success
    return (true);
}

// ====================================================================================================================
// Pop():  Pops an entry from the execution stack, returns the type and a void* to the value address
// ====================================================================================================================
void* CExecStack::Pop(eVarType& contenttype)
{
    if (CScriptContext::gDebugExecStack)
    {
        // -- get the depth only
        int depth = DebugPrintStack(true);

        // -- Print the stack top, before we pop
        eVarType dbg_content_type;
        void* dbg_content = Peek(dbg_content_type);
        TinPrint(mContextOwner, "    <<< [%d] Stack POP: %s\n", depth - 1, DebugPrintVar(dbg_content, dbg_content_type, true));
    }

	uint32 stacksize = kPointerDiffUInt32(mStackTop, mStackTopReserve) / sizeof(uint32);
    Unused_(stacksize);
    if (stacksize == 0)
    {
        ScriptAssert_(TinScript::GetContext(), 0, "<internal>", -1,
                        "Error - attempting to pop a value off an empty stack\n");
        return (nullptr);
    }

    --mStackTop;
    contenttype = (eVarType)(*mStackTop);

    // -- if what's on the stack isn't a valid content type, leave the stack alone, but
    // -- return NULL - the calling operation should catch the NULL and assert
    if (contenttype < 0 || contenttype >= TYPE_COUNT)
    {
        ++mStackTop;
        return (nullptr);
    }

	// -- ensure we have enough data on the stack, both the content, and the type
    uint32 contentsize = kBytesToWordCount(gRegisteredTypeSize[contenttype]);
    if (stacksize < contentsize + 1)
    {
        // -- assert, and leave the stack untouched (possibly we're "popping" into
        // local var storage???
        ++mStackTop;
        TinWarning(mContextOwner, "Pop(): Error - the stack doens't contain data to pop content type %s\n",
                                    GetRegisteredTypeName(contenttype));
        return (nullptr);
    }

    Assert_(stacksize >= contentsize + 1);
	mStackTop -= contentsize;
    assert(mStackTop >= mStack);

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

// ====================================================================================================================
// Peek(): doesn't remove the top of the stack, and doesn't assert if the stack is empty
// ====================================================================================================================
void* CExecStack::Peek(eVarType& contenttype, int depth)
{
    uint32* cur_stack_top = mStackTop;
    while (depth >= 0)
    {
		uint32 stacksize = kPointerDiffUInt32(cur_stack_top, mStackTopReserve) / sizeof(uint32);
        if (stacksize == 0)
            return (nullptr);

		contenttype = (eVarType)(*(--cur_stack_top));

        // -- if what's on the stack isn't a valid content type, leave the stack alone, but
        // -- return NULL - the calling operation should catch the NULL and assert
        if (contenttype < 0 || contenttype >= TYPE_COUNT)
            return (nullptr);

		// -- ensure we have enough data on the stack, both the content, and the type
        uint32 contentsize = kBytesToWordCount(gRegisteredTypeSize[contenttype]);
        if (stacksize < contentsize + 1)
        {
            TinWarning(mContextOwner, "Peek(): Error - the stack doens't contain data to pop content type %s\n",
                                        GetRegisteredTypeName(contenttype));
            return (nullptr);
        }

		cur_stack_top -= contentsize;

        // -- pushing and popping strings onto the execstack need to be refcounted
        // -- peeking, however, does not alter either the stack, or the string table

        // -- if we're digging past the top of the stack, keep looping
        --depth;
    }

	return ((void*)cur_stack_top);
}

// ====================================================================================================================
// Reserve():  reserves space for local variables, sets the reserve top so we don't "Pop" below reserved space
// ====================================================================================================================
void CExecStack::Reserve(int32 wordcount)
{
    uint32* cur_stack_top = mStackTop;
    mStackTop += wordcount;
    if (wordcount > 0)
    {
        memset(cur_stack_top, 0, sizeof(uint32) * wordcount);
    }
    mStackTopReserve = mStackTop;
}

// ====================================================================================================================
// UnReserve():  reduces the stack top to un-reserve the space that was used for local variables
// ====================================================================================================================
void CExecStack::UnReserve(int32 wordcount, int32 prev_stack_top_count)
{
    mStackTop -= wordcount;
    mStackTopReserve = mStack + prev_stack_top_count;
}

// ====================================================================================================================
// ForceStackTop():  Forces the stack top, essentially an unreserve, but also past any pushed values
// -- this method is for recovery only...  the VM will probably continue correctly,
// -- and this will prevent the stack from leaking, assuming the leak is within a function call
// -- Examples of where this is needed:
// simply referencing a variable without an assignment:  x;
// this value could be left harmlessly on the stack and there's nothing to pop it - so we force pop when the function
// returns, so the stack top is where the previous function left it, before the call  (probably at the reserve top)
// ====================================================================================================================
void CExecStack::ForceStackTop(int32 new_stack_top)
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

// ====================================================================================================================
// GetStackTop(): returns the stack top, includes the space reserved for local vars
// ====================================================================================================================
int32 CExecStack::GetStackTop()
{
    return (kPointerDiffUInt32(mStackTop, mStack) / sizeof(uint32));
}

// ====================================================================================================================
// GetStackVarAddr():  local vars contain an offset, so their actual storage is wherever the (stack top + offset) is
// ====================================================================================================================
void* CExecStack::GetStackVarAddr(int32 varstacktop, int32 varoffset) const
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

// ====================================================================================================================
// DebugPrintStack():  dumps the contents of the stack for debugging
// ====================================================================================================================
int CExecStack::DebugPrintStack(bool depth_only)
{
    // -- this should be using GetStackEntry, so we can dereference stack vars
    //tStackEntry stack_entry;
    //bool8 GetStackEntry(CScriptContext * script_context, CExecStack & execstack,
    //    CFunctionCallStack & funccallstack, tStackEntry & stack_entry, bool peek, int depth)

    int32 depth = 0;
    eVarType content_type = TYPE_void;
    void* content_ptr = Peek(content_type);
    while (content_ptr != nullptr)
    {
        // -- Print out whatever it was we found
        if (!depth_only)
        {
            TinPrint(GetContext(), "STACK: %s\n", DebugPrintVar(content_ptr, content_type, true));
        }

        // -- next
        content_ptr = Peek(content_type, ++depth);
    }

    return depth;
}

}  // TinScript

// -- eof -------------------------------------------------------------------------------------------------------------
