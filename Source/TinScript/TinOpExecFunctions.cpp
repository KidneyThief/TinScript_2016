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
// TinOpExecFunctions.cpp : Implementation of the virtual machine operations.
// ====================================================================================================================

// -- lib includes
#include <assert.h>
#include <string.h>
#include <stdio.h>

// -- includes
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
// DebugTrace():  Prints a text version of the operations being executed by the virtual machine.
// ====================================================================================================================
#if DEBUG_TRACE

void DebugTrace(eOpCode opcode, const char* fmt, ...)
{
    if (!CScriptContext::gDebugTrace)
        return;
    // -- expand the formated buffer
    va_list args;
    va_start(args, fmt);
    char tracebuf[kMaxTokenLength];
    vsprintf_s(tracebuf, kMaxTokenLength, fmt, args);
    va_end(args);

    printf("OP [%s]: %s\n", GetOperationString(opcode), tracebuf);
}

#else

void VoidFunction() {}
#define DebugTrace(...) VoidFunction()

#endif

// ====================================================================================================================
// GetStackVarAddr():  Get the address of a stack veriable, given the actual variable entry
// ====================================================================================================================
void* GetStackVarAddr(CScriptContext* script_context, CExecStack& execstack, CFunctionCallStack& funccallstack,
                      CVariableEntry& ve, int32 array_var_index)
{
    // -- ensure the variable is a stack variable
    if (!ve.IsStackVariable(funccallstack))
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - GetStackVarAddr() failed\n");
        return NULL;
    }

    int32 executing_stacktop = 0;
    CObjectEntry* oe = NULL;
    CFunctionEntry* fe_executing = funccallstack.GetExecuting(oe, executing_stacktop);

    int32 calling_stacktop = 0;
    CFunctionEntry* fe_top = funccallstack.GetTop(oe, calling_stacktop);

    CFunctionEntry* use_fe = ve.IsParameter() && fe_top && ve.GetFunctionEntry() == fe_top ? fe_top :
                             fe_executing && ve.GetFunctionEntry() == fe_executing ? fe_executing :
                             NULL;

    int32 use_stacktop = ve.IsParameter() && fe_top && ve.GetFunctionEntry() == fe_top ? calling_stacktop :
                             fe_executing && ve.GetFunctionEntry() == fe_executing ? executing_stacktop :
                             0;

    if (!use_fe || ve.GetStackOffset() < 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - GetStackVarAddr() failed\n");
        return NULL;
    }

    void* varaddr = execstack.GetStackVarAddr(use_stacktop, ve.GetStackOffset());

    // -- see if this is an array
    if (varaddr && ve.IsArray() && array_var_index > 0)
    {
        if (array_var_index >= ve.GetArraySize())
        {
            ScriptAssert_(script_context, 0, "<internal>", -1, "Error - Array index out of range: %s[%d]\n",
                          UnHash(ve.GetHash()), array_var_index);
            return NULL;
        }

        // -- offset the address by the array index - all variables on the stack, including arrays
        // -- are reserved the max type size amount of space.
        varaddr = (void*)((char*)varaddr + (gRegisteredTypeSize[ve.GetType()] * array_var_index));
    }

    return (varaddr);
}

// ====================================================================================================================
// GetStackVarAddr():  Get the address of a stack veriable, given the local variable index
// ====================================================================================================================
void* GetStackVarAddr(CScriptContext* script_context, CExecStack& execstack, CFunctionCallStack& funccallstack,
                      int32 stackvaroffset)
{
    int32 stacktop = 0;
    CObjectEntry* oe = NULL;
    CFunctionEntry* fe = funccallstack.GetExecuting(oe, stacktop);
    if (!fe || stackvaroffset < 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - GetStackVarAddr() failed\n");
        return NULL;
    }

    void* varaddr = execstack.GetStackVarAddr(stacktop, stackvaroffset);
    return varaddr;
}

// ====================================================================================================================
// GetStackValue():  From an exec stack entry, extract the type, value, variable, and/or object values. 
// ====================================================================================================================
bool8 GetStackValue(CScriptContext* script_context, CExecStack& execstack,
                    CFunctionCallStack& funccallstack, void*& valaddr, eVarType& valtype,
                    CVariableEntry*& ve, CObjectEntry*& oe)
{
    // -- we'll always return a value, but if that comes from a var or an object member,
    // -- return those as well
    ve = NULL;
    oe = NULL;

	// -- if a variable was pushed, use the var addr instead
	if (valtype == TYPE__var || valtype == TYPE__hashvarindex)
    {
        uint32 val1ns = ((uint32*)valaddr)[0];
        uint32 val1func = ((uint32*)valaddr)[1];
        uint32 val1hash = ((uint32*)valaddr)[2];

        // -- one more level of dereference for variables that are actually hashtables or arrays
        int32 ve_array_hash_index = (valtype == TYPE__hashvarindex) ? ((int32*)valaddr)[3] : 0;

        // -- this method will return the object, if the 4x parameters resolve to an object member
        ve = GetObjectMember(script_context, oe, val1ns, val1func, val1hash, ve_array_hash_index);

        // -- if not, search for a global/local variable
        if (!ve)
		    ve = GetVariable(script_context, script_context->GetGlobalNamespace()->GetVarTable(), val1ns, val1func,
                             val1hash, ve_array_hash_index);

        // -- if we still haven't found the variable, assert and fail
        if (!ve)
        {
            ScriptAssert_(script_context, 0, "<internal>", -1,
                          "Error - Unable to find variable %d\n", UnHash(val1hash));
            return (false);
        }

        // -- set the type
        valtype = ve->GetType();

        // -- if the ve belongs to a function, and is not a hash table or parameter array, we need
        // -- to find the stack address, as all local variable live on the stack
        if (ve && ve->IsStackVariable(funccallstack))
        {
            valaddr = GetStackVarAddr(script_context, execstack, funccallstack, *ve, ve_array_hash_index);
        }
        else
        {
            // -- if the variable is not a hashtable, but an array, we need to get the address of the array element
            if (ve->IsArray())
                valaddr = ve->GetArrayVarAddr(oe ? oe->GetAddr() : NULL, ve_array_hash_index);
            else
		        valaddr = ve->GetAddr(oe ? oe->GetAddr() : NULL);
        }
	}

	// -- if a member was pushed, use the var addr instead
    else if (valtype == TYPE__member)
    {
        uint32 varhash = ((uint32*)valaddr)[0];
        uint32 varsource = ((uint32*)valaddr)[1];

        // -- find the object
        oe = script_context->FindObjectEntry(varsource);
        if (!oe)
        {
            ScriptAssert_(script_context, 0, "<internal>", -1,
                          "Error - Unable to find object %d\n", varsource);
            return false;
        }

        // -- find the variable entry from the object's namespace variable table
        ve = oe->GetVariableEntry(varhash);
        if (!ve)
            return (false);

        valaddr = ve->GetAddr(oe->GetAddr());
		valtype = ve->GetType();
    }

    // -- if a stack variable was pushed...
    else if (valtype == TYPE__stackvar)
    {
        // -- we already know to do a stackvar lookup - replace the var with the actual value type
        valtype = (eVarType)((uint32*)valaddr)[0];
        int32 stackvaroffset = ((uint32*)valaddr)[1];
        int32 local_var_index = ((uint32*)valaddr)[2];

        // -- get the corresponding stack variable
        int32 stacktop = 0;
        CObjectEntry* oe = NULL;
        CFunctionEntry* fe = funccallstack.GetExecuting(oe, stacktop);
        if (!fe)
            return (false);

        // -- would be better to have random access to a hash table
        tVarTable* var_table = fe->GetContext()->GetLocalVarTable();
        int32 var_index = 0;
        ve = var_table->First();
        while (ve && var_index < local_var_index)
        {
            ve = var_table->Next();
            ++var_index;
        }

        // -- if we're pulling a stack var of type_hashtable, then the hash table isn't a "value" that can be
        // -- modified locally, but rather it lives in the function context, and must be manually emptied
        // -- as part of "ClearParameters"
        if (valtype != TYPE_hashtable)
        {
            valaddr = GetStackVarAddr(script_context, execstack, funccallstack, ve->GetStackOffset());
            if (!valaddr)
            {
                ScriptAssert_(script_context, 0, "<internal>", -1, "Error - Unable to find stack var\n");
                return false;
            }

		    // -- if we have a debugger attached, also find the variable entry associated with the stack var
		    int32 debugger_session = 0;
		    if (script_context->IsDebuggerConnected(debugger_session))
		    {
			    int32 stacktop = 0;
			    CObjectEntry* oe = NULL;
			    CFunctionEntry* fe = funccallstack.GetExecuting(oe, stacktop);
			    if (fe && fe->GetLocalVarTable())
			    {
				    // -- find the variable with the matching stackvaroffset
				    tVarTable* vartable = fe->GetLocalVarTable();
				    CVariableEntry* test_ve = vartable->First();
				    while (test_ve)
				    {
					    if (test_ve->GetStackOffset() == stackvaroffset)
					    {
						    ve = test_ve;
						    break;
					    }
					    test_ve = vartable->Next();
				    }
			    }
		    }
        }

        // -- else it is a hash table... find the ve in the function context
        else
        {
            // -- ensure the offset is within range of the local variable stack space
            if (stackvaroffset >= fe->GetContext()->CalculateLocalVarStackSize())
                return (false);

            // -- ensure the variable we found *is* a hash table
            if (!ve || ve->GetType() != TYPE_hashtable)
            {
                ScriptAssert_(script_context, 0, "<internal>", -1,
                              "Error - Unable to find stack var of type hashtable\n");
                return (false);
            }

            // -- otherwise, adjust the value address to be the actual hashtable
            valaddr = ve->GetAddr(oe ? oe->GetAddr() : NULL);
        }
    }

    // -- if a POD member was pushed...
    else if (valtype == TYPE__podmember)
    {
        // -- the type and address of the variable/value has already been pushed
        valtype = (eVarType)((uint32*)valaddr)[0];
        valaddr = (void*)((uint32*)valaddr)[1];
    }

    // -- if the valtype wasn't either a var or a member, they remain unchanged
    return true;
}

// ====================================================================================================================
// GetBinOpValues():  Pull the top two stack entries, and get the type and address for each.
// ====================================================================================================================
bool8 GetBinOpValues(CScriptContext* script_context, CExecStack& execstack,
                     CFunctionCallStack& funccallstack, void*& val0, eVarType& val0type, void*& val1,
                     eVarType& val1type)
{
	// -- Note:  values come off the stack in reverse order
	// -- get the 2nd value
    CVariableEntry* ve1 = NULL;
    CObjectEntry* oe1 = NULL;
	val1 = execstack.Pop(val1type);
    if (!GetStackValue(script_context, execstack, funccallstack, val1, val1type, ve1, oe1))
        return false;

	// -- get the 1st value
    CVariableEntry* ve0 = NULL;
    CObjectEntry* oe0 = NULL;
	val0 = execstack.Pop(val0type);
    if (!GetStackValue(script_context, execstack, funccallstack, val0, val0type, ve0, oe0))
        return false;

	return true;
}

// ====================================================================================================================
// PerformBinaryOpPush():  
// This is to consolidate all the operations that pop two values from the stack and combine them,
// and pushing the result onto the stack.
// ====================================================================================================================
bool8 PerformBinaryOpPush(CScriptContext* script_context, CExecStack& execstack, CFunctionCallStack& funccallstack,
                          eOpCode op)
{
	// -- Get both args from the stacks
	eVarType val0type;
	void* val0 = NULL;
	eVarType val1type;
	void* val1 = NULL;
	if (!GetBinOpValues(script_context, execstack, funccallstack, val0, val0type, val1, val1type))
    {
		printf("Error - failed GetBinopValues() for operation: %s\n",
				GetOperationString(op));
		return false;
	}

    // -- see if there's an override for the given types
    // -- NOTE:  We test in type order - float preceeds int, etc...
    eVarType priority_type = val0type < val1type ? val0type : val1type;
    eVarType secondary_type = val0type < val1type ? val1type : val0type;
    TypeOpOverride priority_op_func = GetTypeOpOverride(op, priority_type);
    TypeOpOverride secondary_op_func = GetTypeOpOverride(op, secondary_type);

    // -- if we found an operation, see if it can be performed successfully
    char result[MAX_TYPE_SIZE * sizeof(uint32)];
    eVarType result_type;

    bool success = (priority_op_func && priority_op_func(script_context, op, result_type, (void*)result,
                                                         val0type, val0, val1type, val1));

    // -- if the priority version didn't pan out, try the secondary type version
    if (!success)
    {
        success = (secondary_op_func && secondary_op_func(script_context, op, result_type, (void*)result,
                                                          val0type, val0, val1type, val1));
    }

    // -- hopefully one of them worked
    if (success)
    {
        // -- push the result onto the stack
        execstack.Push((void*)result, result_type);
        DebugTrace(op, "%s", DebugPrintVar(result, result_type));

        return (true);
    }

    // -- failed
    return (false);
}

// ====================================================================================================================
// PerformAssignOp():  Consolidates all variations of the assignment operation execution.
// ====================================================================================================================
bool8 PerformAssignOp(CScriptContext* script_context, CExecStack& execstack, CFunctionCallStack& funccallstack,
                      eOpCode op)
{
    // -- if we're not doing a straight up assignment, we need to pop the variable and value off the stack,
    // -- so we can cache the variable to be modified by the result of the operation
    if (op != OP_Assign)
    {
	    eVarType assign_valtype;
	    void* assign_valaddr = execstack.Peek(assign_valtype, 1); // look at the 2nd stack entry (not the top)
        if (!assign_valaddr)
        {
            return (false);
        }

        // -- store the 2nd entry on the stack - it had better be a variable of some type,
        // -- and we'll want push it back on the stack for the assignment, after the operation
        uint32 assign_buf[MAX_TYPE_SIZE];
        memcpy(assign_buf, assign_valaddr, MAX_TYPE_SIZE * sizeof(uint32));

        // -- here we have to map between the assign version of the op, and the actual op
        eOpCode perform_op = op;
        switch (op)
        {
            case OP_AssignAdd:         perform_op = OP_Add;             break;
            case OP_AssignSub:         perform_op = OP_Sub;             break;
            case OP_AssignMult:        perform_op = OP_Mult;            break;
            case OP_AssignDiv:         perform_op = OP_Div;             break;
            case OP_AssignMod:         perform_op = OP_Mod;             break;
            case OP_AssignLeftShift:   perform_op = OP_BitLeftShift;    break;
            case OP_AssignRightShift:  perform_op = OP_BitRightShift;   break;
            case OP_AssignBitAnd:      perform_op = OP_BitAnd;          break;
            case OP_AssignBitOr:       perform_op = OP_BitOr;           break;
            case OP_AssignBitXor:      perform_op = OP_BitXor;          break;
            default:
                ScriptAssert_(script_context, 0, "<internal>", -1,
                              "Error - Assign operation not mapped to a binary op\n");
                return (false);
        }

        // -- if the operation isn't a simple assignement, we've got the variable to be assigned - perform the op
        // -- this will replace the top two stack entries, with the result
        if (!PerformBinaryOpPush(script_context, execstack, funccallstack, perform_op))
        {
            return (false);
        }

        // -- now we have the result, we need to pop it, then push the variable, then the result again
	    eVarType valtype;
	    void* valaddr = execstack.Pop(valtype);
        uint32 valbuf[MAX_TYPE_SIZE];
        if (!valaddr)
        {
            return (false);
        }
        memcpy(valbuf, valaddr, MAX_TYPE_SIZE * sizeof(uint32));

        // -- push the variable to be assigned, back on the stack
        execstack.Push((void*)assign_buf, assign_valtype);

        // -- push the operation result back onto the stack
        execstack.Push(valbuf, valtype);
    }

    // -- perform the assignment
	// -- pop the value
    CVariableEntry* ve1 = NULL;
    CObjectEntry* oe1 = NULL;
	eVarType val1type;
	void* val1addr = execstack.Pop(val1type);
    if (!GetStackValue(script_context, execstack, funccallstack, val1addr, val1type, ve1, oe1))
    {
        return false;
    }

	// -- pop the var
    CVariableEntry* ve0 = NULL;
    CObjectEntry* oe0 = NULL;
	eVarType varhashtype;
	void* var = execstack.Pop(varhashtype);
    bool8 is_stack_var = (varhashtype == TYPE__stackvar);
    bool8 is_pod_member = (varhashtype == TYPE__podmember);
    bool8 use_var_addr = (is_stack_var || is_pod_member);
    if (!GetStackValue(script_context, execstack, funccallstack, var, varhashtype, ve0, oe0))
    {
        return (false);
    }

    // -- if the variable is a local variable, we also have the actual address already
    use_var_addr = use_var_addr || (ve0 && ve0->IsStackVariable(funccallstack));

    // -- ensure we're assigning to a variable, an object member, or a local stack variable
    if (!ve0 && !use_var_addr)
    {
        return (false);
    }

    // -- if we've been given the actual address of the var, copy directly to it
    if (use_var_addr)
    {
        // -- we're not allowed to stomp local variables that are actually hashtables
        if (ve0 && ve0->GetType() == TYPE_hashtable && !ve0->IsParameter())
        {
            ScriptAssert_(script_context, 0, "<internal>", -1,
                          "Error - Assigning to hashtable var '%s' would stomp and leak memory\n",
                          UnHash(ve0->GetHash()));
            return (false);
        }

        val1addr = TypeConvert(script_context, val1type, val1addr, varhashtype);
        if (!val1addr)
        {
            ScriptAssert_(script_context, 0, "<internal>", -1,
                          "Error - fail to conver from type %s to type %s\n",
                          GetRegisteredTypeName(val1type), GetRegisteredTypeName(varhashtype));
            return (false);
        }
        memcpy(var, val1addr, gRegisteredTypeSize[varhashtype]);
        DebugTrace(op, is_stack_var ? "StackVar: %s" : "PODMember: %s", DebugPrintVar(var, varhashtype));

       	// -- if we're connected to the debugger, then the variable entry associated with the stack var will be returned,
		// -- notify we're breaking on it
		if (ve0)
			ve0->NotifyWrite(script_context, &execstack, &funccallstack);
    }

    // -- else set the value through the variable entry
    else
    {
        val1addr = TypeConvert(script_context, val1type, val1addr, ve0->GetType());
        if (!val1addr)
        {
            ScriptAssert_(script_context, 0, "<internal>", -1,
                          "Error - fail to convert from type %s to type %s\n",
                          GetRegisteredTypeName(val1type), GetRegisteredTypeName(ve0->GetType()));
            return (false);
        }

        // -- If the destination is an array parameter that has yet not been initialized,
        // -- then the first assignment to it happens when the function is called.
        // -- It would be preferable to initialize the parameter through a specific code path,
        // -- but since the assignment uses the same path, we use the test below and trust
        // -- the compiled code.  This implementation is 100% solid, just less preferable than
        // -- a deliberate initialization.
        if (ve0->IsParameter() && ve0->GetArraySize() == -1 && ve1 && ve1->IsArray() &&
            ve0->GetType() == ve1->GetType())
        {
            ve0->InitializeArrayParameter(ve1, oe1, execstack, funccallstack);
        }

        else if (!ve0->IsArray())
        {
    	    ve0->SetValue(oe0 ? oe0->GetAddr() : NULL, val1addr, &execstack, &funccallstack);
            DebugTrace(op, "Var %s: %s", UnHash(ve0->GetHash()),
                        DebugPrintVar(val1addr, ve0->GetType()));
        }
        else
        {
            // $$$TZA need a better way to determine the array index
            void* ve0_addr = ve0->GetAddr(oe0 ? oe0->GetAddr() : NULL);
            int32 byte_count = kPointerDiffUInt32(var, ve0_addr);
            int32 array_index = byte_count / gRegisteredTypeSize[ve0->GetType()];
            ve0->SetValue(oe0 ? oe0->GetAddr() : NULL, val1addr, &execstack, &funccallstack, array_index);
        }
    }

    // -- success
    return (true);
}

// ====================================================================================================================
// OpExecNULL():  NULL operation, should never be executed, and will trigger an assert.
// ====================================================================================================================
bool8 OpExecNULL(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                 CFunctionCallStack& funccallstack)
{
    DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                    "Error - OP_NULL is not a valid op, indicating an error in this codeblock: %s\n");
    return (false);
}

// ====================================================================================================================
// OpExecNOP():  NOP operation, benign.
// ====================================================================================================================
bool8 OpExecNOP(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                CFunctionCallStack& funccallstack)
{
    DebugTrace(op, "");
    return (true);
}

// ====================================================================================================================
// OpExecVarDecl():  Operation to declare a variable.
// ====================================================================================================================
bool8 OpExecVarDecl(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                    CFunctionCallStack& funccallstack)
{
    uint32 varhash = *instrptr++;
    eVarType vartype = (eVarType)(*instrptr++);
    int32 array_size = *instrptr++;

    // -- if we're in the middle of a function definition, the var is local
    // -- otherwise it's global... there are no nested function definitions allowed
    int32 stacktop = 0;
    CObjectEntry* oe = NULL;
    AddVariable(cb->GetScriptContext(), cb->GetScriptContext()->GetGlobalNamespace()->GetVarTable(),
                funccallstack.GetTop(oe, stacktop), UnHash(varhash), varhash, vartype, array_size);
    DebugTrace(op, "Var: %s", UnHash(varhash));
    return (true);
}

// ====================================================================================================================
// OpExecParamDecl():  Parameter declaration.
// ====================================================================================================================
bool8 OpExecParamDecl(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                      CFunctionCallStack& funccallstack)
{
    uint32 varhash = *instrptr++;
    eVarType vartype = (eVarType)(*instrptr++);
    int32 array_size = *instrptr++;

    int32 stacktop = 0;
    CObjectEntry* oe = NULL;
    CFunctionEntry* fe = funccallstack.GetTop(oe, stacktop);
    assert(fe != NULL);

    fe->GetContext()->AddParameter(UnHash(varhash), varhash, vartype, array_size, 0);
    DebugTrace(op, "Var: %s", UnHash(varhash));
    return (true);
}

// ====================================================================================================================
// OpExecAssign():  Assignment operation.
// ====================================================================================================================
bool8 OpExecAssign(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                   CFunctionCallStack& funccallstack)
{
    if (!PerformAssignOp(cb->GetScriptContext(), execstack, funccallstack, op))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - unable to perform op: %s\nEnsure the variable exists, and the types are valid.\n.",
                        GetOperationString(op));
        return (false);
    }
    return (true);
}

// ====================================================================================================================
// OpExecAssignAdd():  Add Assignment operation.
// ====================================================================================================================
bool8 OpExecAssignAdd(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                      CFunctionCallStack& funccallstack)
{
    if (!PerformAssignOp(cb->GetScriptContext(), execstack, funccallstack, op))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - unable to perform op: %s\nEnsure the variable exists, and the types are valid.\n.",
                        GetOperationString(op));
        return (false);
    }
    return (true);
}

// ====================================================================================================================
// OpExecAssignSub():  Sub assignment operation.
// ====================================================================================================================
bool8 OpExecAssignSub(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                      CFunctionCallStack& funccallstack)
{
    if (!PerformAssignOp(cb->GetScriptContext(), execstack, funccallstack, op))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - unable to perform op: %s\nEnsure the variable exists, and the types are valid.\n.",
                        GetOperationString(op));
        return (false);
    }
    return (true);
}

// ====================================================================================================================
// OpExecAssignMult():  Mult assignment operation.
// ====================================================================================================================
bool8 OpExecAssignMult(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                       CFunctionCallStack& funccallstack)
{
    if (!PerformAssignOp(cb->GetScriptContext(), execstack, funccallstack, op))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - unable to perform op: %s\nEnsure the variable exists, and the types are valid.\n.",
                        GetOperationString(op));
        return (false);
    }
    return (true);
}

// ====================================================================================================================
// OpExecAssignDiv():  Div assignment operation.
// ====================================================================================================================
bool8 OpExecAssignDiv(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                      CFunctionCallStack& funccallstack)
{
    if (!PerformAssignOp(cb->GetScriptContext(), execstack, funccallstack, op))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - unable to perform op: %s\nEnsure the variable exists, and the types are valid.\n.",
                        GetOperationString(op));
        return (false);
    }
    return (true);
}

// ====================================================================================================================
// OpExecAssignMod():  Mod assignment operation.
// ====================================================================================================================
bool8 OpExecAssignMod(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                      CFunctionCallStack& funccallstack)
{
    if (!PerformAssignOp(cb->GetScriptContext(), execstack, funccallstack, op))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - unable to perform op: %s\nEnsure the variable exists, and the types are valid.\n.",
                        GetOperationString(op));
        return (false);
    }
    return (true);
}

// ====================================================================================================================
// OpExecAssignLeftShift():  Left shift assignment operation.
// ====================================================================================================================
bool8 OpExecAssignLeftShift(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                            CFunctionCallStack& funccallstack)
{
    if (!PerformAssignOp(cb->GetScriptContext(), execstack, funccallstack, op))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - unable to perform op: %s\nEnsure the variable exists, and the types are valid.\n.",
                        GetOperationString(op));
        return (false);
    }
    return (true);
}

// ====================================================================================================================
// OpExecAssignRightShift():  Right shift assignment operation.
// ====================================================================================================================
bool8 OpExecAssignRightShift(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                             CFunctionCallStack& funccallstack)
{
    if (!PerformAssignOp(cb->GetScriptContext(), execstack, funccallstack, op))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - unable to perform op: %s\nEnsure the variable exists, and the types are valid.\n.",
                        GetOperationString(op));
        return (false);
    }
    return (true);
}

// ====================================================================================================================
// OpExecAssignBitAnd():  Bit And assignment operation.
// ====================================================================================================================
bool8 OpExecAssignBitAnd(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                         CFunctionCallStack& funccallstack)
{
    if (!PerformAssignOp(cb->GetScriptContext(), execstack, funccallstack, op))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - unable to perform op: %s\nEnsure the variable exists, and the types are valid.\n.",
                        GetOperationString(op));
        return (false);
    }
    return (true);
}

// ====================================================================================================================
// OpExecAssignBitOr():  Bit Or assignment operation.
// ====================================================================================================================
bool8 OpExecAssignBitOr(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                        CFunctionCallStack& funccallstack)
{
    if (!PerformAssignOp(cb->GetScriptContext(), execstack, funccallstack, op))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - unable to perform op: %s\nEnsure the variable exists, and the types are valid.\n.",
                        GetOperationString(op));
        return (false);
    }
    return (true);
}

// ====================================================================================================================
// OpExecAssignBitXor():  Bit Xor assignment operation.
// ====================================================================================================================
bool8 OpExecAssignBitXor(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                         CFunctionCallStack& funccallstack)
{
    if (!PerformAssignOp(cb->GetScriptContext(), execstack, funccallstack, op))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - unable to perform op: %s\nEnsure the variable exists, and the types are valid.\n.",
                        GetOperationString(op));
        return (false);
    }
    return (true);
}

// ====================================================================================================================
// OpExecAssignPreInc():  Pre-increment unary operation.
// ====================================================================================================================
bool8 OpExecUnaryPreInc(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                        CFunctionCallStack& funccallstack)
{
    // $$$TZA For now, we do not allow chaining of assignments, which includes
    // -- int x = ++y;
    // -- unary preinc is the same as "x += 1", except we also push the variable back onto the stack
    // -- as it's also used as a value

    // -- first get the variable we're assigning
    eVarType assign_type;
	void* assign_var = execstack.Peek(assign_type);
    if (!assign_var)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - Failed to pop stack variable, performing: %s\n", GetOperationString(op));
        return (false);
    }
    // -- cache the variable - we'll want to push it's value after it has been incremented
    uint32 assign_buf[MAX_TYPE_SIZE];
    memcpy(assign_buf, assign_var, MAX_TYPE_SIZE * sizeof(uint32));

    // push a "1" onto the stack, and perform an AssignAdd
    int32 value = 1;
    execstack.Push(&value, TYPE_int);
    if (!PerformAssignOp(cb->GetScriptContext(), execstack, funccallstack, OP_AssignAdd))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - Failed to perform op: %s\n", GetOperationString(op));
        return (false);
    }

    // $$$TZA For now, we do not allow chaining of assignments, which includes
    // -- int x = ++y;
    // -- push the variable back onto the stack
    //execstack.Push((void*)assign_buf, assign_type);

    // -- success
    return (true);
}

// ====================================================================================================================
// OpExecAssignPreDec():  Pre-decrement unary operation.
// ====================================================================================================================
bool8 OpExecUnaryPreDec(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                        CFunctionCallStack& funccallstack)
{
    // $$$TZA For now, we do not allow chaining of assignments, which includes
    // -- int x = ++y;
    // -- unary predec is the same as "x += -1", except we also push the variable back onto the stack
    // -- as it's also used as a value

    // -- first get the variable we're assigning
    eVarType assign_type;
	void* assign_var = execstack.Peek(assign_type);
    if (!assign_var)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - Failed to pop stack variable, performing: %s\n", GetOperationString(op));
        return (false);
    }
    // -- cache the variable - we'll want to push it's value after it has been incremented
    uint32 assign_buf[MAX_TYPE_SIZE];
    memcpy(assign_buf, assign_var, MAX_TYPE_SIZE * sizeof(uint32));

    // push a "-1" onto the stack, and perform an AssignAdd
    int32 value = -1;
    execstack.Push(&value, TYPE_int);
    if (!PerformAssignOp(cb->GetScriptContext(), execstack, funccallstack, OP_AssignAdd))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - Failed to perform op: %s\n", GetOperationString(op));
        return (false);
    }

    // -- push the variable back onto the stack
    // $$$TZA For now, we do not allow chaining of assignments, which includes
    // -- int x = ++y;
    //execstack.Push((void*)assign_buf, assign_type);

    // -- success
    return (true);
}

// ====================================================================================================================
// OpExecAssignUnaryNeg():  Negate unary operation.
// ====================================================================================================================
bool8 OpExecUnaryNeg(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                     CFunctionCallStack& funccallstack)
{
    // -- we're going to push a -1 onto the stack, and then allow the type to perform a multiplication
    int32 value = -1;
    execstack.Push(&value, TYPE_int);
    DebugTrace(op, "%s", DebugPrintVar((void*)&value, TYPE_int));
    return (PerformBinaryOpPush(cb->GetScriptContext(), execstack, funccallstack, OP_Mult));
}

// ====================================================================================================================
// OpExecAssignUnaryPos():  Positive unary operation.  (actually has no side effects).
// ====================================================================================================================
bool8 OpExecUnaryPos(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                     CFunctionCallStack& funccallstack)
{
    // -- Unary pos has no effect on anything - leave the value on the stack "as is"
    return (true);
}

// ====================================================================================================================
// OpExecUnaryBitInvert():  Bit Invert unary operation.
// ====================================================================================================================
bool8 OpExecUnaryBitInvert(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                           CFunctionCallStack& funccallstack)
{
	// -- pop the value
    CVariableEntry* ve = NULL;
    CObjectEntry* oe = NULL;
	eVarType valtype;
	void* valaddr = execstack.Pop(valtype);
    if (!GetStackValue(cb->GetScriptContext(), execstack, funccallstack, valaddr, valtype, ve, oe))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - Failed pop value for op: %s\n", GetOperationString(op));
        return false;
    }

    // -- convert the value to an int (the only valid type a bit-invert operator is implemented for)
    void* convertaddr = TypeConvert(cb->GetScriptContext(), valtype, valaddr, TYPE_int);
    if (!convertaddr)
    {
          DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                          "Error - unable to convert from type %s to type TYPE_int, performing op: %s\n",
                          gRegisteredTypeNames[valtype], GetOperationString(op));
        return false;
    }

    int32 result = *(int32*)convertaddr;
    result = ~result;

    execstack.Push(&result, TYPE_int);
    DebugTrace(op, "%s", DebugPrintVar((void*)&result, TYPE_int));

    // -- success
    return (true);
}

// ====================================================================================================================
// OpExecUnaryUnaryNot():  Not unary operation.
// ====================================================================================================================
bool8 OpExecUnaryNot(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                     CFunctionCallStack& funccallstack)
{
	// -- pop the value
    CVariableEntry* ve = NULL;
    CObjectEntry* oe = NULL;
	eVarType valtype;
	void* valaddr = execstack.Pop(valtype);
    if (!GetStackValue(cb->GetScriptContext(), execstack, funccallstack, valaddr, valtype, ve, oe))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - Failed pop value for op: %s\n", GetOperationString(op));
        return false;
    }

    // -- convert the value to a bool (the only valid type a not operator is implemented for)
    void* convertaddr = TypeConvert(cb->GetScriptContext(), valtype, valaddr, TYPE_bool);
    if (!convertaddr)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - unable to convert from type %s to type TYPE_bool, performing op: %s\n",
                        gRegisteredTypeNames[valtype], GetOperationString(op));
        return false;
    }

    bool result = *(bool*)convertaddr;
    result = !result;

    execstack.Push(&result, TYPE_bool);
    DebugTrace(op, "%s", DebugPrintVar((void*)&result, TYPE_bool));

    // -- success
    return (true);
}

// ====================================================================================================================
// OpExecPush():  Push a var/value onto the execution stack.
// ====================================================================================================================
bool8 OpExecPush(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                 CFunctionCallStack& funccallstack)
{
    // -- next instruction is the type
    eVarType contenttype = (eVarType)(*instrptr);
    instrptr++;
    assert(contenttype >= 0 && contenttype < TYPE_COUNT);

    // -- push the the value onto the stack, and update the instrptr
    execstack.Push((void*)instrptr, contenttype);
    DebugTrace(op, "%s", DebugPrintVar((void*)instrptr, contenttype));

    // -- advance the instruction pointer
    int32 contentsize = kBytesToWordCount(gRegisteredTypeSize[contenttype]);
    instrptr += contentsize;
    return (true);
}

// ====================================================================================================================
// OpExecPushLocalVar():  Push a local variable onto the exec stack.
// ====================================================================================================================
bool8 OpExecPushLocalVar(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                         CFunctionCallStack& funccallstack)
{
    // -- next instruction is the variable hash followed by the function context hash
    execstack.Push((void*)instrptr, TYPE__stackvar);
    DebugTrace(op, "StackVar [%s : %d]", GetRegisteredTypeName((eVarType)instrptr[0]), instrptr[1]);

    // -- advance the instruction pointer
    int32 contentsize = kBytesToWordCount(gRegisteredTypeSize[TYPE__stackvar]);
    instrptr += contentsize;
    return (true);
}

// ====================================================================================================================
// OpExecPushLocalValue():  Push the value of a local variable onto the exec stack.
// ====================================================================================================================
bool8 OpExecPushLocalValue(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                           CFunctionCallStack& funccallstack)
{
    // -- next instruction is the type
    eVarType valtype = (eVarType)(*instrptr++);

    // -- next instruction is the stack offset
    int32 stackoffset = (int32)*instrptr++;

    // -- next instruction is the local var index
    int32 local_var_index = (int32)*instrptr++;

    // -- get the stack top for this function call
    void* stackvaraddr = GetStackVarAddr(cb->GetScriptContext(), execstack, funccallstack,
                                         stackoffset);
    if (!stackvaraddr)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - Unable to get StackVarAddr()\n");
        return false;
    }

    execstack.Push(stackvaraddr, valtype);
    DebugTrace(op, "StackVar [%d]: %s", stackoffset, DebugPrintVar(stackvaraddr, valtype));
    return (true);
}

// ====================================================================================================================
// OpExecPushGlobalVar():  Push a global variable onto the exec stack.
// ====================================================================================================================
bool8 OpExecPushGlobalVar(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                          CFunctionCallStack& funccallstack)
{
    // -- next instruction is the variable hash followed by the function context hash
    execstack.Push((void*)instrptr, TYPE__var);
    DebugTrace(op, "Var: %s", UnHash(instrptr[2]));

    // -- advance the instruction pointer
    int32 contentsize = kBytesToWordCount(gRegisteredTypeSize[TYPE__var]);
    instrptr += contentsize;
    return (true);
}

// ====================================================================================================================
// OpExecPushGlobalValue():  Push the value of a global variable onto the exec stack.
// ====================================================================================================================
bool8 OpExecPushGlobalValue(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                            CFunctionCallStack& funccallstack)
{
    // -- next instruction is the variable name
    uint32 nshash = *instrptr++;
    uint32 varfunchash = *instrptr++;
    uint32 varhash = *instrptr++;
    CVariableEntry* ve =
        GetVariable(cb->GetScriptContext(), cb->GetScriptContext()->GetGlobalNamespace()->GetVarTable(), nshash,
                    varfunchash, varhash, 0);
    if (!ve)
    {
        ScriptAssert_(cb->GetScriptContext(), 0, cb->GetFileName(), cb->CalcLineNumber(instrptr),
                      "Error - PushGlobalValue(): unable to find variable %d\n", UnHash(varhash));
        return false;
    }

    void* val = ve->GetAddr(NULL);
    eVarType valtype = ve->GetType();

    execstack.Push(val, valtype);
    DebugTrace(op, "Var: %s, %s", UnHash(varhash), DebugPrintVar(val, valtype));
    return (true);
}

// ====================================================================================================================
// OpExecPushArrayVar():  Push a variable belonging to a hashtable onto the exec stack.
// ====================================================================================================================
bool8 OpExecPushArrayVar(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                         CFunctionCallStack& funccallstack)
{
    // -- hash value will have already been pushed
    eVarType contenttype;
    void* contentptr = execstack.Pop(contenttype);
    if (contenttype != TYPE_int)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain TYPE_int\n");
        return false;
    }
    uint32 arrayvarhash = *(uint32*)contentptr;

    // -- next, pop the hash table variable off the stack
    // -- pull the hashtable variable off the stack
    CVariableEntry* ve0 = NULL;
    CObjectEntry* oe0 = NULL;
    eVarType val0type;
    void* val0 = execstack.Pop(val0type);
    if (!GetStackValue(cb->GetScriptContext(), execstack, funccallstack, val0, val0type, ve0, oe0))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain a hashtable variable\n");
        return false;
    }

    if (!ve0 || (ve0->GetType() != TYPE_hashtable && !ve0->IsArray()))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain hashtable or array variable\n");
        return false;
    }

    // -- now fill in the details of what we need to retrieve this variable:
    // -- if the ns hash is zero, then the next word is the object ID
    // -- if the ns hash is non-zero, then
    // --    the next word is non-zero means the var is a local var in a function
    // --    (note:  the ns hash could be "_global" for global functions)
    // --    else if the next word is zero, it's a global variable
    // -- the last two words are, the hash table variable name, and the hash value of the entry

    uint32 ns_hash = 0;
    uint32 func_or_obj = 0;
    uint32 var_hash = ve0->GetHash();

    // -- if this is an object member...
    if (oe0)
    {
        ns_hash = 0;
        func_or_obj = oe0->GetID();
    }

    // -- global hash table variable
    else if (!ve0->GetFunctionEntry())
    {
        ns_hash = CScriptContext::kGlobalNamespaceHash;
    }

    // -- function local variable
    else
    {
        ns_hash = ve0->GetFunctionEntry()->GetNamespaceHash();
        func_or_obj = ve0->GetFunctionEntry()->GetHash();
    }

    // -- push the hashvar (note:  could also be an index)
    uint32 arrayvar[4];
    arrayvar[0] = ns_hash;
    arrayvar[1] = func_or_obj;
    arrayvar[2] = var_hash;
    arrayvar[3] = arrayvarhash;

    // -- next instruction is the variable hash followed by the function context hash
    execstack.Push((void*)arrayvar, TYPE__hashvarindex);
    if (oe0)
        DebugTrace(op, "ArrayVar: %d.%s[%s], %s", oe0->GetID(), UnHash(var_hash), UnHash(arrayvarhash),
                   DebugPrintVar(ve0->GetAddr(oe0 ? oe0->GetAddr() : NULL), ve0->GetType()));
    else
        DebugTrace(op, "ArrayVar: %s[%s], %s", UnHash(var_hash), UnHash(arrayvarhash),
                   DebugPrintVar(ve0->GetAddr(oe0 ? oe0->GetAddr() : NULL), ve0->GetType()));

    return (true);
}

// ====================================================================================================================
// OpExecPushArrayValue():  Push the value of a variable belonging to a hashtable onto the exec stack.
// ====================================================================================================================
bool8 OpExecPushArrayValue(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                           CFunctionCallStack& funccallstack)
{
    // -- hash value will have already been pushed
    eVarType contenttype;
    void* contentptr = execstack.Pop(contenttype);
    if (contenttype != TYPE_int)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain TYPE_int\n");
        return false;
    }
    int32 arrayvarhash = *(int32*)contentptr;

    // -- next, pop the hash table variable off the stack
    // -- pull the hashtable variable off the stack
    CVariableEntry* ve0 = NULL;
    CObjectEntry* oe0 = NULL;
    eVarType val0type;
    void* val0 = execstack.Pop(val0type);
    if (!GetStackValue(cb->GetScriptContext(), execstack, funccallstack, val0, val0type, ve0, oe0))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain a hashtable variable\n");
        return false;
    }

    if (!ve0 || (ve0->GetType() != TYPE_hashtable && !ve0->IsArray()))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain hashtable variable\n");
        return false;
    }

    // -- now fill in the details of what we need to retrieve this variable:
    // -- if the ns hash is zero, then the next word is the object ID
    // -- if the ns hash is non-zero, then
    // --    the next word is non-zero means the var is a local var in a function
    // --    (note:  the ns hash could be "_global" for global functions)
    // --    else if the next word is zero, it's a global variable
    // -- the last two words are, the hash table variable name, and the hash value of the entry

    uint32 ns_hash = 0;
    uint32 func_or_obj = 0;
    uint32 var_hash = ve0->GetHash();

    // -- if this is an object member...
    if (oe0)
    {
        ns_hash = 0;
        func_or_obj = oe0->GetID();
    }

    // -- global hash table variable
    else if (!ve0->GetFunctionEntry())
    {
        ns_hash = CScriptContext::kGlobalNamespaceHash;
    }

    // -- function local variable
    else
    {
        ns_hash = ve0->GetFunctionEntry()->GetNamespaceHash();
        func_or_obj = ve0->GetFunctionEntry()->GetHash();
    }

    // -- now find the variable
    CVariableEntry* ve = GetVariable(cb->GetScriptContext(), cb->GetScriptContext()->GetGlobalNamespace()->GetVarTable(),
                                     ns_hash, func_or_obj, ve0->GetHash(), arrayvarhash);
    if (!ve)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack, "Error - OP_PushArrayValue failed\n");
        return false;
    }

    // -- push the variable onto the stack
    // -- if the variable is a stack parameter, we need to push it's value from the stack
    eVarType vetype = ve->GetType();
    void* veaddr = NULL;
    if (ve->IsStackVariable(funccallstack))
    {
        veaddr = GetStackVarAddr(cb->GetScriptContext(), execstack, funccallstack, *ve, arrayvarhash);
    }
    else
    {
        veaddr = ve->IsArray() ? ve->GetArrayVarAddr(oe0 ? oe0->GetAddr() : NULL, arrayvarhash)
                               : ve->GetAddr(oe0 ? oe0->GetAddr() : NULL);
    }

    execstack.Push(veaddr, vetype);
    if (oe0)
        DebugTrace(op, "ArrayVar: %d.%s [%s], %s", oe0->GetID(), UnHash(var_hash), UnHash(arrayvarhash),
                   DebugPrintVar(veaddr, vetype));
    else
        DebugTrace(op, "ArrayVar: %s [%s], %s", UnHash(var_hash), UnHash(arrayvarhash), DebugPrintVar(veaddr, vetype));

    return (true);
}

// ====================================================================================================================
// OpExecPushMember():  Push an object member onto the exec stack.
// ====================================================================================================================
bool8 OpExecPushMember(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                       CFunctionCallStack& funccallstack)
{
    // -- next instruction is the member name
    uint32 varhash = *instrptr++;

    // -- what will previously have been pushed on the stack, is the object ID
    eVarType contenttype;
    void* contentptr = execstack.Pop(contenttype);
    if (contenttype != TYPE_object)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain TYPE_object\n");
        return false;
    }

    // -- TYPE_object is actually just an uint32 ID
    // -- a member, a memberhash followed by the ID of the object
    uint32 member[2];
    member[0] = varhash;
    member[1] = *(uint32*)contentptr;

    // -- push the member onto the stack
    execstack.Push((void*)member, TYPE__member);
    DebugTrace(op, "Obj Mem %s: %s", UnHash(varhash), DebugPrintVar(contentptr, contenttype));
	
    return (true);
}

// ====================================================================================================================
// OpExecPushMemberVal():  Push the value of an object member onto the exec stack.
// ====================================================================================================================
bool8 OpExecPushMemberVal(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                          CFunctionCallStack& funccallstack)
{
    // -- next instruction is the member name
    uint32 varhash = *instrptr++;

    // -- what will previously have been pushed on the stack, is the object ID
    eVarType contenttype;
    void* contentptr = execstack.Pop(contenttype);
    if (contenttype != TYPE_object)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain TYPE_object\n");
        return false;
    }

    // -- TYPE_object is actually just an uint32 ID
    uint32 objectid = *(uint32*)contentptr;

    // -- find the object
    CObjectEntry* oe = cb->GetScriptContext()->FindObjectEntry(objectid);
    if (!oe)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - Unable to find object %d\n", objectid);
        return false;
    }

    // -- find the variable entry from the object's namespace variable table
    CVariableEntry* ve = oe->GetVariableEntry(varhash);
    if (!ve)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - Unable to find member %s for object %d\n",
                        UnHash(varhash), objectid);
        return false;
    }
    assert(ve != NULL);

    // -- if the variable is an array, we want to push it back onto the stack, as
    // -- an array has no value except itself (the value will be an upcoming arrayhash instruction)
    void* val = ve->GetAddr(oe->GetAddr());

    // -- the type is TYPE__var, *if* the variable is an array hash instruction
    if (!ve->IsArray())
    {
        eVarType valtype = ve->GetType();

        // -- push the value of the member
        execstack.Push(val, valtype);
        DebugTrace(op, "Obj Mem %s: %s", UnHash(varhash), DebugPrintVar(val, valtype));
    }
    else
    {
        // -- push the variable onto the stack
        uint32 varbuf[3];
        varbuf[0] = 0;
        varbuf[1] = oe->GetID();
        varbuf[2] = ve->GetHash();
        execstack.Push((void*)varbuf, TYPE__var);
    }
	
    return (true);
}

// ====================================================================================================================
// OpExecPushPODMember():  Push the member of a POD type onto the exec stack.
// ====================================================================================================================
bool8 OpExecPushPODMember(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                          CFunctionCallStack& funccallstack)
{
    // -- next instruction is the POD member name
    uint32 varhash = *instrptr++;

    // -- what will previously have been pushed on the stack, is a variable, member, or stack var
    // -- that is of a registered POD type
    CVariableEntry* ve0 = NULL;
    CObjectEntry* oe0 = NULL;
	eVarType vartype;
	void* varaddr = execstack.Pop(vartype);
    if (!GetStackValue(cb->GetScriptContext(), execstack, funccallstack, varaddr, vartype, ve0, oe0))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - Failed to pop a variable of a registered POD type\n");
        return (false);
    }

    // -- the var and vartype will be set to the actual type and physical address of the
    // -- POD variable we're about to dereference
    eVarType pod_member_type;
    void* pod_member_addr = NULL;
    if (!GetRegisteredPODMember(vartype, varaddr, varhash, pod_member_type, pod_member_addr))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - Failed to pop a variable of a registered POD type\n");
        return (false);
    }

    // -- the new type we're going to push is a TYPE__podmember
    // -- which is of the format:  TYPE__podmember vartype, varaddr
    uint32 varbuf[2];
    varbuf[0] = pod_member_type;
    varbuf[1] = (uint32)pod_member_addr;
    execstack.Push((void*)varbuf, TYPE__podmember);
    DebugTrace(op, "POD Mem %s: %s", UnHash(varhash), DebugPrintVar(pod_member_addr, pod_member_type));

    return (true);
}

// ====================================================================================================================
// OpExecPushPODMemberVal():  Push the value of a member of a POD type onto the exec stack.
// ====================================================================================================================
bool8 OpExecPushPODMemberVal(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                          CFunctionCallStack& funccallstack)
{
    // -- next instruction is the POD member name
    uint32 varhash = *instrptr++;

    // -- what will previously have been pushed on the stack, is a variable, member, or stack var
    // -- that is of a registered POD type
    eVarType contenttype;
    void* contentptr = execstack.Pop(contenttype);

    // -- see if we popped a value of a registered POD type
    eVarType pod_member_type;
    void* pod_member_addr = NULL;
    if (!GetRegisteredPODMember(contenttype, contentptr, varhash, pod_member_type, pod_member_addr))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - Failed to pop a variable of a registered POD type\n");
        return (false);
    }

    // -- push the value of the POD member
    execstack.Push(pod_member_addr, pod_member_type);
    DebugTrace(op, "POD Mem %s: %s", UnHash(varhash), DebugPrintVar(pod_member_addr, pod_member_type));

    return (true);
}

// ====================================================================================================================
// OpExecPushSelf():  Push the ID of the object whose method is currently being executed.
// ====================================================================================================================
bool8 OpExecPushSelf(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                     CFunctionCallStack& funccallstack)
{
    int32 stacktop = 0;
    CObjectEntry* oe = NULL;
    CFunctionEntry* fe = funccallstack.GetExecuting(oe, stacktop);
    if (!fe || !oe)
    {
        ScriptAssert_(cb->GetScriptContext(), 0, cb->GetFileName(), cb->CalcLineNumber(instrptr),
                      "Error - PushSelf from outside a method\n");
        return false;
    }

    // -- need to push the variable
    uint32 objid = oe->GetID();
    execstack.Push((void*)&objid, TYPE_object);
    DebugTrace(op, "Obj ID: %d", objid);
    return (true);
}

// ====================================================================================================================
// OpExecPop():  Discard the top exec stack entry.
// ====================================================================================================================
bool8 OpExecPop(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                CFunctionCallStack& funccallstack)
{
    eVarType contenttype;
    void* content = execstack.Pop(contenttype);
    DebugTrace(op, "Val: %s", DebugPrintVar(content, contenttype));
    return (true);
}

// ====================================================================================================================
// OpExecAdd():  Add operation.
// ====================================================================================================================
bool8 OpExecAdd(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                CFunctionCallStack& funccallstack)
{
    return (PerformBinaryOpPush(cb->GetScriptContext(), execstack, funccallstack, op));
}

// ====================================================================================================================
// OpExecSub():  Sub operation.
// ====================================================================================================================
bool8 OpExecSub(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                CFunctionCallStack& funccallstack)
{
    return (PerformBinaryOpPush(cb->GetScriptContext(), execstack, funccallstack, op));
}

// ====================================================================================================================
// OpExecMult():  Mult operation.
// ====================================================================================================================
bool8 OpExecMult(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                CFunctionCallStack& funccallstack)
{
    return (PerformBinaryOpPush(cb->GetScriptContext(), execstack, funccallstack, op));
}

// ====================================================================================================================
// OpExecDiv():  Div operation.
// ====================================================================================================================
bool8 OpExecDiv(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                CFunctionCallStack& funccallstack)
{
    return (PerformBinaryOpPush(cb->GetScriptContext(), execstack, funccallstack, op));
}

// ====================================================================================================================
// OpExecMod():  Mod operation.
// ====================================================================================================================
bool8 OpExecMod(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                CFunctionCallStack& funccallstack)
{
    return (PerformBinaryOpPush(cb->GetScriptContext(), execstack, funccallstack, op));
}

// ====================================================================================================================
// PerformCompareOp():  Perform comparisons, returning success and a float result.
// ====================================================================================================================
bool8 PerformCompareOp(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                       CFunctionCallStack& funccallstack, float& float_result)
{
    if (!PerformBinaryOpPush(cb->GetScriptContext(), execstack, funccallstack, op))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - unable to perform op: %s\n", GetOperationString(op));
        return (false);
    }

    // -- pull the result off the stack - it should have a numerical value
    eVarType resultType;
    void* resultPtr = execstack.Pop(resultType);
    float32* convertAddr = (float32*)TypeConvert(cb->GetScriptContext(), resultType, resultPtr, TYPE_float);
    if (! convertAddr)
        return (false);

    // -- success
    float_result = *convertAddr;
    return (true);
}

// ====================================================================================================================
// OpExecBooleanAnd():  Boolean And operation.
// ====================================================================================================================
bool8 OpExecBooleanAnd(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                      CFunctionCallStack& funccallstack)
{
    float32 float_result = 0.0f;
    if (!PerformCompareOp(cb, op, instrptr, execstack, funccallstack, float_result))
        return (false);

    bool8 boolresult = (float_result != 0.0f);
    execstack.Push((void*)&boolresult, TYPE_bool);
    DebugTrace(op, "%s", boolresult ? "true" : "false");
    return (true);
}

// ====================================================================================================================
// OpExecBooleanOr():  Boolean Or operation.
// ====================================================================================================================
bool8 OpExecBooleanOr(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                      CFunctionCallStack& funccallstack)
{
    float32 float_result = 0.0f;
    if (!PerformCompareOp(cb, op, instrptr, execstack, funccallstack, float_result))
        return (false);

    bool8 boolresult = (float_result != 0.0f);
    execstack.Push((void*)&boolresult, TYPE_bool);
    DebugTrace(op, "%s", boolresult ? "true" : "false");
    return (true);
}

// ====================================================================================================================
// OpExecCompareEqual():  Compare Equal operation.
// ====================================================================================================================
bool8 OpExecCompareEqual(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                         CFunctionCallStack& funccallstack)
{
    float32 float_result = 0.0f;
    if (!PerformCompareOp(cb, op, instrptr, execstack, funccallstack, float_result))
        return (false);

    bool8 boolresult = (float_result == 0.0f);
    execstack.Push((void*)&boolresult, TYPE_bool);
    DebugTrace(op, "%s", boolresult ? "true" : "false");
    return (true);
}

// ====================================================================================================================
// OpExecCompareNotEqual():  Compare Not equal operation.
// ====================================================================================================================
bool8 OpExecCompareNotEqual(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                         CFunctionCallStack& funccallstack)
{
    float32 float_result = 0.0f;
    if (!PerformCompareOp(cb, op, instrptr, execstack, funccallstack, float_result))
        return (false);

    bool8 boolresult = (float_result != 0.0f);
    execstack.Push((void*)&boolresult, TYPE_bool);
    DebugTrace(op, "%s", boolresult ? "true" : "false");
    return (true);
}

// ====================================================================================================================
// OpExecCompareLess():  Compare Less Than operation.
// ====================================================================================================================
bool8 OpExecCompareLess(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                         CFunctionCallStack& funccallstack)
{
    float32 float_result = 0.0f;
    if (!PerformCompareOp(cb, op, instrptr, execstack, funccallstack, float_result))
        return (false);

    bool8 boolresult = (float_result < 0.0f);
    execstack.Push((void*)&boolresult, TYPE_bool);
    DebugTrace(op, "%s", boolresult ? "true" : "false");
    return (true);
}

// ====================================================================================================================
// OpExecCompareLessEqual():  Compare Less Than Equal To operation.
// ====================================================================================================================
bool8 OpExecCompareLessEqual(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                             CFunctionCallStack& funccallstack)
{
    float32 float_result = 0.0f;
    if (!PerformCompareOp(cb, op, instrptr, execstack, funccallstack, float_result))
        return (false);

    bool8 boolresult = (float_result <= 0.0f);
    execstack.Push((void*)&boolresult, TYPE_bool);
    DebugTrace(op, "%s", boolresult ? "true" : "false");
    return (true);
}

// ====================================================================================================================
// OpExecCompareGreater():  Compare Greater Than operation.
// ====================================================================================================================
bool8 OpExecCompareGreater(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                           CFunctionCallStack& funccallstack)
{
    float32 float_result = 0.0f;
    if (!PerformCompareOp(cb, op, instrptr, execstack, funccallstack, float_result))
        return (false);

    bool8 boolresult = (float_result > 0.0f);
    execstack.Push((void*)&boolresult, TYPE_bool);
    DebugTrace(op, "%s", boolresult ? "true" : "false");
    return (true);
}

// ====================================================================================================================
// OpExecCompareGreaterEqual():  Compare Greater Than Equal To operation.
// ====================================================================================================================
bool8 OpExecCompareGreaterEqual(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                                CFunctionCallStack& funccallstack)
{
    float32 float_result = 0.0f;
    if (!PerformCompareOp(cb, op, instrptr, execstack, funccallstack, float_result))
        return (false);

    bool8 boolresult = (float_result >= 0.0f);
    execstack.Push((void*)&boolresult, TYPE_bool);
    DebugTrace(op, "%s", boolresult ? "true" : "false");
    return (true);
}

// ====================================================================================================================
// OpExecBitLeftShift():  Left Shift operation.
// ====================================================================================================================
bool8 OpExecBitLeftShift(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                      CFunctionCallStack& funccallstack)
{
    return (PerformBinaryOpPush(cb->GetScriptContext(), execstack, funccallstack, op));
}

// ====================================================================================================================
// OpExecBitRightShift():  Right Shift operation.
// ====================================================================================================================
bool8 OpExecBitRightShift(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                      CFunctionCallStack& funccallstack)
{
    return (PerformBinaryOpPush(cb->GetScriptContext(), execstack, funccallstack, op));
}

// ====================================================================================================================
// OpExecBitAnd():  Bitwise And operation.
// ====================================================================================================================
bool8 OpExecBitAnd(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                      CFunctionCallStack& funccallstack)
{
    return (PerformBinaryOpPush(cb->GetScriptContext(), execstack, funccallstack, op));
}

// ====================================================================================================================
// OpExecBitOr():  Bitwise Or operation.
// ====================================================================================================================
bool8 OpExecBitOr(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                  CFunctionCallStack& funccallstack)
{
    return (PerformBinaryOpPush(cb->GetScriptContext(), execstack, funccallstack, op));
}

// ====================================================================================================================
// OpExecBitXor():  Bitwise Xor operation.
// ====================================================================================================================
bool8 OpExecBitXor(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                   CFunctionCallStack& funccallstack)
{
    return (PerformBinaryOpPush(cb->GetScriptContext(), execstack, funccallstack, op));
}

// ====================================================================================================================
// OpExecBranch():  Branch operation.
// ====================================================================================================================
bool8 OpExecBranch(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                   CFunctionCallStack& funccallstack)
{
    int32 jumpcount = *instrptr++;
    instrptr += jumpcount;
    DebugTrace(op, "count: %d", jumpcount);
    return (true);
}

// ====================================================================================================================
// OpExecBranchTrue():  Branch If True operation.
// ====================================================================================================================
bool8 OpExecBranchTrue(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                       CFunctionCallStack& funccallstack)
{
    int32 jumpcount = *instrptr++;

    // -- top of the stack had better be a bool8
    eVarType valtype;
    void* valueraw = execstack.Pop(valtype);
    bool8* convertAddr = (bool8*)TypeConvert(cb->GetScriptContext(), valtype, valueraw, TYPE_bool);
    if (! convertAddr)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - expecting a bool\n");
        return (false);
    }

    // -- branch
    if (*convertAddr)
    {
        instrptr += jumpcount;
    }
    DebugTrace(op, "%s, count: %d", *convertAddr ? "true" : "false", jumpcount);

    return (true);
}

// ====================================================================================================================
// OpExecBranchFalse():  Branch If False operation.
// ====================================================================================================================
bool8 OpExecBranchFalse(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                        CFunctionCallStack& funccallstack) {
    int32 jumpcount = *instrptr++;

    // -- top of the stack had better be a bool8
    eVarType valtype;
    void* valueraw = execstack.Pop(valtype);
    bool8* convertAddr = (bool8*)TypeConvert(cb->GetScriptContext(), valtype, valueraw, TYPE_bool);
    if (! convertAddr)
    {
        ScriptAssert_(cb->GetScriptContext(), 0, cb->GetFileName(), cb->CalcLineNumber(instrptr),
                      "Error - expecting a bool\n");
        return (false);
    }

    // -- branch
    if (!*convertAddr)
    {
        instrptr += jumpcount;
    }
    DebugTrace(op, "%s, count: %d", !*convertAddr ? "true" : "false", jumpcount);
    return (true);
}

// ====================================================================================================================
// OpExecFuncDecl():  Function declaration operation.
// ====================================================================================================================
bool8 OpExecFuncDecl(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                     CFunctionCallStack& funccallstack)
{
    uint32 funchash = *instrptr++;
    uint32 namespacehash = *instrptr++;
    uint32 parent_ns_hash = *instrptr++;
    uint32 funcoffset = *instrptr++;
    CFunctionEntry* fe = FuncDeclaration(cb->GetScriptContext(), namespacehash, UnHash(funchash),
                                         funchash, eFuncTypeScript);
    if (!fe)
    {
        ScriptAssert_(cb->GetScriptContext(), 0, cb->GetFileName(), cb->CalcLineNumber(instrptr),
                      "Error - failed to declare function - hash: 0x%08x\n", funchash);
        return false;
    }

    // -- if we have a parent namespace, now is the time to link namespaces
    if (parent_ns_hash != 0)
    {
        // -- see if we can link the namespaces
        CNamespace* function_ns = cb->GetScriptContext()->FindNamespace(namespacehash);
        CNamespace* parent_ns = cb->GetScriptContext()->FindNamespace(parent_ns_hash);
        if (!cb->GetScriptContext()->LinkNamespaces(function_ns, parent_ns))
        {
            ScriptAssert_(cb->GetScriptContext(), 0, cb->GetFileName(), cb->CalcLineNumber(instrptr),
                          "Error - Derivation %s : %s failed.\n", UnHash(namespacehash), UnHash(parent_ns_hash));
            return (false);
        }
    }

    // -- this being a script function, set the offset, and add this function
    // -- to the list of functions this codeblock implements
    fe->SetCodeBlockOffset(cb, funcoffset);

    // -- push the function entry onto the call stack, so all var declarations
    // -- will be associated with this function
    funccallstack.Push(fe, NULL, execstack.GetStackTop());
    DebugTrace(op, "%s", UnHash(fe->GetHash()));
    return (true);
}

// ====================================================================================================================
// OpExecFuncDeclEnd():  Notification that the function declaration has conconcluded.
// ====================================================================================================================
bool8 OpExecFuncDeclEnd(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                        CFunctionCallStack& funccallstack)
{
    // -- pop the function stack
    CObjectEntry* oe = NULL;
    int32 var_offset = 0;
    CFunctionEntry* fe = funccallstack.Pop(oe, var_offset);
    fe->GetContext()->InitStackVarOffsets(fe);
    DebugTrace(op, "%s", UnHash(fe->GetHash()));
    return (true);
}

// ====================================================================================================================
// OpExecFuncCallArgs():  Preparation to call a function after we've assigned all the arguments.
// ====================================================================================================================
bool8 OpExecFuncCallArgs(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                         CFunctionCallStack& funccallstack)
{
    // -- we're about to call a new function - next will be however many assign ops
    // -- to set the parameter values, finally OP_FuncCall will actually execute

    // -- get the hash of the function name
    uint32 nshash = *instrptr++;
    uint32 funchash = *instrptr++;
    tFuncTable* functable = cb->GetScriptContext()->FindNamespace(nshash)->GetFuncTable();
    CFunctionEntry* fe = functable->FindItem(funchash);
    if (!fe)
    {
        if (nshash != 0)
        {
            DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                            "Error - undefined function: %s::%s()\n",
                            UnHash(nshash), UnHash(funchash));
        }
        else
        {
            DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                            "Error - undefined function: %s()\n", UnHash(funchash));
        }
        return false;
    }

    // -- push the function entry onto the call stack
    // -- we're also going to zero out all parameters - by default, calling
    // -- a function without passing a parameter value is the same as that
    // -- passing 0
    fe->GetContext()->ClearParameters();

    funccallstack.Push(fe, NULL, execstack.GetStackTop());
    DebugTrace(op, "%s", UnHash(fe->GetHash()));

    // -- create space on the execstack, if this is a script function
    if (fe->GetType() != eFuncTypeGlobal)
    {
        int32 localvarcount = fe->GetContext()->CalculateLocalVarStackSize();
        execstack.Reserve(localvarcount * MAX_TYPE_SIZE);
    }

    return (true);
}

// ====================================================================================================================
// OpExecPushParam():  Push a parameter variable.
// ====================================================================================================================
bool8 OpExecPushParam(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                      CFunctionCallStack& funccallstack)
{
    // -- the next word is the parameter index for the current function we're calling
    uint32 paramindex = *instrptr++;

    // -- get the function about to be called
    int32 stackoffset = 0;
    CObjectEntry* oe = NULL;
    CFunctionEntry* fe = funccallstack.GetTop(oe, stackoffset);
    if (!fe)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - assigning parameters outside a function call\n");
        return false;
    }

    uint32 paramcount = fe->GetContext()->GetParameterCount();
    if (paramindex >= paramcount)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - too many parameters calling function: %s\n",
                        UnHash(fe->GetHash()));
        return false;
    }

    // -- get the parameter
    CVariableEntry* ve = fe->GetContext()->GetParameter(paramindex);

    // -- push the variable onto the stack
    uint32 varbuf[3];
    varbuf[0] = fe->GetNamespaceHash();
    varbuf[1] = fe->GetHash();
    varbuf[2] = ve->GetHash();
    execstack.Push((void*)varbuf, TYPE__var);

    DebugTrace(op, "%s, param %d", UnHash(fe->GetHash()), paramindex);

    return (true);
}

// ====================================================================================================================
// OpExecMethodCallArgs():  Preparation to call a method after we've assigned all the arguments.
// ====================================================================================================================
bool8 OpExecMethodCallArgs(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                           CFunctionCallStack& funccallstack)
{
    // -- get the hash of the namespace, in case we want a specific one
    uint32 nshash = *instrptr++;

    // -- get the hash of the method name
    uint32 methodhash = *instrptr++;

    // -- what will previously have been pushed on the stack, is the object ID
    eVarType contenttype;
    void* contentptr = execstack.Pop(contenttype);
    if (contenttype != TYPE_object)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain TYPE_object\n");
        return false;
    }

    // -- TYPE_object is actually just an uint32 ID
    uint32 objectid = *(uint32*)contentptr;

    // -- find the object
    CObjectEntry* oe = cb->GetScriptContext()->FindObjectEntry(objectid);
    if (!oe)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - Unable to find object %d\n", objectid);
        return false;
    }

    // -- find the method entry from the object's namespace hierarachy
    // -- if nshash is 0, then it's from the top of the hierarchy
    CFunctionEntry* fe = oe->GetFunctionEntry(nshash, methodhash);
    if (!fe)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - Unable to find method %s for object %d\n",
                        UnHash(methodhash), objectid);
        return false;
    }

    // -- push the function entry onto the call stack
    // -- we're also going to zero out all parameters - by default, calling
    // -- a function without passing a parameter value is the same as that
    // -- passing 0
    fe->GetContext()->ClearParameters();

    // -- push the function entry onto the call stack
    funccallstack.Push(fe, oe, execstack.GetStackTop());

    // -- create space on the execstack, if this is a script function
    if (fe->GetType() != eFuncTypeGlobal)
    {
        int32 localvarcount = fe->GetContext()->CalculateLocalVarStackSize();
        execstack.Reserve(localvarcount * MAX_TYPE_SIZE);
    }

    DebugTrace(op, "obj: %d, ns: %s, func: %s", objectid, UnHash(nshash),
               UnHash(fe->GetHash()));
    return (true);
}

// ====================================================================================================================
// OpExecFuncCall():  Call a function.
// ====================================================================================================================
extern bool8 CodeBlockCallFunction(CFunctionEntry* fe, CObjectEntry* oe, CExecStack& execstack,
                                   CFunctionCallStack& funccallstack, bool copy_stack_parameters);

bool8 OpExecFuncCall(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                     CFunctionCallStack& funccallstack)
{
    int32 stackoffset = 0;
    CObjectEntry* oe = NULL;
    CFunctionEntry* fe = funccallstack.GetTop(oe, stackoffset);
    assert(fe != NULL);

    // -- notify the stack that we're now actually executing the top function
    // -- this is to ensure that stack variables now reference this function's
    // -- reserved space on the stack.
    funccallstack.BeginExecution(instrptr - 1);

    DebugTrace(op, "func: %s", UnHash(fe->GetHash()));

    bool8 result = CodeBlockCallFunction(fe, oe, execstack, funccallstack, false);
    if (!result)
    {
        if (funccallstack.mDebuggerObjectDeleted == 0 && funccallstack.mDebuggerFunctionReload == 0)
        {
            DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                            "Error - Unable to call function: %s()\n",
                            UnHash(fe->GetHash()));
        }
        return false;
    }

    // -- the return value of the call is guaranteed - even void is forced to push a 0
    // -- don't pop it, however, as it could also be used in an assignment - use Peek()
    eVarType return_valtype;
    CVariableEntry* return_ve = NULL;
    CObjectEntry* return_oe = NULL;
	void* return_val = execstack.Peek(return_valtype);
    if (!GetStackValue(cb->GetScriptContext(), execstack, funccallstack, return_val, return_valtype, return_ve,
                      return_oe))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - no return value (even void pushes 0) from function: %s()\n", UnHash(fe->GetHash()));
        return false;
    }

    // -- store the stack value in the code block, so ExecF has something to retrieve
    cb->GetScriptContext()->SetFunctionReturnValue(return_val, return_valtype);

    return (true);
}

// ====================================================================================================================
// OpExecFuncReturn():  Return from a function operation.
// ====================================================================================================================
bool8 OpExecFuncReturn(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                       CFunctionCallStack& funccallstack)
{
    // -- pop the function entry from the stack
    CObjectEntry* oe = NULL;
    int32 var_offset = 0;
    CFunctionEntry* fe = funccallstack.Pop(oe, var_offset);
    if (!fe)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - return with no function\n");
        return false;
    }

    // -- pop the return value while we unreserve the local var space on the stack
    uint32 stacktopcontent[MAX_TYPE_SIZE];

    // -- pop the return value off the stack
    eVarType contenttype;
    void* content = execstack.Pop(contenttype);
    memcpy(stacktopcontent, content, MAX_TYPE_SIZE * sizeof(uint32));

    // -- unreserve space from the exec stack
    int32 localvarcount = fe->GetContext()->CalculateLocalVarStackSize();
    execstack.UnReserve(localvarcount * MAX_TYPE_SIZE);

    // -- ensure our current stack top is what it was before we reserved
    int32 cur_stack_top = execstack.GetStackTop();
    if (cur_stack_top != var_offset)
    {
        // -- this is somewhat bad - it means there's a leak - some combination of
        // -- operations is pushing without matching pops.
        // -- however, forcing the "excess" to be popped to reset the stack to the state it
        // -- was when the function was called is relatively safe.
        ScriptAssert_(cb->GetScriptContext(), 0, cb->GetFileName(), cb->CalcLineNumber(instrptr),
                      "Error - The stack has not been balanced - forcing Pops\n");
        execstack.ForceStackTop(var_offset);
    }

    // -- re-push the stack top contents
    execstack.Push((void*)stacktopcontent, contenttype);

    // -- clear all parameters for the function - this will ensure all
    // -- strings are decremented, keeping the string table clear of unassigned values
    fe->GetContext()->ClearParameters();

    DebugTrace(op, "func: %s, val: %s", UnHash(fe->GetHash()),
               DebugPrintVar(stacktopcontent, contenttype));

    // -- note:  when this function returns, the VM while loop will exit
    return (true);
}

// ====================================================================================================================
// OpExecArrayHash():  Appends a value to the current hash, to be used indexing a hashtable or array variable.
// ====================================================================================================================
bool8 OpExecArrayHash(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                      CFunctionCallStack& funccallstack)
{
    // -- peek at what's on the stack underneath the array hash...  if it's a variable of TYPE_hashtable
    // -- we pull a string off the stack and hash it
    // -- otherwise, it had better be a regular variable (with an array count)
    // -- we pull an integer off the stack, verify the range, and index
    bool found_array_var = true;
    eVarType peek_type;
	void* peek_val = execstack.Peek(peek_type, 2);
    if (!peek_val)
        found_array_var = false;

    // -- resolve the stack content
    CVariableEntry* peek_ve = NULL;
    CObjectEntry* peek_oe = NULL;
    if (!found_array_var || !GetStackValue(cb->GetScriptContext(), execstack, funccallstack, peek_val, peek_type,
                                           peek_ve, peek_oe))
    {
        found_array_var = false;
    }

    // -- see if the stack content is an variable entry
    if (!found_array_var || !peek_ve || (!peek_ve->IsArray() && peek_ve->GetType() != TYPE_hashtable))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - Failed to find an array or hashtable variable on the stack\n");
        return false;
    }

    // -- see if it's a hashtable var
    bool is_hashtable_var = (peek_ve->GetType() == TYPE_hashtable);

    // -- if we have a hashtable var, pop the string, append the hash, and push the result
    if (is_hashtable_var)
    {
        // -- pop the value of the next string to append to the hash
        CVariableEntry* ve1 = NULL;
        CObjectEntry* oe1 = NULL;
        eVarType val1type;
        void* val1 = execstack.Pop(val1type);
        if (!GetStackValue(cb->GetScriptContext(), execstack, funccallstack, val1, val1type, ve1, oe1))
        {
            DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                            "Error - Failed to pop string to hash\n");
            return false;
        }

        // -- ensure it actually is a string
        void* val1addr = TypeConvert(cb->GetScriptContext(), val1type, val1, TYPE_string);

        // -- get the current hash
        eVarType contenttype;
        void* contentptr = execstack.Pop(contenttype);
        if (contenttype != TYPE_int)
        {
            DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                            "Error - ExecStack should contain TYPE_int, a hash value\n");
            return false;
        }

        // -- calculate the updated hash
        uint32 hash = *(uint32*)contentptr;
        hash = HashAppend(hash, "_");
        hash = HashAppend(hash, (const char*)val1addr);

        // -- push the result
        execstack.Push((void*)&hash, TYPE_int);
        DebugTrace(op, "ArrayHash: %s", UnHash(hash));
    }

    // -- else we have an array variable
    else
    {
        // -- pop the value of the next integer - in a variable array, we add the indices
        // -- consecutive integers, it allows an array[100] to be indexed as array[10,6]
        // -- which is array[16], or the 6th column of row 1, where there are 10 columns per row
        CVariableEntry* ve1 = NULL;
        CObjectEntry* oe1 = NULL;
        eVarType val1type;
        void* val1 = execstack.Pop(val1type);
        if (!GetStackValue(cb->GetScriptContext(), execstack, funccallstack, val1, val1type, ve1, oe1))
        {
            DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                            "Error - Failed to pop an array index\n");
            return false;
        }

        // -- ensure the index is an integer
        void* val1addr = TypeConvert(cb->GetScriptContext(), val1type, val1, TYPE_int);

        // -- get the current array index (so far)
        eVarType contenttype;
        void* contentptr = execstack.Pop(contenttype);
        if (contenttype != TYPE_int)
        {
            DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                            "Error - ExecStack should contain TYPE_int, an array index\n");
            return false;
        }

        // -- calculate the updated array index (adding together, as per above)
        int32 array_index = *(int32*)contentptr + *(int32*)val1addr;

        // -- push the result
        execstack.Push((void*)&array_index, TYPE_int);
        DebugTrace(op, "ArrayIndex: %d", array_index);
    }

    return (true);
}

// ====================================================================================================================
// OpExecArrayVarDecl():  Declare a variable to be inserted into a hashtable.
// ====================================================================================================================
bool8 OpExecArrayVarDecl(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                         CFunctionCallStack& funccallstack)
{
    // -- next instruction is the type
    eVarType vartype = (eVarType)(*instrptr++);

    // -- pull the hash value for the hash table entry
    eVarType contenttype;
    void* contentptr = execstack.Pop(contenttype);
    if (contenttype != TYPE_int)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain TYPE_int\n");
        return false;
    }
    uint32 hashvalue = *(uint32*)contentptr;

    // -- pull the hashtable variable off the stack
    CVariableEntry* ve0 = NULL;
    CObjectEntry* oe0 = NULL;
    eVarType val0type;
    void* val0 = execstack.Pop(val0type);
    if (!GetStackValue(cb->GetScriptContext(), execstack, funccallstack, val0, val0type, ve0, oe0))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain a hashtable variable\n");
        return false;
    }
    if (val0type != TYPE_hashtable)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain hashtable variable\n");
        return false;
    }

    // -- now that we've got our hashtable var, and the type, create (or verify)
    // -- the hash table entry
    tVarTable* hashtable = (tVarTable*)ve0->GetAddr(oe0 ? oe0->GetAddr() : NULL);
    CVariableEntry* hte = hashtable->FindItem(hashvalue);

    // -- if the entry already exists, ensure it's the same type
    if (hte && hte->GetType() != vartype)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - HashTable variable: %s already has an entry of type: %s\n",
                        UnHash(ve0->GetHash()), GetRegisteredTypeName(hte->GetType()));
        return false;
    }

    // -- otherwise add the variable entry to the hash table
    // -- note:  by definition, hash table entries are dynamic
    else if (!hte)
    {
        CVariableEntry* hte = TinAlloc(ALLOC_VarEntry, CVariableEntry, cb->GetScriptContext(), UnHash(hashvalue),
                                       hashvalue, vartype, 1, false, 0, true);
        hashtable->AddItem(*hte, hashvalue);
    }

    DebugTrace(op, "ArrayVar: %s", UnHash(hashvalue));

    return (true);
}

// ====================================================================================================================
// OpExecArrayDecl():  Pops the size and converts a variable to an array variable.
// ====================================================================================================================
bool8 OpExecArrayDecl(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                       CFunctionCallStack& funccallstack)
{
    // -- pull the array size from the stack
    eVarType contenttype;
    void* contentptr = execstack.Pop(contenttype);
    if (contenttype != TYPE_int)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain a positive TYPE_int value\n");
        return false;
    }
    uint32 array_size = *(uint32*)contentptr;

    // -- pull the variable off the stack
    CVariableEntry* ve0 = NULL;
    CObjectEntry* oe0 = NULL;
    eVarType val0type;
    void* val0 = execstack.Pop(val0type);

    // -- when "converting" a variable into an array, if the array is a stack variable, it's already
    // -- the correct size when stack space was reserved, or it's a parameter in which case
    // -- it refers to an actually array variable entry that has already been allocated.
    // -- either way, if this is a stack variable, we're done
    if (val0type == TYPE__stackvar)
    {
        return (true);
    }

    // -- not a stack variable - resolve to the acctual variable then...
    if (!GetStackValue(cb->GetScriptContext(), execstack, funccallstack, val0, val0type, ve0, oe0))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain a hashtable variable\n");
        return false;
    }

    // -- ensure we have a non-hashtable variable (no arrays of hashtables)
    if (val0type == TYPE_hashtable || val0type < FIRST_VALID_TYPE)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain a non-hashtable variable\n");
        return false;
    }

    // -- set the array size
    bool8 result = ve0->ConvertToArray(array_size);

    DebugTrace(op, "Array: %s[%d]", UnHash(ve0->GetHash()), array_size);

    return (result);
}

// ====================================================================================================================
// OpExecSelfVarDecl():  Declare a member for the object executing the current method.
// ====================================================================================================================
bool8 OpExecSelfVarDecl(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                        CFunctionCallStack& funccallstack)
{
    // -- next instruction is the variable hash
    uint32 varhash = *instrptr++;

    // -- followed by the type
    eVarType vartype = (eVarType)(*instrptr++);

    // -- followed by the array size
    int32 array_size = *instrptr++;

    // -- get the object from the stack
    CObjectEntry* oe = NULL;
    CFunctionEntry* fe = funccallstack.GetTopMethod(oe);
    if (!fe || !oe)
    {
        ScriptAssert_(cb->GetScriptContext(), 0, cb->GetFileName(), cb->CalcLineNumber(instrptr),
                      "Error - Unable to declare a self.var from outside a method\n");
        return false;
    }

    // -- add the dynamic variable
    cb->GetScriptContext()->AddDynamicVariable(oe->GetID(), varhash, vartype, array_size);
    DebugTrace(op, "Obj Id [%d] Var: %s", oe->GetID(), UnHash(varhash));

    return (true);
}

// ====================================================================================================================
// OpExecObjMemberDecl():  Declare an object member variable.
// ====================================================================================================================
bool8 OpExecObjMemberDecl(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                          CFunctionCallStack& funccallstack)
{
    // -- next instruction is the variable hash
    uint32 varhash = *instrptr++;

    // -- followed by the type
    eVarType vartype = (eVarType)(*instrptr++);

    // -- followed by the array size
    int32 array_size = *instrptr++;

    // -- what will previously have been pushed on the stack, is the object ID
    eVarType contenttype;
    void* contentptr = execstack.Pop(contenttype);
    if (contenttype != TYPE_object)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain TYPE_object\n");
        return false;
    }

    // -- TYPE_object is actually just an uint32 ID
    uint32 objectid = *(uint32*)contentptr;

    // -- find the object
    CObjectEntry* oe = cb->GetScriptContext()->FindObjectEntry(objectid);
    if (!oe)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - Unable to find object %d\n", objectid);
        return false;
    }

    // -- add the dynamic variable
    cb->GetScriptContext()->AddDynamicVariable(oe->GetID(), varhash, vartype, array_size);
    DebugTrace(op, "Obj Id [%d] Var: %s", oe->GetID(), UnHash(varhash));

    return (true);
}

// ====================================================================================================================
// OpExecScheduleBegin():  Operation at the beginning of a scheduled function call.
// ====================================================================================================================
bool8 OpExecScheduleBegin(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                          CFunctionCallStack& funccallstack)
{
    // -- ensure we're not in the middle of a schedule construction already
    if (cb->GetScriptContext()->GetScheduler()->mCurrentSchedule != NULL)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - A schedule() is already being processed\n");
        return false;
    }

    // -- read the next instruction - see if this is an immediate execution call
    uint32 immediate_execution = *instrptr++;

    // -- the function hash will have been pushed most recently
    eVarType contenttype;
    void* contentptr = execstack.Pop(contenttype);
    if (contenttype != TYPE_int)
    {
        ScriptAssert_(cb->GetScriptContext(), 0, cb->GetFileName(), cb->CalcLineNumber(instrptr),
                      "Error - ExecStack should contain TYPE_int\n");
        return false;
    }
    uint32 funchash = *(uint32*)(contentptr);

    // -- next pull the object ID off the stack
    contenttype;
    contentptr = execstack.Pop(contenttype);
    if (contenttype != TYPE_object)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain TYPE_object\n");
        return false;
    }

    // -- TYPE_object is actually just an uint32 ID
    uint32 objectid = *(uint32*)contentptr;

    // -- pull the repeat flag off the stack
    contentptr = execstack.Pop(contenttype);
    if (contenttype != TYPE_bool)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain TYPE_bool\n");
        return false;
    }
    bool8 repeat = *(bool8*)(contentptr);

    // -- pull the delay time off the stack
    contentptr = execstack.Pop(contenttype);
    if (contenttype != TYPE_int)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain TYPE_int\n");
        return false;
    }
    int32 delaytime = *(int32*)(contentptr);


    // -- create the schedule 
    cb->GetScriptContext()->GetScheduler()->mCurrentSchedule =
        cb->GetScriptContext()->GetScheduler()->ScheduleCreate(objectid, delaytime, funchash,
                                                               immediate_execution != 0 ? true : false,
                                                               repeat);

    if (objectid > 0)
        DebugTrace(op, "Obj Id [%d] Function: %s", objectid, UnHash(funchash));
    else
        DebugTrace(op, "Function: %s", UnHash(funchash));

    return (true);
}

// ====================================================================================================================
// OpExecScheduleParam():  Assign a parameter value as part of a scheduled function call.
// ====================================================================================================================
bool8 OpExecScheduleParam(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                          CFunctionCallStack& funccallstack)
{
    // -- ensure we are in the middle of a schedule construction
    if (cb->GetScriptContext()->GetScheduler()->mCurrentSchedule == NULL) {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - There is no schedule() being processed\n");
        return false;
    }

    // -- get the parameter index
    int32 paramindex = *instrptr++;

    // -- pull the parameter value off the stack
    eVarType contenttype;
    void* contentptr = execstack.Pop(contenttype);

    // -- add the parameter to the function context, inheriting the type from whatever
    // -- was pushed
    char varnamebuf[32];
    sprintf_s(varnamebuf, 32, "_%d", paramindex);
    cb->GetScriptContext()->GetScheduler()->mCurrentSchedule->mFuncContext->
        AddParameter(varnamebuf, Hash(varnamebuf), contenttype, 1, paramindex, 0);

    // -- assign the value
    CVariableEntry* ve = cb->GetScriptContext()->GetScheduler()->mCurrentSchedule->
                         mFuncContext->GetParameter(paramindex);
    ve->SetValue(NULL, contentptr);
    DebugTrace(op, "Param: %d, Var: %s", paramindex, varnamebuf);

    return (true);
}

// ====================================================================================================================
// OpExecScheduleEnd():  Construction of a scheduled function call is complete.
// ====================================================================================================================
bool8 OpExecScheduleEnd(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                        CFunctionCallStack& funccallstack)
{
    // -- ensure we are in the middle of a schedule construction
    if (cb->GetScriptContext()->GetScheduler()->mCurrentSchedule == NULL)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - There is no schedule() being processed\n");
        return false;
    }

    // -- now that the schedule has been completely constructed, we need to determine
    // -- if it's scheduled for immediate execution
    CScheduler::CCommand* curcommand = cb->GetScriptContext()->GetScheduler()->mCurrentSchedule;
    if (curcommand->mImmediateExec)
    {
        ExecuteScheduledFunction(cb->GetScriptContext(), curcommand->mObjectID, 0, curcommand->mFuncHash,
                                 curcommand->mFuncContext);

        // -- see if we have a return result
        CVariableEntry* return_ve = curcommand->mFuncContext->GetParameter(0);
        if (!return_ve)
        {
            DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                            "Error - There is no return value available from schedule()\n");
            return false;
        }

        // -- Push the contents of the return_ve onto *this* execstack
        execstack.Push(return_ve->GetAddr(NULL), return_ve->GetType());

        // -- now remove the current command from the scheduler
        cb->GetScriptContext()->GetScheduler()->CancelRequest(curcommand->mReqID);
    }

    // -- not immediate execution - therefore, push the schedule request ID instead
    else
    {
        int32 reqid =  cb->GetScriptContext()->GetScheduler()->mCurrentSchedule->mReqID;
        execstack.Push(&reqid, TYPE_int);
    }

    // -- clear the current schedule
    cb->GetScriptContext()->GetScheduler()->mCurrentSchedule = NULL;
    DebugTrace(op, "");

    return (true);
}

// ====================================================================================================================
// OpExecCreateObject():  Create Object instruction.
// ====================================================================================================================
bool8 OpExecCreateObject(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                         CFunctionCallStack& funccallstack)
{
    // -- The next instruction is the class to instantiate
    uint32 classhash = *instrptr++;

    // -- what will previously have been pushed on the stack, is the object ID
    eVarType contenttype;
    void* contentptr = execstack.Pop(contenttype);
    void* objnameaddr = TypeConvert(cb->GetScriptContext(), contenttype, contentptr, TYPE_string);
    if (!objnameaddr)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain TYPE_string\n");
        return false;
    }

    // -- strings are already hashed, when pulled from the stack
    uint32 objid = cb->GetScriptContext()->CreateObject(classhash, *(uint32*)objnameaddr);

    // -- if we failed to create the object, assert
    if (objid == 0)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - Failed to create object of class:  %s\n", UnHash(classhash));
        return false;
    }

    // -- push the objid onto the stack, and update the instrptr
    execstack.Push(&objid, TYPE_object);
    DebugTrace(op, "Obj ID: %d", objid);

    return (true);
}

// ====================================================================================================================
// OpExecDestroyObject():  Destroy Object instruction.
// ====================================================================================================================
bool8 OpExecDestroyObject(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                          CFunctionCallStack& funccallstack)
{
    // -- what will previously have been pushed on the stack, is the object ID
    eVarType contenttype;
    void* contentptr = execstack.Pop(contenttype);
    if (contenttype != TYPE_object)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain TYPE_object\n");
        return false;
    }

    // -- TYPE_object is actually just an uint32 ID
    uint32 objectid = *(uint32*)contentptr;

    // -- find the object
    CObjectEntry* oe = cb->GetScriptContext()->FindObjectEntry(objectid);
    if (!oe)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - Unable to find object %d\n", objectid);
        return false;
    }

    // $$$TZA possible opportunity to ensure that if the current object on the function call stack
    // is this object, there are no further instructions referencing it...
    cb->GetScriptContext()->DestroyObject(objectid);
    DebugTrace(op, "Obj ID: %d", objectid);
    return (true);
}

// ====================================================================================================================
// OpExecEOF():  Notification of the end of the script file.
// ====================================================================================================================
bool8 OpExecEOF(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                CFunctionCallStack& funccallstack)
{
    DebugTrace(op, "");
    return (true);
}

// ====================================================================================================================
// SetDebugTrace():  Debug function to enable tracing as the virtual machine executes the code block.
// ====================================================================================================================
void SetDebugTrace(bool8 torf)
{
    CScriptContext::gDebugTrace = torf;
}

REGISTER_FUNCTION_P1(SetDebugTrace, SetDebugTrace, void, bool8);

}  // namespace TinScript

// ====================================================================================================================
// EOF
// ====================================================================================================================
