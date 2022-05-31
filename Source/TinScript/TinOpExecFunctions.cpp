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

// -- class include
#include "TinOpExecFunctions.h"

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
#include "TinExecStack.h"
#include "TinHashtable.h"
#include "TinObjectGroup.h"
#include "TinRegBinding.h"

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
// GetStackEntry():  Peeks/Pops an entry from the stack, and fills in ve, oe, content ptr, type (if possible) 
// ====================================================================================================================
bool8 GetStackEntry(CScriptContext* script_context, CExecStack& execstack,
                    CFunctionCallStack& funccallstack, tStackEntry& stack_entry, bool peek, int depth)
{
        // -- next, pop the hash table variable off the stack
    // -- pull the hashtable variable off the stack
    stack_entry.valaddr = peek ? execstack.Peek(stack_entry.valtype, depth) : execstack.Pop(stack_entry.valtype);
    bool result = GetStackValue(script_context, execstack, funccallstack, stack_entry.valaddr, stack_entry.valtype,
                                stack_entry.ve, stack_entry.oe);
    return result;
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
            // -- if the variable is not a hashtable, but an array...
            if (ve->IsArray())
            {
                // -- the array could be uninitialized, possibly a reference, or declared to be copied to
                if (ve->GetArraySize() > 0)
                    valaddr = ve->GetArrayVarAddr(oe ? oe->GetAddr() : NULL, ve_array_hash_index);
                else
                    valaddr = nullptr;
            }
            else
		        valaddr = ve->GetAddr(oe ? oe->GetAddr() : NULL);
        }
	}

	// -- if a member was pushed, find the oe, the ve, and return a valtype of var
    else if (valtype == TYPE__member)
    {
        uint32 obj_id = ((uint32*)valaddr)[0];
        uint32 member_hash = ((uint32*)valaddr)[1];

        // -- find the object
        oe = script_context->FindObjectEntry(obj_id);
        if (!oe)
        {
            TinPrint(script_context, "Error - Unable to find object %d\n", obj_id);
            return false;
        }

        // -- find the variable entry from the object's namespace variable table
        ve = oe->GetVariableEntry(member_hash);
        if (!ve)
            return (false);

        valaddr = ve->GetAddr(oe->GetAddr());
		valtype = ve->GetType();
    }

    // -- if an object was pushed, ensure we fill in the oe
    else if (valtype == TYPE_object)
    {
        uint32 obj_id = ((uint32*)valaddr)[0];

        // -- find the object (it's legitimate to not find an object...)
        oe = script_context->FindObjectEntry(obj_id);
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
            // lets make sure we're not trying to get the address of an uninitialized array var
            if (!ve->IsArray() || ve->GetArraySize() > 0)
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

    // -- if we weren't able to resolve the address for the actual value storage,
    // then we'd better have an uninitialized array, or a valid stack variable
    bool8 valid_result = valaddr != nullptr ||
                         (ve != nullptr && ve->IsArray() && ve->GetArraySize() == -1) ||
                          (ve != nullptr && ve->IsStackVariable(funccallstack, false));
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
    tStackEntry stack_entry_0;
    if (!GetStackEntry(script_context, execstack, funccallstack, stack_entry_0, true, 1))
    {
        TinPrint(script_context, "Error - ExecStack should contain a hashtable variable\n");
        return false;
    }
    if (!stack_entry_0.ve || (stack_entry_0.ve->GetType() != TYPE_hashtable && !stack_entry_0.ve->IsArray()))
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
    uint32 var_hash = stack_entry_0.ve->GetHash();

    // -- if this is an object member...
    if (stack_entry_0.oe)
    {
        ns_hash = 0;
        func_or_obj = stack_entry_0.oe->GetID();
    }

    // -- global hash table variable
    else if (!stack_entry_0.ve->GetFunctionEntry())
    {
        ns_hash = CScriptContext::kGlobalNamespaceHash;
    }

    // -- function local variable
    else
    {
        ns_hash = stack_entry_0.ve->GetFunctionEntry()->GetNamespaceHash();
        func_or_obj = stack_entry_0.ve->GetFunctionEntry()->GetHash();
    }

    // -- now find the variable
    ve = GetVariable(script_context, script_context->GetGlobalNamespace()->GetVarTable(),
                     ns_hash, func_or_obj, stack_entry_0.ve->GetHash(), arrayvarhash);
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
        valaddr = ve->IsArray() ? ve->GetArrayVarAddr(stack_entry_0.oe ? stack_entry_0.oe->GetAddr() : NULL, arrayvarhash)
                                : ve->GetAddr(stack_entry_0.oe ? stack_entry_0.oe->GetAddr() : NULL);
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
    tStackEntry stack_entry_1;
    if (!GetStackEntry(script_context, execstack, funccallstack, stack_entry_1))
        return false;

    // -- cache the result value, because we'll need to push it back onto the stack if we have consecutive assignments
    // $$$TZA SendArray - need to support array return values
    if (stack_entry_1.valtype != TYPE_hashtable && stack_entry_1.ve != nullptr && !stack_entry_1.ve->IsArray())
    {
        g_lastAssignResultType = stack_entry_1.valtype;
        memcpy(g_lastAssignResultBuffer, stack_entry_1.valaddr, MAX_TYPE_SIZE * sizeof(uint32));
    }
    else
    {
        g_lastAssignResultType = TYPE_void;
    }

    // -- we're also going to convert val1add to the required type for assignment
    void* val1_convert = stack_entry_1.valaddr;

	// -- pop the var
    tStackEntry stack_entry_0;
    stack_entry_0.valaddr = execstack.Pop(stack_entry_0.valtype);
    bool8 is_stack_var = (stack_entry_0.valtype == TYPE__stackvar);
    bool8 is_pod_member = (stack_entry_0.valtype == TYPE__podmember);
    bool8 use_var_addr = (is_stack_var || is_pod_member);
    if (!GetStackValue(script_context, execstack, funccallstack, stack_entry_0.valaddr, stack_entry_0.valtype,
                       stack_entry_0.ve, stack_entry_0.oe))
    {
        return (false);
    }

    // -- if the variable is a local variable, we also have the actual address already
    use_var_addr = use_var_addr || (stack_entry_0.ve && stack_entry_0.ve->IsStackVariable(funccallstack, false));

    // -- ensure we're assigning to a variable, an object member, or a local stack variable
    if (!stack_entry_0.ve && !use_var_addr)
        return (false);

    // -- if we've been given the actual address of the var, copy directly to it
    if (use_var_addr)
    {
        // -- we're not allowed to stomp local variables that are actually hashtables
        if (stack_entry_0.ve && stack_entry_0.ve->GetType() == TYPE_hashtable && !stack_entry_0.ve->IsParameter())
        {
            ScriptAssert_(script_context, 0, "<internal>", -1,
                          "Error - Assigning to hashtable var '%s' would stomp and leak memory\n",
                          UnHash(stack_entry_0.ve->GetHash()));
            return (false);
        }

        val1_convert = TypeConvert(script_context, stack_entry_1.valtype, stack_entry_1.valaddr, stack_entry_0.valtype);
        if (!val1_convert)
        {
            ScriptAssert_(script_context, 0, "<internal>", -1,
                          "Error - fail to conver from type %s to type %s\n",
                          GetRegisteredTypeName(stack_entry_1.valtype), GetRegisteredTypeName(stack_entry_0.valtype));
            return (false);
        }
        memcpy(stack_entry_0.valaddr, val1_convert, gRegisteredTypeSize[stack_entry_0.valtype]);
        DebugTrace(op, is_stack_var ? "StackVar: %s" :
                       is_pod_member ? "PodMember: %s" :
                       "Var : % s", DebugPrintVar(stack_entry_0.valaddr, stack_entry_0.valtype));

        // -- apply any post-unary ops (increment/decrement)
        ApplyPostUnaryOpEntry(stack_entry_1.valtype, stack_entry_1.valaddr);
        ApplyPostUnaryOpEntry(stack_entry_0.valtype, stack_entry_0.valaddr);

        // -- if we're connected to the debugger, then the variable entry associated with the stack var will be returned,
		// -- notify we're breaking on it
		if (stack_entry_0.ve)
            stack_entry_0.ve->NotifyWrite(script_context, &execstack, &funccallstack);
    }

    // -- else set the value through the variable entry
    else
    {
        // -- this is a special case, if we're using a POD method, and what we need to assign, is a parameter CVariableEntry*
        // e.g.  TypeVariableArray_Copy()..
        CObjectEntry* cur_func_oe = NULL;
        int32 cur_func_var_offset = -1;
        CFunctionEntry* cur_func = funccallstack.GetTop(cur_func_oe, cur_func_var_offset);
        if (cur_func != nullptr && cur_func->GetContext()->IsPODMethod() && stack_entry_0.ve->IsParameter() &&
            stack_entry_0.ve->GetType() == TYPE__var)
        {
            if (stack_entry_0.ve != nullptr)
            {
                stack_entry_0.ve->SetReferenceAddr(stack_entry_1.ve, stack_entry_1.ve->GetAddr(nullptr));
            }
            return (true);
        }

        val1_convert = TypeConvert(script_context, stack_entry_1.valtype, stack_entry_1.valaddr, stack_entry_0.ve->GetType());
        if (!val1_convert)
        {
            ScriptAssert_(script_context, 0, "<internal>", -1,
                          "Error - fail to convert from type %s to type %s\n",
                          GetRegisteredTypeName(stack_entry_1.valtype), GetRegisteredTypeName(stack_entry_0.ve->GetType()));
            return (false);
        }

        // -- If the destination is an array parameter that has yet not been initialized,
        // -- then the first assignment to it happens when the function is called.
        // -- It would be preferable to initialize the parameter through a specific code path,
        // -- but since the assignment uses the same path, we use the test below and trust
        // -- the compiled code.  This implementation is 100% solid, just less preferable than
        // -- a deliberate initialization.
        if (stack_entry_0.ve->IsParameter() && stack_entry_1.ve && stack_entry_1.ve->IsArray() &&
            stack_entry_0.ve->GetType() == stack_entry_1.ve->GetType())
        {
            stack_entry_0.ve->InitializeArrayParameter(stack_entry_1.ve, stack_entry_1.oe, execstack, funccallstack);
        }

        else if (!stack_entry_0.ve->IsArray())
        {
            stack_entry_0.ve->SetValue(stack_entry_0.oe ? stack_entry_0.oe->GetAddr() : NULL, val1_convert, &execstack, &funccallstack);
            DebugTrace(op, "Var %s: %s", UnHash(stack_entry_0.ve->GetHash()),
                       DebugPrintVar(val1_convert, stack_entry_0.ve->GetType()));

            // -- apply any post-unary ops (increment/decrement)
            // -- (to the non-convert, original address)
            ApplyPostUnaryOpEntry(stack_entry_1.valtype, stack_entry_1.valaddr);
            ApplyPostUnaryOpEntry(stack_entry_0.valtype, stack_entry_0.valaddr);
        }
        else
        {
            // $$$TZA need a better way to determine the array index
            void* ve0_addr = stack_entry_0.ve->GetAddr(stack_entry_0.oe ? stack_entry_0.oe->GetAddr() : NULL);
            int32 byte_count = kPointerDiffUInt32(stack_entry_0.valaddr, ve0_addr);
            int32 array_index = byte_count / gRegisteredTypeSize[stack_entry_0.ve->GetType()];
            stack_entry_0.ve->SetValue(stack_entry_0.oe ? stack_entry_0.oe->GetAddr() : NULL,
                                       val1_convert, &execstack, &funccallstack, array_index);

            // -- apply any post-unary ops (increment/decrement)
            // -- I *believe* the actual a ve0->SetValue() will end up writing to the same address as "var"...
            ApplyPostUnaryOpEntry(stack_entry_1.valtype, stack_entry_1.valaddr);
            ApplyPostUnaryOpEntry(stack_entry_0.valtype, stack_entry_0.valaddr);
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
    // -- pop the value
    tStackEntry stack_entry;
    if (!GetStackEntry(cb->GetScriptContext(), execstack, funccallstack, stack_entry, true))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - Failed peek value for op: %s\n", GetOperationString(op));
        return false;
    }

    AddPostUnaryOpEntry(stack_entry.valtype, stack_entry.valaddr, op == OP_UnaryPostInc ? 1 : -1);

    DebugTrace(op, "%s", DebugPrintVar((void*)stack_entry.valaddr, stack_entry.valtype));

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
    tStackEntry stack_entry;
    if (!GetStackEntry(cb->GetScriptContext(), execstack, funccallstack, stack_entry))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - Failed pop value for op: %s\n", GetOperationString(op));
        return false;
    }

    // -- convert the value to an int (the only valid type a bit-invert operator is implemented for)
    void* convertaddr = TypeConvert(cb->GetScriptContext(), stack_entry.valtype, stack_entry.valaddr, TYPE_int);
    if (!convertaddr)
    {
          DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                          "Error - unable to convert from type %s to type TYPE_int, performing op: %s\n",
                          gRegisteredTypeNames[stack_entry.valtype], GetOperationString(op));
        return false;
    }

    int32 result = *(int32*)convertaddr;
    result = ~result;

    execstack.Push(&result, TYPE_int);
    DebugTrace(op, "%s", DebugPrintVar((void*)&result, TYPE_int));

    // -- why you would post increment/decrement a variable after bit-inverting is questionable...  but supported
    ApplyPostUnaryOpEntry(stack_entry.valtype, stack_entry.valaddr);

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
    tStackEntry stack_entry;
    if (!GetStackEntry(cb->GetScriptContext(), execstack, funccallstack, stack_entry))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - Failed pop value for op: %s\n", GetOperationString(op));
        return false;
    }

    // -- convert the value to a bool (the only valid type a not operator is implemented for)
    void* convertaddr = TypeConvert(cb->GetScriptContext(), stack_entry.valtype, stack_entry.valaddr, TYPE_bool);
    if (!convertaddr)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - unable to convert from type %s to type TYPE_bool, performing op: %s\n",
                        gRegisteredTypeNames[stack_entry.valtype], GetOperationString(op));
        return false;
    }

    bool result = *(bool*)convertaddr;
    result = !result;

    execstack.Push(&result, TYPE_bool);
    DebugTrace(op, "%s", DebugPrintVar((void*)&result, TYPE_bool));

    // -- post increment/decrement support
    ApplyPostUnaryOpEntry(stack_entry.valtype, stack_entry.valaddr);

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
    tStackEntry stack_entry;
    if (!GetStackEntry(cb->GetScriptContext(), execstack, funccallstack, stack_entry, true))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - Failed pop value for op: %s\n", GetOperationString(op));
        return false;
    }

    if (stack_entry.valaddr == nullptr)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - stack is empty, op: %s\n", GetOperationString(op));
        return false;
    }

    // -- push the a copy back onto the stack
	// $$$TZA should we push the VE back onto the stack?  or just the value...
	// this is only used in a switch() statement atm, where only the value is used
    execstack.Push(stack_entry.valaddr, stack_entry.valtype);
    DebugTrace(op, "%s", DebugPrintVar(stack_entry.valaddr, stack_entry.valtype));

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
    // -- pop the value
    tStackEntry stack_entry_0;
    if (!GetStackEntry(cb->GetScriptContext(), execstack, funccallstack, stack_entry_0))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain a hashtable variable\n");
        return false;
    }

    if (!stack_entry_0.ve || (stack_entry_0.ve->GetType() != TYPE_hashtable && !stack_entry_0.ve->IsArray()))
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
    uint32 var_hash = stack_entry_0.ve->GetHash();

    // -- if this is an object member...
    if (stack_entry_0.oe)
    {
        ns_hash = 0;
        func_or_obj = stack_entry_0.oe->GetID();
    }

    // -- global hash table variable
    else if (!stack_entry_0.ve->GetFunctionEntry())
    {
        ns_hash = CScriptContext::kGlobalNamespaceHash;
    }

    // -- function local variable
    else
    {
        ns_hash = stack_entry_0.ve->GetFunctionEntry()->GetNamespaceHash();
        func_or_obj = stack_entry_0.ve->GetFunctionEntry()->GetHash();
    }

    // -- push the hashvar (note:  could also be an index)
    uint32 arrayvar[4];
    arrayvar[0] = ns_hash;
    arrayvar[1] = func_or_obj;
    arrayvar[2] = var_hash;
    arrayvar[3] = arrayvarhash;

    // -- next instruction is the variable hash followed by the function context hash
    execstack.Push((void*)arrayvar, TYPE__hashvarindex);
    if (stack_entry_0.oe)
        DebugTrace(op, "ArrayVar: %d.%s[%s], %s", stack_entry_0.oe->GetID(), UnHash(var_hash), UnHash(arrayvarhash),
                   DebugPrintVar(stack_entry_0.ve->GetAddr(stack_entry_0.oe ? stack_entry_0.oe->GetAddr() : NULL),
                                 stack_entry_0.ve->GetType()));
    else
        DebugTrace(op, "ArrayVar: %s[%s], %s", UnHash(var_hash), UnHash(arrayvarhash),
                   DebugPrintVar(stack_entry_0.ve->GetAddr(stack_entry_0.oe ? stack_entry_0.oe->GetAddr() : NULL),
                                 stack_entry_0.ve->GetType()));

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
    tStackEntry stack_entry_0;
    if (!GetStackEntry(cb->GetScriptContext(), execstack, funccallstack, stack_entry_0))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain a hashtable variable\n");
        return false;
    }

    if (!stack_entry_0.ve || (stack_entry_0.ve->GetType() != TYPE_hashtable && !stack_entry_0.ve->IsArray()))
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
    uint32 var_hash = stack_entry_0.ve->GetHash();

    // -- if this is an object member...
    if (stack_entry_0.oe)
    {
        ns_hash = 0;
        func_or_obj = stack_entry_0.oe->GetID();
    }

    // -- global hash table variable
    else if (!stack_entry_0.ve->GetFunctionEntry())
    {
        ns_hash = CScriptContext::kGlobalNamespaceHash;
    }

    // -- function local variable
    else
    {
        ns_hash = stack_entry_0.ve->GetFunctionEntry()->GetNamespaceHash();
        func_or_obj = stack_entry_0.ve->GetFunctionEntry()->GetHash();
    }

    // -- now find the variable
    CVariableEntry* ve = GetVariable(cb->GetScriptContext(), cb->GetScriptContext()->GetGlobalNamespace()->GetVarTable(),
                                     ns_hash, func_or_obj, stack_entry_0.ve->GetHash(), arrayvarhash);
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
        veaddr = ve->IsArray() ? ve->GetArrayVarAddr(stack_entry_0.oe ? stack_entry_0.oe->GetAddr() : NULL, arrayvarhash)
                               : ve->GetAddr(stack_entry_0.oe ? stack_entry_0.oe->GetAddr() : NULL);
    }

    if (!execstack.Push(veaddr, vetype))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack, "Error - OP_PushArrayValue failed\n");
        return false;
    }

    if (stack_entry_0.oe)
        DebugTrace(op, "ArrayVar: %d.%s [%s], %s", stack_entry_0.oe->GetID(), UnHash(var_hash), UnHash(arrayvarhash),
                   DebugPrintVar(veaddr, vetype));
    else
        DebugTrace(op, "ArrayVar: %s [%s], %s", UnHash(var_hash), UnHash(arrayvarhash), DebugPrintVar(veaddr, vetype));

    return (true);
}

// ====================================================================================================================
// OpExecPushMember():  Push an object member (value) onto the exec stack
// ====================================================================================================================
bool8 OpExecPushMember(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                       CFunctionCallStack& funccallstack)
{
    // -- next instruction is the member name
    uint32 varhash = *instrptr++;

    // -- what will previously have been pushed on the stack, is the object ID
    tStackEntry stack_entry_0;
    if (!GetStackEntry(cb->GetScriptContext(), execstack, funccallstack, stack_entry_0) ||
        stack_entry_0.valtype != TYPE_object || stack_entry_0.oe == nullptr)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain TYPE_object\n");
        return false;
    }

    // -- ensure the object has the member requested
    CVariableEntry* member_ve = stack_entry_0.oe->GetVariableEntry(varhash);
    if (member_ve == nullptr)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - object %d does not contain member: %s\n", stack_entry_0.oe->GetID(),
                        UnHash(varhash));
        return false;
    }

    // -- TYPE_object is actually just an uint32 ID
    uint32 member[2];
    member[0] = stack_entry_0.oe->GetID();
    member[1] = varhash;
    execstack.Push((void*)member, TYPE__member);
     
    // -- push the variable onto the stack
    /*
    uint32 varbuf[3];
    varbuf[0] = 0;
    varbuf[1] = stack_entry_0.oe->GetID();
    varbuf[2] = varhash;
    execstack.Push((void*)varbuf, TYPE__var);
    */

    // -- push the member onto the stack
    DebugTrace(op, "Obj Mem %s: %s", UnHash(varhash), DebugPrintVar(stack_entry_0.valaddr, stack_entry_0.valtype));
	
    return (true);
}

// ====================================================================================================================
// OpExecPushMemberVal():  Push an object member (variable) onto the exec stack...
// ====================================================================================================================
bool8 OpExecPushMemberVal(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                          CFunctionCallStack& funccallstack)
{
    // -- next instruction is the member name
    uint32 varhash = *instrptr++;

    // -- what will previously have been pushed on the stack, is the object ID
    tStackEntry stack_entry_0;
    if (!GetStackEntry(cb->GetScriptContext(), execstack, funccallstack, stack_entry_0) ||
        stack_entry_0.valtype != TYPE_object)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain TYPE_object\n");
        return false;
    }

    // -- TYPE_object is actually just an uint32 ID
    uint32 objectid = *(uint32*)stack_entry_0.valaddr;

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
    tStackEntry stack_entry;
    if (!GetStackEntry(cb->GetScriptContext(), execstack, funccallstack, stack_entry))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - Failed to pop a variable of a registered POD type\n");
        return (false);
    }

    // -- the var and vartype will be set to the actual type and physical address of the
    // -- POD variable we're about to dereference
    eVarType pod_member_type;
    void* pod_member_addr = NULL;
    if (!GetRegisteredPODMember(stack_entry.valtype, stack_entry.valaddr, varhash, pod_member_type, pod_member_addr))
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
    tStackEntry stack_entry;
    if (!GetStackEntry(cb->GetScriptContext(), execstack, funccallstack, stack_entry))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - Failed to find a variable of a registered POD type\n");
        return (false);
    }

    // -- see if we popped a value of a registered POD type
    eVarType pod_member_type;
    void* pod_member_addr = NULL;
    if (!GetRegisteredPODMember(stack_entry.valtype, stack_entry.valaddr, varhash, pod_member_type, pod_member_addr))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - Failed to find a variable of a registered POD type\n");
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
    tStackEntry stack_entry_container;
    if (!GetStackEntry(cb->GetScriptContext(), execstack, funccallstack, stack_entry_container, true, 2) ||
        stack_entry_container.valaddr == nullptr)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - foreach loop expecting a container variable (e.g. array) on the stack\n");
        return false;
    }

    // -- we support three types of containers to iterate through with a foreach() loop
    bool container_is_hashtable = stack_entry_container.ve != nullptr && !stack_entry_container.ve->IsArray() &&
                                  stack_entry_container.ve->GetType() == TYPE_hashtable;
    CObjectSet* container_set = nullptr;
    if (!container_is_hashtable && stack_entry_container.ve != nullptr &&
        stack_entry_container.ve->GetType() == TYPE_object)
    {
        // -- TYPE_object is actually just an uint32 ID
        uint32 objectid = *(uint32*)stack_entry_container.valaddr;

        // -- find the object, and see if it's an object set
        static uint32 object_set_hash = Hash("CObjectSet");
        CObjectEntry* oe = cb->GetScriptContext()->FindObjectEntry(objectid);
        void* obj_addr = oe != nullptr ? oe->GetAddr() : nullptr;
        if (obj_addr != nullptr && oe->HasNamespace(object_set_hash))
        {
            container_set = static_cast<CObjectSet*>(obj_addr);
        }
    }

    // -- by default, even if this isn't actually an array, we can treat a variable as an array of size 1
    // $$$TZA Arrays!  iterate over an array of CObjectSets?
    bool container_is_array = !container_is_hashtable && container_set == nullptr &&
                              stack_entry_container.ve != nullptr &&
                              stack_entry_container.ve->GetArraySize() >= 1;

    // -- make sure we got a valid address for the container entry value
    if (!container_is_array && !container_is_hashtable && container_set == nullptr)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - foreach() can only iterate on an array, hashtable, or CObjectSet.\n");
        return (false);
    }

    // -- uses still uses peek, so the stack is unchanged
    tStackEntry stack_entry_iter;
    if (!GetStackEntry(cb->GetScriptContext(), execstack, funccallstack, stack_entry_iter, true, 1) ||
        stack_entry_iter.valaddr == nullptr)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - foreach loop expecting a container variable (e.g. array) on the stack\n");
        return false;
    }

    // -- pop the index from the stack
    eVarType index_valtype;
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
    uint32 container_entry_obj_id = 0;
    void* container_entry_val = nullptr;
    if (container_is_array)
    {
        // -- ensure it's within range
        if (cur_index >= 0 && cur_index < stack_entry_container.ve->GetArraySize())
        {
            // -- get the address for the value at the specific index
            container_entry_val =
                stack_entry_container.ve->GetArrayVarAddr(stack_entry_container.oe
                                                          ? stack_entry_container.oe->GetAddr()
                                                          : nullptr, cur_index);

            // -- see if we can convert that value to our iterator type
            container_entry_val = TypeConvert(cb->GetScriptContext(), stack_entry_container.ve->GetType(),
                                              container_entry_val, stack_entry_iter.ve->GetType());

            // -- we have a valid container entry, but won't be able to assign it (incompatible types) -
            if (container_entry_val == nullptr)
            {
                DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                                "Error - foreach() unable to assign container value to iter variable\n");
                return (false);
            }
        }
    }

    // -- object groups/sets
    else if (container_set != nullptr)
    {
        // -- this is a bit weird, but if we're able to get the object by index, then
        // what we're copying to the iterator variable, is the object ID...
        // -- setting an address and then using a memcpy...  keeps the logic simple
        container_entry_obj_id = container_set->GetObjectByIndex(cur_index);
        if (container_entry_obj_id != 0)
        {
            container_entry_val = &container_entry_obj_id;
        }
    }

    // -- hashtable
    else if (container_is_hashtable)
    {
        // -- pull the var table from the hashtable variable entry
        tVarTable* ht_vars = static_cast<tVarTable*>(stack_entry_container.valaddr);
        if (ht_vars != nullptr)
        {
            // -- see if we can get the VariableEntry by index
            CVariableEntry* ht_ve = ht_vars->FindItemByIndex(cur_index);
            if (ht_ve != nullptr)
            {
                // -- if we found an entry, get the address, and get set the (converted) address for its value 
                void* ht_ve_val = ht_ve->GetAddr(nullptr);

                // -- see if we can convert that value to our iterator type
                container_entry_val = TypeConvert(cb->GetScriptContext(), ht_ve->GetType(), ht_ve_val,
                                                  stack_entry_iter.ve->GetType());

                // -- we have a valid container entry, but won't be able to assign it (incompatible types) -
                if (container_entry_val == nullptr)
                {
                    DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                                    "Error - foreach() unable to assign container value to iter variable\n");
                    return (false);
                }
            }
        }
    }

    // -- debug trace output
    DebugTrace(op, "Container var: %s, iter var: %s, index: %d, valid: %s", UnHash(stack_entry_container.ve->GetHash()),
                    UnHash(stack_entry_iter.ve->GetHash()), cur_index - 1, (container_entry_val != nullptr ? "true" : "false"));

    // -- if we have a "next", make the assignment
    if (container_entry_val != nullptr)
    {
        // -- the assignment is a simple memcpy (from the converted value, so the types match of course)
        memcpy(stack_entry_iter.valaddr, container_entry_val, gRegisteredTypeSize[stack_entry_iter.ve->GetType()]);

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

#if VM_DETECT_INFINITE_LOOP
    if (CFunctionCallStack::NotifyBranchInstruction(instrptr))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - loop count of %d exceeded (infinte loop?)\n", kExecBranchMaxLoopCount);
        return false;
    }
#endif

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
    // note:  tracking these branches will catch infinite loops involving (e.g. recursive) function calls
    // the direct branch will catch infinite loops in for/while loops... both are needed
    if (*convertAddr == branch_true)
    {
        instrptr += jumpcount;

#if VM_DETECT_INFINITE_LOOP
        if (CFunctionCallStack::NotifyBranchInstruction(instrptr))
        {
            DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                            "Error - loop count of %d exceeded (infinte loop?)\n", kExecBranchMaxLoopCount);
            return false;
        }
#endif

    }

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
    tStackEntry stack_entry;
    if (!GetStackEntry(cb->GetScriptContext(), execstack, funccallstack, stack_entry))
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
    void* val_obj_addr = stack_entry.valtype == TYPE_object ? stack_entry.valaddr : nullptr;
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
// OpExecPODCallArgs():  Preparation to call a type method after we've assigned all the arguments.
// ====================================================================================================================
bool8 OpExecPODCallArgs(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                          CFunctionCallStack& funccallstack)
{
    // -- get the hash of the method name
    uint32 methodhash = *instrptr++;

    // -- peek the POD variable - we'll pop after we're sure we don't need to reassign the method call result back
    tStackEntry stack_entry_pod;
    if (!GetStackEntry(cb->GetScriptContext(), execstack, funccallstack, stack_entry_pod, true))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should a POD var\n");
        return false;
    }

    // -- find the function to call
    uint32 ns_hash = GetRegisteredTypeHash(stack_entry_pod.valtype);
    CNamespace* type_ns = cb->GetScriptContext()->FindNamespace(ns_hash);
    CFunctionEntry* fe = type_ns != nullptr ? type_ns->GetFuncTable()->FindItem(methodhash) : nullptr;
    if (fe == nullptr || fe->GetType() != eFuncTypeRegistered)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - no c++ registered method for the given type:  %s:%s()\n",
                        GetRegisteredTypeName(stack_entry_pod.valtype), UnHash(methodhash));
        return (false);
    }

    // -- POD methods are always global functions, where the first arg is the CVariableEntry* value...
    // $$$TZA PODMethod - need to enforce this!
    // -- therefore, we need to assign the first parameter (index 1, not the _return param, as always),
    // the value of our POD
    CFunctionContext* fe_context = fe->GetContext();
    CVariableEntry* param_1_ve = fe_context->GetParameter(1);

    // -- if we're executing a method with _p1 being a copy of the POD, set the value directly
    // note:  some POD methods (e.g. for TYPE_hashtable) will take a CVariableEntry* as its first param, since
    // essentially there's no viable "copy by value" that we'd want to use
    eVarType p1_type = param_1_ve != nullptr ? param_1_ve->GetType() : TYPE_void;
    if ((!fe_context->IsPODMethod() && p1_type != stack_entry_pod.valtype) ||
        (fe_context->IsPODMethod() && p1_type != stack_entry_pod.valtype && p1_type != TYPE__var))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
            "Error - POD method:  %s:%s() does not take a POD value as its first parameter\n",
            GetRegisteredTypeName(stack_entry_pod.valtype), UnHash(methodhash));
        return (false);
    }

    // -- set the p1 value - if it's not a TYPE__var, copy the value
    if (p1_type != TYPE__var)
    {
        param_1_ve->SetValueAddr(nullptr, stack_entry_pod.valaddr, 0);
    }
    else
    {
        if (stack_entry_pod.ve != nullptr)
        {
            param_1_ve->SetReferenceAddr(stack_entry_pod.ve, stack_entry_pod.valaddr);
        }
        else
        {
            DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                            "Error - POD method:  %s:%s() unable to assign the POD variable\n",
                            GetRegisteredTypeName(stack_entry_pod.valtype), UnHash(methodhash));
            return (false);
        }
    }

    // -- push the function entry onto the call stack
    funccallstack.Push(fe, stack_entry_pod.oe, execstack.GetStackTop());

    DebugTrace(op, "POD type: %s, func: %s", GetRegisteredTypeName(stack_entry_pod.valtype), UnHash(fe->GetHash()));
    return (true);
}

// ====================================================================================================================
// OpExecPODCallComplete():  Once the POD call has completed, we need to assign the _P1 param back to the original POD
// ====================================================================================================================
bool8 OpExecPODCallComplete(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                            CFunctionCallStack& funccallstack)
{
    // -- the return value should be on the top of the stack...
    // POD methods only return values (obviously)
    eVarType return_val_type;
    void* return_val = execstack.Pop(return_val_type);
    uint32 stacktopcontent[MAX_TYPE_SIZE];
    memcpy(stacktopcontent, return_val, MAX_TYPE_SIZE * sizeof(uint32));

    // -- next on the stack we should have the POD variable for which the method was called - pop it
    eVarType unused_type;
    execstack.Pop(unused_type);

    // -- push the return value back onto the stack (so it can be assigned, etc...)
    execstack.Push(stacktopcontent, return_val_type);

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

    // -- determine how much local variable space was reserved for the current function call
    int32 local_var_space = fe->GetContext()->CalculateLocalVarStackSize() * MAX_TYPE_SIZE;

    // -- ideally, what should be left on the stack is just the reserved storage
    int32 cur_stack_top = execstack.GetStackTop();
    int32 reserved_space = cur_stack_top - var_offset;
    if (reserved_space < local_var_space)
    {
        // -- this is serious - we've somehow Pop()'d into reserved space
        ScriptAssert_(cb->GetScriptContext(), 0, cb->GetFileName(), cb->CalcLineNumber(instrptr),
                      "Error - The stack has not been balanced - forcing Pops\n");
    }

    // -- we need to restore the previous function's mStackTopReserve as well...
    // -- the var_offset is essentially where the exec stack "starts" for this function call
    // -- first we reserve local var space, then we push/pop
    // -- we *never* pop below the "var_offset + local_var_space" (obviously)
    // -- this is the mStackTopReserve value in the execstack
    CObjectEntry* prev_oe = nullptr;
    int32 prev_var_offset = 0;
    int32 prev_stack_top_reserve = 0;
    CFunctionEntry* prev_function = funccallstack.GetTop(prev_oe, prev_var_offset);
    if (prev_function != nullptr && prev_function->GetContext() != nullptr)
    {
        int32 prev_local_space = prev_function->GetContext()->CalculateLocalVarStackSize();
        prev_stack_top_reserve = prev_var_offset + (prev_local_space * MAX_TYPE_SIZE);
    }

    // -- ideally, we want nothing left on the stack - but there are statements that push
    // values, that are never used... (e.g..   simply de-refencing an array[3], without assigning it)
    execstack.UnReserve(reserved_space, prev_stack_top_reserve);

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
        tStackEntry stack_entry;
        if (!GetStackEntry(cb->GetScriptContext(), execstack, funccallstack, stack_entry))
        {
            DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                            "Error - Failed to pop string to hash\n");
            return false;
        }

        // -- ensure it actually is a string
        void* val1addr = TypeConvert(cb->GetScriptContext(), stack_entry.valtype, stack_entry.valaddr, TYPE_string);
		if (val1addr == nullptr)
        {
            DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                            "Error - Failed to pop string to hash\n");
            return false;
        }

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
        tStackEntry stack_entry;
        if (!GetStackEntry(cb->GetScriptContext(), execstack, funccallstack, stack_entry))
        {
            DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                            "Error - Failed to pop an array index\n");
            return false;
        }

        // -- ensure the index is an integer
        void* val1addr = TypeConvert(cb->GetScriptContext(), stack_entry.valtype, stack_entry.valaddr, TYPE_int);
		if (val1addr == nullptr)
        {
            DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                            "Error - Failed to pop an array index\n");
            return false;
        }

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
        ApplyPostUnaryOpEntry(stack_entry.valtype, stack_entry.valaddr);
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
    tStackEntry stack_entry;
    if (!GetStackEntry(cb->GetScriptContext(), execstack, funccallstack, stack_entry))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain a hashtable variable\n");
        return false;
    }
    if (stack_entry.valtype != TYPE_hashtable)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain hashtable variable\n");
        return false;
    }

    // -- now that we've got our hashtable var, and the type, create (or verify)
    // -- the hash table entry
    tVarTable* hashtable = (tVarTable*)stack_entry.ve->GetAddr(stack_entry.oe ? stack_entry.oe->GetAddr() : NULL);
    CVariableEntry* hte = hashtable->FindItem(hashvalue);

    // -- if the entry already exists, ensure it's the same type
    if (hte && hte->GetType() != vartype)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - HashTable variable: %s already has an entry of type: %s\n",
                        UnHash(stack_entry.ve->GetHash()), GetRegisteredTypeName(hte->GetType()));
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

    // -- not a stack variable - resolve to the actual variable then...
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
    tStackEntry stack_entry;
    if (!GetStackEntry(cb->GetScriptContext(), execstack, funccallstack, stack_entry))
    {
		DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
					    "Error - ExecStack should contain a float value\n");
		return false;
	}

    // -- convert the value to an int (the only valid type a bit-invert operator is implemented for)
    void* convertaddr = TypeConvert(cb->GetScriptContext(), stack_entry.valtype, stack_entry.valaddr, TYPE_float);
    if (!convertaddr)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - unable to convert from type %s to type TYPE_float, performing op: %s\n",
                        gRegisteredTypeNames[stack_entry.valtype], GetOperationString(op));
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
    tStackEntry stack_entry_1;
    if (!GetStackEntry(cb->GetScriptContext(), execstack, funccallstack, stack_entry_1))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain two float values\n");
        return false;
    }

    // -- convert the value to an int (the only valid type a bit-invert operator is implemented for)
    void* convertaddr_1 = TypeConvert(cb->GetScriptContext(), stack_entry_1.valtype, stack_entry_1.valaddr, TYPE_float);
    if (!convertaddr_1)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - unable to convert from type %s to type TYPE_float, performing op: %s\n",
                        gRegisteredTypeNames[stack_entry_1.valtype], GetOperationString(op));
        return false;
    }

    float float_val_1 = *(float*)(convertaddr_1);

        // -- pull the 2nd float value off the stack
    tStackEntry stack_entry_0;
    if (!GetStackEntry(cb->GetScriptContext(), execstack, funccallstack, stack_entry_0))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain two float values\n");
        return false;
    }

    // -- convert the value to an int (the only valid type a bit-invert operator is implemented for)
    void* convertaddr_0 = TypeConvert(cb->GetScriptContext(), stack_entry_0.valtype, stack_entry_0.valaddr, TYPE_float);
    if (!convertaddr_0)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - unable to convert from type %s to type TYPE_float, performing op: %s\n",
                        gRegisteredTypeNames[stack_entry_0.valtype], GetOperationString(op));
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
// OpExecHashtableCopy():  copies the given ht to either another hashtable, or a CHashtableObject (accessible from C++)
// ====================================================================================================================
bool8 OpExecHashtableCopy(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
	CFunctionCallStack& funccallstack)
{
    CScriptContext* script_context = cb->GetScriptContext();

    // -- pop the bool, if we're making a complete copy, or just wrapping the hashtable
    bool is_wrap = (*instrptr++) != 0 ? true : false;

    // -- pull the hashtable variable off the stack
    tStackEntry stack_entry_1;
    if (!GetStackEntry(cb->GetScriptContext(), execstack, funccallstack, stack_entry_1))
    {
		DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
					    "Error - ExecStack should contain a hashtable or CHashtable object value\n");
		return false;
	}

    CObjectEntry* target_ht_oe = nullptr;
    if (stack_entry_1.valtype != TYPE_hashtable)
    {
        void* object_id = TypeConvert(script_context, stack_entry_1.valtype, stack_entry_1.valaddr, TYPE_object);
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
    else if (stack_entry_1.valtype == TYPE_hashtable && is_wrap)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - hashtable_wrap() 2nd param must be a CHashtable object, not a hashtable var\n");
        return false;
    }

    // -- if we didn't find an appropriate target to copy the hashtable to, we're done
    if (stack_entry_1.valtype != TYPE_hashtable && target_ht_oe == nullptr)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
					    "Error - ExecStack should contain a hashtable or CHashtable object value\n");
		return false;
    }

    // -- pull the hashtable variable off the stack
    tStackEntry stack_entry_0;
    if (!GetStackEntry(cb->GetScriptContext(), execstack, funccallstack, stack_entry_0))
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
            cpp_ht->Wrap(stack_entry_0.ve);
        }

        else if (!cpp_ht->CopyFromHashtableVE(stack_entry_0.ve))
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
        if (!CHashtable::CopyHashtableVEToVe(stack_entry_0.ve, stack_entry_1.ve))
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
    tStackEntry stack_entry;
    if (!GetStackEntry(cb->GetScriptContext(), execstack, funccallstack, stack_entry))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain a hashtable variable\n");
        return false;
    }

    const char* type_str = GetRegisteredTypeName(stack_entry.valtype);
    if (type_str == nullptr)
        type_str = "";

    uint32 string_hash = Hash(type_str);
    execstack.Push(&string_hash, TYPE_string);
	DebugTrace(op, "Type: %s", GetRegisteredTypeName(stack_entry.valtype));

	return (true);
}

// ====================================================================================================================
// OpEnsure():  Pops the error message, and the conditional result, pushes the conditional result back on.
// ====================================================================================================================
bool8 OpExecEnsure(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
	CFunctionCallStack& funccallstack)
{
    // -- pop the error string
    tStackEntry stack_entry_0;
    if (!GetStackEntry(cb->GetScriptContext(), execstack, funccallstack, stack_entry_0))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain a hashtable variable\n");
        return false;
    }
    if (stack_entry_0.valtype != TYPE_string)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain a string\n");
        return false;
    }

    // -- pop the conditional string
    tStackEntry stack_entry_1;
    if (!GetStackEntry(cb->GetScriptContext(), execstack, funccallstack, stack_entry_1) ||
        stack_entry_1.valtype != TYPE_bool)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain a bool\n");
        return false;
    }

    // -- get the conditional - if true, we debug trace, push true back onto the stack
    bool conditional = *(bool*)stack_entry_1.valaddr;
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
        void* ensure_msg = TypeConvert(script_context, stack_entry_0.valtype, stack_entry_0.valaddr, TYPE_string);
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
    tStackEntry stack_entry_obj;
    if (!GetStackEntry(cb->GetScriptContext(), execstack, funccallstack, stack_entry_obj) ||
        stack_entry_obj.valtype != TYPE_object)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain TYPE_object\n");
        return false;
    }

    // -- TYPE_object is actually just an uint32 ID
    uint32 objectid = *(uint32*)stack_entry_obj.valaddr;

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
    if (contenttype != TYPE_string && contenttype != TYPE_int)
    {
        ScriptAssert_(cb->GetScriptContext(), 0, cb->GetFileName(), cb->CalcLineNumber(instrptr),
                      "Error - ExecStack should contain TYPE_string or TYPE_int (function name or hash)\n");
        return false;
    }
    uint32 funchash = *(uint32*)(contentptr);

    // -- pull the delay time off the stack
    tStackEntry stack_entry_delay;
    if (!GetStackEntry(cb->GetScriptContext(), execstack, funccallstack, stack_entry_delay) ||
        stack_entry_delay.valtype != TYPE_int)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain TYPE_int\n");
        return false;
    }
    int32 delaytime = *(int32*)(stack_entry_delay.valaddr);

    // -- pull the object ID off the stack
    tStackEntry stack_entry_obj;
    if (!GetStackEntry(cb->GetScriptContext(), execstack, funccallstack, stack_entry_obj) ||
        stack_entry_obj.valtype != TYPE_object)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain TYPE_object\n");
        return false;
    }

    // -- TYPE_object is actually just an uint32 ID
    uint32 objectid = *(uint32*)stack_entry_obj.valaddr;

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
    tStackEntry stack_entry;
    if (!GetStackEntry(cb->GetScriptContext(), execstack, funccallstack, stack_entry))
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
        AddParameter(varnamebuf, Hash(varnamebuf), stack_entry.valtype, 1, paramindex, 0);

    // -- get the parameter to assign
    // copy the entire hashtable, as deferred execution - the original may be altered/deleted)
    CVariableEntry* ve = cb->GetScriptContext()->GetScheduler()->mCurrentSchedule->
                         mFuncContext->GetParameter(paramindex);

    // --if the variable is a hashtable, we have to actually
    if (ve->GetType() == TYPE_hashtable)
    {
        // -- we're going to copy from ve_0 to ve_1
        // (we've already checked for is_wrap to a non-object hashtable variable
        if (!CHashtable::CopyHashtableVEToVe(stack_entry.ve, ve))
        {
            DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                            "Error - Failed to copy hashtable to hashtable variable\n");
            cb->GetScriptContext()->GetScheduler()->mCurrentSchedule = nullptr;
            return false;
        }
    }
    else
    {
        ve->SetValue(NULL, stack_entry.valaddr);
    }

    DebugTrace(op, "Param: %d, Var: %s", paramindex, varnamebuf);

    // -- post increment/decrement support
    ApplyPostUnaryOpEntry(stack_entry.valtype, stack_entry.valaddr);

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

    // -- what will previously have been pushed on the stack, is the object name
    tStackEntry stack_entry;
    if (!GetStackEntry(cb->GetScriptContext(), execstack, funccallstack, stack_entry))
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain TYPE_string\n");
        return false;
    }

    void* objnameaddr = TypeConvert(cb->GetScriptContext(), stack_entry.valtype, stack_entry.valaddr, TYPE_string);
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
    ApplyPostUnaryOpEntry(stack_entry.valtype, stack_entry.valaddr);

    return (true);
}

// ====================================================================================================================
// OpExecDestroyObject():  Destroy Object instruction.
// ====================================================================================================================
bool8 OpExecDestroyObject(CCodeBlock* cb, eOpCode op, const uint32*& instrptr, CExecStack& execstack,
                          CFunctionCallStack& funccallstack)
{
    // -- what will previously have been pushed on the stack, is the object ID
    tStackEntry stack_entry;
    if (!GetStackEntry(cb->GetScriptContext(), execstack, funccallstack, stack_entry) ||
        stack_entry.valtype != TYPE_object)
    {
        DebuggerAssert_(false, cb, instrptr, execstack, funccallstack,
                        "Error - ExecStack should contain TYPE_object\n");
        return false;
    }

    // -- TYPE_object is actually just an uint32 ID
    uint32 objectid = *(uint32*)stack_entry.valaddr;

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

// ====================================================================================================================
// SetDebugExecStack():  Debug function to print the exec stack push/pops
// ====================================================================================================================
void SetDebugExecStack(bool8 torf)
{
#if !BUILD_64												
    CScriptContext::gDebugExecStack = torf;
#else
    TinPrint(TinScript::GetContext(), "SetDebugExecStack() not available in 64-bit builds");
#endif
}


REGISTER_FUNCTION(SetDebugTrace, SetDebugTrace);
REGISTER_FUNCTION(SetDebugExecStack, SetDebugExecStack);

}  // namespace TinScript

// ====================================================================================================================
// EOF
// ====================================================================================================================
