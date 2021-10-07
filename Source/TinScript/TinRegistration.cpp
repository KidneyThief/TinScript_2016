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
        methodtable->AddItem(*fe,m_FunctionNameHash);
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
