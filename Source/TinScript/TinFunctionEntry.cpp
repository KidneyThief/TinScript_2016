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
// TinFunctionEntry.h:  Defines the classes for a registered function, type (script or code), context (parameters...)
// ====================================================================================================================

#include "integration.h"
#include "TinFunctionEntry.h"
#include "TinVariableEntry.h"
#include "TinCompile.h"

namespace TinScript
{

// == class CFunctionContext ==========================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CFunctionContext::CFunctionContext()
{
    localvartable = TinAlloc(ALLOC_VarTable, tVarTable, eMaxLocalVarCount);
    paramcount = 0;
    for(int32 i = 0; i < eMaxParameterCount; ++i)
        parameterlist[i] = NULL;
}

// ====================================================================================================================
// Destructor
// ====================================================================================================================
CFunctionContext::~CFunctionContext()
{
    // -- delete all the variable entries
    localvartable->DestroyAll();

    // -- delete the actual table
    TinFree(localvartable);
}

// ====================================================================================================================
// AddParameter():  Parameter declaration for a function definition, for a specific index.
// ====================================================================================================================
bool8 CFunctionContext::AddParameter(const char* varname, uint32 varhash, eVarType type, int32 array_size,
                                     int32 paramindex, uint32 actual_type_id, bool is_thread_exec)
{
    // add the entry to the parameter list as well
    assert(paramindex >= 0 && paramindex < eMaxParameterCount);
    if (paramindex >= eMaxParameterCount)
    {
        TinPrint(TinScript::GetContext(), "Error - Max parameter count %d exceeded, parameter: %s\n",
                eMaxParameterCount, varname);
        return (false);
    }

    if (parameterlist[paramindex] != NULL)
    {
        TinPrint(TinScript::GetContext(), "Error - parameter %d has already been added\n",paramindex);
        return (false);
    }

    // -- if the parameter type is invalid,
    if (paramindex >= 1 && type < FIRST_VALID_TYPE)
    {
        TinPrint(TinScript::GetContext(), "Error - invalid parameter %d", paramindex);
        return false;
    }

    // -- create the Variable entry
    CVariableEntry* ve = AddLocalVar(varname, varhash, type, array_size, true, is_thread_exec);
    if (!ve)
        return (false);

    // -- parameters that are registered as TYPE_object, but are actually
    // -- pointers to registered classes, can be automatically
    // -- converted
    if (type == TYPE_object && actual_type_id != 0 && actual_type_id != GetTypeID<uint32>())
    {
        ve->SetDispatchConvertFromObject(actual_type_id);
    }

    // -- bump the count if we need to
    if (paramindex >= paramcount)
        paramcount = paramindex + 1;
    parameterlist[paramindex] = ve;

    return (true);
}

// ====================================================================================================================
// AddParameter():  Parameter declaration for a function definition.
// ====================================================================================================================
bool8 CFunctionContext::AddParameter(const char* varname, uint32 varhash, eVarType type, int32 array_size,
                                     uint32 actual_type_id, bool is_thread_exec)
{
    // -- adding automatically increments the paramcount if needed
    return AddParameter(varname, varhash, type, array_size, paramcount, actual_type_id, is_thread_exec);
}

// ====================================================================================================================
// AddLocalVar():  Local variable declaration for a function definition.
// ====================================================================================================================
CVariableEntry* CFunctionContext::AddLocalVar(const char* varname, uint32 varhash, eVarType type, int32 array_size,
                                              bool8 is_param, bool is_thread_exec)
{

    // -- ensure the variable doesn't already exist
    CVariableEntry* exists = localvartable->FindItem(varhash);
    if (exists != NULL)
    {
        TinPrint(TinScript::GetContext(), "Error - variable already exists: %s\n", varname);
        return (NULL);
    }

    // -- create the Variable entry
    // note:  if this variable is from a SocketExec() remote thread, we need to construct the variable
    // in the main thread, not the current...
    CScriptContext* use_context = is_thread_exec ? TinScript::GetMainThreadContext() : TinScript::GetContext();
    CVariableEntry* ve = TinAlloc(ALLOC_VarEntry, CVariableEntry, use_context, varname, varhash, type,
                                  array_size, false, 0, false, is_param);
    uint32 hash = ve->GetHash();
    localvartable->AddItem(*ve, hash);

    return (ve);
}

// ====================================================================================================================
// GetParameterCount():  Returns the number of parameters for a function definition.
// ====================================================================================================================
int32 CFunctionContext::GetParameterCount()
{
    return (paramcount);
}

// ====================================================================================================================
// GetParameter():  Returns the variable entry for a paremeter, by index.
// ====================================================================================================================
CVariableEntry* CFunctionContext::GetParameter(int32 index)
{
    assert(index >= 0 && index < paramcount);
    return (parameterlist[index]);
}

// ====================================================================================================================
// GetLocalVar():  Get the variable entry for a local variable.
// ====================================================================================================================
CVariableEntry* CFunctionContext::GetLocalVar(uint32 varhash)
{
    return (localvartable->FindItem(varhash));
}

// ====================================================================================================================
// GetLocalVarTable():  Get the variable table for all local variables and parameters.
// ====================================================================================================================
tVarTable* CFunctionContext::GetLocalVarTable()
{
    return (localvartable);
}

// ====================================================================================================================
// CalculateLocalVarStackSize():  Calculate the space needed to be reserved on the stack, for a function call.
// ====================================================================================================================
int32 CFunctionContext::CalculateLocalVarStackSize()
{
    int32 count = 0;
    CVariableEntry* ve = localvartable->First();
    while (ve)
    {
        if (ve->IsArray() && !ve->IsParameter())
            count += ve->GetArraySize();
        else
            ++count;
        ve = localvartable->Next();
    }

    // -- return the result
    return (count);
}

// ====================================================================================================================
// IsParameter():  Returns true if the given variable is a parameter, and not just a local variable.
// ====================================================================================================================
bool8 CFunctionContext::IsParameter(CVariableEntry* ve)
{
    if (!ve)
        return (false);
    for(int32 i = 0; i < paramcount; ++i)
    {
        if (parameterlist[i]->GetHash() == ve->GetHash())
            return (true);
    }

    return (false);
}

// ====================================================================================================================
// ClearParameters():  Reset the value of all parameters so ensure a "clean" function call.
// ====================================================================================================================
void CFunctionContext::ClearParameters()
{
    // -- first, remove any data breakpoints, since they're not valid before, or after a function has executed
    tVarTable* local_vars = GetLocalVarTable();
    CVariableEntry* ve = local_vars->First();
    while (ve)
    {
        ve->ClearBreakOnWrite();
        ve = local_vars->Next();
    }

    const int32 max_size = MAX_TYPE_SIZE * (int32)sizeof(uint32);
    char buf[max_size];
    memset(buf, 0, max_size);
    for(int32 i = 0; i < paramcount; ++i)
    {
        CVariableEntry* param_ve = parameterlist[i];

        if (param_ve->GetType() == TYPE_hashtable || param_ve->IsArray())
            param_ve->ClearArrayParameter();
        else
            param_ve->SetValue(NULL, (void*)&buf);
    }

    // -- next, we want to ensure any local variables (not parameters) belonging to this function
    // -- which are of TYPE_hashtable, are *empty* tables, to ensure clean execution
    // -- as well as no memory leaks
    ve = local_vars->First();
    while (ve)
    {
        if (!ve->IsParameter() && ve->GetType() == TYPE_hashtable)
        {
            tVarTable* hashtable = (tVarTable*)ve->GetAddr(NULL);
            hashtable->DestroyAll();
        }
        ve = local_vars->Next();
    }
}

// ====================================================================================================================
// InitDefaultArgs():  Reset the value of all parameters to either '0', or to the default value
// ====================================================================================================================
bool CFunctionContext::InitDefaultArgs(CFunctionEntry* fe)
{
    // -- sanity check
    if (fe == nullptr)
        return false;

    // -- for registered functions, we may have a default values registration object
    CRegDefaultArgValues* default_args = fe->GetType() == eFuncTypeRegistered && fe->GetRegObject()
        ? fe->GetRegObject()->GetDefaultArgValues()
        : nullptr;

    // -- if we have no default args, simply clear
    if (default_args == nullptr)
    {
        ClearParameters();
        return true;
    }

    // -- initialize the input parameters (starting at 1, since param 0 is the return value)
    for (int32 i = 1; i < GetParameterCount(); ++i)
    {
        // -- subsequent params need commas...
        CVariableEntry* ve = GetParameter(i);

        bool has_default_value = false;
        const char* default_arg_name = nullptr;
        eVarType default_arg_type = TYPE_COUNT;
        void* default_arg_value = nullptr;
        if (default_args != nullptr && default_args->GetDefaultArgValue(i, default_arg_name, default_arg_type, default_arg_value))
        {
            // $$$TZA hashtable/array defaults are still "zero"
            if (ve->GetType() == TYPE_hashtable || ve->IsArray())
            {
                ve->ClearArrayParameter();
            }
            else if (ve->GetType() != default_arg_type)
            {
                TinPrint(TinScript::GetContext(), "Error - %s(): InitDefaultArgs() - default arg %d is a different type\n",
                    TinScript::UnHash(fe->GetHash()), i);
                return false;
            }
            else
            {
                // note:  default_arg_value is the address of the value.. so if the value is a const char*, we've
                // actually got a const char**... as always, strings are a pain
                if (default_arg_type == TYPE_string)
                {
                    ve->SetValueAddr(NULL, (void*)(*(const char**)default_arg_value));
                }
                else
                {
                    // -- we're going to avoid the conversion, and require the types to be exact
                    ve->SetValueAddr(NULL, (void*)default_arg_value);
                }
            }
        }
    }

    // -- success
    return true;
}

// ====================================================================================================================
// InitStackVarOffsets():  Initialize the offset where the memory for a local variable can be found.
// ====================================================================================================================
void CFunctionContext::InitStackVarOffsets(CFunctionEntry* fe)
{
    int32 stackoffset = 0;

    // -- loop the parameters
    for(int32 i = 0; i < paramcount; ++i)
    {
        CVariableEntry* ve = GetParameter(i);
        assert(ve);

        // -- set the stackoffset (note:  parameters are never entire arrays, just references to them)
        // -- so the stack offset can simply be incremented
        if (ve->GetStackOffset() < 0)
        {
            ve->SetStackOffset(stackoffset);
            ve->SetFunctionEntry(fe);
        }
        ++stackoffset;
    }

    // -- now declare the rest of the local vars
    tVarTable* vartable = GetLocalVarTable();
    assert(vartable);
    if (vartable)
    {
        CVariableEntry* ve = vartable->First();
        while (ve)
        {
            if (!ve->IsParameter())
            {
                // -- set the stackoffset
                if (ve->GetStackOffset() < 0)
                {
                    ve->SetStackOffset(stackoffset);
                    ve->SetFunctionEntry(fe);
                }

                // -- if the variable is an array, we need to account for the size
                if (ve->IsArray())
                    stackoffset += ve->GetArraySize();
                else
                    ++stackoffset;
            }
            ve = vartable->Next();
        }
    }
}

// ====================================================================================================================
// CalcHash():  Calculate the hash based on the parameter types, to support overloading 
// ====================================================================================================================
uint32 CFunctionContext::CalcHash()
{
    // -- the strategy here is, each paremeter causes the current hash to be multiplied
    // -- by 3x the number of valid types...  then we add the current type, multiplied by 2 if
    // -- it's an array...  there should be no numerical collisions based on this
    const uint32 next_hash_multiplier = 3 * (uint32)TYPE_COUNT;
    uint32 param_hash = 0;

    // -- note:  we don't inclue the return value
    int param_count = GetParameterCount();
    for(int i = 1; i < param_count; ++i)
    {
        // -- get the type for the parameter
        CVariableEntry* param = GetParameter(i);
        eVarType param_type = param->GetType();

        // -- first multiply the current hash by the number of parameter types, times 2
        param_hash *= next_hash_multiplier;

        // -- now add the current type
        param_hash += (uint32)param_type;
    }

    // -- return the result
    return param_hash;
}

// == class CFunctionEntry ============================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CFunctionEntry::CFunctionEntry(uint32 _nshash, const char* _name, uint32 _hash, EFunctionType _type, void* _addr)
{
    SafeStrcpy(mName, sizeof(mName), _name, kMaxNameLength);
    mType = _type;
    mHash = _hash;
    mNamespaceHash = _nshash;
    mAddr = _addr;
    mCodeblock = NULL;
    mInstrOffset = 0;
    mRegObject = NULL;
}

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CFunctionEntry::CFunctionEntry(uint32 _nshash, const char* _name, uint32 _hash, EFunctionType _type, CRegFunctionBase* _func)
{
    SafeStrcpy(mName, sizeof(mName), _name, kMaxNameLength);
    mType = _type;
    mHash = _hash;
    mNamespaceHash = _nshash;
    mCodeblock = NULL;
    mInstrOffset = 0;
    mRegObject = _func;
}

// ====================================================================================================================
// Destructor
// ====================================================================================================================
CFunctionEntry::~CFunctionEntry()
{
    // -- if any execution stack is currently executing this function, we need to notify the VM so we can cleanly
    // abort execution... this is legitimate for reloading scripts at, say, a breakpoint
    CFunctionCallStack::NotifyFunctionDeleted(this);

    // -- notify the codeblock that this entry no longer exists
    if (mCodeblock)
    {
        mCodeblock->RemoveFunction(this);
    }
}

// ====================================================================================================================
// GetAddr():  Return the address of a registered (non scripted) function.
// ====================================================================================================================
void* CFunctionEntry::GetAddr() const
{
    assert(mType != eFuncTypeScript);
    return mAddr;
}

// ====================================================================================================================
// SetCodeBlockOffset():  Set the offset where the byte code begins for a scripted function.
// ====================================================================================================================
void CFunctionEntry::SetCodeBlockOffset(CCodeBlock* _codeblock, uint32 _offset)
{
    // -- if we're switching codeblocks (recompiling...) change owners
    if (mCodeblock && mCodeblock != _codeblock)
        mCodeblock->RemoveFunction(this);

    mCodeblock = _codeblock;
    mInstrOffset = _offset;
    if (mCodeblock)
        mCodeblock->AddFunction(this);
}

// ====================================================================================================================
// GetCodeBlockOffset():  Get the offset from within a codeblock for the start of the byte code.
// ====================================================================================================================
uint32 CFunctionEntry::GetCodeBlockOffset(CCodeBlock*& _codeblock) const
{
    assert(mType == eFuncTypeScript);
    _codeblock = mCodeblock;
    return mInstrOffset;
}

// ====================================================================================================================
// GetContext():  Return the function context.
// ====================================================================================================================
CFunctionContext* CFunctionEntry::GetContext()
{
    return &mContext;
}

// ====================================================================================================================
// GetReturnType():  Get the value type returned by the function.
// ====================================================================================================================
eVarType CFunctionEntry::GetReturnType()
{
    // -- return value is always the first var entry in the array
    assert(mContext.GetParameterCount() > 0);
    return (mContext.GetParameter(0)->GetType());
}

// ====================================================================================================================
// GetLocalVarTable():  Get the variable table for all local variables and parameters for the function.
// ====================================================================================================================
tVarTable* CFunctionEntry::GetLocalVarTable()
{
    return mContext.GetLocalVarTable();
}

// ====================================================================================================================
// GetRegObject():  Get the registration 
// ====================================================================================================================
CRegFunctionBase* CFunctionEntry::GetRegObject()
{
    return mRegObject;
}

}  // TinScript

// ====================================================================================================================
// EOF
// ====================================================================================================================
