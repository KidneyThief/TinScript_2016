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
// TinRegistration.cpp
// ====================================================================================================================

// -- class include
#include "TinRegistration.h"

// -- lib includes
#include "stdio.h"

#include "TinScript.h"
#include "TinCompile.h"
#include "TinRegistration.h"
#include "TinOpExecFunctions.h"

// == namespace TinScript =============================================================================================

namespace TinScript
{

// -- class CRegDefaultArgValues --------------------------------------------------------------------------------------

CRegDefaultArgValues* CRegDefaultArgValues::gRegistrationList = nullptr;

// ====================================================================================================================
// constructor
// ====================================================================================================================
CRegDefaultArgValues::CRegDefaultArgValues(::TinScript::CRegFunctionBase* reg_object, int default_arg_count,
                                           const char* help_str)
    : mRegObject(reg_object)
    , mArgCount(default_arg_count)
    , mHelpString(help_str)
{
    mNext = gRegistrationList;
    gRegistrationList = this;
}

// ====================================================================================================================
// Register():  notifies the function registration object of this container of param names and default values
// ====================================================================================================================
void CRegDefaultArgValues::Register()
{
    CScriptContext* script_context = TinScript::GetContext();
    if (script_context != nullptr && mRegObject != nullptr)
    {
        // -- find the function entry, and iterate through the parameters - ensure the count and type match
        CNamespace* namespaceentry = mRegObject->GetClassNameHash() != 0
                                     ? script_context->GetNamespaceDictionary()->FindItem(mRegObject->GetClassNameHash())
                                     : script_context->GetGlobalNamespace();
            
        CFunctionEntry* fe = namespaceentry != nullptr
                             ? namespaceentry->GetFuncTable()->FindItem(mRegObject->GetFunctionNameHash())
                             : nullptr;
        CFunctionContext* fc = fe != nullptr ? fe->GetContext() : nullptr;

        bool verified = false;
        if (fe != nullptr && fc != nullptr)
        {
            tDefaultValue* storage = nullptr;
            int32 default_count = GetDefaultArgStorage(storage);

            // -- ensure we have matching arg counts (account for the extra return)
            if (default_count != fc->GetParameterCount())
            {
                TinPrint(TinScript::GetContext(), "mismatched arg count - specify %d default values\n", default_count);
            }
            else
            {
                verified = true;

                // -- iterate through the default args, checking types
                // note:  because this is C++, and for performance, there's no upside to allowing compatible types at the
                // cost of a conversion - register the default args accurately!
                for (int32 i = 1; i < default_count; ++i)
                {
                    CVariableEntry* ve = fc->GetParameter(i);
                    if (ve->GetType() != storage[i].mType)
                    {
                        verified = false;
                        TinPrint(TinScript::GetContext(), "Type mismatch on param: %d, should be %s\n", i, GetRegisteredTypeName(ve->GetType()));
                        break;
                    }
                }
            }
        }

        if (verified)
        {
            mRegObject->SetDefaultArgValues(this);
        }
        else
        {
            if (mRegObject->GetClassNameHash() != 0)
            {
                TinPrint(script_context, "Error - CRegDefaultArgValues::Register() failed: method %s::%s()",
                                          TinScript::UnHash(mRegObject->GetClassNameHash()),
                                          TinScript::UnHash(mRegObject->GetFunctionNameHash()));
            }
            else
            {
                TinPrint(script_context, "Error - CRegDefaultArgValues::Register() failed: function %s()",
                                         TinScript::UnHash(mRegObject->GetFunctionNameHash()));
            }
        }
    }
}

// ====================================================================================================================
// RegisterDefaultValues():  initialization function, called on TinScript::CreateContext()
// ====================================================================================================================
void CRegDefaultArgValues::RegisterDefaultValues()
{
    CRegDefaultArgValues* current = gRegistrationList;
    while (current != nullptr)
    {
        current->Register();
        current = current->mNext;
    }
}

// ====================================================================================================================
// GetDefaultArgValue():  get the name and default value (address) for a registered function
// ====================================================================================================================
bool CRegDefaultArgValues::GetDefaultArgValue(int32 index, const char*& out_name, eVarType& out_val_type,
                                              void*& out_val_addr)
{
    tDefaultValue* storage = nullptr;
    int32 param_count = GetDefaultArgStorage(storage);

    // sanity check - note param 0 is the return, p1 = arg1 etc, just like CFunctionContext
    if (index < 1 || index >= param_count)
        return false;

    out_name = storage[index].mName;
    out_val_type = storage[index].mType;
    out_val_addr = (void*)(&(storage[index].mValue));
    return true;
}

// -- class CRegFunctionBase ------------------------------------------------------------------------------------------

CRegFunctionBase* CRegFunctionBase::gRegistrationList = NULL;

// ====================================================================================================================
// constructor
// ====================================================================================================================
CRegFunctionBase::CRegFunctionBase(const char* _classname,const char* _funcname)
{
    m_ClassName = _classname;
    m_FunctionName = _funcname;
    m_ClassNameHash = Hash(m_ClassName);
    m_FunctionNameHash = Hash(m_FunctionName);

    next = gRegistrationList;
    gRegistrationList = this;
}

// ====================================================================================================================
// CreateContext():  create the function entry and context (ie. parameter list) for a registered function
// ====================================================================================================================
CFunctionContext* CRegFunctionBase::CreateContext()
{
    // -- if we don't already have a function context, we need to create and register a function entry
    CFunctionContext* found = GetContext();
    if (found == nullptr)
    {
        CFunctionEntry* fe =
            new CFunctionEntry(m_ClassNameHash, m_FunctionName, m_FunctionNameHash, eFuncTypeRegistered, this);

        // -- we also want to be sure the function and class names are in the string table
        if (m_ClassNameHash != 0)
            TinScript::GetContext()->GetStringTable()->AddString(m_ClassName,-1, m_ClassNameHash,true);
        TinScript::GetContext()->GetStringTable()->AddString(m_FunctionName, -1, m_FunctionNameHash, true);

        tFuncTable* methodtable = TinScript::GetContext()->FindNamespace(m_ClassNameHash)->GetFuncTable();
        methodtable->AddItem(*fe, m_FunctionNameHash);
        found = fe->GetContext();
    }

    return (found);
}

// ====================================================================================================================
// GetContext():  returns the function context (parameter list) for a registered function
// ====================================================================================================================
CFunctionContext* CRegFunctionBase::GetContext()
{
    tFuncTable* func_table = TinScript::GetContext()->FindNamespace(m_ClassNameHash)->GetFuncTable();
    CFunctionEntry* fe = func_table != nullptr ? func_table->FindItem(m_FunctionNameHash) : nullptr;
    CFunctionContext* func_context = fe != nullptr ? fe->GetContext() : nullptr;
    return (func_context);
}

} // namespace TinScript

// -- eof -------------------------------------------------------------------------------------------------------------
