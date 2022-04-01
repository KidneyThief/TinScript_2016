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

// ------------------------------------------------------------------------------------------------
// unittest.cpp
// ------------------------------------------------------------------------------------------------

#include "integration.h"

#if PLATFORM_UE4 && TS_PLATFORM_WINDOWS
	#undef TEXT
	#define WIN32_LEAN_AND_MEAN
#endif

// -- lib includes
#include "stdio.h"

// -- platform includes
#if TS_PLATFORM_WINDOWS
    #include <Windows.h>
#endif

#include <chrono>

#include "mathutil.h"

// -- includes required by any system wanting access to TinScript
#include "TinHash.h"
#include "TinHashtable.h"
#include "TinScript.h"
#include "TinRegistration.h"
#include "registrationexecs.h"

#if PLATFORM_UE4 && TS_PLATFORM_WINDOWS
    #undef WIN32_LEAN_AND_MEAN
#endif

// -- use the DECLARE_FILE/REGISTER_FILE macros to prevent deadstripping
DECLARE_FILE(unittest_cpp);

// -- constants -----------------------------------------------------------------------------------
#if PLATFORM_UE4
    static const char* kUnitTestScriptName = "unittest.ts";
    static const char* kProfilingTestScriptName = "profilingtest.ts";
#else
    static const char* kUnitTestScriptName = "../Source/TinScript/unittest.ts";
    static const char* kProfilingTestScriptName = "../Source/TinScript/profilingtest.ts";
#endif

// --------------------------------------------------------------------------------------------------------------------
// -- Print function, for use by the unit tests

void MTPrint(const char* fmt, ...) {
    TinScript::CScriptContext* script_context = ::TinScript::GetContext();
    va_list args;
    va_start(args, fmt);
    char buf[2048];
    vsprintf_s(buf, 2048, fmt, args);
    va_end(args);
    TinPrint(script_context, "%s", buf);
}

// --  REGISTERED CLASS----------------------------------------------------------------------------
class CBase {
    public:
        CBase() {
            MTPrint("Enter constructor CBase()\n");
            floatvalue = 27.0;
            intvalue = 33;
            boolvalue = true;
            v3member = CVector3f(1.0f, 2.0f, 3.0f);
            objmember = NULL;

            for (int i = 0; i < 20; ++i)
                intArray[i] = 0;

            // -- note:  if the string value is non-empty,
            // -- you must use the string table to set the value from code, so script access
            // $$$TZA it's been too long - I can't recall how to initialize a string registered member from code  :(
            for (int i = 0; i < 20; ++i)
            {
                /*
                char initial_value[32];
                snprintf(initial_value, sizeof(initial_value), "string %d", i);
                stringArray[i] = TinScript::GetContext()->GetStringTable()->AddString(initial_value, -1, TinScript::Hash(initial_value), true);
                */
                stringArray[i] = nullptr;
            }
        }

        virtual ~CBase() {
            MTPrint("Enter destructor ~CBase()\n");
        }

        float32 GetFloatValue() {
            return floatvalue;
        }

        void SetFloatValue(float32 val) {
            floatvalue = val;
        }

        int32 GetIntValue() {
            return intvalue;
        }

        virtual void SetIntValue(int32 val) {
            MTPrint("Enter CBase::SetIntValue()\n");
            intvalue = val;
        }

        bool8 GetBoolValue() {
            return boolvalue;
        }

        void SetBoolValue(bool8 val) {
            boolvalue = val;
        }

        int32 TestP1(int32 a)
        {
            MTPrint("CBase P1: %d\n", a);
            return (a);
        }

        void VoidP1(int32 a)
        {
            MTPrint("CBase void1: %d\n", a);
        }

        float32 floatvalue;
        int32 intvalue;
        bool8 boolvalue;

        // -- additional members, using non-standard registration types
        CVector3f v3member;
        CBase* objmember;

        int32 intArray[20];
        const char* stringArray[20];
};

REGISTER_SCRIPT_CLASS_BEGIN(CBase, VOID)
    REGISTER_MEMBER(CBase, floatvalue, floatvalue);
    REGISTER_MEMBER(CBase, intvalue, intvalue);
    REGISTER_MEMBER(CBase, boolvalue, boolvalue);
    REGISTER_MEMBER(CBase, v3member, v3member);
    REGISTER_MEMBER(CBase, objmember, objmember);
    REGISTER_MEMBER(CBase, intArray, intArray);
    REGISTER_MEMBER(CBase, stringArray, stringArray);
REGISTER_SCRIPT_CLASS_END()

REGISTER_METHOD(CBase, GetFloatValue, GetFloatValue);
REGISTER_METHOD(CBase, GetIntValue, GetIntValue);
REGISTER_METHOD(CBase, GetBoolValue, GetBoolValue);

REGISTER_METHOD(CBase, SetFloatValue, SetFloatValue);
REGISTER_METHOD(CBase, SetIntValue, SetIntValue);
REGISTER_METHOD(CBase, SetBoolValue, SetBoolValue);

class CChild : public CBase {
    public:
        CChild() : CBase() {
            MTPrint("Enter constructor CChild()\n");
            floatvalue = 19.0;
            intvalue = 11;
            boolvalue = false;
        }
        virtual ~CChild() {
            MTPrint("Enter destructor ~CChild()\n");
        }

        virtual void SetIntValue(int32 val) {
            MTPrint("Enter CChild::SetIntValue()\n");
            intvalue = 2 * val;
        }
};

REGISTER_SCRIPT_CLASS_BEGIN(CChild, CBase)
REGISTER_SCRIPT_CLASS_END()

// -- SetIntValue already registered in the base class
//REGISTER_METHOD(CChild, SetIntValue, SetIntValue);

// --------------------------------------------------------------------------------------------------------------------
// class CUnitTest:  A generic way of specifying unit tests, allowing both scripted and code functions
// to be executed, and verifying the results from either
// --------------------------------------------------------------------------------------------------------------------
class CUnitTest
{
    public:
        typedef void (*UnitTestFunc)();
        CUnitTest(const char* name, const char* description, const char* script_command, const char* script_result,
                  UnitTestFunc code_test = NULL, const char* code_result = NULL, bool execute_code_last = false)
        {
            // -- copy the unit test parameters
            TinScript::SafeStrcpy(mName, sizeof(mName), name, TinScript::kMaxTokenLength);
            TinScript::SafeStrcpy(mDescription, sizeof(mDescription), description, TinScript::kMaxTokenLength);
            TinScript::SafeStrcpy(mScriptCommand, sizeof(mScriptCommand), script_command, TinScript::kMaxTokenLength);
            TinScript::SafeStrcpy(mScriptResult, sizeof(mScriptResult), script_result, TinScript::kMaxTokenLength);

            mExecuteCodeLast = execute_code_last;
            mCodeTest = code_test;
            TinScript::SafeStrcpy(mCodeResult, sizeof(mCodeResult), code_result, TinScript::kMaxTokenLength);
        }

        // -- members
        char mName[kMaxArgLength];
        char mDescription[kMaxArgLength];
        char mScriptCommand[kMaxArgLength];
        char mScriptResult[kMaxArgLength];

        bool mExecuteCodeLast;
        UnitTestFunc mCodeTest;
        char mCodeResult[kMaxArgLength];

        static const char* gScriptResult;
        static char gCodeResult[kMaxArgLength];

        static TinScript::CHashTable<CUnitTest>* gUnitTests;
};

// --------------------------------------------------------------------------------------------------------------------
// declare static members of CUnitTest, including registrations
TinScript::CHashTable<CUnitTest>* CUnitTest::gUnitTests = NULL;
const char* CUnitTest::gScriptResult = "";
char CUnitTest::gCodeResult[kMaxArgLength];

REGISTER_GLOBAL_VAR(gUnitTestScriptResult, CUnitTest::gScriptResult);

// -- GLOBAL VARIABLES ----------------------------------------------------------------------------
int32 gUnitTestRegisteredInt = 17;
REGISTER_GLOBAL_VAR(gUnitTestRegisteredInt, gUnitTestRegisteredInt);

static int32 gUnitTestIntArray[17];
REGISTER_GLOBAL_VAR(gUnitTestIntArray, gUnitTestIntArray);

static const char* gUnitTestStringArray[17];
REGISTER_GLOBAL_VAR(gUnitTestStringArray, gUnitTestStringArray);

// -- Registered enum -----------------------------------------------------------------------------

#define MyEnum(TestEnum, ENUM_ENTRY)	\
	ENUM_ENTRY(TestEnum, Foo, 0)		\
	ENUM_ENTRY(TestEnum, Bar, 17)		\
	ENUM_ENTRY(TestEnum, Count, 49)

CREATE_ENUM_CLASS(TestEnum, MyEnum);
CREATE_ENUM_STRINGS(TestEnum, MyEnum);
REGISTER_ENUM_CLASS(TestEnum, MyEnum);

//REGISTER_SCRIPT_ENUM(TestEnum, MyEnum);

// -- GLOBAL FUNCTIONS ----------------------------------------------------------------------------
// -- these are registered, and called from script for unit testing
int32 UnitTest_MultiplyBy2(int32 number) {
    return (number << 1);
}

float32 UnitTest_DivideBy3(float32 number) {
    return (number / 3.0f);
}

bool UnitTest_IsGreaterThan(float32 number0, float32 number1)
{
    return (number0 > number1);
}

const char* UnitTest_AnimalType(const char* animal_name)
{
    if (!_stricmp(animal_name, "spot"))
        return ("dog");
    else if(!_stricmp(animal_name, "felix"))
        return ("cat");
    else if(!_stricmp(animal_name, "fluffy"))
        return ("goldfish");
    else
        return ("unknown");
}

Vector3fClass UnitTest_V3fNormalize(Vector3fClass v0)
{
    Vector3fClass result = TS_V3fNormalized(v0);
    return (result);
}

REGISTER_FUNCTION(UnitTest_MultiplyBy2, UnitTest_MultiplyBy2);
REGISTER_FUNCTION(UnitTest_DivideBy3, UnitTest_DivideBy3);
REGISTER_FUNCTION(UnitTest_IsGreaterThan, UnitTest_IsGreaterThan);
REGISTER_FUNCTION(UnitTest_AnimalType, UnitTest_AnimalType);
REGISTER_FUNCTION(UnitTest_V3fNormalize, UnitTest_V3fNormalize);

// -- these functions contain calls to scripted functions to test reliably receiving return values
void UnitTest_GetScriptReturnInt()
{
    int result;
    if (!TinScript::ExecF(result, "UnitTest_ScriptReturnInt(%d);", -5))
    {
        ScriptAssert_(TinScript::GetContext(), false, "<internal>", -1,
                      "Error - failed to execute UnitTest_ScriptReturnInt()\n");
    }
    else
    {
        // -- print the result to a testable string
        snprintf(CUnitTest::gCodeResult, sizeof(CUnitTest::gCodeResult), "%d", result);
    }
}

void UnitTest_GetScriptReturnFloat()
{
    float result;
    if (!TinScript::ExecF(result, "UnitTest_ScriptReturnFloat(%f);", 15.0f))
    {
        ScriptAssert_(TinScript::GetContext(), false, "<internal>", -1,
                      "Error - failed to execute UnitTest_ScriptReturnFloat()\n");
    }
    else
    {
        // -- print the result to a testable string
        snprintf(CUnitTest::gCodeResult, sizeof(CUnitTest::gCodeResult), "%.4f", result);
    }
}

void UnitTest_GetScriptReturnBool()
{
    bool result;
    if (!TinScript::ExecF(result, "UnitTest_ScriptReturnBool(%f, %f);", 5.1f, 5.0f))
    {
        ScriptAssert_(TinScript::GetContext(), false, "<internal>", -1,
                      "Error - failed to execute UnitTest_ScriptReturnBool()\n");
    }
    else
    {
        // -- print the result to a testable string
        snprintf(CUnitTest::gCodeResult, sizeof(CUnitTest::gCodeResult), "%s", result ? "true" : "false");
    }
}

void UnitTest_GetScriptReturnString()
{
	// -- there are two ways to execute a script function from code - this is the slow way:
    // construct the entire command as a single string, to be parsed and executed
    const char* result;
    if (!TinScript::ExecF(result, "UnitTest_ScriptReturnString('goldfish');"))
    {
        ScriptAssert_(TinScript::GetContext(), false, "<internal>", -1,
                      "Error - failed to execute UnitTest_ScriptReturnString()\n");
    }
    else
    {
        // -- print the result to a testable string
        snprintf(CUnitTest::gCodeResult, sizeof(CUnitTest::gCodeResult), "%s", result);
    }
}

void UnitTest_GetScriptReturnStringExec()
{
    // -- there are two ways to execute a script function from code - this is the much faster way
    // schedule an immediate call to the function, with args... no parsing!
	const char* result;
	if (!TinScript::ExecFunction(result, TinScript::Hash("UnitTest_ScriptReturnString"), "goldfish"))
	{
		ScriptAssert_(TinScript::GetContext(), false, "<internal>", -1,
			"Error - failed to execute UnitTest_ScriptReturnString()\n");
	}
	else
	{
		// -- print the result to a testable string
        snprintf(CUnitTest::gCodeResult, sizeof(CUnitTest::gCodeResult), "%s", result);
	}
}

void UnitTest_GetScriptReturnVector3f()
{
    // -- note, the vector3f pass by value to script, is a string which will automatically be converted
    Vector3fClass result;
    if (!TinScript::ExecF(result, "UnitTest_ScriptReturnVector3f('1 2 3');"))
    {
        ScriptAssert_(TinScript::GetContext(), false, "<internal>", -1,
                      "Error - failed to execute UnitTest_ReturnTypeVector3f()\n");
    }
    else
    {
        Vector3fToString(TinScript::GetContext(), (void*)&result, CUnitTest::gCodeResult,
                         sizeof(CUnitTest::gCodeResult));
    }
}

// --------------------------------------------------------------------------------------------------------------------
void UnitTest_CallScriptedMethodExecf()
{
    // -- create our test object from code this time
    CChild* test_obj = TinAlloc(ALLOC_Debugger, CChild);

    // -- manually register our test object
    TinScript::GetContext()->RegisterObject(test_obj, "CChild", "TestCodeNSObject");

    // -- now call a scripted method in the object's namespace, and retrieve a result
    const char* result = NULL;
    if (!TinScript::ObjExecF(test_obj, result, "ModifyTestMemberString('Moooo');"))
    {
        ScriptAssert_(TinScript::GetContext(), false, "<internal>", -1,
                      "Error - failed to execute method ModifyTestMember()\n");
        return;
    }

    // -- print the result to a testable string
    strcpy_s(CUnitTest::gCodeResult, result);

    // -- unregister the object
    TinScript::GetContext()->UnregisterObject(test_obj);

    // -- delete the object
    TinFree(test_obj);
}

// --------------------------------------------------------------------------------------------------------------------
void UnitTest_CallScriptedMethodHashed()
{
    // -- create our test object from code this time
    CChild* test_obj = TinAlloc(ALLOC_Debugger,CChild);

    // -- manually register our test object
    TinScript::GetContext()->RegisterObject(test_obj,"CChild","TestCodeNSObject");

    // -- now call a scripted method in the object's namespace, and retrieve a result
    const char* result = NULL;
    uint32 hash_ModifyTestMember = TinScript::Hash("ModifyTestMemberInt");
    if(!TinScript::ObjExecMethod(test_obj,result,hash_ModifyTestMember, 67))
    {
        ScriptAssert_(TinScript::GetContext(),false,"<internal>",-1,
                      "Error - failed to execute method ModifyTestMember()\n");
        return;
    }

    // -- print the result to a testable string
    strcpy_s(CUnitTest::gCodeResult,result);

    // -- unregister the object
    TinScript::GetContext()->UnregisterObject(test_obj);

    // -- delete the object
    TinFree(test_obj);
}

// --------------------------------------------------------------------------------------------------------------------
void UnitTest_CallScriptedMethodObjectArg()
{
    // -- create our test object from code this time
    CChild* test_obj = TinAlloc(ALLOC_Debugger, CChild);

    // -- manually register our test object
    uint32 obj_id = TinScript::GetContext()->RegisterObject(test_obj,"CChild","TestCodeNSObject");

    // -- now call a scripted method in the object's namespace, and retrieve a result
    const char* result = NULL;
    uint32 hash_VerifySelfByID = TinScript::Hash("VerifySelfByID");
    if (!TinScript::ObjExecMethod(test_obj,result,hash_VerifySelfByID, obj_id))
    {
        ScriptAssert_(TinScript::GetContext(),false,"<internal>",-1,
                      "Error - failed to execute method VerifySelfByID()\n");
        return;
    }

    // -- print the result to a testable string
    strcpy_s(CUnitTest::gCodeResult,result);

    // -- unregister the object
    TinScript::GetContext()->UnregisterObject(test_obj);

    // -- delete the object
    TinFree(test_obj);
}

// --------------------------------------------------------------------------------------------------------------------
void UnitTest_CallScriptedMethodObjectAddrArg()
{
    // -- create our test object from code this time
    CChild* test_obj = TinAlloc(ALLOC_Debugger, CChild);

    // -- manually register our test object
    uint32 obj_id = TinScript::GetContext()->RegisterObject(test_obj,"CChild","TestCodeNSObject");

    // -- now call a scripted method in the object's namespace, and retrieve a result
    const char* result = NULL;
    uint32 hash_VerifySelfByAddr = TinScript::Hash("VerifySelfByAddr");
    if (!TinScript::ObjExecMethod(test_obj,result,hash_VerifySelfByAddr, test_obj))
    {
        ScriptAssert_(TinScript::GetContext(),false,"<internal>",-1,
                      "Error - failed to execute method VerifySelfByAddr()\n");
        return;
    }

    // -- print the result to a testable string
    strcpy_s(CUnitTest::gCodeResult,result);

    // -- unregister the object
    TinScript::GetContext()->UnregisterObject(test_obj);

    // -- delete the object
    TinFree(test_obj);
}


bool8 AddUnitTest(const char* name, const char* description, const char* script_command, const char* script_result,
                  CUnitTest::UnitTestFunc code_test = NULL, const char* code_result = NULL, bool execute_code_last = false)
{
    // -- unit tests are run on the main thread
    TinScript::CScriptContext* script_context = TinScript::GetContext();

    // -- sanity check
    if (!script_context || !CUnitTest::gUnitTests)
        return (false);

    // -- validate the input
    // -- neither the code_test, nor the code_result need to be specified
    bool8 valid = name && strlen(name) < kMaxArgLength;
    valid = valid && description && strlen(description) < kMaxArgLength;
    valid = valid && script_command && strlen(script_command) < kMaxArgLength;
    valid = valid && script_result && strlen(script_result) < kMaxArgLength;

    // -- the code_test and the code_result only need to be specified if we're explicitly executing them last
    if (execute_code_last)
    {
        valid = valid && code_test != NULL;
        valid = valid && code_result && strlen(code_result) < kMaxArgLength;
    }

    // -- hash the name, and ensure a unit test with that name hasn't already been created
    uint32 hash = TinScript::Hash(name);
    valid = valid && hash != 0 && CUnitTest::gUnitTests->FindItem(hash) == NULL;

    ScriptAssert_(script_context, valid, "<internal>", -1,
                  "Error - Invalid unit test: %s\n", name ? name : "<unnamed>");
    if (!valid)
        return (false);

    // -- create and add the test to the hash table
    CUnitTest* new_test = TinAlloc(ALLOC_Debugger, CUnitTest, name, description, script_command, script_result,
                                   code_test, code_result, execute_code_last);
    CUnitTest::gUnitTests->AddItem(*new_test, hash);

    // -- success
    return (true);
}

void ClearResults()
{
    // -- unit tests are run on the main thread
    TinScript::CScriptContext* script_context = TinScript::GetContext();

    // -- sanity check
    if (!script_context || !CUnitTest::gUnitTests)
        return;

    // -- keep the string table clean
    // $$$TZA Replace this with an actual string class that supports const char* conversions, but
    // -- ensures the value is *always* stored in the string table
    TinScript::SetGlobalVar(script_context, "gUnitTestScriptResult", "");

    // -- the code result is on the code side only, not a string table entry
    CUnitTest::gCodeResult[0] = '\0';
}

uint32 PerformUnitTests(bool8 results_only, const char* specific_test)
{
    // -- unit tests are run on the main thread
    TinScript::CScriptContext* script_context = TinScript::GetContext();

    // -- sanity check
    if (!script_context || !CUnitTest::gUnitTests)
        return (0);

    // -- get the hash value, if we're using a specific test
    uint32 specific_test_hash = TinScript::Hash(specific_test);

    // -- loop through and perform the unit tests
    uint32 error_test_hash = 0;
    uint32 current_test_hash = 0;
    int32 test_number = 0;
    const CUnitTest* current_test = CUnitTest::gUnitTests->First(&current_test_hash);
    while (current_test)
    {
        // -- if we're performing a specific test, ensure this is the correct one
        if (specific_test_hash != 0 && current_test_hash != specific_test_hash)
        {
            // -- next test
            current_test = CUnitTest::gUnitTests->Next(&current_test_hash);
            continue;
        }

        // -- clear the script and code results
        ClearResults();

        // -- increment the count
        ++test_number;

        // -- Print the name and description
        results_only ? 0 : MTPrint("\n[%d] Unit test: %s\nDesc: %s\nScript result: %s\nCode result: %s\n", test_number,
                                   current_test->mName, current_test->mDescription,
                                   current_test->mScriptResult[0] ? current_test->mScriptResult : "\"\"",
                                   current_test->mCodeResult[0] ? current_test->mCodeResult : "\"\"");

        // -- call the code test (if it exists, and is not explicitly after the script command)
        if (current_test->mCodeTest && !current_test->mExecuteCodeLast)
            current_test->mCodeTest();

        // -- Execute the command
        script_context->ExecCommand(current_test->mScriptCommand);

        // -- call the code test (if it exists, and *is* explicitly after the script command)
        if (current_test->mCodeTest && current_test->mExecuteCodeLast)
            current_test->mCodeTest();

        // -- compare the results
        bool8 current_result = true;
        if (strcmp(CUnitTest::gScriptResult, current_test->mScriptResult) != 0)
        {
            ScriptAssert_(script_context, false, "<unit test>", -1,
                          "Error() - Unit test '%s' failed the script result\n", current_test->mName);
            current_result = false;
        }
        if (strcmp(CUnitTest::gCodeResult, current_test->mCodeResult) != 0)
        {
            ScriptAssert_(script_context, false, "<unit test>", -1,
                          "Error() - Unit test '%s' failed the code result\n", current_test->mName);
            current_result = false;
        }

        // -- handle the result of this test
        if (!current_result)
        {
            // -- store the hash
            if (error_test_hash == 0)
                error_test_hash = current_test_hash;
        }
        else
        {
            results_only ? 0 : MTPrint("*** Passed\n");
        }

        // -- next test
        current_test = CUnitTest::gUnitTests->Next(&current_test_hash);
    }

    // -- results
    return (error_test_hash);
}

void UnitTest_RegisteredIntAccess()
{
    gUnitTestRegisteredInt = 17;
}

void UnitTest_RegisteredIntModify()
{
    // -- the script will have modified the registered int to the value of 23
    snprintf(CUnitTest::gCodeResult, sizeof(CUnitTest::gCodeResult), "%d", gUnitTestRegisteredInt);
}

void UnitTest_ScriptIntAccess()
{
    int32 script_value;
    if (!TinScript::GetGlobalVar(TinScript::GetContext(), "gUnitTestScriptInt", script_value))
    {
        ScriptAssert_(TinScript::GetContext(), false, "<internal>", -1,
                      "Error - unable to access global script variable: gUnitTestScriptInt\n");
    }

    // -- otherwise, pring the value to the code result
    else
    {
        snprintf(CUnitTest::gCodeResult, sizeof(CUnitTest::gCodeResult), "%d", script_value);
    }
}

void UnitTest_ScriptIntModify()
{
    TinScript::SetGlobalVar(TinScript::GetContext(), "gUnitTestScriptInt", 23);
}

void UnitTest_RegisteredIntArrayModify()
{
    snprintf(CUnitTest::gCodeResult, sizeof(CUnitTest::gCodeResult), "%d %d", gUnitTestIntArray[3], gUnitTestIntArray[5]);
}

void UnitTest_RegisteredStringArrayModify()
{
    snprintf(CUnitTest::gCodeResult, sizeof(CUnitTest::gCodeResult), "%s %s", gUnitTestStringArray[4], gUnitTestStringArray[9]);
}

bool8 CreateUnitTests()
{
    // -- initialize the result
    bool8 success = true;

    // -- ensure our unit test dictionary exits
    if (! CUnitTest::gUnitTests)
    {
        CUnitTest::gUnitTests = TinAlloc(ALLOC_Debugger, TinScript::CHashTable<CUnitTest>, kGlobalFuncTableSize);

        // -- int unit tests ----------------------------------------------------------------------------------------
        // -- int math unit tests
        success = success && AddUnitTest("int_add", "3 + 4", "gUnitTestScriptResult = StringCat(3 + 4);", "7");
        success = success && AddUnitTest("int_sub", "3 - 4", "gUnitTestScriptResult = StringCat(3 - 4);", "-1");
        success = success && AddUnitTest("int_mult", "-3 * 4", "int var_int = -3 * 4; gUnitTestScriptResult = StringCat(var_int);", "-12");
        success = success && AddUnitTest("int div", "12 / 4", "gUnitTestScriptResult = StringCat(12 / 4);", "3");
        success = success && AddUnitTest("int_mod", "17 % 3f", "gUnitTestScriptResult = StringCat(17 % 3);", "2");

        // -- int comparison tests
        success = success && AddUnitTest("int_lt_t", "3 < 4", "gUnitTestScriptResult = StringCat(3 < 4);", "true");
        success = success && AddUnitTest("int_lt_f", "4 < 4", "gUnitTestScriptResult = StringCat(4 < 4);", "false");
        success = success && AddUnitTest("int_le_t", "4 <= 4", "gUnitTestScriptResult = StringCat(4 <= 4);", "true");
        success = success && AddUnitTest("int_le_f", "5 <= 4", "gUnitTestScriptResult = StringCat(5 <= 4);", "false");
        success = success && AddUnitTest("int_gt_t", "4 > 3", "gUnitTestScriptResult = StringCat(4 > 3);", "true");
        success = success && AddUnitTest("int_gt_f", "4 > 4", "gUnitTestScriptResult = StringCat(4 > 4);", "false");
        success = success && AddUnitTest("int_ge_t", "3 >= 3", "gUnitTestScriptResult = StringCat(3 >= 3);", "true");
        success = success && AddUnitTest("int_ge_f", "2 >= 3", "gUnitTestScriptResult = StringCat(2 >= 3);", "false");
        success = success && AddUnitTest("int_eq_t", "3 == 3", "gUnitTestScriptResult = StringCat(3 == 3);", "true");
        success = success && AddUnitTest("int_eq_f", "4 == 3", "gUnitTestScriptResult = StringCat(4 == 3);", "false");
        success = success && AddUnitTest("int_ne_t", "4 != 3", "gUnitTestScriptResult = StringCat(4 != 3);", "true");
        success = success && AddUnitTest("int_ne_f", "3 != 3", "gUnitTestScriptResult = StringCat(3 != 3);", "false");

        // -- int boolean tests
        success = success && AddUnitTest("int_and_t", "3 && 4", "gUnitTestScriptResult = StringCat(3 && 4);", "true");
        success = success && AddUnitTest("int_and_f", "0 && 4", "gUnitTestScriptResult = StringCat(0 && 4);", "false");
        success = success && AddUnitTest("int_or_t", "3 || 4", "gUnitTestScriptResult = StringCat(3 || 4);", "true");
        success = success && AddUnitTest("int_or_f", "0 || 4", "gUnitTestScriptResult = StringCat(0 || 4);", "true");

        // -- int bitwise tests
        success = success && AddUnitTest("bit_leftshift", "1 << 8", "gUnitTestScriptResult = StringCat(1 << 8);", "256");
        success = success && AddUnitTest("bit_rightshift", "20 >> 2", "gUnitTestScriptResult = StringCat(20 >> 2);", "5");
        success = success && AddUnitTest("bit_and", "0b1010 & 0b0110", "gUnitTestScriptResult = StringCat(0b1010 & 0b0110);", "2");
        success = success && AddUnitTest("bit_or", "0b1010 | 0b0110", "gUnitTestScriptResult = StringCat(0b1010 | 0b0110);", "14");
        success = success && AddUnitTest("bit_xor", "0b1010 ^ 0b0110", "gUnitTestScriptResult = StringCat(0b1010 ^ 0b0110);", "12");

        // -- int convert tests
        success = success && AddUnitTest("int_float", "var_int = 5.3f", "int var_int = 5.3f; gUnitTestScriptResult = StringCat(var_int);", "5");
        success = success && AddUnitTest("int_bool", "var_int = true", "int var_int = true; gUnitTestScriptResult = StringCat(var_int);", "1");
        success = success && AddUnitTest("int_string", "var_int = '5.3f';", "int var_int = '5.3f'; gUnitTestScriptResult = StringCat(var_int);", "5");

        // -- float unit tests ----------------------------------------------------------------------------------------
        // -- float math unit tests
        success = success && AddUnitTest("float_add", "3.0f + 4.0f", "gUnitTestScriptResult = StringCat(3.0f + 4.0f);", "7.0000");
        success = success && AddUnitTest("float_sub", "3.0f - 4.0f", "gUnitTestScriptResult = StringCat(3.0f - 4.0f);", "-1.0000");
        success = success && AddUnitTest("float_mult", "-3.0f * 4.0f", "gUnitTestScriptResult = StringCat(-3.0f * 4.0f);", "-12.0000");
        success = success && AddUnitTest("float_div", "3.0f / 4.0f", "gUnitTestScriptResult = StringCat(3.0f / 4.0f);", "0.7500");
        success = success && AddUnitTest("float_mod", "13.5f % 4.1f", "gUnitTestScriptResult = StringCat(13.5f % 4.1f);", "1.2000");

        // -- float comparison tests
        success = success && AddUnitTest("float_lt_t", "3.5f < 4.3f", "gUnitTestScriptResult = StringCat(3.5f < 4.3f);", "true");
        success = success && AddUnitTest("float_lt_f", "4.2f < 4.2f", "gUnitTestScriptResult = StringCat(4.2f < 4.2f);", "false");
        success = success && AddUnitTest("float_le_t", "4.6f <= 4.6f", "gUnitTestScriptResult = StringCat(4.6f <= 4.6f);", "true");
        success = success && AddUnitTest("float_le_f", "5.1f <= 4.9f", "gUnitTestScriptResult = StringCat(5.1f <= 4.9f);", "false");
        success = success && AddUnitTest("float_gt_t", "3.3f > 3.0f", "gUnitTestScriptResult = StringCat(3.3f > 3.0f);", "true");
        success = success && AddUnitTest("float_gt_f", "4.8f > 4.8f", "gUnitTestScriptResult = StringCat(4.8f > 4.8f);", "false");
        success = success && AddUnitTest("float_ge_t", "3.4f >= 3.4f", "gUnitTestScriptResult = StringCat(3.4f >= 3.4f);", "true");
        success = success && AddUnitTest("float_ge_f", "2.9f >= 3.0f", "gUnitTestScriptResult = StringCat(2.9f >= 3.0f);", "false");
        success = success && AddUnitTest("float_eq_t", "3.0f == 3.0f", "gUnitTestScriptResult = StringCat(3.0f == 3.0f);", "true");
        success = success && AddUnitTest("float_eq_f", "3.1f == 3", "gUnitTestScriptResult = StringCat(3.1f == 3);", "false");
        success = success && AddUnitTest("float_ne_t", "3.1f != 3", "gUnitTestScriptResult = StringCat(3.1f != 3);", "true");
        success = success && AddUnitTest("float_ne_f", "3.0f != 3", "gUnitTestScriptResult = StringCat(3.0f != 3);", "false");

        // -- float boolean tests
        success = success && AddUnitTest("float_and_t", "3.0f && 4.1f", "gUnitTestScriptResult = StringCat(3.0f && 4.1f);", "true");
        success = success && AddUnitTest("float_and_f", "0.0f && 0.1f", "gUnitTestScriptResult = StringCat(0.0f && 0.1f);", "false");
        success = success && AddUnitTest("float_or_t", "0.1f || 0.2f", "gUnitTestScriptResult = StringCat(0.1f || 0.2f);", "true");
        success = success && AddUnitTest("float_or_f", "0.0f || 0.1f", "gUnitTestScriptResult = StringCat(0.0f || 0.1f);", "true");

        // -- float convert tests
        success = success && AddUnitTest("float_int", "var_float = 5", "float var_float = 5; gUnitTestScriptResult = StringCat(var_float);", "5.0000");
        success = success && AddUnitTest("float_bool", "var_float = true", "float var_float = true; gUnitTestScriptResult = StringCat(var_float);", "1.0000");
        success = success && AddUnitTest("float_string", "var_float = '5.3f';", "float var_float = '5.3f'; gUnitTestScriptResult = StringCat(var_float);", "5.3000");

        // -- post-unary op tests (e.g. x++)
        success = success && AddUnitTest("post_inc_int", "var_int++;", "int var_int = 5; var_int++; gUnitTestScriptResult = StringCat(var_int);", "6");
        success = success && AddUnitTest("post_inc_float", "var_float++;", "float var_float = -5.25f; var_float++; gUnitTestScriptResult = StringCat(var_float);", "-4.2500");
        success = success && AddUnitTest("post_inc_assign", "var_int = a++;", "int a = 3; int var_int = a++; gUnitTestScriptResult = StringCat(var_int, ' ', a);", "3 4");
        success = success && AddUnitTest("post_inc_array", "foo[3]++;", "int[5] foo; foo[3] = 7; foo[3]++; gUnitTestScriptResult = StringCat(foo[3]);", "8");
        success = success && AddUnitTest("post_inc_index", "foo[bar++];", "int[5] foo; int bar = 2; foo[bar++] = 9; gUnitTestScriptResult = StringCat(bar, ' ', foo[2]);", "3 9");
        success = success && AddUnitTest("post_inc_array_index", "foo[bar++]++;", "int[5] foo; int bar = 4; foo[bar] = 7; foo[bar++]++; gUnitTestScriptResult = StringCat(bar, ' ', foo[4]);", "5 8");
        success = success && AddUnitTest("post_inc_v3f", "pos:y++;", "vector3f pos = '1 2 3'; pos:y++; gUnitTestScriptResult = StringCat(pos:y);", "3.0000");
        success = success && AddUnitTest("post_inc_member", "obj.mem++;", "object obj = create CChild('testChild1'); obj.intvalue++; gUnitTestScriptResult = StringCat(obj.intvalue);", "12");
        success = success && AddUnitTest("post_inc_member_array", "obj.intArray[bar++];", "int bar = 3; object obj = create CChild('testChild2'); obj.intArray[bar++] = 7; gUnitTestScriptResult = StringCat(bar, ' ', obj.intArray[3]);", "4 7");
        success = success && AddUnitTest("post_inc_member_array2", "obj.mem++;", "int bar = 3; object obj = create CChild('testChild3'); obj.intArray[3] = 17; obj.intArray[bar++]++; gUnitTestScriptResult = StringCat(bar, ' ', obj.intArray[3]);", "4 18");

        // -- bool unit tests -----------------------------------------------------------------------------------------
        // -- bool boolean unit tests
        success = success && AddUnitTest("bool_and_tt", "true && true", "gUnitTestScriptResult = StringCat(true && true);", "true");
        success = success && AddUnitTest("bool_and_tf", "true && false", "gUnitTestScriptResult = StringCat(true && false);", "false");
        success = success && AddUnitTest("bool_and_ff", "false && false", "gUnitTestScriptResult = StringCat(false && false);", "false");

        success = success && AddUnitTest("bool_or_tt", "true || true", "gUnitTestScriptResult = StringCat(true || true);", "true");
        success = success && AddUnitTest("bool_or_tf", "true || false", "gUnitTestScriptResult = StringCat(true || false);", "true");
        success = success && AddUnitTest("bool_or_ff", "false || false", "gUnitTestScriptResult = StringCat(false || false);", "false");

        // -- vector3f unit tests -----------------------------------------------------------------------------------------
        success = success && AddUnitTest("vector3f_assign", "v0 = (1, 2, 3)", "vector3f v0 = '1, 2, 3'; gUnitTestScriptResult = StringCat(v0);", "1.0000 2.0000 3.0000");
        success = success && AddUnitTest("vector3f_add", "(1, 2, 3) + (4, 5, 6)", "vector3f v0 = '1, 2, 3'; vector3f v1 = '4 5 6'; gUnitTestScriptResult = StringCat(v0 + v1);", "5.0000 7.0000 9.0000");
        success = success && AddUnitTest("vector3f_sub", "(1, 2, 3) - (4, 5, 6)", "vector3f v0 = '1, 2, 3'; vector3f v1 = '4 5 6'; gUnitTestScriptResult = StringCat(v0 - v1);", "-3.0000 -3.0000 -3.0000");
        success = success && AddUnitTest("vector3f_scale1", "(1, 2, 3) * 3.5f", "vector3f v0 = '1, 2, 3'; gUnitTestScriptResult = StringCat(v0 * 3.5f);", "3.5000 7.0000 10.5000");
        success = success && AddUnitTest("vector3f_scale2", "-2.9f * (1, 2, 3)", "vector3f v0 = '1, 2, 3'; gUnitTestScriptResult = StringCat(-2.9f * v0);", "-2.9000 -5.8000 -8.7000");
        success = success && AddUnitTest("vector3f_scale3", "-2.9f * (1, 2, 3) * 0.4f", "vector3f v0 = '1, 2, 3'; gUnitTestScriptResult = StringCat(-2.9f * v0 * 0.4f);", "-1.1600 -2.3200 -3.4800");
        success = success && AddUnitTest("vector3f_scale4", "(1, 2, 3) / 0.3f", "vector3f v0 = '1, 2, 3'; gUnitTestScriptResult = StringCat(v0 / 0.3f);", "3.3333 6.6667 10.0000");

        // -- vector3f POD test
        success = success && AddUnitTest("vector3f_podx", "Print the 'x' of (1, 2, 3)", "vector3f v0 = '1, 2, 3'; gUnitTestScriptResult = StringCat(v0:x);", "1.0000");
        success = success && AddUnitTest("vector3f_pody", "Print the 'y' of (1, 2, 3)", "vector3f v0 = '1, 2, 3'; gUnitTestScriptResult = StringCat(v0:y);", "2.0000");
        success = success && AddUnitTest("vector3f_podz", "Print the 'z' of (1, 2, 3)", "vector3f v0 = '1, 2, 3'; gUnitTestScriptResult = StringCat(v0:z);", "3.0000");

        // -- vector3f registered functions for this registered type
        success = success && AddUnitTest("vector3f_length", "Length of (1, 2, 3)", "vector3f v0 = '1, 2, 3'; gUnitTestScriptResult = StringCat(V3fLength(v0));", "3.7417");
        success = success && AddUnitTest("vector3f_cross", "(1, 2, 3) cross (4 5 6)", "vector3f v0 = '1, 2, 3'; vector3f v1 = '4 5 6'; gUnitTestScriptResult = StringCat(V3fCross(v0, v1));", "-3.0000 6.0000 -3.0000");
        success = success && AddUnitTest("vector3f_dot", "(1, 2, 3) dot (4 5 6)", "vector3f v0 = '1, 2, 3'; vector3f v1 = '4 5 6'; gUnitTestScriptResult = StringCat(V3fDot(v0, v1));", "32.0000");
        success = success && AddUnitTest("vector3f_norm", "(1, 2, 3) normalized", "vector3f v0 = '1, 2, 3'; gUnitTestScriptResult = StringCat(V3fNormalized(v0));", "0.2673 0.5345 0.8018");

        // -- script access to registered variables -------------------------------------------------------------------
        success = success && AddUnitTest("scriptaccess_regint", "gUnitTestRegisteredInt, value 17 read from script", "UnitTest_RegisteredIntAccess();", "17", UnitTest_RegisteredIntAccess);
        success = success && AddUnitTest("scriptmodify_regint", "Modify gUnitTestRegisteredInt set to 23 from script", "UnitTest_RegisteredIntModify();", "", UnitTest_RegisteredIntModify, "23", true);

        success = success && AddUnitTest("codeaccess_scriptint", "Retrieve scripted gUnitTestScriptInt", "UnitTest_CodeAccess();", "", UnitTest_ScriptIntAccess, "49", true);
        success = success && AddUnitTest("codemodify_scriptint", "Modify scripted gUnitTestScriptInt", "UnitTest_CodeModify();", "23", UnitTest_ScriptIntModify, "", false);

        // -- flow control --------------------------------------------------------------------------------------------
        success = success && AddUnitTest("flow_if", "If input > 9", "UnitTest_IfStatement(10);", "10 is greater than 9");
        success = success && AddUnitTest("flow_elseif", "If input < 9", "UnitTest_IfStatement(8);", "8 is less than 9");
        success = success && AddUnitTest("flow_else", "If input == 9", "UnitTest_IfStatement(9);", "9 is equal to 9");

        success = success && AddUnitTest("flow_while", "while loop - count 5 to 1", "UnitTest_WhileStatement();", " 5 4 3 2 1");
        success = success && AddUnitTest("flow_for", "for loop - count 0 to 4", "UnitTest_ForLoop();", " 0 1 2 3 4");

        success = success && AddUnitTest("parenthesis", "Expr: (((3 + 4) * 17) - (3.0f + 6)) % (42 / 3)", "TestParenthesis();", "12.0000");

        // -- code functions with return types ------------------------------------------------------------------------
        // -- each of the following, the script function will call a registered code function with a return type
        // -- and then we verify that the result returned by code is what the scripted function received
        success = success && AddUnitTest("code_return_int", "Code multiply by 2", "UnitTest_ReturnTypeInt(-5);", "-10");
        success = success && AddUnitTest("code_return_float", "Code divide by 3.0f", "UnitTest_ReturnTypeFloat(15.0f);", "5.0000");
        success = success && AddUnitTest("code_return_bool_false", "Code 5.0f > 5.0f?", "UnitTest_ReturnTypeBool(5.0f, 5.0f);", "false");
        success = success && AddUnitTest("code_return_bool_true", "Code 5.0001f > 5.0f?", "UnitTest_ReturnTypeBool(5.0001f, 5.0f);", "true");
        success = success && AddUnitTest("code_return_string1", "Code get animal type", "UnitTest_ReturnTypeString('spot');", "dog");
        success = success && AddUnitTest("code_return_string2", "Code get animal type", "UnitTest_ReturnTypeString('felix');", "cat");
        success = success && AddUnitTest("code_return_string3", "Code get animal type", "UnitTest_ReturnTypeString('fluffy');", "goldfish");
        success = success && AddUnitTest("code_return_v3f", "Code normalize vector", "UnitTest_ReturnTypeVector3f('1 2 3');", "0.2673 0.5345 0.8018");

        // -- scripted functions with return types --------------------------------------------------------------------
        // -- each of the following, the code will execute a scripted function and reliably retrieve the return value
        success = success && AddUnitTest("script_return_int", "Script multiply by 2", "", "", UnitTest_GetScriptReturnInt, "-10");
        success = success && AddUnitTest("script_return_float", "Script divide by 3.0f", "", "", UnitTest_GetScriptReturnFloat, "5.0000");
        success = success && AddUnitTest("script_return_bool", "Script 5.1f > 5.0f", "", "", UnitTest_GetScriptReturnBool, "true");
        success = success && AddUnitTest("script_return_string", "Script name of goldfish", "", "", UnitTest_GetScriptReturnString, "fluffy");
		success = success && AddUnitTest("script_return_string_exec", "Script name of goldfish", "", "", UnitTest_GetScriptReturnStringExec, "fluffy");
        success = success && AddUnitTest("script_return_v3f", "Script 2D normalized", "", "", UnitTest_GetScriptReturnVector3f, "0.3162 0.0000 0.9487");

        // -- recursive scripted function -----------------------------------------------------------------------------
        success = success && AddUnitTest("script_fib_recur", "Calc the 10th fibonnaci", "UnitTest_ScriptRecursiveFibonacci(10);", "55");
        success = success && AddUnitTest("script_string_recur", "Print the first 9 letters", "UnitTest_ScriptRecursiveString(9);", "abcdefghi");

        // -- object functions  ---------------------------------------------------------------------------------------
        success = success && AddUnitTest("object_base", "Create a CBase object", "UnitTest_CreateBaseObject();", "BaseObject 27.0000");
        success = success && AddUnitTest("object_child", "Create a CChild object", "UnitTest_CreateChildObject();", "ChildObject 19.0000");
        success = success && AddUnitTest("object_testns", "Create a Namespaced object", "UnitTest_CreateTestNSObject();", "TestNSObject 55.3000 198 foobar");
        success = success && AddUnitTest("objexecf", "Call a scripted object method", "", "", UnitTest_CallScriptedMethodExecf, "TestCodeNSObject foobar Moooo");
        success = success && AddUnitTest("objexecmethod","Call a scripted object method optimized","","",UnitTest_CallScriptedMethodHashed,"TestCodeNSObject foobar 67");
        success = success && AddUnitTest("objexecobjarg","Call a scripted object method with an object arg","","",UnitTest_CallScriptedMethodObjectArg,"TestCodeNSObject self found");
        success = success && AddUnitTest("objexecobjaddrarg","Call a scripted object method with an object arg by address","","",UnitTest_CallScriptedMethodObjectAddrArg,"TestCodeNSObject self found");

        // -- array tests
        success = success && AddUnitTest("global_hashtable", "Global hashtable", "UnitTest_GlobalHashtable();", "goodbye hello goodbye 3.1416");
        success = success && AddUnitTest("param_hashtable", "Hashtable passes as a parameter", "UnitTest_ParameterHashtable();", "Chakakah");
        success = success && AddUnitTest("local_hashtable", "Function hashtable local variable", "UnitTest_LocalHashtable();", "white Chakakah");
        success = success && AddUnitTest("member_hashtable", "Object hashtable member variable", "UnitTest_MemberHashtable();", "Bar Chakakah");
        success = success && AddUnitTest("script_int_array", "Scripted global int[15]", "UnitTest_ScriptIntArray();", "17 67");
        success = success && AddUnitTest("script_string_array", "Scripted global string[15]", "UnitTest_ScriptStringArray();", "Hello Goodbye");
        success = success && AddUnitTest("local_int_array", "Scripted local int[15]", "UnitTest_ScriptLocalIntArray();", "21 67");
        success = success && AddUnitTest("local_string_array", "Scripted local string[15]", "UnitTest_ScriptLocalStringArray();", "Foobar Goodbye");
        success = success && AddUnitTest("member_int_array", "Scripted member int[15]", "UnitTest_ScriptMemberIntArray();", "16 67");
        success = success && AddUnitTest("member_string_array", "Scripted member string[15]", "UnitTest_ScriptMemberStringArray();", "Never say Goodbye");
        success = success && AddUnitTest("registered_int_array", "Registered int[15]", "UnitTest_CodeIntArray();", "", UnitTest_RegisteredIntArrayModify, "67 39", true);
        success = success && AddUnitTest("registered_string_array", "Registered string[15]", "UnitTest_CodeStringArray();", "", UnitTest_RegisteredStringArrayModify, "Winter Goodbye", true);
        success = success && AddUnitTest("registered_member_int_array", "Registered int[15]", "UnitTest_CodeMemberIntArray();", "19 67");
        success = success && AddUnitTest("registered_member_string_array", "Registered int[15]", "UnitTest_CodeMemberStringArray();", "Foobar Goodbye");
    }

    // -- return success
    return (success);
}

void BeginUnitTests(bool8 results_only = false, const char* specific_test = NULL)
{
    // -- required to ensure registered functions from memory.cpp are linked, even if the memory tracker is not enabled
    REGISTER_FILE(tinmemory_cpp);

    // -- first create the unit tests
    if (!CreateUnitTests())
        return;

    // -- unit tests are run on the main thread
    TinScript::CScriptContext* script_context = TinScript::GetContext();

    // -- sanity check
    if (!script_context || !CUnitTest::gUnitTests)
        return;

    // -- Execute the unit test script
    results_only ? 0 : MTPrint("\n*** TinScript Unit Tests ***\n");

    results_only ? 0 : MTPrint("\nExecuting unittest.ts\n");
	if (!script_context->ExecScript(kUnitTestScriptName, true, false))
    {
		MTPrint("Error - unable to parse file: %s\n", kUnitTestScriptName);
		return;
	}

    // -- execute
    uint32 fail_test_hash = PerformUnitTests(results_only, specific_test);

    // -- display the results
    MTPrint("\n*** End Unit Tests ***\n");
    if (fail_test_hash == 0)
    {
		MTPrint("Unit tests completed successfully\n");
    }
    else
    {
        const CUnitTest* current_test = CUnitTest::gUnitTests->FindItem(fail_test_hash);
		MTPrint("Unit test failed: %s\n", current_test && current_test->mName ? current_test->mName : "<unnamed>");
        MTPrint("Delete unittest.tso and run from a fresh environment, to ensure no stale compiles\nor pre-defined globals interfere with the tests.\n");
    }
}

void ReloadUnitTests(bool recompile)
{
    TinScript::CScriptContext* script_context = TinScript::GetContext();
    if (recompile)
    {
        MTPrint("Compiling/Executing unittest.ts\n");
    }
    else
    {
        MTPrint("Executing unittest.ts\n");
    }
    if (recompile && !script_context->CompileScript(kUnitTestScriptName))
    {
        MTPrint("Error - unable to compile file: %s\n", kUnitTestScriptName);
        return;
    }
    if (!script_context->ExecScript(kUnitTestScriptName, true, true))
    {
        MTPrint("Error - unable to execute file: %s\n", kUnitTestScriptName);
        return;
    }
}

#if TS_PLATFORM_WINDOWS
DWORD WINAPI MyThreadFunction(LPVOID lpParam)
{
    // -- create a new script context
	auto ThreadPrintf = [](int32 severity, const char* fmt, ...)
	{
		// -- compose the message
		va_list args;
		va_start(args, fmt);
		char msg_buf[512];
		vsprintf_s(msg_buf, 512, fmt, args);
		va_end(args);

		printf(msg_buf);
		return 0;
	};

    TinScript::CScriptContext* thread_context = TinScript::CScriptContext::Create(ThreadPrintf, NULL, false);

    MTPrint("ALT THREAD:  Executing unit tests (results only) in a separate thread with its own context\n");
    BeginUnitTests(true);

    // -- create a thread object
    MTPrint("ALT THREAD:  Creating an AltThreadObject\n");
    CBase* altThreadObject = TinAlloc(ALLOC_Debugger, CBase);

    // -- note:  Calling GetContext() instead of using thread_context, ensures we're using
    // -- the thread local singleton
    uint32 altObjectID = TinScript::GetContext()->RegisterObject(altThreadObject, "CBase", "AltThreadObject");

    // -- execute a function from a common script, executed from separate threads
    // -- in this case, it's a wrapper for "ListObjects()", but since each thread
    // -- created objects, they should show different sets of objects
    MTPrint("ALT THREAD:  Calling ListObjects()\n");
    TinScript::GetContext()->ExecCommand("MultiThreadTestFunction('AltThread');");

    Sleep(300);

    // -- get the value of the script global variable - unique to each thread
    const char* global_script_value = NULL;
    if (!TinScript::GetGlobalVar(TinScript::GetContext(), "gMultiThreadVariable", global_script_value))
    {
        ScriptAssert_(TinScript::GetContext(), 0, "<internal>", -1,
                      "Error - unable to find script global variable: 'gMultiThreadVariable'\n");
    }
    else
    {
        MTPrint("ALT THREAD:  Script global variable 'gMultiThreadVariable' is %s\n", global_script_value);
    }

    // -- Sleep for a second, just to allow the other thread time to execute
    Sleep(1000);

    MTPrint("ALT THREAD:  End of test\n");

    // -- cleanup
    TinScript::GetContext()->DestroyObject(altObjectID);
    TinFree(altThreadObject);

    // -- destroy the thread context
    TinScript::CScriptContext::Destroy();

    return (0);
}

// -- note:  this is called from the main thread
void BeginMultiThreadTest()
{
    MTPrint("*** MULTI THREAD TEST ****\n");
    MTPrint("*  *** A second thread is spawned, and each thread does the following:\n");
    MTPrint("*  1.  Run the unit tests\n");
    MTPrint("*  2.  Create a named object named for the thread\n");
    MTPrint("*  3.  ListObjects() is executed from each thread\n");
    MTPrint("*  4.  A script global (string) variable is set to the name of the thread.\n");
    MTPrint("*  5.  The value of the script global for each thread is printed.\n");
    MTPrint("*  *** The test is successful if:\n");
    MTPrint("*  1.  Unit tests complete successfully on each thread\n");
    MTPrint("*  2.  ListObjects() only shows the object(s) created by that thread.\n");
    MTPrint("*  3.  The printed value of the global is either 'MainThread' or 'AltThread'\n\n");


    MTPrint("MAIN THREAD:  Executing unit tests (results only)\n");
    BeginUnitTests(true);

    // -- create an object on the main thread
    MTPrint("MAIN THREAD:  Creating a MainThreadObject");
    CBase* mainThreadObject = TinAlloc(ALLOC_Debugger, CBase);
    uint32 mtObjectID = TinScript::GetContext()->RegisterObject(mainThreadObject, "CBase", "MainThreadObject");

    // -- create the thread
    DWORD threadID;
    HANDLE threadHandle = CreateThread(
            NULL,                   // default security attributes
            0,                      // use default stack size  
            MyThreadFunction,       // thread function name
            NULL,                   // argument to thread function 
            0,                      // use default creation flags 
            &threadID);             // returns the thread identifier 

    // -- our Main Thread can sleep for a shorter period of time, so it's "listObjects" will get called after
    // -- the alt thread has created an object, but before it shuts down
    Sleep(500);

    // -- as above
    // -- execute a function from a common script, executed from separate threads
    // -- in this case, it's a wrapper for "ListObjects()", but since each thread
    // -- created objects, they should show different sets of objects
    MTPrint("MAIN THREAD:  Calling ListObjects()\n");
    TinScript::GetContext()->ExecCommand("MultiThreadTestFunction('MainThread');");

    Sleep(500);

    // -- get the value of the script global variable - unique to each thread
    const char* global_script_value = NULL;
    if (!TinScript::GetGlobalVar(TinScript::GetContext(), "gMultiThreadVariable", global_script_value))
    {
        ScriptAssert_(TinScript::GetContext(), 0, "<internal>", -1,
                      "Error - unable to find script global variable: 'gMultiThreadVariable'\n");
    }
    else
    {
        MTPrint("MAIN THREAD:  Script global variable 'gMultiThreadVariable' is %s\n", global_script_value);
    }

    Sleep(500);

    MTPrint("MAIN THREAD:  End of test\n");

    // -- cleanup
    TinScript::GetContext()->DestroyObject(mtObjectID);
    TinFree(mainThreadObject);

    Sleep(500);
    MTPrint("*** MULTI THREAD TEST COMPLETE ****\n");
}

#else
void BeginMultiThreadTest()
{
    MTPrint("Multi-threading requires (e.g.) TS_PLATFORM_WINDOWS to be defined\n");
}
#endif // TS_PLATFORM_WINDOWS

REGISTER_FUNCTION(BeginUnitTests, BeginUnitTests);
REGISTER_FUNCTION(ReloadUnitTests, ReloadUnitTests);
REGISTER_FUNCTION(BeginMultiThreadTest, BeginMultiThreadTest);

// -- useful for profiling
void BeginProfilingTests(int loop_count)
{
    TinScript::ExecScript(kProfilingTestScriptName);

    uint32 func_hash = TinScript::Hash("CallFromCode");

    TinPrint(TinScript::GetContext(), "TinScript Start CallMe()\n");

    auto time_start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < loop_count; ++i)
    {
        int32 result = 0;
        //TinScript::ExecF(result, "CallMe();", 56, 24);
        TinScript::ExecFunction(result, func_hash, 56, 24, "cat ");
    }
    auto time_stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::micro> elapsed_mirco = time_stop - time_start;

    TinPrint(TinScript::GetContext(), "TinScript time: %lf\n", elapsed_mirco.count());
}

REGISTER_FUNCTION(BeginProfilingTests, BeginProfilingTests);

// --------------------------------------------

#define VA_LENGTH_(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, N, ...) N

#if 1
  #define MSVC_HACK(FUNC, ARGS) FUNC ARGS
  #define APPLY(FUNC, ...) MSVC_HACK(FUNC, (__VA_ARGS__))
  #define VA_LENGTH(...) APPLY(VA_LENGTH_, 0, ## __VA_ARGS__, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#else
  #define VA_LENGTH(...) VA_LENGTH_(0, ## __VA_ARGS__, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#endif

// -- For this test, since I'm just printing out the types, this cuts down on the repeated spam
template <typename T>
void PrintType(int index)
{
    TinPrint(TinScript::GetContext(), "t%d: %s\n", index, TinScript::GetRegisteredTypeName(TinScript::GetRegisteredType(TinScript::GetTypeID<T>())));
}

// -- macro is simply a wrapper to the templated implementations - it'll find the one with the matching number of args...
#define PRINT_TYPES(...) PrintTypes<__VA_ARGS__>();
template<typename... Types>
struct count {
    static const std::size_t value = sizeof...(Types);
};

#define TYPE_COUNT(...) count<##__VA_ARGS__>::value

// -----------------------------------------------------

int32 TestArg0()
{
    return (8);
}

int32 TestArg1(int32 arg1)
{
    TinPrint(TinScript::GetContext(), "%d\n", arg1 * 2);
    return (arg1 * 2);
}

float TestArg2(float arg1, bool arg2)
{
    return (arg2 ? arg1 : 0.0f);
}

int TestArg3(float arg1, bool arg2, int arg3)
{
    TinPrint(TinScript::GetContext(), "%2f\n", arg2 ? arg1 : (float)arg3);
    return (69);
}

void VoidArg1(float arg1)
{
    TinPrint(TinScript::GetContext(), "%2f\n", arg1);
}

void VoidStr1(const char* in_str)
{
    TinPrint(TinScript::GetContext(), "In String: %s\n", in_str);
}

template<int N, typename S>
class Signature;

template<int N, typename R, typename... Args>
class Signature<N, R(Args...)>
{
    public:
        using return_type = R;
        using argument_types = std::tuple<Args...>;
        //static const int arg_count = sizeof...(Args);

        Signature() { }
        void PrintArgs() { }
};

template<typename R, typename... Args>
class Signature<0, R(Args...)>
{
    public:
        using return_type = R;
        using argument_types = std::tuple<Args...>;

        Signature() { printf("Arg Count: 0\n"); PrintArgs(); }
        void PrintArgs() { }
};

template<typename R, typename... Args>
class Signature<1, R(Args...)>
{
    public:
        using return_type = R;
        using argument_types = std::tuple<Args...>;

        Signature() { printf("Arg Count: 1\n"); PrintArgs(); }
        void PrintArgs()
        {
            using arg1 = std::tuple_element<0, argument_types>::type;
            PrintType<arg1>(1);
        }
};

template<typename R, typename... Args>
class Signature<2, R(Args...)>
{
    public:
        using return_type = R;
        using argument_types = std::tuple<Args...>;

        Signature() { printf("Arg Count: 2\n"); PrintArgs(); }
        void PrintArgs()
        {
            using arg1 = std::tuple_element<0, argument_types>::type;
            using arg2 = std::tuple_element<1, argument_types>::type;
            PrintType<arg1>(1);
            PrintType<arg2>(2);
        }
};

template<typename R, typename... Args>
class Signature<3, R(Args...)>
{
    public:
        using return_type = R;
        using argument_types = std::tuple<Args...>;

        Signature() { printf("Arg Count: 3\n"); PrintArgs(); }
        void PrintArgs()
        {
            using arg1 = std::tuple_element<0, argument_types>::type;
            using arg2 = std::tuple_element<1, argument_types>::type;
            using arg3 = std::tuple_element<2, argument_types>::type;
            PrintType<arg1>(1);
            PrintType<arg2>(2);
            PrintType<arg3>(3);
        }
};

#define PRINT_SIGNATURE(Func) \
{   \
    const int n = TinScript::SignatureArgCount<decltype(Func)>::arg_count; \
    Signature<n, decltype(Func)>(); \
}

void TestB()
{
    PRINT_SIGNATURE(TestArg0);
    PRINT_SIGNATURE(TestArg1);
    PRINT_SIGNATURE(TestArg2);
    PRINT_SIGNATURE(TestArg3);
}

REGISTER_FUNCTION(TestB, TestB);

// -- NEW REGISTRATION ------------------------------------------------------------------------------------------------

REGISTER_FUNCTION(TestArg3, TestArg3)
REGISTER_FUNCTION(VoidArg1, VoidArg1)
REGISTER_FUNCTION(VoidStr1, VoidStr1)

REGISTER_METHOD(CBase, TestP1, TestP1)
REGISTER_METHOD(CBase, VoidP1, VoidP1)

void TestDefaults(float testFName, int testIName, const char* testSName)
{
    TinPrint(TinScript::GetContext(), "### TestDefaults: %.2f, %d, %s\n", testFName, testIName, testSName);
}
REGISTER_FUNCTION(TestDefaults, TestDefaults);
REGISTER_FUNCTION_DEFAULT_ARGS_P3(TestDefaults, "return", "in_float", 67.0f, "in_int", 49, "in_str", "foobar", "This is the help string for my function!");

class TestFoo
{
public:
    void TestDefaults(float testFName, int testIName, const char* testSName)
    {
        TinPrint(TinScript::GetContext(), "### TestDefaults: %.2f, %d, %s\n", testFName, testIName, testSName);
    }
};

REGISTER_SCRIPT_CLASS_BEGIN(TestFoo, VOID)
REGISTER_SCRIPT_CLASS_END()

REGISTER_METHOD(TestFoo, TestDefaults, TestDefaults);
REGISTER_METHOD_DEFAULT_ARGS_P3(TestFoo, TestDefaults, "return", "in_float", 67.0f, "in_int", 49, "in_str", "foobar", "This is the help string for my function!");

void TestCppHashTable(TinScript::CHashtable* ht_param)
{
    if (ht_param == nullptr)
        return;

    TinPrint(TinScript::GetContext(), "### ht_param:\n");
    ht_param->Dump();

    // -- see if we can pull out a string_arg
    const char* string_arg = "";
    if (ht_param->GetValue<const char*>("string_arg", string_arg))
    {
        TinPrint(TinScript::GetContext(), "### ht_param['string_arg']: %s\n", string_arg);
    }
    else
    {
        TinPrint(TinScript::GetContext(), "### ht_param['string_arg'] not found\n");
    }

    // -- see if we can pull out a float_arg
    float float_arg = 0.0f;
    if (ht_param->GetValue<float>("float_arg", float_arg))
    {
        TinPrint(TinScript::GetContext(), "### ht_param['float_arg']: %.2f\n", float_arg);
    }
    else
    {
        TinPrint(TinScript::GetContext(), "### ht_param['float_arg'] not found\n");
    }
    // -- see if we can pull out a vector3f arg
    CVector3f location_arg = 0.0f;
    if (ht_param->GetValue<CVector3f>("vector3f_arg", location_arg))
    {
        TinPrint(TinScript::GetContext(), "### ht_param['vector3f_arg']: (%.2f, %.2f, %.2f)\n", location_arg.x, location_arg.y, location_arg.z);
    }
    else
    {
        TinPrint(TinScript::GetContext(), "### ht_param['vector3f_arg'] not found\n");
    }

    // -- see if we can pull out a vector3f arg
    bool object_found = false;
    CBase* object_arg;
    if (ht_param->GetValue<CBase*>("object_arg", object_arg))
    {
        object_found = true;
        TinPrint(TinScript::GetContext(), "### ht_param['object_arg']: floatvalue %.2f\n", object_arg->GetFloatValue());

        // -- call a method on the object passed via a hashtable:
        int32 param_count = 0;
        if (TinScript::ObjHasMethod(object_arg, TinScript::Hash("TestMethod"), param_count))
        {
            float result = 0.0f;
            TinScript::ObjExecMethod(object_arg, result, TinScript::Hash("TestMethod"));
            TinPrint(TinScript::GetContext(), "### ht_param['object_arg'].TestMethod(): %.2f\n", result);
        }
    }

    if (!object_found)
    {
        TinPrint(TinScript::GetContext(), "### ht_param['vector3f_arg'] not found\n");
    }

    // -- see if we can add a value from C++
    ht_param->AddEntry("float_fromCpp", 6.78f);
    ht_param->AddEntry("string_fromCpp", "dogmaticallaciousness");

}

REGISTER_FUNCTION(TestCppHashTable, TestCppHashTable);


// ------------------------------------------------------------------------------------------------
// eof
// --------------------------------------------------------------------------------------------------------------------
