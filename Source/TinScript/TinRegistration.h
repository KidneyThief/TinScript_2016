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
// TinRegistration.h:  This is the second header file that must be included for any source requiring access.
// ====================================================================================================================

#pragma once

// -- includes

#include <tuple>
#include <type_traits>

#include "TinNamespace.h"
#include "TinStringTable.h"
#include "TinInterface.h"
#include "TinFunctionEntry.h"
#include "TinVariableEntry.h"

// == namespace TinScript =============================================================================================

namespace TinScript
{

// --------------------------------------------------------------------------------------------------------------------
// -- forward declarations

class CCodeBlock;

typedef CHashTable<CVariableEntry> tVarTable;
typedef CHashTable<CFunctionEntry> tFuncTable;

class CRegFunctionBase;

// ====================================================================================================================
// class RegDefaultArgValues:  Base class for registering the default values, for registered functions.
// ====================================================================================================================
class CRegDefaultArgValues
{
public:
    CRegDefaultArgValues(::TinScript::CRegFunctionBase* reg_object = nullptr, int default_arg_count = 0,
                         const char*_help_str = "");

    // -- we need to loop through the default variables "generically", so for each default values, we store a table
    // -- of the max number of parameters
    struct tDefaultValue
    {
        const char* mName = nullptr;
        eVarType mType = TYPE_COUNT;
        uint32 mValue[MAX_TYPE_SIZE];
    };

    virtual int32 GetDefaultArgStorage(tDefaultValue*& out_storage) = 0;
    const char* GetHelpString() const { return mHelpString; }

    // -- our base class can look up in the derived registered values, the name and address
    bool GetDefaultArgValue(int32 index, const char*& out_name, eVarType& out_type, void*& out_val_addr);

    // -- the only thing we need to do for registration, is hook up the default values object to the registration object
    void Register();
    static void RegisterDefaultValues();

    // -- we need a way to convert to a readable string -
    // locally (e.g. for ListFunctions()), default values store const char** string literals
    // remote (e.g. from the TinQtConsole), the serialized value is the string hash
    static const char* GetDefaultValueAsString(eVarType type, void* value, bool uses_ste);

private:
    CRegFunctionBase* mRegObject = nullptr;
    int32 mArgCount = 0;
    const char* mHelpString = "";

    static CRegDefaultArgValues* gRegistrationList;
    CRegDefaultArgValues* mNext = nullptr;
};

// ====================================================================================================================
// class CRegFunctionBase:  Base class for registering functions.
// Templated versions for each parameter variation are derived from this class, implemented in registrationclasses.h.
// ====================================================================================================================
class CRegFunctionBase
{
    public:
        CRegFunctionBase(const char* _classname = "", const char* _funcname = "");

        virtual ~CRegFunctionBase() { }

        const char* GetClassName() { return m_ClassName; }
        const char* GetName() { return m_FunctionName; }
        uint32 GetClassNameHash() { return m_ClassNameHash; }
        uint32 GetFunctionNameHash() { return m_FunctionNameHash; }

        // -- the only registration that doesn't use the standard macros, is the (new-ish) POD method registration,
        // where the "class name" is actually the POD type...  *nothing* else should use this
        void SetTypeAsClassName(const char* type_name);

        // -- the function context is used to determine parameter lists for calling whatever is registered
        // and a function entry is what contains a function context
        CFunctionContext* CreateContext();
        CFunctionContext* GetContext();

        void SetDefaultArgValues(CRegDefaultArgValues* default_args) { m_DefaultArgs = default_args; }
        CRegDefaultArgValues* GetDefaultArgValues() const { return m_DefaultArgs; }

        virtual void DispatchFunction(void*) = 0;

        bool IsRegistered() const { return m_isRegistered;  }
        virtual bool Register() = 0;
        CRegFunctionBase* GetNext() { return (next); }

        static CRegFunctionBase* gRegistrationList;

    private:
        bool m_isRegistered = false;
        bool m_isPODMethod = false;
        const char* m_ClassName;
        const char* m_FunctionName;
        uint32 m_ClassNameHash;
        uint32 m_FunctionNameHash;

        CRegFunctionBase* next;

        // -- used by ListFunctions() to pring a more helpful signature,
        // and when preparing to call a registered function, we initialize with default args
        CRegDefaultArgValues* m_DefaultArgs = nullptr;
};

}  // TinScript

// ====================================================================================================================
// EOF
// ====================================================================================================================
