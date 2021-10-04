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

// == namespace TinScript =============================================================================================

namespace TinScript
{

// --------------------------------------------------------------------------------------------------------------------
// -- forward declarations

class CVariableEntry;
class CFunctionEntry;
class CCodeBlock;

typedef CHashTable<CVariableEntry> tVarTable;
typedef CHashTable<CFunctionEntry> tFuncTable;

// ====================================================================================================================
// registration macros
// ====================================================================================================================
// -- these macros are generated from the python script genregclasses.py
// -- default usage is:  python genregclasses.py -maxparams 8
#include "registrationmacros.h"

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

        // -- the function context is used to determine parameter lists for calling whatever is registered
        // and a function entry is what contains a function context
        CFunctionContext* CreateContext();
        CFunctionContext* GetContext();

        virtual void DispatchFunction(void*) = 0;

        virtual void Register() = 0;
        CRegFunctionBase* GetNext() { return (next); }

        static CRegFunctionBase* gRegistrationList;

    private:
        const char* m_ClassName;
        const char* m_FunctionName;
        uint32 m_ClassNameHash;
        uint32 m_FunctionNameHash;

        CRegFunctionBase* next;
};

// ====================================================================================================================
// registration classes
// ====================================================================================================================
// -- these classes are generated from the python script genregclasses.py
// -- default usage is:  python genregclasses.py -maxparams 8
// -- included after the definition of CRegFunctionBase
#include "registrationclasses.h"
#include "variadicclasses.h"

}  // TinScript

// ====================================================================================================================
// EOF
// ====================================================================================================================
