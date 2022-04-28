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

#include <functional>

// -- includes
#include "TinScript.h"
#include "TinCompile.h"
#include "TinNamespace.h"
#include "TinScheduler.h"
#include "TinExecute.h"
#include "TinHashtable.h"
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

    TinPrint(TinScript::GetContext(), "OP [%s]: %s\n", GetOperationString(opcode), tracebuf);
}

#else

void VoidFunction() {}
#define DebugTrace(...) VoidFunction()

#endif

// --------------------------------------------------------------------------------------------------------------------
// struct tPostUnaryOpEntry
// Used to cache the variable info, and the request ID
// the variable info is read from the stack per the operation, but not applied until after the var is popped
// --------------------------------------------------------------------------------------------------------------------
struct tPostUnaryOpEntry
{
    void Set(eVarType value_type, void* value_addr, int32 adjust, bool8 append)
    {
        m_valType = value_type;
        m_valAddr = value_addr;

        if (append)
            m_postOpAdjust += adjust;
        else
            m_postOpAdjust = adjust;
    }

    eVarType m_valType;
    void* m_valAddr;
    int32 m_postOpAdjust;
};

const int32 k_maxPostOpEntryCount = 32;
int32 g_postOpEntryCount = 0;
tPostUnaryOpEntry g_postOpEntryList[k_maxPostOpEntryCount];

bool8 AddPostUnaryOpEntry(eVarType value_type, void* value_addr, int32 adjust)
{
    // -- sanity check
    if (value_addr == nullptr || (value_type != TYPE_int && value_type != TYPE_float))
    {
        ScriptAssert_(TinScript::GetContext(), 0, "<internal>", -1,
                      "Error - AddPostUnaryOpEntry(): invalid type to apply a post-inc/dec op\n");
        return (false);
    }

    for (int32 i = 0; i < g_postOpEntryCount; ++i)
    {
        if (g_postOpEntryList[i].m_valAddr == value_addr)
        {
            g_postOpEntryList[i].Set(value_type, value_addr, adjust, true);
            return (true);
        }
    }

    // -- add the post unary op request
    if (g_postOpEntryCount >= k_maxPostOpEntryCount)
    {
        ScriptAssert_(TinScript::GetContext(), 0, "<internal>", -1,
                      "Error - AddPostUnaryOpEntry(): request list is full, increase count\n");
        return (false);
    }

    // -- add the request
    g_postOpEntryList[g_postOpEntryCount++].Set(value_type, value_addr, adjust, false);

    if (CScriptContext::gDebugTrace)
    {
        TinPrint(TinScript::GetContext(), "***  Add POST OP: 0x%x, count: %d\n",
                                          kPointerToUInt32(value_addr), g_postOpEntryCount);
    }

    // -- success
    return (true);
}

bool8 ApplyPostUnaryOpEntry(eVarType value_type, void* value_addr)
{
    // -- sanity check
    if (value_addr == nullptr || (value_type != TYPE_int && value_type != TYPE_float))
    {
        return (false);
    }

    // -- find the request in the list, and apply the adjust
    int32 found = -1;
    for (int32 i = 0; i < g_postOpEntryCount; ++i)
    {
        if (g_postOpEntryList[i].m_valAddr == value_addr)
        {
            found = i;
            break;
        }
    }

    // -- if we found our entry
    bool8 success = true;
    if (found >= 0)
    {
        // -- ensure the types match, as a safety precaution
        if (value_type != g_postOpEntryList[found].m_valType)
        {
            ScriptAssert_(TinScript::GetContext(), 0, "<internal>", -1,
                          "Error - AddPostUnaryOpEntry(): mismatched value type - corrupt variable?\n");
            success = false;
        }
        else
        {
            if (g_postOpEntryList[found].m_valType == TYPE_int)
                *(int32*)(g_postOpEntryList[found].m_valAddr) += g_postOpEntryList[found].m_postOpAdjust;
            else if (g_postOpEntryList[found].m_valType == TYPE_float)
                *(float32*)(g_postOpEntryList[found].m_valAddr) += (float32)(g_postOpEntryList[found].m_postOpAdjust);
        }

        if (CScriptContext::gDebugTrace)
        {
            TinPrint(TinScript::GetContext(), "***  found POST OP: 0x%x, count: %d\n",
                                              kPointerToUInt32(g_postOpEntryList[found].m_valAddr),
                                              g_postOpEntryCount - 1);
        }

        // -- remove the entry (replace with the last)
        if (found < g_postOpEntryCount - 1)
        {
            g_postOpEntryList[found].Set(g_postOpEntryList[g_postOpEntryCount - 1].m_valType,
                                         g_postOpEntryList[g_postOpEntryCount - 1].m_valAddr,
                                         g_postOpEntryList[g_postOpEntryCount - 1].m_postOpAdjust, true);
        }

        // -- decrement the count
        --g_postOpEntryCount;
    }

    // -- return the result
    return (success);
}

// ====================================================================================================================
// GetStackVarAddr():  Get the address of a stack veriable, given the actual variable entry
// ====================================================================================================================
void* GetStackVarAddr(CScriptContext* script_context, const CExecStack& execstack,
                      const CFunctionCallStack& funccallstack, CVariableEntry& ve, int32 array_var_index)
{
    // -- ensure the variable is a stack variable
    if (!ve.IsStackVariable(funccallstack, array_var_index == 0))
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - GetStackVarAddr() failed\n");
        return NULL;
    }

    int32 executing_stacktop = 0;
    CObjectEntry* oe = NULL;
    uint32 oe_id;
    CFunctionEntry* fe_executing = funccallstack.GetExecuting(oe_id, oe, executing_stacktop);

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
void* GetStackVarAddr(CScriptContext* script_context, const CExecStack& execstack,
                      const CFunctionCallStack& funccallstack, int32 stackvaroffset)
{
    int32 stacktop = 0;
    CObjectEntry* oe = NULL;
    uint32 oe_id;
    CFunctionEntry* fe = funccallstack.GetExecuting(oe_id, oe, stacktop);
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
    // -- sanity check
    if (valaddr == nullptr)
        return (false);

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
        bool val_is_hash_index = (valtype == TYPE__hashvarindex);
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
            TinPrint(script_context, "Error - Unable to find variable %d\n", UnHash(val1hash));
            return (false);
        }

        // -- set the type
        valtype = ve->GetType();

        // -- if the ve belongs to a function, and is not a hash table or parameter array, we need
        // -- to find the stack address, as all local variable live on the stack
        if (ve && ve->IsStackVariable(funccallstack, !val_is_hash_index))
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
            TinPrint(script_context, "Error - Unable to find object %d\n", varsource);
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
        CObjectEntry* stack_oe = NULL;
        uint32 stack_oe_id = 0;
        CFunctionEntry* fe = funccallstack.GetExecuting(stack_oe_id, stack_oe, stacktop);
        if (!fe)
            return (false);

        // -- would be better to have random access to a hash table
        tVarTable* var_table = fe->GetContext()->GetLocalVarTable();
        ve = var_table->FindItemByIndex(local_var_index);

        // -- if we're pulling a stack var of type_hashtable, then the hash table isn't a "value" that can be
        // -- modified locally, but rather it lives in the function context, and must be manually emptied
        // -- as part of "ClearParameters"
        if (valtype != TYPE_hashtable)
        {
            valaddr = GetStackVarAddr(script_context, execstack, funccallstack, ve->GetStackOffset());
            if (!valaddr)
            {
                TinPrint(script_context, "Error - Unable to find stack var\n");
                return false;
            }

		    // -- if we have a debugger attached, also find the variable entry associated with the stack var
		    int32 debugger_session = 0;
		    if (script_context->IsDebuggerConnected(debugger_session))
		    {
			    stacktop = 0;
			    stack_oe = NULL;
                stack_oe_id = 0;
			    fe = funccallstack.GetExecuting(stack_oe_id, stack_oe, stacktop);
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
                TinPrint(script_context, "Error - Unable to find stack var of type hashtable\n");
                return (false);
            }

            // -- otherwise, adjust the value address to be the actual hashtable
            // $$$TZA if this is a TYPE__stackvar, why are we using the oe??
            //valaddr = ve->GetAddr(oe ? oe->GetAddr() : NULL);
            valaddr = ve->GetAddr(NULL);
        }
    }

    // -- if a POD member was pushed...
    else if (valtype == TYPE__podmember)
    {
        // -- the type and address of the variable/value has already been pushed
// -- in a 64-bit environment, we need to pull the type, and re-assemble
// the address from the stack		
#if BUILD_64		
        uint32* val_addr_64 = (uint32*)(valaddr);
        valtype = (eVarType)(val_addr_64[0]);
        valaddr = kPointer64FromUInt32(val_addr_64[1], val_addr_64[2]);
#else
        valtype = (eVarType)((uint32*)valaddr)[0];
        valaddr = (void*)((uint32*)valaddr)[1];
#endif
    }

    // -- if we weren't able to resolve the address for the actual value storage, then we'd better
    // -- have a valid stack variable
    bool8 valid_result = valaddr != nullptr || (ve != nullptr && ve->IsStackVariable(funccallstack, false));
    return (valid_result);
}

// ====================================================================================================================
// GetStackArrayVarAddr():  Return the address of the actual variable value, for the array + hash, on the stack
// ====================================================================================================================
bool8 GetStackArrayVarAddr(CScriptContext* script_context, CExecStack& execstack,
                           CFunctionCallStack& funccallstack, void*& valaddr, eVarType& valtype,
                           CVariableEntry*& ve, CObjectEntry*& oe)
{
    // -- hash value will have already been pushed
    eVarType contenttype;
    void* contentptr = execstack.Peek(contenttype);
    if (contenttype != TYPE_int)
    {
        TinPrint(script_context, "Error - ExecStack should contain TYPE_int\n");
        return false;
    }
    int32 arrayvarhash = *(int32*)contentptr;

    // -- next, pop the hash table variable off the stack
    // -- pull the hashtable variable off the stack
    CVariableEntry* ve0 = NULL;
    CObjectEntry* oe0 = NULL;
    eVarType val0type;
    void* val0 = execstack.Peek(val0type, 1);
    if (!GetStackValue(script_context, execstack, funccallstack, val0, val0type, ve0, oe0))
    {
        TinPrint(script_context, "Error - ExecStack should contain a hashtable variable\n");
        return false;
    }

    if (!ve0 || (ve0->GetType() != TYPE_hashtable && !ve0->IsArray()))
    {
        TinPrint(script_context, "Error - ExecStack should contain hashtable variable\n");
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
    ve = GetVariable(script_context, script_context->GetGlobalNamespace()->GetVarTable(),
                     ns_hash, func_or_obj, ve0->GetHash(), arrayvarhash);
    if (!ve)
    {
        TinPrint(script_context, "Error - Unable to find a variable entry\n");
        return false;
    }

    // -- push the variable onto the stack
    // -- if the variable is a stack parameter, we need to push it's value from the stack
    valtype = ve->GetType();
    if (ve->IsStackVariable(funccallstack, arrayvarhash == 0))
    {
        valaddr = GetStackVarAddr(script_context, execstack, funccallstack, *ve, arrayvarhash);
    }
    else
    {
        valaddr = ve->IsArray() ? ve->GetArrayVarAddr(oe0 ? oe0->GetAddr() : NULL, arrayvarhash)
                                : ve->GetAddr(oe0 ? oe0->GetAddr() : NULL);
    }

    // -- success
    return (true);
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
        TinPrint(TinScript::GetContext(), "Error - failed GetBinopValues() for operation: %s\n",
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
    eVarType result_type = TYPE__resolve;

    bool success = (priority_op_func && priority_op_func(script_context, op, result_type, (void*)result,
                                                         val0type, val0, val1type, val1));

    // -- if the priority version didn't pan out, try the secondary type version
    if (!success)
    {
        success = (secondary_op_func && secondary_op_func(script_context, op, result_type, (void*)result,
                                                          val0type, val0, val1type, val1));
    }

    // -- apply any post-unary ops (increment/decrement)
    ApplyPostUnaryOpEntry(val0type, val0);
    ApplyPostUnaryOpEntry(val1type, val1);

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

// --------------------------------------------------------------------------------------------------------------------
// -- for consecutive assignments, we need to push the previous assignment result back onto the stack
eVarType g_lastAssignResultType = TYPE_void;
uint32 g_lastAssignResultBuffer[MAX_TYPE_SIZE];

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

        // -- if the operation isn't a simple assignment, we've got the variable to be assigned - perform the op
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
            return (false);
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
        return (false);

    // -- cache the result value, because we'll need to push it back onto the stack if we have consecutive assignments
    // $$$TZA SendArray - need to support array return values
    if (val1type != TYPE_hashtable && ve1 != nullptr && !ve1->IsArray())
    {
        g_lastAssignResultType = val1type;
        memcpy(g_lastAssignResultBuffer, val1addr, MAX_TYPE_SIZE * sizeof(uint32));
    }
    else
    {
        g_lastAssignResultType = TYPE_void;
    }

    // -- we're also going to convert val1add to the required type for assignment
    void* val1_convert = val1addr;

	// -- pop the var
    CVariableEntry* ve0 = NULL;
    CObjectEntry* oe0 = NULL;
	eVarType varhashtype;
	void* var = execstack.Pop(varhashtype);
    bool8 is_stack_var = (varhashtype == TYPE__stackvar);
    bool8 is_pod_member = (varhashtype == TYPE__podmember);
    bool8 use_var_addr = (is_stack_var || is_pod_member);
    if (!GetStackValue(script_context, execstack, funccallstack, var, varhashtype, ve0, oe0))
        return (false);

    // -- if the variable is a local variable, we also have the actual address already
    use_var_addr = use_var_addr || (ve0 && ve0->IsStackVariable(funccallstack, false));

    // -- ensure we're assigning to a variable, an object member, or a local stack variable
    if (!ve0 && !use_var_addr)
        return (false);

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

        val1_convert = TypeConvert(script_context, val1type, val1addr, varhashtype);
        if (!val1_convert)
        {
            ScriptAssert_(script_context, 0, "<internal>", -1,
                          "Error - fail to conver from type %s to type %s\n",
                          GetRegisteredTypeName(val1type), GetRegisteredTypeName(varhashtype));
            return (false);
        }
        memcpy(var, val1_convert, gRegisteredTypeSize[varhashtype]);
        DebugTrace(op, is_stack_var ? "StackVar: %s" : "PODMember: %s", DebugPrintVar(var, varhashtype));

        // -- apply any post-unary ops (increment/decrement)
        ApplyPostUnaryOpEntry(val1type, val1addr);
        ApplyPostUnaryOpEntry(varhashtype, var);

        // -- if we're connected to the debugger, then the variable entry associated with the stack var will be returned,
		// -- notify we're breaking on it
		if (ve0)
			ve0->NotifyWrite(script_context, &execstack, &funccallstack);
    }

    // -- else set the value through the variable entry
    else
    {
        val1_convert = TypeConvert(script_context, val1type, val1addr, ve0->GetType());
        if (!val1_convert)
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
        //if (ve0->IsParameter() && ve0->GetArraySize() == -1 && ve1 && ve1->IsArray() &&
        //    ve0->GetType() == ve1->GetType())
        if (ve0->IsParameter() && ve1 && ve1->IsArray() && ve0->GetType() == ve1->GetType())
        {
            ve0->InitializeArrayParameter(ve1, oe1, execstack, funccallstack);
        }

        else if (!ve0->IsArray())
        {
            ve0->SetValue(oe0 ? oe0->GetAddr() : NULL, val1_convert, &execstack, &funccallstack);
            DebugTrace(op, "Var %s: %s", UnHash(ve0->GetHash()), DebugPrintVar(val1_convert, ve0->GetType()));

            // -- apply any post-unary ops (increment/decrement)
            // -- (to the non-convert, original address)
            ApplyPostUnaryOpEntry(val1type, val1addr);
            ApplyPostUnaryOpEntry(varhashtype, var);
        }
        else
        {
            // $$$TZA need a better way to determine the array index
            void* ve0_addr = ve0->GetAddr(oe0 ? oe0->GetAddr() : NULL);
            int32 byte_count = kPointerDiffUInt32(var, ve0_addr);
            int32 array_index = byte_count / gRegisteredTypeSize[ve0->GetType()];
            ve0->SetValue(oe0 ? oe0->GetAddr() : NULL, val1_convert, &execstack, &funccallstack, array_index);

            // -- apply any post-unary ops (increment/decrement)
            // -- I *believe* the actual a ve0->SetValue() will end up writing to the same address as "var"...
            ApplyPostUnaryOpEntry(val1type, val1addr);
            ApplyPostUnaryOpEntry(varhashtype, var);
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
// OpExecDebugMsg():  DebugMsg operation, benign.
// ====================================================================================================================
bool8 OpExecDebugMsg(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                     CFunctionCallStack& funccallstack)
{
    DebugTrace(op, "");

    // -- get the debug string
    uint32 string_hash = *instrptr++;
    const char* debug_msg = cb->GetScriptContext()->GetStringTable()->FindString(string_hash);
    TinPrint(cb->GetScriptContext(), "\n%s\n", debug_msg);

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
// OpPushAssignValue() : Push the last assignment value back onto the stack.
// ====================================================================================================================
bool8 OpExecPushAssignValue(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                            CFunctionCallStack& funccallstack)
{
    if (g_lastAssignResultType == TYPE_void)
    {
        ScriptAssert_(cb->GetScriptContext(), 0, "<internal>", -1,
                      "Error - Consecutive Assign operation without a previous result\n");
        return (false);
    }

    // -- push the last value assigned
    execstack.Push((void*)g_lastAssignResultBuffer, g_lastAssignResultType);

    // -- success
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
                        "Error - unable to perform op: %s\nEnsure the variable exists, and the types are valid.\n",
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
                        "Error - unable to perform op: %s\nEnsure the variable exists, and the types are valid.\n",
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
                        "Error - unable to perform op: %s\nEnsure the variable exists, and the types are valid.\n",
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
                        "Error - unable to perform op: %s\nEnsure the variable exists, and the types are valid.\n",
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
                        "Error - unable to perform op: %s\nEnsure the variable exists, and the types are valid.\n",
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
                        "Error - unable to perform op: %s\nEnsure the variable exists, and the types are valid.\n",
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
                        "Error - unable to perform op: %s\nEnsure the variable exists, and the types are valid.\n",
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
                        "Error - unable to perform op: %s\nEnsure the variable exists, and the types are valid.\n",
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
                        "Error - unable to perform op: %s\nEnsure the variable exists, and the types are valid.\n",
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
                        "Error - unable to perform op: %s\nEnsure the variable exists, and the types are valid.\n",
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
                        "Error - unable to perform op: %s\nEnsure the variable exists, and the types are valid.\n",
                        GetOperationString(op));
        return (false);
    }
    return (true);
}


// --------------------------------------------------------------------------------------------------------------------
// PerformUnaryPreOp():  Pre-increment/decrement unary operation.
// --------------------------------------------------------------------------------------------------------------------
bool8 PerformUnaryPreOp(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                        CFunctionCallStack& funccallstack)
{
    // -- first get the variable we're assigning
    eVarType assign_type;
	void* assign_var = execstack.Peek(assign_type);
    if (!assign_var)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - Failed to pop stack variable, performing: %s\n", GetOperationString(op));
        return (false);
    }

    // push the adjustment onto the stack, and perform an AssignAdd
    int32 value = (op == OP_UnaryPreInc ? 1 : -1);
    execstack.Push(&value, TYPE_int);
    if (!PerformAssignOp(cb->GetScriptContext(), execstack, funccallstack, OP_AssignAdd))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - Failed to perform op: %s\n", GetOperationString(op));
        return (false);
    }

    // -- success
    return (true);
}

// ====================================================================================================================
// OpExecUnaryPreInc():  Pre-increment unary operation.
// ====================================================================================================================
bool8 OpExecUnaryPreInc(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
    CFunctionCallStack& funccallstack)
{
    return (PerformUnaryPreOp(cb, op, instrptr, execstack, funccallstack));
}

// ====================================================================================================================
// OpExecUnaryPreDec():  Pre-increment unary operation.
// ====================================================================================================================
bool8 OpExecUnaryPreDec(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
    CFunctionCallStack& funccallstack)
{
    return (PerformUnaryPreOp(cb, op, instrptr, execstack, funccallstack));
}

// --------------------------------------------------------------------------------------------------------------------
// PerformUnaryPostOp():  Post-increment unary operation.
// --------------------------------------------------------------------------------------------------------------------
bool8 PerformUnaryPostOp(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                         CFunctionCallStack& funccallstack)
{
    bool is_array_var = *instrptr++ != 0;

    // -- these are the details we need to find out where to apply the post inc
    CVariableEntry* ve = NULL;
    CObjectEntry* oe = NULL;
    eVarType valtype;
    void* valaddr;
    
    // -- if we're incrementing a hashtable or array element, we need to peek at the top two stack entries
    if (is_array_var)
    {
        if (!GetStackArrayVarAddr(cb->GetScriptContext(), execstack, funccallstack, valaddr, valtype, ve, oe))
        {
            DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                            "Error - no hashtable/array, index on the stack for op: %s\n", GetOperationString(op));
            return false;

        }
    }

    // -- otherwise, the top entry is the variable to be incremented
    else
    {
        valaddr = execstack.Peek(valtype);
        if (!GetStackValue(cb->GetScriptContext(), execstack, funccallstack, valaddr, valtype, ve, oe))
        {
            DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                            "Error - no variable on the stack for op: %s\n", GetOperationString(op));
            return false;
        }
    }

    // -- add a post op adjust (OP_UnaryPostInc == 1) to the specific address peek'd from the stack
    AddPostUnaryOpEntry(valtype, valaddr, op == OP_UnaryPostInc ? 1 : -1);

    DebugTrace(op, "%s", DebugPrintVar((void*)valaddr, valtype));

    // -- success
    return (true);
}

// ====================================================================================================================
// OpExecUnaryPostInc():  Post-increment unary operation.
// ====================================================================================================================
bool8 OpExecUnaryPostInc(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                         CFunctionCallStack& funccallstack)
{
    return (PerformUnaryPostOp(cb, op, instrptr, execstack, funccallstack));
}

// ====================================================================================================================
// OpExecUnaryPostDec():  Post-decrement unary operation.
// ====================================================================================================================
bool8 OpExecUnaryPostDec(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                         CFunctionCallStack& funccallstack)
{
    return (PerformUnaryPostOp(cb, op, instrptr, execstack, funccallstack));
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

    // -- why you would post increment/decrement a variable after bit-inverting is questionable...  but supported
    ApplyPostUnaryOpEntry(valtype, valaddr);

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

    // -- post increment/decrement support
    ApplyPostUnaryOpEntry(valtype, valaddr);

    // -- success
    return (true);
}

// ====================================================================================================================
// OpExecInclude():  Executes the given script *immediately*, so "included" globals are defined for this script
// ====================================================================================================================
bool8 OpExecInclude(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                    CFunctionCallStack& funccallstack)
{
    // -- next instruction is the variable name
    uint32 filename_hash = *instrptr++;
    const char* filename = UnHash(filename_hash);
    cb->GetScriptContext()->ExecScript(filename, true, true);
    DebugTrace(op, "Script: %s", filename = UnHash(filename_hash));
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
// OpExecPushCopy():  Pushes a copy of whatever is on top of the stack.
// ====================================================================================================================
bool8 OpExecPushCopy(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
    CFunctionCallStack& funccallstack)
{
    eVarType valtype;
    CVariableEntry* ve = NULL;
    CObjectEntry* oe = NULL;
    void* val = execstack.Peek(valtype);
    if (val == nullptr)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - stack is empty, op: %s\n", GetOperationString(op));
        return false;
    }

    // -- push the a copy back onto the stack
    execstack.Push(val, valtype);
    DebugTrace(op, "%s", DebugPrintVar((void*)val, valtype));

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
    // $$$TZA FIXME arrays of hashtables?
    eVarType vetype = ve->GetType();
    void* veaddr = NULL;
    if (ve->IsStackVariable(funccallstack, false))
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
// note:  64-bit, we push the upper and lower 64-bit address
#if BUILD_64
    uint32 varbuf[3];
    varbuf[0] = pod_member_type;
    varbuf[1] = kPointer64UpperUInt32(pod_member_addr);
	varbuf[2] = kPointer64LowerUInt32(pod_member_addr);
#else
    uint32 varbuf[2];
    varbuf[0] = pod_member_type;
    varbuf[1] = (uint32)pod_member_addr;
#endif	

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
    uint32 oe_id = 0;
    CFunctionEntry* fe = funccallstack.GetExecuting(oe_id, oe, stacktop);

    // if the stack is *supposed* to be pushing an object, but it no longer exists, this is a runtime error
    // we'll re-acquire it here
    if (oe_id != 0)
    {
        oe = cb->GetScriptContext()->FindObjectEntry(oe_id);
    }

    if (!fe || !oe)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - PushSelf() - object no longer exists (or not a self method).\n");
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

    // -- if we have a pending post unary op to apply, we have to find out what was on the stack, and potentially
    // -- apply the unary op
    if (g_postOpEntryCount > 0)
    {
        CVariableEntry* ve = NULL;
        CObjectEntry* oe = NULL;
        if (!GetStackValue(cb->GetScriptContext(), execstack, funccallstack, content, contenttype, ve, oe))
        {
            DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                            "Error - GetStackValue() failed\n");
            return (false);
        }

        // -- post increment/decrement support
        ApplyPostUnaryOpEntry(contenttype, content);
    }

    return (true);
}

// ====================================================================================================================
// OpExecForeachIterInit():  Initializes the stack with the interator var, assigned to the containsers "first" value
// ====================================================================================================================
bool8 OpExecForeachIterInit(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                            CFunctionCallStack& funccallstack)
{
    // -- on the stack, we should have:
    // the container at a depth 1 below the top
    // the iterator variable at the top...

    // -- at the start of the foreach loop, all we need to do is push a -1 index, and then get the "next" container value
    int32 initial_index = -1;
    execstack.Push(&initial_index, TYPE_int);

    return (OpExecForeachIterNext(cb, op, instrptr, execstack, funccallstack));    
}

// ====================================================================================================================
// OpExecForeachIterValid():  
// ====================================================================================================================
bool8 OpExecForeachIterValid(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                            CFunctionCallStack& funccallstack)
{
    return false;
}


// ====================================================================================================================
// OpExecForeachIterNext():  
// ====================================================================================================================
bool8 OpExecForeachIterNext(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                            CFunctionCallStack& funccallstack)
{
    // -- on the stack, we should have:
    // the container at a depth 2 below the top
    // the iterator variable at 1 below top...
    // the "index" at top

    // -- uses peek, so the stack is unchanged
    eVarType container_valtype;
    CVariableEntry* container_ve = NULL;
    CObjectEntry* container_oe = NULL;
    void* container_val = execstack.Peek(container_valtype, 2);
    if (!GetStackValue(cb->GetScriptContext(), execstack, funccallstack, container_val, container_valtype, container_ve,
        container_oe) || container_val == nullptr)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - foreach loop expecting a container variable (e.g. array) on the stack\n");
        return false;
    }

    // -- make sure we got a valid address for the container entry value
    if (container_ve == nullptr || !container_ve->IsArray())
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - foreach() only supports arrays, CObjectgGroup and hashtable variable support coming.\n");
        return (false);
    }

    // -- uses still uses peek, so the stack is unchanged
    eVarType iter_valtype;
    CVariableEntry* iter_ve = NULL;
    CObjectEntry* iter_oe = NULL;
    void* iter_val = execstack.Peek(iter_valtype, 1);
    if (!GetStackValue(cb->GetScriptContext(), execstack, funccallstack, iter_val, iter_valtype, iter_ve,
        iter_oe) || iter_val == nullptr)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - foreach loop expecting a container variable (e.g. array) on the stack\n");
        return false;
    }

    // -- pop the index from the stack
    eVarType index_valtype;
    CVariableEntry* index_ve = NULL;
    CObjectEntry* index_oe = NULL;
    void* index_val = execstack.Pop(index_valtype);
    if (index_valtype != TYPE_int)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                       "Error - foreach loop expecting a int index on the stack\n");
        return false;
    }

    // -- get the current index, and increment
    int32 cur_index = *(int32*)index_val;
    ++cur_index;

    // -- push the current index back onto the stack, for the next loop iteration
    // note:  exiting the foreach loop always expects to pop three stack entries
    execstack.Push(&cur_index, TYPE_int);

    // -- if we have a next, we assign the next value to the iterator, and push the current index,
    // and push true (for the while loop to continue)
    // -- otherwise, we pop the container and iter vars off the stack since they're no longer needed, and
    // push false on the stack, to exit the while loop
    // $$$TZA break and continue also need to pop the stack....!!
    void* container_entry_val = nullptr;
    if (container_ve->IsArray())
    {
        // -- ensure it's within range
        if (cur_index >= 0 && cur_index < container_ve->GetArraySize())
        {
            // -- get the address for the value at the specific index
            container_entry_val = container_ve->GetArrayVarAddr(container_oe ? container_oe->GetAddr() : nullptr, cur_index);

            // -- see if we can convert that value to our iterator type
            container_entry_val = TypeConvert(cb->GetScriptContext(), container_ve->GetType(), container_entry_val,
                                              iter_ve->GetType());

            // -- we have a valid container entry, but won't be able to assign it (incompatible types) -
            if (container_entry_val == nullptr)
            {
                DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                                "Error - foreach() unable to assign container value to iter variable\n");
                return (false);
            }
        }
    }

    // -- debug trace output
    DebugTrace(op, "Container var: %s, iter var: %s, index: %d, valid: %s", UnHash(container_ve->GetHash()),
                    UnHash(iter_ve->GetHash()), cur_index - 1, (container_entry_val != nullptr ? "true" : "false"));

    // -- if we have a "next", make the assignment
    if (container_entry_val != nullptr)
    {
        // -- the assignment is a simple memcpy (from the converted value, so the types match of course)
        memcpy(iter_val, container_entry_val, gRegisteredTypeSize[iter_ve->GetType()]);

        // -- push 'true', so the while loop will step into the body
        bool val_true = true;
        execstack.Push(&val_true, TYPE_bool);
    }
    
    // -- else, we don't have a "next value"...
    else
    {
        // -- push false on the stack, so our while loop exits
        bool val_false = false;
        execstack.Push(&val_false, TYPE_bool);
    }

    // -- either way, the operation executed successfully
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
// OpExecBranchCond():  Branch based on the conditional type (true/false), and if a short-circuit, don't pop the stack
// ====================================================================================================================
bool8 OpExecBranchCond(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                       CFunctionCallStack& funccallstack)
{
    bool8 branch_true = (*instrptr++ != 0);
    bool8 short_circuit = (*instrptr++ != 0);
    int32 jumpcount = *instrptr++;

	// -- top of the stack had better be a bool8
	eVarType valtype;
	void* valueraw = execstack.Pop(valtype);
	bool8* convertAddr = (bool8*)TypeConvert(cb->GetScriptContext(), valtype, valueraw, TYPE_bool);
	if (!convertAddr)
	{
		DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
						"Error - expecting a bool\n");
		return (false);
	}

    // -- if this is a short-circuit conditional, push the result back on the stack, as a bool
    if (short_circuit)
    {
        bool8 boolresult = *convertAddr;
        execstack.Push((void*)&boolresult, TYPE_bool);
    }

	// -- branch, if the conditional matches
    if (*convertAddr == branch_true)
        instrptr += jumpcount;

	DebugTrace(op, "%s, count: %d", *convertAddr ? "true" : "false", jumpcount);

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
        if (function_ns == nullptr)
            function_ns = cb->GetScriptContext()->FindOrCreateNamespace(UnHash(namespacehash));
        CNamespace* parent_ns = cb->GetScriptContext()->FindNamespace(parent_ns_hash);
        if (parent_ns == nullptr)
            parent_ns = cb->GetScriptContext()->FindOrCreateNamespace(UnHash(parent_ns_hash));
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
    // -- we're also going to initialize the parameters to the default values (if set, zero otherwise)
    fe->GetContext()->InitDefaultArgs(fe);

    funccallstack.Push(fe, NULL, execstack.GetStackTop());
    DebugTrace(op, "%s", UnHash(fe->GetHash()));

    // -- create space on the execstack, if this is a script function
    if (fe->GetType() != eFuncTypeRegistered)
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

    // -- see if this is a "super" method call
    // (e.g.  call it in the hierarchy, starting with the parent of the current namespace)
    bool is_super = *instrptr++ == 0 ? false : true;

    // -- get the hash of the method name
    uint32 methodhash = *instrptr++;

    // -- pull the object variable off the stack
    CVariableEntry* ve_obj = NULL;
    CObjectEntry* oe_obj = NULL;
    eVarType valtype_obj;
    void* val_obj = execstack.Pop(valtype_obj);
    if (!GetStackValue(cb->GetScriptContext(), execstack, funccallstack, val_obj, valtype_obj, ve_obj, oe_obj))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain an object id/variable\n");
        return false;
    }

    // -- convert the value to an object id
    // $$$TZA need to think about this - do we want to allow coercion of type int to type object?
    // for now, no - originally there was no assert when a var was declared (say, as an int),
    // and later declared and assigned as an object...
    // caused confusion when object foo = ..., foo.ListMethods(), without knowing foo was actually an int
    
    // conversion from int to object, if we allow, is here...
    //void* val_obj_addr = TypeConvert(cb->GetScriptContext(), valtype_obj, val_obj, TYPE_object);
    void* val_obj_addr = valtype_obj == TYPE_object ? val_obj : nullptr;

    if (val_obj_addr == nullptr)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain TYPE_object\n");
        return false;
    }

    // -- TYPE_object is actually just an uint32 ID
    uint32 objectid = *(uint32*)val_obj_addr;

    // -- find the object
    CObjectEntry* oe = cb->GetScriptContext()->FindObjectEntry(objectid);
    if (!oe)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - Unable to find object %d\n", objectid);
        return false;
    }

    // -- find the function to call
    CFunctionEntry* fe = nullptr;

    // -- if we're looking for a super::method(), then we want the fe from an ancestor
    // of the current ns_hash for the object
    if (is_super)
    {
        fe = oe->GetSuperFunctionEntry(nshash, methodhash);
    }

    // else find the method entry from the object's namespace hierarchy
    // -- if nshash is 0, then it's from the top of the hierarchy
    else
    {
        fe = oe->GetFunctionEntry(nshash, methodhash);
    }

    if (!fe)
    {
        // -- $$$TZA hmm...  should we allow super::method() calls when there isn't anything defined
        // in the hierarchy?  we may want to modify HasMethod(), or add HasSuperMethod()...
        if (is_super)
        {
            DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                            "Error - failed to execute super::%s()\nno ancestor defines an implementation in the hierarchy of namespace %s::\nfor object %d",
                            UnHash(methodhash), UnHash(nshash), oe->GetID());
        }
        else
        {
            if (nshash != 0)
            {
                DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                                "Error - Unable to find method %s::%s() for object %d\n",
                                 UnHash(nshash), UnHash(methodhash), oe->GetID());
            }
            else
            {
                DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                                "Error - Unable to find a method %s() for object %d\n",
                                UnHash(nshash), UnHash(methodhash), oe->GetID());
            }
        }
        return false;
    }

    // -- push the function entry onto the call stack
    // -- we're also going to initialize the parameters to the default values (if set, zero otherwise)
    fe->GetContext()->InitDefaultArgs(fe);

    // -- push the function entry onto the call stack
    funccallstack.Push(fe, oe, execstack.GetStackTop());

    // -- create space on the execstack, if this is a script function
    if (fe->GetType() != eFuncTypeRegistered)
    {
        int32 localvarcount = fe->GetContext()->CalculateLocalVarStackSize();
        execstack.Reserve(localvarcount * MAX_TYPE_SIZE);
    }

    DebugTrace(op, "obj: %d, ns: %s, func: %s", oe->GetID(), UnHash(nshash),
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

    // -- output the trace message
    DebugTrace(op, "func: %s", UnHash(fe->GetHash()));
    
    // -- execute the function
    bool8 result = CodeBlockCallFunction(fe, oe, execstack, funccallstack, false);

    // -- if executing a function call failed, assert
    if (!result || funccallstack.mDebuggerFunctionReload != 0)
    {
        // -- we only assert if the failure is not because of a function reload
        if (funccallstack.mDebuggerFunctionReload == 0)
        { 
            DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                            "Error - Failure executing function: %s()\n",
                            UnHash(fe->GetHash()));
        }

        // -- either way, we're done
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

    // -- in addition, all post-unary ops had better have been applied
    if (g_postOpEntryCount > 0)
    {
        ScriptAssert_(cb->GetScriptContext(), 0, cb->GetFileName(), cb->CalcLineNumber(instrptr),
                      "Error - There is still an outstanding post unary op that has not been applied\n");
    }

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

        // -- calculate the updated hash (note:  we only append a '_' between hash string elements)
        // -- this allows us to view a hashtable key of an unappended string, the same as hash(string)
        const char* val1String = UnHash(*(uint32*)val1addr);
        uint32 hash = *(uint32*)contentptr;
        if (hash != 0)
        {
            hash = HashAppend(hash, "_");
            hash = HashAppend(hash, val1String);
        }
        else
        {
            hash = Hash(val1String, -1, false);
        }

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

        // -- post increment/decrement support
        ApplyPostUnaryOpEntry(val1type, val1);
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
        hte = TinAlloc(ALLOC_VarEntry, CVariableEntry, cb->GetScriptContext(), UnHash(hashvalue),
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
// OpExecArrayCount():  Pops the array variable, and pushes the size of the array back onto the stack.
// ====================================================================================================================
bool8 OpExecArrayCount(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
	CFunctionCallStack& funccallstack)
{
	// -- pull the array variable off the stack
	CVariableEntry* ve = NULL;
	CObjectEntry* oe = NULL;
	eVarType valtype;
	void* val = execstack.Pop(valtype);
	if (!GetStackValue(cb->GetScriptContext(), execstack, funccallstack, val, valtype, ve, oe))
	{
		DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
					    "Error - ExecStack should contain an array variable\n");
		return false;
	}

	// -- ensure we found an array
	if (!ve || !ve->IsArray())
	{
		DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
			"Error - ExecStack should contain an array variable\n");
		return false;
	}

	// -- get and the array count
	int32 count = ve->GetArraySize();
	execstack.Push(&count, TYPE_int);

	DebugTrace(op, "Array: %s[%d]", UnHash(ve->GetHash()), count);

	return (true);
}

// ====================================================================================================================
// OpExecArrayCopy():  implement me!
// ====================================================================================================================
bool8 OpExecArrayCopy(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
	CFunctionCallStack& funccallstack)
{
    // $$$TZA implement me!
	return (false);
}

// ====================================================================================================================
// OpExecArrayContains():  Pushes a bool, if the pushed array contains the pushed value
// ====================================================================================================================
bool8 OpExecArrayContains(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
	CFunctionCallStack& funccallstack)
{
	// -- pull the array variable off the stack
	CVariableEntry* ve_1 = NULL;
	CObjectEntry* oe_1 = NULL;
	eVarType valtype_1;
	void* val_1 = execstack.Pop(valtype_1);
	if (!GetStackValue(cb->GetScriptContext(), execstack, funccallstack, val_1, valtype_1, ve_1, oe_1))
	{
		DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
					    "Error - ExecStack should contain a value\n");
		return false;
	}

    // -- pull the array variable off the stack
    CVariableEntry* ve_0 = NULL;
    CObjectEntry* oe_0 = NULL;
    eVarType valtype_0;
    void* val_0 = execstack.Pop(valtype_0);
    if (!GetStackValue(cb->GetScriptContext(), execstack, funccallstack, val_0, valtype_0, ve_0, oe_0))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain an array variable\n");
        return false;
    }

	// -- ensure we found an array
	if (!ve_0 || !ve_0->IsArray())
	{
		DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
			            "Error - ExecStack should contain an array variable\n");
		return false;
	}

    // -- first, ensure the value can be converted to that contained by the array
    void* compare_val = TypeConvert(cb->GetScriptContext(), valtype_1, val_1, valtype_0);
    if (compare_val == nullptr)
    {
        bool return_false = false;
        execstack.Push(&return_false, TYPE_bool);

        DebugTrace(op, "Array: %s[] does not contain: %s", UnHash(ve_0->GetHash()), DebugPrintVar(ve_1, valtype_1));
        return true;
    }

    TypeOpOverride compare_func = GetTypeOpOverride(OP_CompareEqual, valtype_0);
    if (compare_func == nullptr)
    {
        bool return_false = false;
        execstack.Push(&return_false, TYPE_bool);

        DebugTrace(op, "Array: %s[] has no compare for type: %s", UnHash(ve_0->GetHash()), GetRegisteredTypeName(valtype_0));
        return false;
    }

    // -- this is a painful linear search...
	// -- get and the array count
	int32 count = ve_0->GetArraySize();
    bool found = false;
    for (int32 i = 0; i < count; ++i)
    {
        void* array_val = ve_0->GetArrayVarAddr(oe_0 ? oe_0->GetAddr() : nullptr, i);
        if (array_val == nullptr)
            continue;

        // -- if we found an operation, see if it can be performed successfully
        char result[MAX_TYPE_SIZE * sizeof(uint32)];
        eVarType result_type = TYPE__resolve;
        bool success = (compare_func(cb->GetScriptContext(), OP_CompareEqual, result_type, (void*)result,
                                     valtype_0, array_val, valtype_0, compare_val));
        if (!success)
            continue;

        // -- note:  compare ops return -1, 0, 1 for (less than, equal, greater than), so we need a 0 return value
        // but in the type of the original args...  the most accurate here is to convert to a float
        void* compare_result = TypeConvert(cb->GetScriptContext(), result_type, result, TYPE_float);
        if (compare_result != nullptr && *(float*)compare_result == 0.0f)
        {
            found = true;
            break;
        }
    }

    // -- push the result, if the value was found
	execstack.Push(&found, TYPE_bool);

    // -- debug trace
    DebugTrace(op, "Array: %s[] %s: %s", UnHash(ve_0->GetHash()), (found ? "contains" : "not contains"),
                    DebugPrintVar(ve_1, valtype_1));

	return (true);
}

// ====================================================================================================================
// OpExecMathUnaryFunc():  Pops the top float value, and performs the math function (e.g. abs()), pushes the result
// ====================================================================================================================

using MathUnaryFunc = std::function<float(float)>;
MathUnaryFunc gMathUnaryFunctionTable[] =
{
    #define MathKeywordUnaryEntry(a, b) b,
    MathKeywordUnaryTuple
    #undef MathKeywordUnaryEntry
};

bool8 OpExecMathUnaryFunc(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
	                      CFunctionCallStack& funccallstack)
{
	// -- pull the float value off the stack
	CVariableEntry* ve = NULL;
	CObjectEntry* oe = NULL;
	eVarType valtype;
	void* valaddr = execstack.Pop(valtype);
	if (!GetStackValue(cb->GetScriptContext(), execstack, funccallstack, valaddr, valtype, ve, oe))
	{
		DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
					    "Error - ExecStack should contain a float value\n");
		return false;
	}

    // -- convert the value to an int (the only valid type a bit-invert operator is implemented for)
    void* convertaddr = TypeConvert(cb->GetScriptContext(), valtype, valaddr, TYPE_float);
    if (!convertaddr)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
            "Error - unable to convert from type %s to type TYPE_float, performing op: %s\n",
            gRegisteredTypeNames[valtype], GetOperationString(op));
        return false;
    }

    // -- get the unary math function type we're expecting to perform
    eMathUnaryFunctionType math_func_type = (eMathUnaryFunctionType)*instrptr++;

    float float_val = *(float*)(convertaddr);

    // -- perform the math unary op
    float float_result = gMathUnaryFunctionTable[math_func_type](float_val);

    // -- push the result
    execstack.Push(&float_result, TYPE_float);

	DebugTrace(op, "%s(%.4f) result: %.4f", GetMathUnaryFuncString(math_func_type), float_val, float_result);

	return (true);
}

// ====================================================================================================================
// OpExecMathBinaryFunc():  Pops 2x float values, and performs the math function (e.g. min()), pushes the result
// ====================================================================================================================

using MathBinaryFunc = std::function<float(float, float)>;
MathBinaryFunc gMathBinaryFunctionTable[] =
{
    #define MathKeywordBinaryEntry(a, b) b,
    MathKeywordBinaryTuple
    #undef MathKeywordBinaryEntry
};

bool8 OpExecMathBinaryFunc(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
    CFunctionCallStack& funccallstack)
{
    // -- pull the float value off the stack
    CVariableEntry* ve_1 = NULL;
    CObjectEntry* oe_1 = NULL;
    eVarType valtype_1;
    void* valaddr_1 = execstack.Pop(valtype_1);
    if (!GetStackValue(cb->GetScriptContext(), execstack, funccallstack, valaddr_1, valtype_1, ve_1, oe_1))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
            "Error - ExecStack should contain two float values\n");
        return false;
    }

    // -- convert the value to an int (the only valid type a bit-invert operator is implemented for)
    void* convertaddr_1 = TypeConvert(cb->GetScriptContext(), valtype_1, valaddr_1, TYPE_float);
    if (!convertaddr_1)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
            "Error - unable to convert from type %s to type TYPE_float, performing op: %s\n",
            gRegisteredTypeNames[valtype_1], GetOperationString(op));
        return false;
    }

    float float_val_1 = *(float*)(convertaddr_1);

        // -- pull the 2nd float value off the stack
    CVariableEntry* ve_0 = NULL;
    CObjectEntry* oe_0 = NULL;
    eVarType valtype_0;
    void* valaddr_0 = execstack.Pop(valtype_0);
    if (!GetStackValue(cb->GetScriptContext(), execstack, funccallstack, valaddr_0, valtype_0, ve_0, oe_0))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
            "Error - ExecStack should contain two float values\n");
        return false;
    }

    // -- convert the value to an int (the only valid type a bit-invert operator is implemented for)
    void* convertaddr_0 = TypeConvert(cb->GetScriptContext(), valtype_0, valaddr_0, TYPE_float);
    if (!convertaddr_0)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
            "Error - unable to convert from type %s to type TYPE_float, performing op: %s\n",
            gRegisteredTypeNames[valtype_0], GetOperationString(op));
        return false;
    }

    // -- get the binary math function type we're expecting to perform
    eMathBinaryFunctionType math_func_type = (eMathBinaryFunctionType)*instrptr++;

    float float_val_0 = *(float*)(convertaddr_0);

    // -- perform the math binary op
    float float_result = gMathBinaryFunctionTable[math_func_type](float_val_0, float_val_1);

    // -- push the result
    execstack.Push(&float_result, TYPE_float);

    DebugTrace(op, "%s() result: %.4f", GetMathBinaryFuncString(math_func_type), float_result);

    return (true);
}

// ====================================================================================================================
// OpExecHashtableHasKey():  Pops the hashtable variable, and pushes a bool if the given key exists.
// ====================================================================================================================
bool8 OpExecHashtableHasKey(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
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

    if (!ve0 || (ve0->GetType() != TYPE_hashtable))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain hashtable variable\n");
        return false;
    }

    // -- get the var table
    // note:  hashtable isn't a natural C++ type, so there's no such thing as
    // -- an addr + offset to an object's registered hashtable
    tVarTable* vartable = (tVarTable*)ve0->GetAddr(NULL);

    // -- look for the entry in the vartable
    CVariableEntry* vte = vartable->FindItem(arrayvarhash);

    // -- push true if we found an entry
    bool8 found = (vte != nullptr);
    execstack.Push(&found, TYPE_bool);
 
	DebugTrace(op, "HashTable: %s[%s] %s", UnHash(ve0->GetHash()), UnHash(arrayvarhash), found ? "found" : "not found");

	return (true);
}

// ====================================================================================================================
// OpExecHashtableContains():  Pushes a bool, if the pushed hashtable contains the pushed value
// ====================================================================================================================
bool8 OpExecHashtableContains(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
	CFunctionCallStack& funccallstack)
{
	// -- pull the hashtable variable off the stack
	CVariableEntry* ve_1 = NULL;
	CObjectEntry* oe_1 = NULL;
	eVarType valtype_1;
	void* val_1 = execstack.Pop(valtype_1);
	if (!GetStackValue(cb->GetScriptContext(), execstack, funccallstack, val_1, valtype_1, ve_1, oe_1))
	{
		DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
					    "Error - ExecStack should contain a value\n");
		return false;
	}

    // -- pull the hashtable "index value" off the stack
    CVariableEntry* ve_0 = NULL;
    CObjectEntry* oe_0 = NULL;
    eVarType valtype_0;
    void* val_0 = execstack.Pop(valtype_0);
    if (!GetStackValue(cb->GetScriptContext(), execstack, funccallstack, val_0, valtype_0, ve_0, oe_0))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain a hashtable variable\n");
        return false;
    }

	// -- ensure we found an array
    if (!ve_0 || ve_0->GetType() != TYPE_hashtable)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain hashtable variable\n");
        return false;
    }

    // -- get the var table
    // note:  hashtable isn't a natural C++ type, so there's no such thing as
    // -- an addr + offset to an object's registered hashtable
    tVarTable* vartable = (tVarTable*)ve_0->GetAddr(NULL);
    if (vartable == nullptr)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain hashtable variable\n");
        return false;
    }

    // -- iterate through the hashtable, see if the given value is there
    bool found = false;
    CVariableEntry* ht_ve = vartable->First();
    while (ht_ve != nullptr)
    {
        void* ht_val = ht_ve->GetAddr(nullptr);
        if (ht_val == nullptr)
        {
            ht_ve = vartable->Next();
            continue;
        }

        // -- see if we can convert the given value to the type stored in the hashtable
        void* convert_val = TypeConvert(cb->GetScriptContext(), valtype_1, val_1, ht_ve->GetType());
        if (convert_val == nullptr)
        {
            ht_ve = vartable->Next();
            continue;
        }

        TypeOpOverride compare_func = GetTypeOpOverride(OP_CompareEqual, ht_ve->GetType());
        if (compare_func == nullptr)
        {
            ht_ve = vartable->Next();
            continue;
        }

        // -- if we found an operation, see if it can be performed successfully
        char result[MAX_TYPE_SIZE * sizeof(uint32)];
        eVarType result_type = TYPE__resolve;
        bool success = (compare_func(cb->GetScriptContext(), OP_CompareEqual, result_type, (void*)result,
                                     ht_ve->GetType(), ht_val, ht_ve->GetType(), convert_val));
        if (!success)
        {
            ht_ve = vartable->Next();
            continue;
        }

        // -- note:  compare ops return -1, 0, 1 for (less than, equal, greater than), so we need a 0 return value
        // but in the type of the original args...  the most accurate here is to convert to a float
        void* compare_result = TypeConvert(cb->GetScriptContext(), result_type, result, TYPE_float);
        if (compare_result != nullptr && *(float*)compare_result == 0.0f)
        {
            found = true;
            break;
        }

        ht_ve = vartable->Next();
    }

    // -- push the result, if the value was found
	execstack.Push(&found, TYPE_bool);

    // -- debug trace
    DebugTrace(op, "hashtable: %s[] %s: %s", UnHash(ve_0->GetHash()), (found ? "contains" : "not contains"),
                    DebugPrintVar(ve_1, valtype_1));

	return (true);
}

// ====================================================================================================================
// OpExecHashtableIter():  Pops the hashtable, pushes the first key (string).
// ====================================================================================================================
bool8 OpExecHashtableIter(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
	CFunctionCallStack& funccallstack)
{
    // -- pop the bool, if this iter call is first() or next()
    int32 iter_type = *instrptr++;

    // -- pop the hashtable variable to get the first/next key
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

    if (!ve0 || ve0->GetType() != TYPE_hashtable)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain hashtable variable\n");
        return false;
    }

    // -- get the var table
    // note:  hashtable isn't a natural C++ type, so there's no such thing as
    // -- an addr + offset to an object's registered hashtable
    tVarTable* vartable = (tVarTable*)ve0->GetAddr(NULL);
    if (vartable != nullptr)
    {
        // -- if we're calling first() or next(), push the iteration value
        if (iter_type == 0 || iter_type == 1)
        {
            CVariableEntry* hte = iter_type == 0 ? vartable->First() : vartable->Next();

            // -- Push the contents of the return_ve onto *this* execstack
            if (hte != nullptr)
            {
                execstack.Push(hte->GetAddr(NULL), hte->GetType());
	            DebugTrace(op, "HashTable: %s iteration value: %s", UnHash(ve0->GetHash()), DebugPrintVar(hte, hte->GetType()), 0);
            }
            else
            {
                int32 null_val = 0;
                execstack.Push(&null_val, TYPE_int);
	            DebugTrace(op, "HashTable: %s, hashtable iter is at end", UnHash(ve0->GetHash()), 0);
            }
 
            return true;
        }

        // -- else we're checking for the end of the hashtable
        else
        {
            // -- calling Current() will see if the internal iterator is at the end
            CVariableEntry* hte = vartable->Current();

            bool8 at_end = (hte == nullptr);
            execstack.Push(&at_end, TYPE_bool);
            if (at_end)
	            DebugTrace(op, "HashTable: %s, iterator is at end", UnHash(ve0->GetHash()), 0);
            else
	            DebugTrace(op, "HashTable: %s, iterator is at valid entry", UnHash(ve0->GetHash()), 0);
        }

        // -- success
        return true;
    }
 
    // -- this should be impossible - to have successfully resolved a hashtable variable with no tVarTable...
    DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                    "Error - invalid hashtable \n");

	return (false);
}

// ====================================================================================================================
// OpExecHashtableCopy():  copies the given ht to either another hashtable, or a CHashtableObject (accessible from C++)
// ====================================================================================================================
bool8 OpExecHashtableCopy(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
	CFunctionCallStack& funccallstack)
{
    CScriptContext* script_context = cb->GetScriptContext();

    // -- pop the bool, if we're making a complete copy, or just wrapping the hashtable
    bool is_wrap = (*instrptr++) != 0 ? true : false;

    // -- pull the hashtable variable off the stack
	CVariableEntry* ve_1 = NULL;
	CObjectEntry* oe_1 = NULL;
	eVarType valtype_1;
	void* val_1 = execstack.Pop(valtype_1);
	if (!GetStackValue(script_context, execstack, funccallstack, val_1, valtype_1, ve_1, oe_1))
	{
		DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
					    "Error - ExecStack should contain a hashtable or CHashtable object value\n");
		return false;
	}

    CObjectEntry* target_ht_oe = nullptr;
    if (valtype_1 != TYPE_hashtable)
    {
        void* object_id = TypeConvert(script_context, valtype_1, val_1, TYPE_object);
        target_ht_oe = object_id != nullptr ? script_context->FindObjectEntry(*(uint32*)object_id)
                                            : nullptr;
        
        // -- see if we found an object entry to copy to
        if (target_ht_oe != nullptr)
        {
            // -- this is unusual to have the VM reference a registered class directly, however,
            // it is a built-in TinScript class that we use as a way to pass hashtables to registered functions
            static uint32 has_CHashtable = Hash("CHashtable");
            if (!target_ht_oe->HasNamespace(has_CHashtable))
            {
                target_ht_oe = nullptr;
            }
        }
    }

    // -- ensure we don't try to "wrap" a script hashtable with another
    else if (valtype_1 == TYPE_hashtable && is_wrap)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - hashtable_wrap() 2nd param must be a CHashtable object, not a hashtable var\n");
        return false;
    }

    // -- if we didn't find an appropriate target to copy the hashtable to, we're done
    if (valtype_1 != TYPE_hashtable && target_ht_oe == nullptr)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
					    "Error - ExecStack should contain a hashtable or CHashtable object value\n");
		return false;
    }

    // -- pull the hashtable variable off the stack
	CVariableEntry* ve_0 = NULL;
	CObjectEntry* oe_0 = NULL;
	eVarType valtype_0;
	void* val_0 = execstack.Pop(valtype_0);
	if (!GetStackValue(cb->GetScriptContext(), execstack, funccallstack, val_0, valtype_0, ve_0, oe_0))
	{
		DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
					    "Error - ExecStack should contain a hashtable value\n");
		return false;
	}

    // -- now perform the copy
    if (target_ht_oe != nullptr)
    {
        // $$$TZA test/support C++ members that are of type CHashtable*...
        CHashtable* cpp_ht = (CHashtable*)(target_ht_oe->GetAddr());
        if (is_wrap)
        {
            cpp_ht->Wrap(ve_0);
        }

        else if (!cpp_ht->CopyFromHashtableVE(ve_0))
        {
            DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                            "Error - Failed to copy hashtable to CHashTable object\n");
            return false;
        }
    }
    else
    {
        // -- we're going to copy from ve_0 to ve_1
        // (we've already checked for is_wrap to a non-object hashtable variable
        if (!CHashtable::CopyHashtableVEToVe(ve_0, ve_1))
        {
            DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                            "Error - Failed to copy hashtable to hashtable variable\n");
            return false;
        }
    }

    // -- success
    return true;
}

// ====================================================================================================================
// OpType():  Pops the variable, pushes the string representation of its type.
// ====================================================================================================================
bool8 OpExecType(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
	CFunctionCallStack& funccallstack)
{
    // -- pop the variable to get the first/next key
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

    const char* type_str = GetRegisteredTypeName(val0type);
    if (type_str == nullptr)
        type_str = "";

    uint32 string_hash = Hash(type_str);
    execstack.Push(&string_hash, TYPE_string);
	DebugTrace(op, "Type: %s", GetRegisteredTypeName(val0type));

	return (true);
}

// ====================================================================================================================
// OpEnsure():  Pops the error message, and the conditional result, pushes the conditional result back on.
// ====================================================================================================================
bool8 OpExecEnsure(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
	CFunctionCallStack& funccallstack)
{
    // -- pop the error string
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
    if (val0type != TYPE_string)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain a string\n");
        return false;
    }

    // -- pop the conditional string
    CVariableEntry* ve1 = NULL;
    CObjectEntry* oe1 = NULL;
    eVarType val1type;
    void* val1 = execstack.Pop(val1type);
    if (!GetStackValue(cb->GetScriptContext(), execstack, funccallstack, val1, val1type, ve1, oe1))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain a bool\n");
        return false;
    }
    if (val1type != TYPE_bool)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain a bool\n");
        return false;
    }

    // -- get the conditional - if true, we debug trace, push true back onto the stack
    bool conditional = *(bool*)val1;
    if (conditional)
    {
        DebugTrace(op, "ensure(true): no error");
        execstack.Push(&conditional, TYPE_bool);
    }

    // -- otherwise, debug trace, DebugAssert_() to connect to the debugger, output the message, etc...
    else
    {
        // -- get the string
        CScriptContext* script_context = cb->GetScriptContext();
        void* ensure_msg = TypeConvert(script_context, val0type, val0, TYPE_string);
        if (!ensure_msg)
        {
            DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                            "Error - ExecStack should contain TYPE_string\n");
            return false;
        }

        DebugTrace(op, "ensure(false): %s", UnHash(*(uint32*)ensure_msg));

        // -- use the usual assert mechanism
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack, "%s\n", UnHash(*(uint32*)ensure_msg));

        // -- push the conditional back onto the stack, as a "return" value of ensure()
        execstack.Push(&conditional, TYPE_bool);
    }

    // -- even if the ensure() triggers an assert, we still return true, as the ensure() executed successfully
    return (true);
}


// ====================================================================================================================
// OpExecEnsureInterface(): reads the ns hash, and the interface hash, and validates the interface for the ns
// ====================================================================================================================
bool8 OpExecEnsureInterface(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
    CFunctionCallStack& funccallstack)
{
    uint32 ns_hash = *instrptr++;
    uint32 interface_hash = *instrptr++;

    // -- get the namespace
    CNamespace* ns = cb->GetScriptContext()->FindNamespace(ns_hash);
    if (ns == nullptr)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - Namespace %s not found %s\n", UnHash(ns_hash));
        return false;
    }

    // -- get the interface
    CNamespace* interface = cb->GetScriptContext()->FindNamespace(interface_hash);
    if (interface == nullptr)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - Interface %s not found\n", UnHash(interface_hash));
        return false;
    }

    CFunctionEntry* mismatch_fe = nullptr;
    if (!cb->GetScriptContext()->ValidateInterface(ns, interface, mismatch_fe))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - Namespace %s:: failed to validate interface %s::\n",
                        UnHash(ns_hash), UnHash(interface_hash));
        return false;
    }

    return (true);
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

    // -- pull the delay time off the stack
    // $$$TZA  The delay time could be a variable...??
    contentptr = execstack.Pop(contenttype);
    if (contenttype != TYPE_int)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain TYPE_int\n");
        return false;
    }
    int32 delaytime = *(int32*)(contentptr);

    // -- pull the object ID off the stack
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

    // -- if we're tracking memory, find the call origin, so *if* there's a problem executing the schedule,
    // we can know where that call came from
    const char* schedule_origin = nullptr;
#if MEMORY_TRACKER_ENABLE
    uint32 codeblock_hash = cb->GetFilenameHash();
    int32 cur_line = cb->CalcLineNumber(instrptr - 12);
    char call_origin[kMaxNameLength];
    snprintf(call_origin, sizeof(call_origin), "%s @ %d", UnHash(codeblock_hash), cur_line + 1);
    schedule_origin = call_origin;
#endif

    // -- create the schedule 
    cb->GetScriptContext()->GetScheduler()->mCurrentSchedule =
        cb->GetScriptContext()->GetScheduler()->ScheduleCreate(objectid, delaytime, funchash,
                                                               immediate_execution != 0 ? true : false,
                                                               repeat, schedule_origin);

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
    if (cb->GetScriptContext()->GetScheduler()->mCurrentSchedule == nullptr)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - There is no schedule() being processed\n");
        return false;
    }

    // -- get the parameter index
    int32 paramindex = *instrptr++;

	// -- pop the value
    CVariableEntry* stack_ve = NULL;
    CObjectEntry* stack_oe = NULL;
	eVarType stack_valtype;
	void* stack_valaddr = execstack.Pop(stack_valtype);
    if (!GetStackValue(cb->GetScriptContext(), execstack, funccallstack, stack_valaddr, stack_valtype, stack_ve, stack_oe))
    {
        // -- clear the current schedule
        cb->GetScriptContext()->GetScheduler()->mCurrentSchedule = nullptr;
        return (false);
    }

    // -- add the parameter to the function context, inheriting the type from whatever
    // -- was pushed
    char varnamebuf[32];
    snprintf(varnamebuf, sizeof(varnamebuf), "_%d", paramindex);
    cb->GetScriptContext()->GetScheduler()->mCurrentSchedule->mFuncContext->
        AddParameter(varnamebuf, Hash(varnamebuf), stack_valtype, 1, paramindex, 0);

    // -- assign the value
    CVariableEntry* ve = cb->GetScriptContext()->GetScheduler()->mCurrentSchedule->
                         mFuncContext->GetParameter(paramindex);
    ve->SetValue(NULL, stack_valaddr);
    DebugTrace(op, "Param: %d, Var: %s", paramindex, varnamebuf);

    // -- post increment/decrement support
    ApplyPostUnaryOpEntry(stack_valtype, stack_valaddr);

    return (true);
}

// ====================================================================================================================
// OpExecScheduleEnd():  Construction of a scheduled function call is complete.
// ====================================================================================================================
bool8 OpExecScheduleEnd(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                        CFunctionCallStack& funccallstack)
{
    // -- ensure we are in the middle of a schedule construction
    if (cb->GetScriptContext()->GetScheduler()->mCurrentSchedule == nullptr)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - There is no schedule() being processed\n");
        return false;
    }

    // -- now that the schedule has been completely constructed, we need to determine
    // -- if it's scheduled for immediate execution
    CScheduler::CCommand* curcommand = cb->GetScriptContext()->GetScheduler()->mCurrentSchedule;

    // -- we can now clear the current schedule, since we're no longer using it (e.g. to assign params, etc...)
    cb->GetScriptContext()->GetScheduler()->mCurrentSchedule = nullptr;

    if (curcommand->mImmediateExec)
    {
        if (!ExecuteScheduledFunction(cb->GetScriptContext(), curcommand->mObjectID, 0, curcommand->mFuncHash,
                                      curcommand->mFuncContext))
        {
            DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                            "Error - ExecuteScheduledFunction() failed\n");

            return false;
        }

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

        // -- if we're executing it immediately, we want to remove it from the update queue
        cb->GetScriptContext()->GetScheduler()->CancelRequest(curcommand->mReqID);
    }

    // -- not immediate execution - therefore, push the schedule request ID instead
    else
    {
        int32 reqid = curcommand->mReqID;
        execstack.Push(&reqid, TYPE_int);
    }

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
	bool local_object = *instrptr++ != 0;

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

    uint32 codeblock_hash = 0;
    int32 cur_line = -1;

#if MEMORY_TRACKER_ENABLE
    codeblock_hash = cb->GetFilenameHash();
    cur_line = cb->CalcLineNumber(instrptr);

    // -- note:  the funccallstack stores codeblock hashes and the the line executing the function call to the next
    // stack entry...  the top of the funccallstack hasn't called anything, therefore the
    // linenumberfunccall is unused/unset
    funccallstack.DebuggerUpdateStackTopCurrentLine(codeblock_hash, cur_line);
#endif

    // -- strings are already hashed, when pulled from the stack
    uint32 objid = cb->GetScriptContext()->CreateObject(classhash, *(uint32*)objnameaddr, &funccallstack);

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

	// -- if this is a local object, notify the call stack
	if (local_object)
		funccallstack.NotifyLocalObjectID(objid);

    // -- post increment/decrement support  (named by an integer variable, incremented?  it's possible....)
    ApplyPostUnaryOpEntry(contenttype, contentptr);

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

#if MEMORY_TRACKER_ENABLE
    uint32 codeblock_hash = cb->GetFilenameHash();
    int32 cur_line = cb->CalcLineNumber(instrptr - 12);

    // -- used by the memory tracker (if enabled)
    TinObjectDestroyed(objectid);
#endif

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

REGISTER_FUNCTION(SetDebugTrace, SetDebugTrace);

}  // namespace TinScript

// ====================================================================================================================
// EOF
// ====================================================================================================================
