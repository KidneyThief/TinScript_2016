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
// TinOpExecFunctions.h
// ====================================================================================================================

#pragma once

// -- includes

#include "integration.h"
#include "TinCompile.h"

// == namespace TinScript =============================================================================================

namespace TinScript
{

typedef bool8 (*OpExecuteFunction)(CCodeBlock* cb, eOpCode op, const uint32*& instrptr,
                                   CExecStack& execstack, CFunctionCallStack& funccallstack);

extern OpExecuteFunction gOpExecFunctions[OP_COUNT];

void DebugTrace(eOpCode opcode, const char* fmt, ...);

void* GetStackVarAddr(CScriptContext* script_context, const CExecStack& execstack,
                      const CFunctionCallStack& funccallstack, int32 stackvaroffset);

bool8 GetStackValue(CScriptContext* script_context, CExecStack& execstack,
                    CFunctionCallStack& funccallstack, void*& valaddr, eVarType& valtype,
                    CVariableEntry*& ve, CObjectEntry*& oe);

bool8 GetBinOpValues(CScriptContext* script_context, CExecStack& execstack,
                     CFunctionCallStack& funccallstack,
                     void*& val0, eVarType& val0type,
                      void*& val1, eVarType& val1type);

bool8 ObjectNumericalBinOp(CScriptContext* script_context, eOpCode op, eVarType val0type,
                           void* val0addr, eVarType val1type, void* val1addr, int32& result);

bool8 IntegerNumericalBinOp(CScriptContext* script_context, eOpCode op, eVarType val0type,
                            void* val0, eVarType val1type, void* val1, int32& int_result);

bool8 PerformIntegerBinOp(CScriptContext* script_context, CExecStack& execstack,
                          CFunctionCallStack& funccallstack, eOpCode op, int32& int_result);

bool8 PerformNumericalBinOp(CScriptContext* script_context, CExecStack& execstack,
                            CFunctionCallStack& funccallstack, eOpCode op, int32& int_result,
                            float32& float_result);

bool8 PerformIntegerBitwiseOp(CScriptContext* script_context, CExecStack& execstack,
                              CFunctionCallStack& funccallstack, eOpCode op, int32& result);

bool8 PerformAssignOp(CScriptContext* script_context, CExecStack& execstack,
                      CFunctionCallStack& funccallstack, eOpCode op);

bool8 PerformBitAssignOp(CScriptContext* script_context, CExecStack& execstack,
                         CFunctionCallStack& funccallstack, eOpCode op);

bool8 PerformUnaryOp(CScriptContext* script_context, CExecStack& execstack,
                     CFunctionCallStack& funccallstack, eOpCode op);

// ====================================================================================================================
// -- use a macro to declare the function prototypes for each of the OpExec functions
#define OperationEntry(a) bool8 OpExec##a(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack, CFunctionCallStack& funccallstack);
OperationTuple
#undef OperationEntry

}  // TinScript

// ====================================================================================================================
// EOF
// ====================================================================================================================
