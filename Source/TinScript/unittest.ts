
// ------------------------------------------------------------------------------------------------
// unittest.cs
// ------------------------------------------------------------------------------------------------

// -- VARIABLES, FLOW -----------------------------------------------------------------------------
int gUnitTestScriptInt = 12;

void UnitTest_RegisteredIntAccess()
{
    // -- This registered int was set in code - retrieving here
    gUnitTestScriptResult = StringCat(gUnitTestRegisteredInt);
}

void UnitTest_RegisteredIntModify()
{
    // -- This registered int was set in code - modifying the value
    gUnitTestRegisteredInt = 23;
}

void UnitTest_CodeAccess()
{
    gUnitTestScriptInt = 49;
}

void UnitTest_CodeModify()
{
    gUnitTestScriptResult = StringCat(gUnitTestScriptInt);
}

void UnitTest_IfStatement(int testvar)
{
    if(testvar > 9)
        gUnitTestScriptResult = StringCat(testvar, " is greater than 9");
    else if(testvar < 9)
        gUnitTestScriptResult = StringCat(testvar, " is less than 9");
    else
        gUnitTestScriptResult = StringCat(testvar, " is equal to 9");
}

void UnitTest_WhileStatement()
{
    int testvar = 5;
    while(testvar > 0) {
        gUnitTestScriptResult = StringCat(gUnitTestScriptResult, " ", testvar);
        testvar = testvar - 1;
    }
}

void UnitTest_ForLoop()
{
    int testvar;
    for(testvar = 0; testvar < 5; testvar += 1)
    {
        gUnitTestScriptResult = StringCat(gUnitTestScriptResult, " ", testvar);
    }
}

void TestParenthesis()
{
    float result = (((3 + 4) * 17) - (3.0f + 6)) % (42 / 3);
    gUnitTestScriptResult = StringCat(result);
}

// -- Registered function return types --------------------------------------------------------------------------------
void UnitTest_ReturnTypeInt(int number)
{
    int result = UnitTest_MultiplyBy2(number);
    gUnitTestScriptResult = StringCat(result);
}

void UnitTest_ReturnTypeFloat(float number)
{
    float result = UnitTest_DivideBy3(number);
    gUnitTestScriptResult = StringCat(result);
}

void UnitTest_ReturnTypeBool(float number0, float number1)
{
    bool result = UnitTest_IsGreaterThan(number0, number1);
    gUnitTestScriptResult = StringCat(result);
}

void UnitTest_ReturnTypeString(string animal_name)
{
    string result = UnitTest_AnimalType(animal_name);
    gUnitTestScriptResult = StringCat(result);
}

void UnitTest_ReturnTypeVector3f(vector3f v0)
{
    vector3f result = UnitTest_V3fNormalize(v0);
    gUnitTestScriptResult = StringCat(result);
}

// -- Scripted function return types ----------------------------------------------------------------------------------
int UnitTest_ScriptReturnInt(int number)
{
    // -- return 2 * number
    return (number << 1);
}

float UnitTest_ScriptReturnFloat(float number)
{
    // -- return (number / 3.0f);
    return (number / 3.0f);
}

bool UnitTest_ScriptReturnBool(float number0, float number1)
{
    // -- return (number0 > number1)
    return (number0 > number1);
}

string UnitTest_ScriptReturnString(string animal_type)
{
    if (!StringCmp(animal_type, "cat"))
        return ("felix");
    else if (!StringCmp(animal_type, "dog"))
        return ("spot");
    else if (!StringCmp(animal_type, "goldfish"))
        return ("fluffy");
    else
        return ("unknown");
}

vector3f UnitTest_ScriptReturnVector3f(vector3f v0)
{
    // -- return the 2D normalized vector
    v0:y = 0.0f;
    float length = V3fLength(v0);
    if (length > 0.0f)
        return (v0 / length);
    return ('0 0 0');
}

// -- Scripted recursive function returns -----------------------------------------------------------------------------
void UnitTest_ScriptRecursiveFibonacci(int nth_number)
{
    // -- this will blow the stack if nth_number is too large
    // -- TinScript does not implement tail-end recursion, or any
    // -- other VM optimizations - the results will be correct, but
    // -- strongly advise against unbounded recursive scripting
    gUnitTestScriptResult = StringCat(Fibonacci(nth_number));
}

string RecursiveAlphabet(string alphabet, int nth_letter)
{
    // -- return the entire alphabet, up to the nth letter
    if (nth_letter < 1 || nth_letter > 27)
        return (alphabet);
        
    if (nth_letter == 1)
        return StringCat('a', alphabet);
    else
    {
        string letter = IntToChar(CharToInt('a') + (nth_letter - 1));
        alphabet = StringCat(letter, alphabet);
        return StringCat(alphabet, RecursiveAlphabet(alphabet, nth_letter - 1));
    }
}

void UnitTest_ScriptRecursiveString(int nth_letter)
{
    gUnitTestScriptResult = RecursiveAlphabet("", nth_letter);
}

int Fibonacci(int num) {
    if (num == 0)
        return (0);
    else if (num <= 2)
        return 1;
    else {
        int prev = Fibonacci(num - 1);
        int prevprev = Fibonacci(num - 2);
        int result = prev + prevprev;
        return result;
    }
}

int FibIterative(int num) {
    if(num < 2) {
        return num;
    }
    int cur = 1;
    int prev = 1;
    
    // -- the first two numbers have already been taken care of
    num = num - 2;
    while (num > 0) {
        int temp = cur;
        cur = cur + prev;
        prev = temp;
        num = num - 1;
    }
    return (cur);
}

// -- Object functions ------------------------------------------------------------------------------------------------

void UnitTest_CreateBaseObject()
{
    // -- retrieving the name demonstrates both access to an object method, and that the
    // -- object actually exists
    object test_obj = create CBase("BaseObject");
    gUnitTestScriptResult = StringCat(test_obj.GetObjectName(), " ", test_obj.GetFloatValue());
    destroy test_obj;
}

void UnitTest_CreateChildObject()
{
    // -- retrieving the name demonstrates the object exists -
    // -- retrieving the same GetFloatValue() defined in the base class demonstrates
    // -- that the hierarchy is correct, and hierarchical methods and members are also correct
    object test_obj = create CChild("ChildObject");
    gUnitTestScriptResult = StringCat(test_obj.GetObjectName(), " ", test_obj.GetFloatValue());
    destroy test_obj;
}

void TestNSObject::OnCreate()
{
    // -- demonstrate access to a parent member
    self.floatvalue = 55.3f;
    
    // -- demonstrate access to a parent method - note, CChild implements
    // -- and registers it's own version, multiplying the value by 2
    self.SetIntValue(99);
    
    // -- declare a scripted member for the object
    string self.testmember = "foobar";
}

void UnitTest_CreateTestNSObject()
{
    // -- The ::OnCreate() demonstrates access to the parent class, as well as defining a new member
    object test_obj = create CChild("TestNSObject");
    gUnitTestScriptResult = StringCat(test_obj.GetObjectName(), " ", test_obj.GetFloatValue(), " ",
                                      test_obj.GetIntValue(), " ", test_obj.testmember);
    destroy test_obj;
}

// -- creating an object from code, registering it, and linking it to the TestNSObject namespace
void TestCodeNSObject::OnCreate() : TestNSObject
{
}

// -- now create a namespaced method to be executed from code
string TestCodeNSObject::ModifyTestMemberString(string append_value)
{
    // -- append to our test member
    self.testmember = StringCat(self.GetObjectName(), " ", self.testmember, " ", append_value);
    return (self.testmember);
}

// -- now create a namespaced method to be executed from code
string TestCodeNSObject::ModifyTestMemberInt(int append_value)
{
    // -- append to our test member
    self.testmember = StringCat(self.GetObjectName(), " ", self.testmember, " ", append_value);
    return (self.testmember);
}

// -- Array tests -----------------------------------------------------------------------------

hashtable gHashTable;
string gHashTable["hello"] = "goodbye";
string gHashTable["goodbye"] = "hello";
float gHashTable["hello", "goodbye"] = 3.1416f;
void UnitTest_GlobalHashtable()
{
    gUnitTestScriptResult = StringCat(gHashTable["hello"]);
    gUnitTestScriptResult = StringCat(gUnitTestScriptResult, " ", gHashTable["goodbye"]);
    gUnitTestScriptResult = StringCat(gUnitTestScriptResult, " ", gHashTable[gHashTable["goodbye"]]);
    gUnitTestScriptResult = StringCat(gUnitTestScriptResult, " ", gHashTable[gHashTable["goodbye"], gHashTable["hello"]]);
}

void TestParameterHashtable(hashtable test_hashtable)
{
    string test_hashtable["ParamTest"] = "Chakakah";
}

void UnitTest_ParameterHashtable()
{
    TestParameterHashtable(gHashTable);
    gUnitTestScriptResult = StringCat(gHashTable["ParamTest"]);
}

void UnitTest_LocalHashtable()
{
    hashtable local_hashtable;
    string local_hashtable["black"] = "white";
    TestParameterHashtable(local_hashtable);
    gUnitTestScriptResult = StringCat(local_hashtable["black"], " ", local_hashtable["ParamTest"]);
}

void ObjHashMember::OnCreate()
{
    hashtable self.memberHashtable;
}

void UnitTest_MemberHashtable()
{
    object test_obj = create CScriptObject("ObjHashMember");
    string test_obj.memberHashtable["Foo"] = "Bar";
    TestParameterHashtable(test_obj.memberHashtable);
    gUnitTestScriptResult = StringCat(test_obj.memberHashtable["Foo"], " ", test_obj.memberHashtable["ParamTest"]);
    destroy test_obj;
}

void TestParameterIntArray(int[] int_array)
{
    int_array[3] = 67;
}

int[15] gScriptIntArray;
void UnitTest_ScriptIntArray()
{
    gScriptIntArray[5] = 17;
    TestParameterIntArray(gScriptIntArray);
    gUnitTestScriptResult = StringCat(gScriptIntArray[5], " ", gScriptIntArray[3]);
}

void TestParameterStringArray(string[] string_array)
{
    string_array[9] = "Goodbye";
}

string[15] gScriptStringArray;
void UnitTest_ScriptStringArray()
{
    gScriptStringArray[12] = "Hello";
    TestParameterStringArray(gScriptStringArray);
    gUnitTestScriptResult = StringCat(gScriptStringArray[12], " ", gScriptStringArray[9]);
}

void UnitTest_ScriptLocalIntArray()
{
    int[15] local_array;
    local_array[7] = 21;
    TestParameterIntArray(local_array);
    gUnitTestScriptResult = StringCat(local_array[7], " ", local_array[3]);
}

void UnitTest_ScriptLocalStringArray()
{
    string[15] local_array;
    local_array[7] = "Foobar";
    TestParameterStringArray(local_array);
    gUnitTestScriptResult = StringCat(local_array[7], " ", local_array[9]);
}

void ObjArrayMember::OnCreate()
{
    int[15] self.int_array_mem;
    string[15] self.string_array_mem;
}

void UnitTest_ScriptMemberIntArray()
{
    object test_obj = create CScriptObject("ObjArrayMember");
    test_obj.int_array_mem[4] = 16;
    TestParameterIntArray(test_obj.int_array_mem);
    gUnitTestScriptResult = StringCat(test_obj.int_array_mem[4], " ", test_obj.int_array_mem[3]);
    destroy test_obj;
}

void UnitTest_ScriptMemberStringArray()
{
    object test_obj = create CScriptObject("ObjArrayMember");
    test_obj.string_array_mem[4] = "Never say";
    TestParameterStringArray(test_obj.string_array_mem);
    gUnitTestScriptResult = StringCat(test_obj.string_array_mem[4], " ", test_obj.string_array_mem[9]);
    destroy test_obj;
}

void UnitTest_CodeIntArray()
{
    gUnitTestIntArray[5] = 39;
    TestParameterIntArray(gUnitTestIntArray);
}

void UnitTest_CodeStringArray()
{
    gUnitTestStringArray[4] = "Winter";
    TestParameterStringArray(gUnitTestStringArray);
}

void UnitTest_CodeMemberIntArray()
{
    object test_obj = create CBase('testBase2');
    test_obj.intArray[2] = 19;
    TestParameterIntArray(test_obj.intArray);
    gUnitTestScriptResult = StringCat(test_obj.intArray[2], " ", test_obj.intArray[3]);
    destroy test_obj;
}

void UnitTest_CodeMemberStringArray()
{
    object test_obj = create CBase('testBase3');
    test_obj.stringArray[8] = "Foobar";
    TestParameterStringArray(test_obj.stringArray);
    gUnitTestScriptResult = StringCat(test_obj.stringArray[8], " ", test_obj.stringArray[9]);
    destroy test_obj;
}

// -- MultiThread test -----------------------------------------------------------------------------
// -- declaring a global script variable - this must be unique to each thead
string gMultiThreadVariable = "<uninitialized>";
void MultiThreadTestFunction(string value)
{
    gMultiThreadVariable = value;
	ListObjects();
}

// -- DEBUGGER Test Functions ----------------------------------------------------------------------
int MultBy3(int value) {
    int test_watch_var = 17;
    int result = value * 3;
    return (result);
}

int MultBy6(int value) {
    int test_watch_var2 = 29;
    int result = MultBy3(value) * 2;
    return (result);
}

int MultBy24(int value) {
    int test_watch_var3 = 5;
    int result = MultBy6(value) * 4;
    return (result);
}

// ------------------------------------------------------------------------------------------------
// eof
// ------------------------------------------------------------------------------------------------
