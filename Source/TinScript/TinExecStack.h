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

#pragma once

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
        CExecStack();
		virtual ~CExecStack() { }

        CScriptContext* GetContextOwner() const { return (mContextOwner); }

        bool Push(void* content, eVarType contenttype);
        void* Pop(eVarType& contenttype);

        // -- doesn't remove the top of the stack, and doesn't assert if the stack is empty
        void* Peek(eVarType& contenttype, int depth = 0);

        void Reserve(int32 wordcount);
        void UnReserve(int32 wordcount, int32 prev_stack_top_count);

        // -- this method is for recovery only...  the VM will probably continue correctly,
        // -- and this will prevent the stack from leaking, assuming the leak is within a
        // -- function call
        void ForceStackTop(int32 new_stack_top);

        // -- this includes the space reserved for local vars, including reserved local space
        int32 GetStackTop();

        void* GetStackVarAddr(int32 varstacktop, int32 varoffset) const;

        int DebugPrintStack(bool depth_only = false);

	private:
        CScriptContext* mContextOwner;

        uint32 mStackStorage[kExecStackSize];
		uint32* mStack = nullptr;
		uint32 mSize = 0;

		uint32* mStackTop = nullptr;
        uint32* mStackTopReserve = nullptr;
};

}  // TinScript

// ====================================================================================================================
// EOF
// ====================================================================================================================
