// ------------------------------------------------------------------------------------------------
//  The MIT License
//  
//  Copyright (c) 2016 Tim Andersen
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
// Generated classes using variadic templates, for function registration
// ------------------------------------------------------------------------------------------------

#define REGISTER_FUNCTION(name, funcptr) \
    static const int gArgCount_##name = ::TinScript::SignatureArgCount<decltype(funcptr)>::arg_count; \
    static ::TinScript::CRegisterFunction<gArgCount_##name, decltype(funcptr)> gReg_##name(#name, funcptr);

#define REGISTER_METHOD(classname, name, methodptr) \
    static const int gArgCount_##classname##_##name = ::TinScript::SignatureArgCount<decltype(std::declval<classname>().methodptr)>::arg_count; \
    static ::TinScript::CRegisterMethod<gArgCount_##classname##_##name, classname, decltype(std::declval<classname>().methodptr)> gReg_##classname##_##name(#name, &classname::methodptr);

#define REGISTER_CLASS_FUNCTION(classname, name, methodptr) \
    static const int gArgCount_##classname##_##name = ::TinScript::SignatureArgCount<decltype(std::declval<classname>().methodptr)>::arg_count; \
    static ::TinScript::CRegisterFunction<gArgCount_##classname##_##name, decltype(std::declval<classname>().methodptr)> gReg_##classname##_##name(#name, &classname::methodptr); \

template<typename S>
class SignatureArgCount;

template<typename R, typename... Args>
class SignatureArgCount<R(Args...)>
{
    public:
        static const int arg_count = sizeof...(Args);
};

template<int N, typename S>
class CRegisterFunction;

template<int N, typename R, typename... Args>
class CRegisterFunction<N, R(Args...)>
{
    public:
        using argument_types = std::tuple<Args...>;

        CRegisterFunction() { }
        void PrintArgs() { }
};
template<int N, typename C, typename S>
class CRegisterMethod;

template<int N, typename C, typename R, typename... Args>
class CRegisterMethod<N, C, R(Args...)>
{
    public:
        CRegisterMethod() { }
};

// -------------------
// Parameter count: 0
// -------------------

// -- class CRegisterFunction<0, R(Args...)> ----------------------------------------

template<typename R, typename... Args>
class CRegisterFunction<0, R(Args...)> : public CRegFunctionBase
{
public:

    typedef R (*funcsignature)(Args...);
    using argument_types = std::tuple<Args...>;

    // -- constructor
    CRegisterFunction(const char* _funcname, funcsignature _funcptr)
        : CRegFunctionBase(_funcname)
    {
        funcptr = _funcptr;
    }
    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*)
    {
        Dispatch();
    }

    // -- dispatch method
    R Dispatch()
    {
        R r = funcptr();

        assert(GetContext()->GetParameter(0));
        CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual void Register(CScriptContext* script_context)
    {
        CFunctionEntry* fe = new CFunctionEntry(script_context, 0, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), GetRegisteredType(GetTypeID<R>()), 1, GetTypeID<R>());

        uint32 hash = fe->GetHash();
        tFuncTable* globalfunctable = script_context->FindNamespace(0)->GetFuncTable();
        globalfunctable->AddItem(*fe, hash);
    }

private:
    funcsignature funcptr;
};

// -- class CRegisterFunction<0, void(Args...)> ----------------------------------------

template<typename... Args>
class CRegisterFunction<0, void(Args...)> : public CRegFunctionBase
{
public:

    typedef void (*funcsignature)(Args...);
    using argument_types = std::tuple<Args...>;

    // -- constructor
    CRegisterFunction(const char* _funcname, funcsignature _funcptr)
        : CRegFunctionBase(_funcname)
    {
        funcptr = _funcptr;
    }
    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*)
    {
        Dispatch();
    }

    // -- dispatch method
    void Dispatch()
    {
        funcptr();

    }

    // -- registration method
    virtual void Register(CScriptContext* script_context)
    {
        CFunctionEntry* fe = new CFunctionEntry(script_context, 0, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), TYPE_void, 1, 0);
        uint32 hash = fe->GetHash();
        tFuncTable* globalfunctable = script_context->FindNamespace(0)->GetFuncTable();
        globalfunctable->AddItem(*fe, hash);
    }

private:
    funcsignature funcptr;
};

// -- class CRegisterMethod<0, R(Args...)> ----------------------------------------

template<typename C, typename R, typename... Args>
class CRegisterMethod<0, C, R(Args...)> : public CRegFunctionBase
{
public:

    using argument_types = std::tuple<Args...>;
    typedef R (C::*methodsignature)(Args...);

    // -- CRegisterMethod
    CRegisterMethod(const char* _methodname, methodsignature _methodptr) :
                  CRegFunctionBase(_methodname) {
        methodptr = _methodptr;
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr)
    {

        Dispatch(objaddr);
    }

    // -- dispatch method
    R Dispatch(void* objaddr)
    {
        C* object = (C*)(objaddr);
        R r = (object->*methodptr)();

        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = this->GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual void Register(TinScript::CScriptContext* script_context)
    {
        uint32 classname_hash = Hash(C::_GetClassName());
        CFunctionEntry* fe = new CFunctionEntry(script_context, classname_hash, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), GetRegisteredType(GetTypeID<R>()), 1, GetTypeID<R>());
        uint32 hash = fe->GetHash();
        tFuncTable* methodtable = script_context->FindNamespace(classname_hash)->GetFuncTable();
        methodtable->AddItem(*fe, hash);
    }

private:
    methodsignature methodptr;
};
// -- class CRegisterMethod<0, void(Args...)> ----------------------------------------

template<typename C, typename... Args>
class CRegisterMethod<0, C, void(Args...)> : public CRegFunctionBase
{
public:

    using argument_types = std::tuple<Args...>;
    typedef void (C::*methodsignature)(Args...);

    // -- CRegisterMethod
    CRegisterMethod(const char* _methodname, methodsignature _methodptr) :
                  CRegFunctionBase(_methodname) {
        methodptr = _methodptr;
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr)
    {

        Dispatch(objaddr);
    }

    // -- dispatch method
    void Dispatch(void* objaddr)
    {

        C* object = (C*)(objaddr);
        (object->*methodptr)();

    }

    // -- registration method
    virtual void Register(TinScript::CScriptContext* script_context)
    {

        uint32 classname_hash = Hash(C::_GetClassName());
        CFunctionEntry* fe = new CFunctionEntry(script_context, classname_hash, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), TYPE_void, 1, 0);

        uint32 hash = fe->GetHash();
        tFuncTable* methodtable = script_context->FindNamespace(classname_hash)->GetFuncTable();
        methodtable->AddItem(*fe, hash);
    }

private:
    methodsignature methodptr;
};

// -------------------
// Parameter count: 1
// -------------------

// -- class CRegisterFunction<1, R(Args...)> ----------------------------------------

template<typename R, typename... Args>
class CRegisterFunction<1, R(Args...)> : public CRegFunctionBase
{
public:

    typedef R (*funcsignature)(Args...);
    using argument_types = std::tuple<Args...>;

    // -- constructor
    CRegisterFunction(const char* _funcname, funcsignature _funcptr)
        : CRegFunctionBase(_funcname)
    {
        funcptr = _funcptr;
    }
    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*)
    {
        using T1 = std::tuple_element<0, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);

        Dispatch(&p1);
    }

    // -- dispatch method
    R Dispatch(void* _p1)
    {
        using T1 = std::tuple_element<0, argument_types>::type;

        T1* p1 = (T1*)_p1;

        R r = funcptr(*p1);

        assert(GetContext()->GetParameter(0));
        CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual void Register(CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;

        CFunctionEntry* fe = new CFunctionEntry(script_context, 0, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), GetRegisteredType(GetTypeID<R>()), 1, GetTypeID<R>());
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());

        uint32 hash = fe->GetHash();
        tFuncTable* globalfunctable = script_context->FindNamespace(0)->GetFuncTable();
        globalfunctable->AddItem(*fe, hash);
    }

private:
    funcsignature funcptr;
};

// -- class CRegisterFunction<1, void(Args...)> ----------------------------------------

template<typename... Args>
class CRegisterFunction<1, void(Args...)> : public CRegFunctionBase
{
public:

    typedef void (*funcsignature)(Args...);
    using argument_types = std::tuple<Args...>;

    // -- constructor
    CRegisterFunction(const char* _funcname, funcsignature _funcptr)
        : CRegFunctionBase(_funcname)
    {
        funcptr = _funcptr;
    }
    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*)
    {
        using T1 = std::tuple_element<0, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);

        Dispatch(&p1);
    }

    // -- dispatch method
    void Dispatch(void* _p1)
    {
        using T1 = std::tuple_element<0, argument_types>::type;

        T1* p1 = (T1*)_p1;

        funcptr(*p1);

    }

    // -- registration method
    virtual void Register(CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;

        CFunctionEntry* fe = new CFunctionEntry(script_context, 0, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), TYPE_void, 1, 0);
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());

        uint32 hash = fe->GetHash();
        tFuncTable* globalfunctable = script_context->FindNamespace(0)->GetFuncTable();
        globalfunctable->AddItem(*fe, hash);
    }

private:
    funcsignature funcptr;
};

// -- class CRegisterMethod<1, R(Args...)> ----------------------------------------

template<typename C, typename R, typename... Args>
class CRegisterMethod<1, C, R(Args...)> : public CRegFunctionBase
{
public:

    using argument_types = std::tuple<Args...>;
    typedef R (C::*methodsignature)(Args...);

    // -- CRegisterMethod
    CRegisterMethod(const char* _methodname, methodsignature _methodptr) :
                  CRegFunctionBase(_methodname) {
        methodptr = _methodptr;
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr)
    {

        using T1 = std::tuple_element<0, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        Dispatch(objaddr, &p1);
    }

    // -- dispatch method
    R Dispatch(void* objaddr, void* _p1)
    {
        using T1 = std::tuple_element<0, argument_types>::type;

        T1* p1 = (T1*)_p1;

        C* object = (C*)(objaddr);
        R r = (object->*methodptr)(*p1);

        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = this->GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual void Register(TinScript::CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;

        uint32 classname_hash = Hash(C::_GetClassName());
        CFunctionEntry* fe = new CFunctionEntry(script_context, classname_hash, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), GetRegisteredType(GetTypeID<R>()), 1, GetTypeID<R>());
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());

        uint32 hash = fe->GetHash();
        tFuncTable* methodtable = script_context->FindNamespace(classname_hash)->GetFuncTable();
        methodtable->AddItem(*fe, hash);
    }

private:
    methodsignature methodptr;
};
// -- class CRegisterMethod<1, void(Args...)> ----------------------------------------

template<typename C, typename... Args>
class CRegisterMethod<1, C, void(Args...)> : public CRegFunctionBase
{
public:

    using argument_types = std::tuple<Args...>;
    typedef void (C::*methodsignature)(Args...);

    // -- CRegisterMethod
    CRegisterMethod(const char* _methodname, methodsignature _methodptr) :
                  CRegFunctionBase(_methodname) {
        methodptr = _methodptr;
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr)
    {

        using T1 = std::tuple_element<0, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);

        Dispatch(objaddr, &p1);
    }

    // -- dispatch method
    void Dispatch(void* objaddr, void* _p1)
    {
        using T1 = std::tuple_element<0, argument_types>::type;

        T1* p1 = (T1*)_p1;

        C* object = (C*)(objaddr);
        (object->*methodptr)(*p1);

    }

    // -- registration method
    virtual void Register(TinScript::CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;


        uint32 classname_hash = Hash(C::_GetClassName());
        CFunctionEntry* fe = new CFunctionEntry(script_context, classname_hash, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), TYPE_void, 1, 0);
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());


        uint32 hash = fe->GetHash();
        tFuncTable* methodtable = script_context->FindNamespace(classname_hash)->GetFuncTable();
        methodtable->AddItem(*fe, hash);
    }

private:
    methodsignature methodptr;
};

// -------------------
// Parameter count: 2
// -------------------

// -- class CRegisterFunction<2, R(Args...)> ----------------------------------------

template<typename R, typename... Args>
class CRegisterFunction<2, R(Args...)> : public CRegFunctionBase
{
public:

    typedef R (*funcsignature)(Args...);
    using argument_types = std::tuple<Args...>;

    // -- constructor
    CRegisterFunction(const char* _funcname, funcsignature _funcptr)
        : CRegFunctionBase(_funcname)
    {
        funcptr = _funcptr;
    }
    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);

        Dispatch(&p1, &p2);
    }

    // -- dispatch method
    R Dispatch(void* _p1, void* _p2)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;

        R r = funcptr(*p1, *p2);

        assert(GetContext()->GetParameter(0));
        CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual void Register(CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;

        CFunctionEntry* fe = new CFunctionEntry(script_context, 0, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), GetRegisteredType(GetTypeID<R>()), 1, GetTypeID<R>());
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());

        uint32 hash = fe->GetHash();
        tFuncTable* globalfunctable = script_context->FindNamespace(0)->GetFuncTable();
        globalfunctable->AddItem(*fe, hash);
    }

private:
    funcsignature funcptr;
};

// -- class CRegisterFunction<2, void(Args...)> ----------------------------------------

template<typename... Args>
class CRegisterFunction<2, void(Args...)> : public CRegFunctionBase
{
public:

    typedef void (*funcsignature)(Args...);
    using argument_types = std::tuple<Args...>;

    // -- constructor
    CRegisterFunction(const char* _funcname, funcsignature _funcptr)
        : CRegFunctionBase(_funcname)
    {
        funcptr = _funcptr;
    }
    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);

        Dispatch(&p1, &p2);
    }

    // -- dispatch method
    void Dispatch(void* _p1, void* _p2)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;

        funcptr(*p1, *p2);

    }

    // -- registration method
    virtual void Register(CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;

        CFunctionEntry* fe = new CFunctionEntry(script_context, 0, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), TYPE_void, 1, 0);
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());

        uint32 hash = fe->GetHash();
        tFuncTable* globalfunctable = script_context->FindNamespace(0)->GetFuncTable();
        globalfunctable->AddItem(*fe, hash);
    }

private:
    funcsignature funcptr;
};

// -- class CRegisterMethod<2, R(Args...)> ----------------------------------------

template<typename C, typename R, typename... Args>
class CRegisterMethod<2, C, R(Args...)> : public CRegFunctionBase
{
public:

    using argument_types = std::tuple<Args...>;
    typedef R (C::*methodsignature)(Args...);

    // -- CRegisterMethod
    CRegisterMethod(const char* _methodname, methodsignature _methodptr) :
                  CRegFunctionBase(_methodname) {
        methodptr = _methodptr;
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr)
    {

        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        Dispatch(objaddr, &p1, &p2);
    }

    // -- dispatch method
    R Dispatch(void* objaddr, void* _p1, void* _p2)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;

        C* object = (C*)(objaddr);
        R r = (object->*methodptr)(*p1, *p2);

        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = this->GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual void Register(TinScript::CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;

        uint32 classname_hash = Hash(C::_GetClassName());
        CFunctionEntry* fe = new CFunctionEntry(script_context, classname_hash, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), GetRegisteredType(GetTypeID<R>()), 1, GetTypeID<R>());
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());

        uint32 hash = fe->GetHash();
        tFuncTable* methodtable = script_context->FindNamespace(classname_hash)->GetFuncTable();
        methodtable->AddItem(*fe, hash);
    }

private:
    methodsignature methodptr;
};
// -- class CRegisterMethod<2, void(Args...)> ----------------------------------------

template<typename C, typename... Args>
class CRegisterMethod<2, C, void(Args...)> : public CRegFunctionBase
{
public:

    using argument_types = std::tuple<Args...>;
    typedef void (C::*methodsignature)(Args...);

    // -- CRegisterMethod
    CRegisterMethod(const char* _methodname, methodsignature _methodptr) :
                  CRegFunctionBase(_methodname) {
        methodptr = _methodptr;
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr)
    {

        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);

        Dispatch(objaddr, &p1, &p2);
    }

    // -- dispatch method
    void Dispatch(void* objaddr, void* _p1, void* _p2)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;

        C* object = (C*)(objaddr);
        (object->*methodptr)(*p1, *p2);

    }

    // -- registration method
    virtual void Register(TinScript::CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;


        uint32 classname_hash = Hash(C::_GetClassName());
        CFunctionEntry* fe = new CFunctionEntry(script_context, classname_hash, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), TYPE_void, 1, 0);
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());


        uint32 hash = fe->GetHash();
        tFuncTable* methodtable = script_context->FindNamespace(classname_hash)->GetFuncTable();
        methodtable->AddItem(*fe, hash);
    }

private:
    methodsignature methodptr;
};

// -------------------
// Parameter count: 3
// -------------------

// -- class CRegisterFunction<3, R(Args...)> ----------------------------------------

template<typename R, typename... Args>
class CRegisterFunction<3, R(Args...)> : public CRegFunctionBase
{
public:

    typedef R (*funcsignature)(Args...);
    using argument_types = std::tuple<Args...>;

    // -- constructor
    CRegisterFunction(const char* _funcname, funcsignature _funcptr)
        : CRegFunctionBase(_funcname)
    {
        funcptr = _funcptr;
    }
    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);

        Dispatch(&p1, &p2, &p3);
    }

    // -- dispatch method
    R Dispatch(void* _p1, void* _p2, void* _p3)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;

        R r = funcptr(*p1, *p2, *p3);

        assert(GetContext()->GetParameter(0));
        CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual void Register(CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;

        CFunctionEntry* fe = new CFunctionEntry(script_context, 0, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), GetRegisteredType(GetTypeID<R>()), 1, GetTypeID<R>());
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());

        uint32 hash = fe->GetHash();
        tFuncTable* globalfunctable = script_context->FindNamespace(0)->GetFuncTable();
        globalfunctable->AddItem(*fe, hash);
    }

private:
    funcsignature funcptr;
};

// -- class CRegisterFunction<3, void(Args...)> ----------------------------------------

template<typename... Args>
class CRegisterFunction<3, void(Args...)> : public CRegFunctionBase
{
public:

    typedef void (*funcsignature)(Args...);
    using argument_types = std::tuple<Args...>;

    // -- constructor
    CRegisterFunction(const char* _funcname, funcsignature _funcptr)
        : CRegFunctionBase(_funcname)
    {
        funcptr = _funcptr;
    }
    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);

        Dispatch(&p1, &p2, &p3);
    }

    // -- dispatch method
    void Dispatch(void* _p1, void* _p2, void* _p3)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;

        funcptr(*p1, *p2, *p3);

    }

    // -- registration method
    virtual void Register(CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;

        CFunctionEntry* fe = new CFunctionEntry(script_context, 0, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), TYPE_void, 1, 0);
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());

        uint32 hash = fe->GetHash();
        tFuncTable* globalfunctable = script_context->FindNamespace(0)->GetFuncTable();
        globalfunctable->AddItem(*fe, hash);
    }

private:
    funcsignature funcptr;
};

// -- class CRegisterMethod<3, R(Args...)> ----------------------------------------

template<typename C, typename R, typename... Args>
class CRegisterMethod<3, C, R(Args...)> : public CRegFunctionBase
{
public:

    using argument_types = std::tuple<Args...>;
    typedef R (C::*methodsignature)(Args...);

    // -- CRegisterMethod
    CRegisterMethod(const char* _methodname, methodsignature _methodptr) :
                  CRegFunctionBase(_methodname) {
        methodptr = _methodptr;
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr)
    {

        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);
        Dispatch(objaddr, &p1, &p2, &p3);
    }

    // -- dispatch method
    R Dispatch(void* objaddr, void* _p1, void* _p2, void* _p3)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;

        C* object = (C*)(objaddr);
        R r = (object->*methodptr)(*p1, *p2, *p3);

        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = this->GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual void Register(TinScript::CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;

        uint32 classname_hash = Hash(C::_GetClassName());
        CFunctionEntry* fe = new CFunctionEntry(script_context, classname_hash, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), GetRegisteredType(GetTypeID<R>()), 1, GetTypeID<R>());
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());

        uint32 hash = fe->GetHash();
        tFuncTable* methodtable = script_context->FindNamespace(classname_hash)->GetFuncTable();
        methodtable->AddItem(*fe, hash);
    }

private:
    methodsignature methodptr;
};
// -- class CRegisterMethod<3, void(Args...)> ----------------------------------------

template<typename C, typename... Args>
class CRegisterMethod<3, C, void(Args...)> : public CRegFunctionBase
{
public:

    using argument_types = std::tuple<Args...>;
    typedef void (C::*methodsignature)(Args...);

    // -- CRegisterMethod
    CRegisterMethod(const char* _methodname, methodsignature _methodptr) :
                  CRegFunctionBase(_methodname) {
        methodptr = _methodptr;
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr)
    {

        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);

        Dispatch(objaddr, &p1, &p2, &p3);
    }

    // -- dispatch method
    void Dispatch(void* objaddr, void* _p1, void* _p2, void* _p3)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;

        C* object = (C*)(objaddr);
        (object->*methodptr)(*p1, *p2, *p3);

    }

    // -- registration method
    virtual void Register(TinScript::CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;


        uint32 classname_hash = Hash(C::_GetClassName());
        CFunctionEntry* fe = new CFunctionEntry(script_context, classname_hash, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), TYPE_void, 1, 0);
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());


        uint32 hash = fe->GetHash();
        tFuncTable* methodtable = script_context->FindNamespace(classname_hash)->GetFuncTable();
        methodtable->AddItem(*fe, hash);
    }

private:
    methodsignature methodptr;
};

// -------------------
// Parameter count: 4
// -------------------

// -- class CRegisterFunction<4, R(Args...)> ----------------------------------------

template<typename R, typename... Args>
class CRegisterFunction<4, R(Args...)> : public CRegFunctionBase
{
public:

    typedef R (*funcsignature)(Args...);
    using argument_types = std::tuple<Args...>;

    // -- constructor
    CRegisterFunction(const char* _funcname, funcsignature _funcptr)
        : CRegFunctionBase(_funcname)
    {
        funcptr = _funcptr;
    }
    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);
        CVariableEntry* ve4 = GetContext()->GetParameter(4);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);
        T4 p4 = ConvertVariableForDispatch<T4>(ve4);

        Dispatch(&p1, &p2, &p3, &p4);
    }

    // -- dispatch method
    R Dispatch(void* _p1, void* _p2, void* _p3, void* _p4)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;
        T4* p4 = (T4*)_p4;

        R r = funcptr(*p1, *p2, *p3, *p4);

        assert(GetContext()->GetParameter(0));
        CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual void Register(CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;

        CFunctionEntry* fe = new CFunctionEntry(script_context, 0, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), GetRegisteredType(GetTypeID<R>()), 1, GetTypeID<R>());
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());
        GetContext()->AddParameter("_p4", Hash("_p4"), GetRegisteredType(GetTypeID<T4>()), 1, GetTypeID<T4>());

        uint32 hash = fe->GetHash();
        tFuncTable* globalfunctable = script_context->FindNamespace(0)->GetFuncTable();
        globalfunctable->AddItem(*fe, hash);
    }

private:
    funcsignature funcptr;
};

// -- class CRegisterFunction<4, void(Args...)> ----------------------------------------

template<typename... Args>
class CRegisterFunction<4, void(Args...)> : public CRegFunctionBase
{
public:

    typedef void (*funcsignature)(Args...);
    using argument_types = std::tuple<Args...>;

    // -- constructor
    CRegisterFunction(const char* _funcname, funcsignature _funcptr)
        : CRegFunctionBase(_funcname)
    {
        funcptr = _funcptr;
    }
    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);
        CVariableEntry* ve4 = GetContext()->GetParameter(4);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);
        T4 p4 = ConvertVariableForDispatch<T4>(ve4);

        Dispatch(&p1, &p2, &p3, &p4);
    }

    // -- dispatch method
    void Dispatch(void* _p1, void* _p2, void* _p3, void* _p4)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;
        T4* p4 = (T4*)_p4;

        funcptr(*p1, *p2, *p3, *p4);

    }

    // -- registration method
    virtual void Register(CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;

        CFunctionEntry* fe = new CFunctionEntry(script_context, 0, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), TYPE_void, 1, 0);
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());
        GetContext()->AddParameter("_p4", Hash("_p4"), GetRegisteredType(GetTypeID<T4>()), 1, GetTypeID<T4>());

        uint32 hash = fe->GetHash();
        tFuncTable* globalfunctable = script_context->FindNamespace(0)->GetFuncTable();
        globalfunctable->AddItem(*fe, hash);
    }

private:
    funcsignature funcptr;
};

// -- class CRegisterMethod<4, R(Args...)> ----------------------------------------

template<typename C, typename R, typename... Args>
class CRegisterMethod<4, C, R(Args...)> : public CRegFunctionBase
{
public:

    using argument_types = std::tuple<Args...>;
    typedef R (C::*methodsignature)(Args...);

    // -- CRegisterMethod
    CRegisterMethod(const char* _methodname, methodsignature _methodptr) :
                  CRegFunctionBase(_methodname) {
        methodptr = _methodptr;
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr)
    {

        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);
        CVariableEntry* ve4 = GetContext()->GetParameter(4);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);
        T4 p4 = ConvertVariableForDispatch<T4>(ve4);
        Dispatch(objaddr, &p1, &p2, &p3, &p4);
    }

    // -- dispatch method
    R Dispatch(void* objaddr, void* _p1, void* _p2, void* _p3, void* _p4)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;
        T4* p4 = (T4*)_p4;

        C* object = (C*)(objaddr);
        R r = (object->*methodptr)(*p1, *p2, *p3, *p4);

        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = this->GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual void Register(TinScript::CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;

        uint32 classname_hash = Hash(C::_GetClassName());
        CFunctionEntry* fe = new CFunctionEntry(script_context, classname_hash, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), GetRegisteredType(GetTypeID<R>()), 1, GetTypeID<R>());
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());
        GetContext()->AddParameter("_p4", Hash("_p4"), GetRegisteredType(GetTypeID<T4>()), 1, GetTypeID<T4>());

        uint32 hash = fe->GetHash();
        tFuncTable* methodtable = script_context->FindNamespace(classname_hash)->GetFuncTable();
        methodtable->AddItem(*fe, hash);
    }

private:
    methodsignature methodptr;
};
// -- class CRegisterMethod<4, void(Args...)> ----------------------------------------

template<typename C, typename... Args>
class CRegisterMethod<4, C, void(Args...)> : public CRegFunctionBase
{
public:

    using argument_types = std::tuple<Args...>;
    typedef void (C::*methodsignature)(Args...);

    // -- CRegisterMethod
    CRegisterMethod(const char* _methodname, methodsignature _methodptr) :
                  CRegFunctionBase(_methodname) {
        methodptr = _methodptr;
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr)
    {

        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);
        CVariableEntry* ve4 = GetContext()->GetParameter(4);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);
        T4 p4 = ConvertVariableForDispatch<T4>(ve4);

        Dispatch(objaddr, &p1, &p2, &p3, &p4);
    }

    // -- dispatch method
    void Dispatch(void* objaddr, void* _p1, void* _p2, void* _p3, void* _p4)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;
        T4* p4 = (T4*)_p4;

        C* object = (C*)(objaddr);
        (object->*methodptr)(*p1, *p2, *p3, *p4);

    }

    // -- registration method
    virtual void Register(TinScript::CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;


        uint32 classname_hash = Hash(C::_GetClassName());
        CFunctionEntry* fe = new CFunctionEntry(script_context, classname_hash, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), TYPE_void, 1, 0);
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());
        GetContext()->AddParameter("_p4", Hash("_p4"), GetRegisteredType(GetTypeID<T4>()), 1, GetTypeID<T4>());


        uint32 hash = fe->GetHash();
        tFuncTable* methodtable = script_context->FindNamespace(classname_hash)->GetFuncTable();
        methodtable->AddItem(*fe, hash);
    }

private:
    methodsignature methodptr;
};

// -------------------
// Parameter count: 5
// -------------------

// -- class CRegisterFunction<5, R(Args...)> ----------------------------------------

template<typename R, typename... Args>
class CRegisterFunction<5, R(Args...)> : public CRegFunctionBase
{
public:

    typedef R (*funcsignature)(Args...);
    using argument_types = std::tuple<Args...>;

    // -- constructor
    CRegisterFunction(const char* _funcname, funcsignature _funcptr)
        : CRegFunctionBase(_funcname)
    {
        funcptr = _funcptr;
    }
    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);
        CVariableEntry* ve4 = GetContext()->GetParameter(4);
        CVariableEntry* ve5 = GetContext()->GetParameter(5);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);
        T4 p4 = ConvertVariableForDispatch<T4>(ve4);
        T5 p5 = ConvertVariableForDispatch<T5>(ve5);

        Dispatch(&p1, &p2, &p3, &p4, &p5);
    }

    // -- dispatch method
    R Dispatch(void* _p1, void* _p2, void* _p3, void* _p4, void* _p5)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;
        T4* p4 = (T4*)_p4;
        T5* p5 = (T5*)_p5;

        R r = funcptr(*p1, *p2, *p3, *p4, *p5);

        assert(GetContext()->GetParameter(0));
        CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual void Register(CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;

        CFunctionEntry* fe = new CFunctionEntry(script_context, 0, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), GetRegisteredType(GetTypeID<R>()), 1, GetTypeID<R>());
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());
        GetContext()->AddParameter("_p4", Hash("_p4"), GetRegisteredType(GetTypeID<T4>()), 1, GetTypeID<T4>());
        GetContext()->AddParameter("_p5", Hash("_p5"), GetRegisteredType(GetTypeID<T5>()), 1, GetTypeID<T5>());

        uint32 hash = fe->GetHash();
        tFuncTable* globalfunctable = script_context->FindNamespace(0)->GetFuncTable();
        globalfunctable->AddItem(*fe, hash);
    }

private:
    funcsignature funcptr;
};

// -- class CRegisterFunction<5, void(Args...)> ----------------------------------------

template<typename... Args>
class CRegisterFunction<5, void(Args...)> : public CRegFunctionBase
{
public:

    typedef void (*funcsignature)(Args...);
    using argument_types = std::tuple<Args...>;

    // -- constructor
    CRegisterFunction(const char* _funcname, funcsignature _funcptr)
        : CRegFunctionBase(_funcname)
    {
        funcptr = _funcptr;
    }
    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);
        CVariableEntry* ve4 = GetContext()->GetParameter(4);
        CVariableEntry* ve5 = GetContext()->GetParameter(5);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);
        T4 p4 = ConvertVariableForDispatch<T4>(ve4);
        T5 p5 = ConvertVariableForDispatch<T5>(ve5);

        Dispatch(&p1, &p2, &p3, &p4, &p5);
    }

    // -- dispatch method
    void Dispatch(void* _p1, void* _p2, void* _p3, void* _p4, void* _p5)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;
        T4* p4 = (T4*)_p4;
        T5* p5 = (T5*)_p5;

        funcptr(*p1, *p2, *p3, *p4, *p5);

    }

    // -- registration method
    virtual void Register(CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;

        CFunctionEntry* fe = new CFunctionEntry(script_context, 0, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), TYPE_void, 1, 0);
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());
        GetContext()->AddParameter("_p4", Hash("_p4"), GetRegisteredType(GetTypeID<T4>()), 1, GetTypeID<T4>());
        GetContext()->AddParameter("_p5", Hash("_p5"), GetRegisteredType(GetTypeID<T5>()), 1, GetTypeID<T5>());

        uint32 hash = fe->GetHash();
        tFuncTable* globalfunctable = script_context->FindNamespace(0)->GetFuncTable();
        globalfunctable->AddItem(*fe, hash);
    }

private:
    funcsignature funcptr;
};

// -- class CRegisterMethod<5, R(Args...)> ----------------------------------------

template<typename C, typename R, typename... Args>
class CRegisterMethod<5, C, R(Args...)> : public CRegFunctionBase
{
public:

    using argument_types = std::tuple<Args...>;
    typedef R (C::*methodsignature)(Args...);

    // -- CRegisterMethod
    CRegisterMethod(const char* _methodname, methodsignature _methodptr) :
                  CRegFunctionBase(_methodname) {
        methodptr = _methodptr;
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr)
    {

        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);
        CVariableEntry* ve4 = GetContext()->GetParameter(4);
        CVariableEntry* ve5 = GetContext()->GetParameter(5);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);
        T4 p4 = ConvertVariableForDispatch<T4>(ve4);
        T5 p5 = ConvertVariableForDispatch<T5>(ve5);
        Dispatch(objaddr, &p1, &p2, &p3, &p4, &p5);
    }

    // -- dispatch method
    R Dispatch(void* objaddr, void* _p1, void* _p2, void* _p3, void* _p4, void* _p5)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;
        T4* p4 = (T4*)_p4;
        T5* p5 = (T5*)_p5;

        C* object = (C*)(objaddr);
        R r = (object->*methodptr)(*p1, *p2, *p3, *p4, *p5);

        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = this->GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual void Register(TinScript::CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;

        uint32 classname_hash = Hash(C::_GetClassName());
        CFunctionEntry* fe = new CFunctionEntry(script_context, classname_hash, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), GetRegisteredType(GetTypeID<R>()), 1, GetTypeID<R>());
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());
        GetContext()->AddParameter("_p4", Hash("_p4"), GetRegisteredType(GetTypeID<T4>()), 1, GetTypeID<T4>());
        GetContext()->AddParameter("_p5", Hash("_p5"), GetRegisteredType(GetTypeID<T5>()), 1, GetTypeID<T5>());

        uint32 hash = fe->GetHash();
        tFuncTable* methodtable = script_context->FindNamespace(classname_hash)->GetFuncTable();
        methodtable->AddItem(*fe, hash);
    }

private:
    methodsignature methodptr;
};
// -- class CRegisterMethod<5, void(Args...)> ----------------------------------------

template<typename C, typename... Args>
class CRegisterMethod<5, C, void(Args...)> : public CRegFunctionBase
{
public:

    using argument_types = std::tuple<Args...>;
    typedef void (C::*methodsignature)(Args...);

    // -- CRegisterMethod
    CRegisterMethod(const char* _methodname, methodsignature _methodptr) :
                  CRegFunctionBase(_methodname) {
        methodptr = _methodptr;
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr)
    {

        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);
        CVariableEntry* ve4 = GetContext()->GetParameter(4);
        CVariableEntry* ve5 = GetContext()->GetParameter(5);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);
        T4 p4 = ConvertVariableForDispatch<T4>(ve4);
        T5 p5 = ConvertVariableForDispatch<T5>(ve5);

        Dispatch(objaddr, &p1, &p2, &p3, &p4, &p5);
    }

    // -- dispatch method
    void Dispatch(void* objaddr, void* _p1, void* _p2, void* _p3, void* _p4, void* _p5)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;
        T4* p4 = (T4*)_p4;
        T5* p5 = (T5*)_p5;

        C* object = (C*)(objaddr);
        (object->*methodptr)(*p1, *p2, *p3, *p4, *p5);

    }

    // -- registration method
    virtual void Register(TinScript::CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;


        uint32 classname_hash = Hash(C::_GetClassName());
        CFunctionEntry* fe = new CFunctionEntry(script_context, classname_hash, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), TYPE_void, 1, 0);
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());
        GetContext()->AddParameter("_p4", Hash("_p4"), GetRegisteredType(GetTypeID<T4>()), 1, GetTypeID<T4>());
        GetContext()->AddParameter("_p5", Hash("_p5"), GetRegisteredType(GetTypeID<T5>()), 1, GetTypeID<T5>());


        uint32 hash = fe->GetHash();
        tFuncTable* methodtable = script_context->FindNamespace(classname_hash)->GetFuncTable();
        methodtable->AddItem(*fe, hash);
    }

private:
    methodsignature methodptr;
};

// -------------------
// Parameter count: 6
// -------------------

// -- class CRegisterFunction<6, R(Args...)> ----------------------------------------

template<typename R, typename... Args>
class CRegisterFunction<6, R(Args...)> : public CRegFunctionBase
{
public:

    typedef R (*funcsignature)(Args...);
    using argument_types = std::tuple<Args...>;

    // -- constructor
    CRegisterFunction(const char* _funcname, funcsignature _funcptr)
        : CRegFunctionBase(_funcname)
    {
        funcptr = _funcptr;
    }
    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);
        CVariableEntry* ve4 = GetContext()->GetParameter(4);
        CVariableEntry* ve5 = GetContext()->GetParameter(5);
        CVariableEntry* ve6 = GetContext()->GetParameter(6);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);
        T4 p4 = ConvertVariableForDispatch<T4>(ve4);
        T5 p5 = ConvertVariableForDispatch<T5>(ve5);
        T6 p6 = ConvertVariableForDispatch<T6>(ve6);

        Dispatch(&p1, &p2, &p3, &p4, &p5, &p6);
    }

    // -- dispatch method
    R Dispatch(void* _p1, void* _p2, void* _p3, void* _p4, void* _p5, void* _p6)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;
        T4* p4 = (T4*)_p4;
        T5* p5 = (T5*)_p5;
        T6* p6 = (T6*)_p6;

        R r = funcptr(*p1, *p2, *p3, *p4, *p5, *p6);

        assert(GetContext()->GetParameter(0));
        CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual void Register(CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;

        CFunctionEntry* fe = new CFunctionEntry(script_context, 0, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), GetRegisteredType(GetTypeID<R>()), 1, GetTypeID<R>());
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());
        GetContext()->AddParameter("_p4", Hash("_p4"), GetRegisteredType(GetTypeID<T4>()), 1, GetTypeID<T4>());
        GetContext()->AddParameter("_p5", Hash("_p5"), GetRegisteredType(GetTypeID<T5>()), 1, GetTypeID<T5>());
        GetContext()->AddParameter("_p6", Hash("_p6"), GetRegisteredType(GetTypeID<T6>()), 1, GetTypeID<T6>());

        uint32 hash = fe->GetHash();
        tFuncTable* globalfunctable = script_context->FindNamespace(0)->GetFuncTable();
        globalfunctable->AddItem(*fe, hash);
    }

private:
    funcsignature funcptr;
};

// -- class CRegisterFunction<6, void(Args...)> ----------------------------------------

template<typename... Args>
class CRegisterFunction<6, void(Args...)> : public CRegFunctionBase
{
public:

    typedef void (*funcsignature)(Args...);
    using argument_types = std::tuple<Args...>;

    // -- constructor
    CRegisterFunction(const char* _funcname, funcsignature _funcptr)
        : CRegFunctionBase(_funcname)
    {
        funcptr = _funcptr;
    }
    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);
        CVariableEntry* ve4 = GetContext()->GetParameter(4);
        CVariableEntry* ve5 = GetContext()->GetParameter(5);
        CVariableEntry* ve6 = GetContext()->GetParameter(6);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);
        T4 p4 = ConvertVariableForDispatch<T4>(ve4);
        T5 p5 = ConvertVariableForDispatch<T5>(ve5);
        T6 p6 = ConvertVariableForDispatch<T6>(ve6);

        Dispatch(&p1, &p2, &p3, &p4, &p5, &p6);
    }

    // -- dispatch method
    void Dispatch(void* _p1, void* _p2, void* _p3, void* _p4, void* _p5, void* _p6)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;
        T4* p4 = (T4*)_p4;
        T5* p5 = (T5*)_p5;
        T6* p6 = (T6*)_p6;

        funcptr(*p1, *p2, *p3, *p4, *p5, *p6);

    }

    // -- registration method
    virtual void Register(CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;

        CFunctionEntry* fe = new CFunctionEntry(script_context, 0, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), TYPE_void, 1, 0);
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());
        GetContext()->AddParameter("_p4", Hash("_p4"), GetRegisteredType(GetTypeID<T4>()), 1, GetTypeID<T4>());
        GetContext()->AddParameter("_p5", Hash("_p5"), GetRegisteredType(GetTypeID<T5>()), 1, GetTypeID<T5>());
        GetContext()->AddParameter("_p6", Hash("_p6"), GetRegisteredType(GetTypeID<T6>()), 1, GetTypeID<T6>());

        uint32 hash = fe->GetHash();
        tFuncTable* globalfunctable = script_context->FindNamespace(0)->GetFuncTable();
        globalfunctable->AddItem(*fe, hash);
    }

private:
    funcsignature funcptr;
};

// -- class CRegisterMethod<6, R(Args...)> ----------------------------------------

template<typename C, typename R, typename... Args>
class CRegisterMethod<6, C, R(Args...)> : public CRegFunctionBase
{
public:

    using argument_types = std::tuple<Args...>;
    typedef R (C::*methodsignature)(Args...);

    // -- CRegisterMethod
    CRegisterMethod(const char* _methodname, methodsignature _methodptr) :
                  CRegFunctionBase(_methodname) {
        methodptr = _methodptr;
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr)
    {

        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);
        CVariableEntry* ve4 = GetContext()->GetParameter(4);
        CVariableEntry* ve5 = GetContext()->GetParameter(5);
        CVariableEntry* ve6 = GetContext()->GetParameter(6);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);
        T4 p4 = ConvertVariableForDispatch<T4>(ve4);
        T5 p5 = ConvertVariableForDispatch<T5>(ve5);
        T6 p6 = ConvertVariableForDispatch<T6>(ve6);
        Dispatch(objaddr, &p1, &p2, &p3, &p4, &p5, &p6);
    }

    // -- dispatch method
    R Dispatch(void* objaddr, void* _p1, void* _p2, void* _p3, void* _p4, void* _p5, void* _p6)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;
        T4* p4 = (T4*)_p4;
        T5* p5 = (T5*)_p5;
        T6* p6 = (T6*)_p6;

        C* object = (C*)(objaddr);
        R r = (object->*methodptr)(*p1, *p2, *p3, *p4, *p5, *p6);

        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = this->GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual void Register(TinScript::CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;

        uint32 classname_hash = Hash(C::_GetClassName());
        CFunctionEntry* fe = new CFunctionEntry(script_context, classname_hash, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), GetRegisteredType(GetTypeID<R>()), 1, GetTypeID<R>());
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());
        GetContext()->AddParameter("_p4", Hash("_p4"), GetRegisteredType(GetTypeID<T4>()), 1, GetTypeID<T4>());
        GetContext()->AddParameter("_p5", Hash("_p5"), GetRegisteredType(GetTypeID<T5>()), 1, GetTypeID<T5>());
        GetContext()->AddParameter("_p6", Hash("_p6"), GetRegisteredType(GetTypeID<T6>()), 1, GetTypeID<T6>());

        uint32 hash = fe->GetHash();
        tFuncTable* methodtable = script_context->FindNamespace(classname_hash)->GetFuncTable();
        methodtable->AddItem(*fe, hash);
    }

private:
    methodsignature methodptr;
};
// -- class CRegisterMethod<6, void(Args...)> ----------------------------------------

template<typename C, typename... Args>
class CRegisterMethod<6, C, void(Args...)> : public CRegFunctionBase
{
public:

    using argument_types = std::tuple<Args...>;
    typedef void (C::*methodsignature)(Args...);

    // -- CRegisterMethod
    CRegisterMethod(const char* _methodname, methodsignature _methodptr) :
                  CRegFunctionBase(_methodname) {
        methodptr = _methodptr;
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr)
    {

        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);
        CVariableEntry* ve4 = GetContext()->GetParameter(4);
        CVariableEntry* ve5 = GetContext()->GetParameter(5);
        CVariableEntry* ve6 = GetContext()->GetParameter(6);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);
        T4 p4 = ConvertVariableForDispatch<T4>(ve4);
        T5 p5 = ConvertVariableForDispatch<T5>(ve5);
        T6 p6 = ConvertVariableForDispatch<T6>(ve6);

        Dispatch(objaddr, &p1, &p2, &p3, &p4, &p5, &p6);
    }

    // -- dispatch method
    void Dispatch(void* objaddr, void* _p1, void* _p2, void* _p3, void* _p4, void* _p5, void* _p6)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;
        T4* p4 = (T4*)_p4;
        T5* p5 = (T5*)_p5;
        T6* p6 = (T6*)_p6;

        C* object = (C*)(objaddr);
        (object->*methodptr)(*p1, *p2, *p3, *p4, *p5, *p6);

    }

    // -- registration method
    virtual void Register(TinScript::CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;


        uint32 classname_hash = Hash(C::_GetClassName());
        CFunctionEntry* fe = new CFunctionEntry(script_context, classname_hash, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), TYPE_void, 1, 0);
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());
        GetContext()->AddParameter("_p4", Hash("_p4"), GetRegisteredType(GetTypeID<T4>()), 1, GetTypeID<T4>());
        GetContext()->AddParameter("_p5", Hash("_p5"), GetRegisteredType(GetTypeID<T5>()), 1, GetTypeID<T5>());
        GetContext()->AddParameter("_p6", Hash("_p6"), GetRegisteredType(GetTypeID<T6>()), 1, GetTypeID<T6>());


        uint32 hash = fe->GetHash();
        tFuncTable* methodtable = script_context->FindNamespace(classname_hash)->GetFuncTable();
        methodtable->AddItem(*fe, hash);
    }

private:
    methodsignature methodptr;
};

// -------------------
// Parameter count: 7
// -------------------

// -- class CRegisterFunction<7, R(Args...)> ----------------------------------------

template<typename R, typename... Args>
class CRegisterFunction<7, R(Args...)> : public CRegFunctionBase
{
public:

    typedef R (*funcsignature)(Args...);
    using argument_types = std::tuple<Args...>;

    // -- constructor
    CRegisterFunction(const char* _funcname, funcsignature _funcptr)
        : CRegFunctionBase(_funcname)
    {
        funcptr = _funcptr;
    }
    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);
        CVariableEntry* ve4 = GetContext()->GetParameter(4);
        CVariableEntry* ve5 = GetContext()->GetParameter(5);
        CVariableEntry* ve6 = GetContext()->GetParameter(6);
        CVariableEntry* ve7 = GetContext()->GetParameter(7);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);
        T4 p4 = ConvertVariableForDispatch<T4>(ve4);
        T5 p5 = ConvertVariableForDispatch<T5>(ve5);
        T6 p6 = ConvertVariableForDispatch<T6>(ve6);
        T7 p7 = ConvertVariableForDispatch<T7>(ve7);

        Dispatch(&p1, &p2, &p3, &p4, &p5, &p6, &p7);
    }

    // -- dispatch method
    R Dispatch(void* _p1, void* _p2, void* _p3, void* _p4, void* _p5, void* _p6, void* _p7)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;
        T4* p4 = (T4*)_p4;
        T5* p5 = (T5*)_p5;
        T6* p6 = (T6*)_p6;
        T7* p7 = (T7*)_p7;

        R r = funcptr(*p1, *p2, *p3, *p4, *p5, *p6, *p7);

        assert(GetContext()->GetParameter(0));
        CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual void Register(CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;

        CFunctionEntry* fe = new CFunctionEntry(script_context, 0, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), GetRegisteredType(GetTypeID<R>()), 1, GetTypeID<R>());
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());
        GetContext()->AddParameter("_p4", Hash("_p4"), GetRegisteredType(GetTypeID<T4>()), 1, GetTypeID<T4>());
        GetContext()->AddParameter("_p5", Hash("_p5"), GetRegisteredType(GetTypeID<T5>()), 1, GetTypeID<T5>());
        GetContext()->AddParameter("_p6", Hash("_p6"), GetRegisteredType(GetTypeID<T6>()), 1, GetTypeID<T6>());
        GetContext()->AddParameter("_p7", Hash("_p7"), GetRegisteredType(GetTypeID<T7>()), 1, GetTypeID<T7>());

        uint32 hash = fe->GetHash();
        tFuncTable* globalfunctable = script_context->FindNamespace(0)->GetFuncTable();
        globalfunctable->AddItem(*fe, hash);
    }

private:
    funcsignature funcptr;
};

// -- class CRegisterFunction<7, void(Args...)> ----------------------------------------

template<typename... Args>
class CRegisterFunction<7, void(Args...)> : public CRegFunctionBase
{
public:

    typedef void (*funcsignature)(Args...);
    using argument_types = std::tuple<Args...>;

    // -- constructor
    CRegisterFunction(const char* _funcname, funcsignature _funcptr)
        : CRegFunctionBase(_funcname)
    {
        funcptr = _funcptr;
    }
    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);
        CVariableEntry* ve4 = GetContext()->GetParameter(4);
        CVariableEntry* ve5 = GetContext()->GetParameter(5);
        CVariableEntry* ve6 = GetContext()->GetParameter(6);
        CVariableEntry* ve7 = GetContext()->GetParameter(7);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);
        T4 p4 = ConvertVariableForDispatch<T4>(ve4);
        T5 p5 = ConvertVariableForDispatch<T5>(ve5);
        T6 p6 = ConvertVariableForDispatch<T6>(ve6);
        T7 p7 = ConvertVariableForDispatch<T7>(ve7);

        Dispatch(&p1, &p2, &p3, &p4, &p5, &p6, &p7);
    }

    // -- dispatch method
    void Dispatch(void* _p1, void* _p2, void* _p3, void* _p4, void* _p5, void* _p6, void* _p7)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;
        T4* p4 = (T4*)_p4;
        T5* p5 = (T5*)_p5;
        T6* p6 = (T6*)_p6;
        T7* p7 = (T7*)_p7;

        funcptr(*p1, *p2, *p3, *p4, *p5, *p6, *p7);

    }

    // -- registration method
    virtual void Register(CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;

        CFunctionEntry* fe = new CFunctionEntry(script_context, 0, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), TYPE_void, 1, 0);
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());
        GetContext()->AddParameter("_p4", Hash("_p4"), GetRegisteredType(GetTypeID<T4>()), 1, GetTypeID<T4>());
        GetContext()->AddParameter("_p5", Hash("_p5"), GetRegisteredType(GetTypeID<T5>()), 1, GetTypeID<T5>());
        GetContext()->AddParameter("_p6", Hash("_p6"), GetRegisteredType(GetTypeID<T6>()), 1, GetTypeID<T6>());
        GetContext()->AddParameter("_p7", Hash("_p7"), GetRegisteredType(GetTypeID<T7>()), 1, GetTypeID<T7>());

        uint32 hash = fe->GetHash();
        tFuncTable* globalfunctable = script_context->FindNamespace(0)->GetFuncTable();
        globalfunctable->AddItem(*fe, hash);
    }

private:
    funcsignature funcptr;
};

// -- class CRegisterMethod<7, R(Args...)> ----------------------------------------

template<typename C, typename R, typename... Args>
class CRegisterMethod<7, C, R(Args...)> : public CRegFunctionBase
{
public:

    using argument_types = std::tuple<Args...>;
    typedef R (C::*methodsignature)(Args...);

    // -- CRegisterMethod
    CRegisterMethod(const char* _methodname, methodsignature _methodptr) :
                  CRegFunctionBase(_methodname) {
        methodptr = _methodptr;
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr)
    {

        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);
        CVariableEntry* ve4 = GetContext()->GetParameter(4);
        CVariableEntry* ve5 = GetContext()->GetParameter(5);
        CVariableEntry* ve6 = GetContext()->GetParameter(6);
        CVariableEntry* ve7 = GetContext()->GetParameter(7);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);
        T4 p4 = ConvertVariableForDispatch<T4>(ve4);
        T5 p5 = ConvertVariableForDispatch<T5>(ve5);
        T6 p6 = ConvertVariableForDispatch<T6>(ve6);
        T7 p7 = ConvertVariableForDispatch<T7>(ve7);
        Dispatch(objaddr, &p1, &p2, &p3, &p4, &p5, &p6, &p7);
    }

    // -- dispatch method
    R Dispatch(void* objaddr, void* _p1, void* _p2, void* _p3, void* _p4, void* _p5, void* _p6, void* _p7)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;
        T4* p4 = (T4*)_p4;
        T5* p5 = (T5*)_p5;
        T6* p6 = (T6*)_p6;
        T7* p7 = (T7*)_p7;

        C* object = (C*)(objaddr);
        R r = (object->*methodptr)(*p1, *p2, *p3, *p4, *p5, *p6, *p7);

        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = this->GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual void Register(TinScript::CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;

        uint32 classname_hash = Hash(C::_GetClassName());
        CFunctionEntry* fe = new CFunctionEntry(script_context, classname_hash, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), GetRegisteredType(GetTypeID<R>()), 1, GetTypeID<R>());
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());
        GetContext()->AddParameter("_p4", Hash("_p4"), GetRegisteredType(GetTypeID<T4>()), 1, GetTypeID<T4>());
        GetContext()->AddParameter("_p5", Hash("_p5"), GetRegisteredType(GetTypeID<T5>()), 1, GetTypeID<T5>());
        GetContext()->AddParameter("_p6", Hash("_p6"), GetRegisteredType(GetTypeID<T6>()), 1, GetTypeID<T6>());
        GetContext()->AddParameter("_p7", Hash("_p7"), GetRegisteredType(GetTypeID<T7>()), 1, GetTypeID<T7>());

        uint32 hash = fe->GetHash();
        tFuncTable* methodtable = script_context->FindNamespace(classname_hash)->GetFuncTable();
        methodtable->AddItem(*fe, hash);
    }

private:
    methodsignature methodptr;
};
// -- class CRegisterMethod<7, void(Args...)> ----------------------------------------

template<typename C, typename... Args>
class CRegisterMethod<7, C, void(Args...)> : public CRegFunctionBase
{
public:

    using argument_types = std::tuple<Args...>;
    typedef void (C::*methodsignature)(Args...);

    // -- CRegisterMethod
    CRegisterMethod(const char* _methodname, methodsignature _methodptr) :
                  CRegFunctionBase(_methodname) {
        methodptr = _methodptr;
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr)
    {

        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);
        CVariableEntry* ve4 = GetContext()->GetParameter(4);
        CVariableEntry* ve5 = GetContext()->GetParameter(5);
        CVariableEntry* ve6 = GetContext()->GetParameter(6);
        CVariableEntry* ve7 = GetContext()->GetParameter(7);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);
        T4 p4 = ConvertVariableForDispatch<T4>(ve4);
        T5 p5 = ConvertVariableForDispatch<T5>(ve5);
        T6 p6 = ConvertVariableForDispatch<T6>(ve6);
        T7 p7 = ConvertVariableForDispatch<T7>(ve7);

        Dispatch(objaddr, &p1, &p2, &p3, &p4, &p5, &p6, &p7);
    }

    // -- dispatch method
    void Dispatch(void* objaddr, void* _p1, void* _p2, void* _p3, void* _p4, void* _p5, void* _p6, void* _p7)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;
        T4* p4 = (T4*)_p4;
        T5* p5 = (T5*)_p5;
        T6* p6 = (T6*)_p6;
        T7* p7 = (T7*)_p7;

        C* object = (C*)(objaddr);
        (object->*methodptr)(*p1, *p2, *p3, *p4, *p5, *p6, *p7);

    }

    // -- registration method
    virtual void Register(TinScript::CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;


        uint32 classname_hash = Hash(C::_GetClassName());
        CFunctionEntry* fe = new CFunctionEntry(script_context, classname_hash, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), TYPE_void, 1, 0);
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());
        GetContext()->AddParameter("_p4", Hash("_p4"), GetRegisteredType(GetTypeID<T4>()), 1, GetTypeID<T4>());
        GetContext()->AddParameter("_p5", Hash("_p5"), GetRegisteredType(GetTypeID<T5>()), 1, GetTypeID<T5>());
        GetContext()->AddParameter("_p6", Hash("_p6"), GetRegisteredType(GetTypeID<T6>()), 1, GetTypeID<T6>());
        GetContext()->AddParameter("_p7", Hash("_p7"), GetRegisteredType(GetTypeID<T7>()), 1, GetTypeID<T7>());


        uint32 hash = fe->GetHash();
        tFuncTable* methodtable = script_context->FindNamespace(classname_hash)->GetFuncTable();
        methodtable->AddItem(*fe, hash);
    }

private:
    methodsignature methodptr;
};

// -------------------
// Parameter count: 8
// -------------------

// -- class CRegisterFunction<8, R(Args...)> ----------------------------------------

template<typename R, typename... Args>
class CRegisterFunction<8, R(Args...)> : public CRegFunctionBase
{
public:

    typedef R (*funcsignature)(Args...);
    using argument_types = std::tuple<Args...>;

    // -- constructor
    CRegisterFunction(const char* _funcname, funcsignature _funcptr)
        : CRegFunctionBase(_funcname)
    {
        funcptr = _funcptr;
    }
    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);
        CVariableEntry* ve4 = GetContext()->GetParameter(4);
        CVariableEntry* ve5 = GetContext()->GetParameter(5);
        CVariableEntry* ve6 = GetContext()->GetParameter(6);
        CVariableEntry* ve7 = GetContext()->GetParameter(7);
        CVariableEntry* ve8 = GetContext()->GetParameter(8);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);
        T4 p4 = ConvertVariableForDispatch<T4>(ve4);
        T5 p5 = ConvertVariableForDispatch<T5>(ve5);
        T6 p6 = ConvertVariableForDispatch<T6>(ve6);
        T7 p7 = ConvertVariableForDispatch<T7>(ve7);
        T8 p8 = ConvertVariableForDispatch<T8>(ve8);

        Dispatch(&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8);
    }

    // -- dispatch method
    R Dispatch(void* _p1, void* _p2, void* _p3, void* _p4, void* _p5, void* _p6, void* _p7, void* _p8)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;
        T4* p4 = (T4*)_p4;
        T5* p5 = (T5*)_p5;
        T6* p6 = (T6*)_p6;
        T7* p7 = (T7*)_p7;
        T8* p8 = (T8*)_p8;

        R r = funcptr(*p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8);

        assert(GetContext()->GetParameter(0));
        CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual void Register(CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;

        CFunctionEntry* fe = new CFunctionEntry(script_context, 0, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), GetRegisteredType(GetTypeID<R>()), 1, GetTypeID<R>());
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());
        GetContext()->AddParameter("_p4", Hash("_p4"), GetRegisteredType(GetTypeID<T4>()), 1, GetTypeID<T4>());
        GetContext()->AddParameter("_p5", Hash("_p5"), GetRegisteredType(GetTypeID<T5>()), 1, GetTypeID<T5>());
        GetContext()->AddParameter("_p6", Hash("_p6"), GetRegisteredType(GetTypeID<T6>()), 1, GetTypeID<T6>());
        GetContext()->AddParameter("_p7", Hash("_p7"), GetRegisteredType(GetTypeID<T7>()), 1, GetTypeID<T7>());
        GetContext()->AddParameter("_p8", Hash("_p8"), GetRegisteredType(GetTypeID<T8>()), 1, GetTypeID<T8>());

        uint32 hash = fe->GetHash();
        tFuncTable* globalfunctable = script_context->FindNamespace(0)->GetFuncTable();
        globalfunctable->AddItem(*fe, hash);
    }

private:
    funcsignature funcptr;
};

// -- class CRegisterFunction<8, void(Args...)> ----------------------------------------

template<typename... Args>
class CRegisterFunction<8, void(Args...)> : public CRegFunctionBase
{
public:

    typedef void (*funcsignature)(Args...);
    using argument_types = std::tuple<Args...>;

    // -- constructor
    CRegisterFunction(const char* _funcname, funcsignature _funcptr)
        : CRegFunctionBase(_funcname)
    {
        funcptr = _funcptr;
    }
    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);
        CVariableEntry* ve4 = GetContext()->GetParameter(4);
        CVariableEntry* ve5 = GetContext()->GetParameter(5);
        CVariableEntry* ve6 = GetContext()->GetParameter(6);
        CVariableEntry* ve7 = GetContext()->GetParameter(7);
        CVariableEntry* ve8 = GetContext()->GetParameter(8);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);
        T4 p4 = ConvertVariableForDispatch<T4>(ve4);
        T5 p5 = ConvertVariableForDispatch<T5>(ve5);
        T6 p6 = ConvertVariableForDispatch<T6>(ve6);
        T7 p7 = ConvertVariableForDispatch<T7>(ve7);
        T8 p8 = ConvertVariableForDispatch<T8>(ve8);

        Dispatch(&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8);
    }

    // -- dispatch method
    void Dispatch(void* _p1, void* _p2, void* _p3, void* _p4, void* _p5, void* _p6, void* _p7, void* _p8)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;
        T4* p4 = (T4*)_p4;
        T5* p5 = (T5*)_p5;
        T6* p6 = (T6*)_p6;
        T7* p7 = (T7*)_p7;
        T8* p8 = (T8*)_p8;

        funcptr(*p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8);

    }

    // -- registration method
    virtual void Register(CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;

        CFunctionEntry* fe = new CFunctionEntry(script_context, 0, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), TYPE_void, 1, 0);
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());
        GetContext()->AddParameter("_p4", Hash("_p4"), GetRegisteredType(GetTypeID<T4>()), 1, GetTypeID<T4>());
        GetContext()->AddParameter("_p5", Hash("_p5"), GetRegisteredType(GetTypeID<T5>()), 1, GetTypeID<T5>());
        GetContext()->AddParameter("_p6", Hash("_p6"), GetRegisteredType(GetTypeID<T6>()), 1, GetTypeID<T6>());
        GetContext()->AddParameter("_p7", Hash("_p7"), GetRegisteredType(GetTypeID<T7>()), 1, GetTypeID<T7>());
        GetContext()->AddParameter("_p8", Hash("_p8"), GetRegisteredType(GetTypeID<T8>()), 1, GetTypeID<T8>());

        uint32 hash = fe->GetHash();
        tFuncTable* globalfunctable = script_context->FindNamespace(0)->GetFuncTable();
        globalfunctable->AddItem(*fe, hash);
    }

private:
    funcsignature funcptr;
};

// -- class CRegisterMethod<8, R(Args...)> ----------------------------------------

template<typename C, typename R, typename... Args>
class CRegisterMethod<8, C, R(Args...)> : public CRegFunctionBase
{
public:

    using argument_types = std::tuple<Args...>;
    typedef R (C::*methodsignature)(Args...);

    // -- CRegisterMethod
    CRegisterMethod(const char* _methodname, methodsignature _methodptr) :
                  CRegFunctionBase(_methodname) {
        methodptr = _methodptr;
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr)
    {

        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);
        CVariableEntry* ve4 = GetContext()->GetParameter(4);
        CVariableEntry* ve5 = GetContext()->GetParameter(5);
        CVariableEntry* ve6 = GetContext()->GetParameter(6);
        CVariableEntry* ve7 = GetContext()->GetParameter(7);
        CVariableEntry* ve8 = GetContext()->GetParameter(8);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);
        T4 p4 = ConvertVariableForDispatch<T4>(ve4);
        T5 p5 = ConvertVariableForDispatch<T5>(ve5);
        T6 p6 = ConvertVariableForDispatch<T6>(ve6);
        T7 p7 = ConvertVariableForDispatch<T7>(ve7);
        T8 p8 = ConvertVariableForDispatch<T8>(ve8);
        Dispatch(objaddr, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8);
    }

    // -- dispatch method
    R Dispatch(void* objaddr, void* _p1, void* _p2, void* _p3, void* _p4, void* _p5, void* _p6, void* _p7, void* _p8)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;
        T4* p4 = (T4*)_p4;
        T5* p5 = (T5*)_p5;
        T6* p6 = (T6*)_p6;
        T7* p7 = (T7*)_p7;
        T8* p8 = (T8*)_p8;

        C* object = (C*)(objaddr);
        R r = (object->*methodptr)(*p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8);

        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = this->GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual void Register(TinScript::CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;

        uint32 classname_hash = Hash(C::_GetClassName());
        CFunctionEntry* fe = new CFunctionEntry(script_context, classname_hash, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), GetRegisteredType(GetTypeID<R>()), 1, GetTypeID<R>());
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());
        GetContext()->AddParameter("_p4", Hash("_p4"), GetRegisteredType(GetTypeID<T4>()), 1, GetTypeID<T4>());
        GetContext()->AddParameter("_p5", Hash("_p5"), GetRegisteredType(GetTypeID<T5>()), 1, GetTypeID<T5>());
        GetContext()->AddParameter("_p6", Hash("_p6"), GetRegisteredType(GetTypeID<T6>()), 1, GetTypeID<T6>());
        GetContext()->AddParameter("_p7", Hash("_p7"), GetRegisteredType(GetTypeID<T7>()), 1, GetTypeID<T7>());
        GetContext()->AddParameter("_p8", Hash("_p8"), GetRegisteredType(GetTypeID<T8>()), 1, GetTypeID<T8>());

        uint32 hash = fe->GetHash();
        tFuncTable* methodtable = script_context->FindNamespace(classname_hash)->GetFuncTable();
        methodtable->AddItem(*fe, hash);
    }

private:
    methodsignature methodptr;
};
// -- class CRegisterMethod<8, void(Args...)> ----------------------------------------

template<typename C, typename... Args>
class CRegisterMethod<8, C, void(Args...)> : public CRegFunctionBase
{
public:

    using argument_types = std::tuple<Args...>;
    typedef void (C::*methodsignature)(Args...);

    // -- CRegisterMethod
    CRegisterMethod(const char* _methodname, methodsignature _methodptr) :
                  CRegFunctionBase(_methodname) {
        methodptr = _methodptr;
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr)
    {

        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);
        CVariableEntry* ve4 = GetContext()->GetParameter(4);
        CVariableEntry* ve5 = GetContext()->GetParameter(5);
        CVariableEntry* ve6 = GetContext()->GetParameter(6);
        CVariableEntry* ve7 = GetContext()->GetParameter(7);
        CVariableEntry* ve8 = GetContext()->GetParameter(8);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);
        T4 p4 = ConvertVariableForDispatch<T4>(ve4);
        T5 p5 = ConvertVariableForDispatch<T5>(ve5);
        T6 p6 = ConvertVariableForDispatch<T6>(ve6);
        T7 p7 = ConvertVariableForDispatch<T7>(ve7);
        T8 p8 = ConvertVariableForDispatch<T8>(ve8);

        Dispatch(objaddr, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8);
    }

    // -- dispatch method
    void Dispatch(void* objaddr, void* _p1, void* _p2, void* _p3, void* _p4, void* _p5, void* _p6, void* _p7, void* _p8)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;
        T4* p4 = (T4*)_p4;
        T5* p5 = (T5*)_p5;
        T6* p6 = (T6*)_p6;
        T7* p7 = (T7*)_p7;
        T8* p8 = (T8*)_p8;

        C* object = (C*)(objaddr);
        (object->*methodptr)(*p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8);

    }

    // -- registration method
    virtual void Register(TinScript::CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;


        uint32 classname_hash = Hash(C::_GetClassName());
        CFunctionEntry* fe = new CFunctionEntry(script_context, classname_hash, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), TYPE_void, 1, 0);
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());
        GetContext()->AddParameter("_p4", Hash("_p4"), GetRegisteredType(GetTypeID<T4>()), 1, GetTypeID<T4>());
        GetContext()->AddParameter("_p5", Hash("_p5"), GetRegisteredType(GetTypeID<T5>()), 1, GetTypeID<T5>());
        GetContext()->AddParameter("_p6", Hash("_p6"), GetRegisteredType(GetTypeID<T6>()), 1, GetTypeID<T6>());
        GetContext()->AddParameter("_p7", Hash("_p7"), GetRegisteredType(GetTypeID<T7>()), 1, GetTypeID<T7>());
        GetContext()->AddParameter("_p8", Hash("_p8"), GetRegisteredType(GetTypeID<T8>()), 1, GetTypeID<T8>());


        uint32 hash = fe->GetHash();
        tFuncTable* methodtable = script_context->FindNamespace(classname_hash)->GetFuncTable();
        methodtable->AddItem(*fe, hash);
    }

private:
    methodsignature methodptr;
};

// -------------------
// Parameter count: 9
// -------------------

// -- class CRegisterFunction<9, R(Args...)> ----------------------------------------

template<typename R, typename... Args>
class CRegisterFunction<9, R(Args...)> : public CRegFunctionBase
{
public:

    typedef R (*funcsignature)(Args...);
    using argument_types = std::tuple<Args...>;

    // -- constructor
    CRegisterFunction(const char* _funcname, funcsignature _funcptr)
        : CRegFunctionBase(_funcname)
    {
        funcptr = _funcptr;
    }
    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);
        CVariableEntry* ve4 = GetContext()->GetParameter(4);
        CVariableEntry* ve5 = GetContext()->GetParameter(5);
        CVariableEntry* ve6 = GetContext()->GetParameter(6);
        CVariableEntry* ve7 = GetContext()->GetParameter(7);
        CVariableEntry* ve8 = GetContext()->GetParameter(8);
        CVariableEntry* ve9 = GetContext()->GetParameter(9);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);
        T4 p4 = ConvertVariableForDispatch<T4>(ve4);
        T5 p5 = ConvertVariableForDispatch<T5>(ve5);
        T6 p6 = ConvertVariableForDispatch<T6>(ve6);
        T7 p7 = ConvertVariableForDispatch<T7>(ve7);
        T8 p8 = ConvertVariableForDispatch<T8>(ve8);
        T9 p9 = ConvertVariableForDispatch<T9>(ve9);

        Dispatch(&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9);
    }

    // -- dispatch method
    R Dispatch(void* _p1, void* _p2, void* _p3, void* _p4, void* _p5, void* _p6, void* _p7, void* _p8, void* _p9)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;
        T4* p4 = (T4*)_p4;
        T5* p5 = (T5*)_p5;
        T6* p6 = (T6*)_p6;
        T7* p7 = (T7*)_p7;
        T8* p8 = (T8*)_p8;
        T9* p9 = (T9*)_p9;

        R r = funcptr(*p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8, *p9);

        assert(GetContext()->GetParameter(0));
        CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual void Register(CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;

        CFunctionEntry* fe = new CFunctionEntry(script_context, 0, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), GetRegisteredType(GetTypeID<R>()), 1, GetTypeID<R>());
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());
        GetContext()->AddParameter("_p4", Hash("_p4"), GetRegisteredType(GetTypeID<T4>()), 1, GetTypeID<T4>());
        GetContext()->AddParameter("_p5", Hash("_p5"), GetRegisteredType(GetTypeID<T5>()), 1, GetTypeID<T5>());
        GetContext()->AddParameter("_p6", Hash("_p6"), GetRegisteredType(GetTypeID<T6>()), 1, GetTypeID<T6>());
        GetContext()->AddParameter("_p7", Hash("_p7"), GetRegisteredType(GetTypeID<T7>()), 1, GetTypeID<T7>());
        GetContext()->AddParameter("_p8", Hash("_p8"), GetRegisteredType(GetTypeID<T8>()), 1, GetTypeID<T8>());
        GetContext()->AddParameter("_p9", Hash("_p9"), GetRegisteredType(GetTypeID<T9>()), 1, GetTypeID<T9>());

        uint32 hash = fe->GetHash();
        tFuncTable* globalfunctable = script_context->FindNamespace(0)->GetFuncTable();
        globalfunctable->AddItem(*fe, hash);
    }

private:
    funcsignature funcptr;
};

// -- class CRegisterFunction<9, void(Args...)> ----------------------------------------

template<typename... Args>
class CRegisterFunction<9, void(Args...)> : public CRegFunctionBase
{
public:

    typedef void (*funcsignature)(Args...);
    using argument_types = std::tuple<Args...>;

    // -- constructor
    CRegisterFunction(const char* _funcname, funcsignature _funcptr)
        : CRegFunctionBase(_funcname)
    {
        funcptr = _funcptr;
    }
    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);
        CVariableEntry* ve4 = GetContext()->GetParameter(4);
        CVariableEntry* ve5 = GetContext()->GetParameter(5);
        CVariableEntry* ve6 = GetContext()->GetParameter(6);
        CVariableEntry* ve7 = GetContext()->GetParameter(7);
        CVariableEntry* ve8 = GetContext()->GetParameter(8);
        CVariableEntry* ve9 = GetContext()->GetParameter(9);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);
        T4 p4 = ConvertVariableForDispatch<T4>(ve4);
        T5 p5 = ConvertVariableForDispatch<T5>(ve5);
        T6 p6 = ConvertVariableForDispatch<T6>(ve6);
        T7 p7 = ConvertVariableForDispatch<T7>(ve7);
        T8 p8 = ConvertVariableForDispatch<T8>(ve8);
        T9 p9 = ConvertVariableForDispatch<T9>(ve9);

        Dispatch(&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9);
    }

    // -- dispatch method
    void Dispatch(void* _p1, void* _p2, void* _p3, void* _p4, void* _p5, void* _p6, void* _p7, void* _p8, void* _p9)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;
        T4* p4 = (T4*)_p4;
        T5* p5 = (T5*)_p5;
        T6* p6 = (T6*)_p6;
        T7* p7 = (T7*)_p7;
        T8* p8 = (T8*)_p8;
        T9* p9 = (T9*)_p9;

        funcptr(*p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8, *p9);

    }

    // -- registration method
    virtual void Register(CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;

        CFunctionEntry* fe = new CFunctionEntry(script_context, 0, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), TYPE_void, 1, 0);
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());
        GetContext()->AddParameter("_p4", Hash("_p4"), GetRegisteredType(GetTypeID<T4>()), 1, GetTypeID<T4>());
        GetContext()->AddParameter("_p5", Hash("_p5"), GetRegisteredType(GetTypeID<T5>()), 1, GetTypeID<T5>());
        GetContext()->AddParameter("_p6", Hash("_p6"), GetRegisteredType(GetTypeID<T6>()), 1, GetTypeID<T6>());
        GetContext()->AddParameter("_p7", Hash("_p7"), GetRegisteredType(GetTypeID<T7>()), 1, GetTypeID<T7>());
        GetContext()->AddParameter("_p8", Hash("_p8"), GetRegisteredType(GetTypeID<T8>()), 1, GetTypeID<T8>());
        GetContext()->AddParameter("_p9", Hash("_p9"), GetRegisteredType(GetTypeID<T9>()), 1, GetTypeID<T9>());

        uint32 hash = fe->GetHash();
        tFuncTable* globalfunctable = script_context->FindNamespace(0)->GetFuncTable();
        globalfunctable->AddItem(*fe, hash);
    }

private:
    funcsignature funcptr;
};

// -- class CRegisterMethod<9, R(Args...)> ----------------------------------------

template<typename C, typename R, typename... Args>
class CRegisterMethod<9, C, R(Args...)> : public CRegFunctionBase
{
public:

    using argument_types = std::tuple<Args...>;
    typedef R (C::*methodsignature)(Args...);

    // -- CRegisterMethod
    CRegisterMethod(const char* _methodname, methodsignature _methodptr) :
                  CRegFunctionBase(_methodname) {
        methodptr = _methodptr;
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr)
    {

        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);
        CVariableEntry* ve4 = GetContext()->GetParameter(4);
        CVariableEntry* ve5 = GetContext()->GetParameter(5);
        CVariableEntry* ve6 = GetContext()->GetParameter(6);
        CVariableEntry* ve7 = GetContext()->GetParameter(7);
        CVariableEntry* ve8 = GetContext()->GetParameter(8);
        CVariableEntry* ve9 = GetContext()->GetParameter(9);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);
        T4 p4 = ConvertVariableForDispatch<T4>(ve4);
        T5 p5 = ConvertVariableForDispatch<T5>(ve5);
        T6 p6 = ConvertVariableForDispatch<T6>(ve6);
        T7 p7 = ConvertVariableForDispatch<T7>(ve7);
        T8 p8 = ConvertVariableForDispatch<T8>(ve8);
        T9 p9 = ConvertVariableForDispatch<T9>(ve9);
        Dispatch(objaddr, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9);
    }

    // -- dispatch method
    R Dispatch(void* objaddr, void* _p1, void* _p2, void* _p3, void* _p4, void* _p5, void* _p6, void* _p7, void* _p8, void* _p9)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;
        T4* p4 = (T4*)_p4;
        T5* p5 = (T5*)_p5;
        T6* p6 = (T6*)_p6;
        T7* p7 = (T7*)_p7;
        T8* p8 = (T8*)_p8;
        T9* p9 = (T9*)_p9;

        C* object = (C*)(objaddr);
        R r = (object->*methodptr)(*p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8, *p9);

        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = this->GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual void Register(TinScript::CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;

        uint32 classname_hash = Hash(C::_GetClassName());
        CFunctionEntry* fe = new CFunctionEntry(script_context, classname_hash, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), GetRegisteredType(GetTypeID<R>()), 1, GetTypeID<R>());
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());
        GetContext()->AddParameter("_p4", Hash("_p4"), GetRegisteredType(GetTypeID<T4>()), 1, GetTypeID<T4>());
        GetContext()->AddParameter("_p5", Hash("_p5"), GetRegisteredType(GetTypeID<T5>()), 1, GetTypeID<T5>());
        GetContext()->AddParameter("_p6", Hash("_p6"), GetRegisteredType(GetTypeID<T6>()), 1, GetTypeID<T6>());
        GetContext()->AddParameter("_p7", Hash("_p7"), GetRegisteredType(GetTypeID<T7>()), 1, GetTypeID<T7>());
        GetContext()->AddParameter("_p8", Hash("_p8"), GetRegisteredType(GetTypeID<T8>()), 1, GetTypeID<T8>());
        GetContext()->AddParameter("_p9", Hash("_p9"), GetRegisteredType(GetTypeID<T9>()), 1, GetTypeID<T9>());

        uint32 hash = fe->GetHash();
        tFuncTable* methodtable = script_context->FindNamespace(classname_hash)->GetFuncTable();
        methodtable->AddItem(*fe, hash);
    }

private:
    methodsignature methodptr;
};
// -- class CRegisterMethod<9, void(Args...)> ----------------------------------------

template<typename C, typename... Args>
class CRegisterMethod<9, C, void(Args...)> : public CRegFunctionBase
{
public:

    using argument_types = std::tuple<Args...>;
    typedef void (C::*methodsignature)(Args...);

    // -- CRegisterMethod
    CRegisterMethod(const char* _methodname, methodsignature _methodptr) :
                  CRegFunctionBase(_methodname) {
        methodptr = _methodptr;
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr)
    {

        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);
        CVariableEntry* ve4 = GetContext()->GetParameter(4);
        CVariableEntry* ve5 = GetContext()->GetParameter(5);
        CVariableEntry* ve6 = GetContext()->GetParameter(6);
        CVariableEntry* ve7 = GetContext()->GetParameter(7);
        CVariableEntry* ve8 = GetContext()->GetParameter(8);
        CVariableEntry* ve9 = GetContext()->GetParameter(9);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);
        T4 p4 = ConvertVariableForDispatch<T4>(ve4);
        T5 p5 = ConvertVariableForDispatch<T5>(ve5);
        T6 p6 = ConvertVariableForDispatch<T6>(ve6);
        T7 p7 = ConvertVariableForDispatch<T7>(ve7);
        T8 p8 = ConvertVariableForDispatch<T8>(ve8);
        T9 p9 = ConvertVariableForDispatch<T9>(ve9);

        Dispatch(objaddr, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9);
    }

    // -- dispatch method
    void Dispatch(void* objaddr, void* _p1, void* _p2, void* _p3, void* _p4, void* _p5, void* _p6, void* _p7, void* _p8, void* _p9)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;
        T4* p4 = (T4*)_p4;
        T5* p5 = (T5*)_p5;
        T6* p6 = (T6*)_p6;
        T7* p7 = (T7*)_p7;
        T8* p8 = (T8*)_p8;
        T9* p9 = (T9*)_p9;

        C* object = (C*)(objaddr);
        (object->*methodptr)(*p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8, *p9);

    }

    // -- registration method
    virtual void Register(TinScript::CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;


        uint32 classname_hash = Hash(C::_GetClassName());
        CFunctionEntry* fe = new CFunctionEntry(script_context, classname_hash, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), TYPE_void, 1, 0);
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());
        GetContext()->AddParameter("_p4", Hash("_p4"), GetRegisteredType(GetTypeID<T4>()), 1, GetTypeID<T4>());
        GetContext()->AddParameter("_p5", Hash("_p5"), GetRegisteredType(GetTypeID<T5>()), 1, GetTypeID<T5>());
        GetContext()->AddParameter("_p6", Hash("_p6"), GetRegisteredType(GetTypeID<T6>()), 1, GetTypeID<T6>());
        GetContext()->AddParameter("_p7", Hash("_p7"), GetRegisteredType(GetTypeID<T7>()), 1, GetTypeID<T7>());
        GetContext()->AddParameter("_p8", Hash("_p8"), GetRegisteredType(GetTypeID<T8>()), 1, GetTypeID<T8>());
        GetContext()->AddParameter("_p9", Hash("_p9"), GetRegisteredType(GetTypeID<T9>()), 1, GetTypeID<T9>());


        uint32 hash = fe->GetHash();
        tFuncTable* methodtable = script_context->FindNamespace(classname_hash)->GetFuncTable();
        methodtable->AddItem(*fe, hash);
    }

private:
    methodsignature methodptr;
};

// -------------------
// Parameter count: 10
// -------------------

// -- class CRegisterFunction<10, R(Args...)> ----------------------------------------

template<typename R, typename... Args>
class CRegisterFunction<10, R(Args...)> : public CRegFunctionBase
{
public:

    typedef R (*funcsignature)(Args...);
    using argument_types = std::tuple<Args...>;

    // -- constructor
    CRegisterFunction(const char* _funcname, funcsignature _funcptr)
        : CRegFunctionBase(_funcname)
    {
        funcptr = _funcptr;
    }
    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;
        using T10 = std::tuple_element<9, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);
        CVariableEntry* ve4 = GetContext()->GetParameter(4);
        CVariableEntry* ve5 = GetContext()->GetParameter(5);
        CVariableEntry* ve6 = GetContext()->GetParameter(6);
        CVariableEntry* ve7 = GetContext()->GetParameter(7);
        CVariableEntry* ve8 = GetContext()->GetParameter(8);
        CVariableEntry* ve9 = GetContext()->GetParameter(9);
        CVariableEntry* ve10 = GetContext()->GetParameter(10);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);
        T4 p4 = ConvertVariableForDispatch<T4>(ve4);
        T5 p5 = ConvertVariableForDispatch<T5>(ve5);
        T6 p6 = ConvertVariableForDispatch<T6>(ve6);
        T7 p7 = ConvertVariableForDispatch<T7>(ve7);
        T8 p8 = ConvertVariableForDispatch<T8>(ve8);
        T9 p9 = ConvertVariableForDispatch<T9>(ve9);
        T10 p10 = ConvertVariableForDispatch<T10>(ve10);

        Dispatch(&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10);
    }

    // -- dispatch method
    R Dispatch(void* _p1, void* _p2, void* _p3, void* _p4, void* _p5, void* _p6, void* _p7, void* _p8, void* _p9, void* _p10)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;
        using T10 = std::tuple_element<9, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;
        T4* p4 = (T4*)_p4;
        T5* p5 = (T5*)_p5;
        T6* p6 = (T6*)_p6;
        T7* p7 = (T7*)_p7;
        T8* p8 = (T8*)_p8;
        T9* p9 = (T9*)_p9;
        T10* p10 = (T10*)_p10;

        R r = funcptr(*p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8, *p9, *p10);

        assert(GetContext()->GetParameter(0));
        CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual void Register(CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;
        using T10 = std::tuple_element<9, argument_types>::type;

        CFunctionEntry* fe = new CFunctionEntry(script_context, 0, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), GetRegisteredType(GetTypeID<R>()), 1, GetTypeID<R>());
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());
        GetContext()->AddParameter("_p4", Hash("_p4"), GetRegisteredType(GetTypeID<T4>()), 1, GetTypeID<T4>());
        GetContext()->AddParameter("_p5", Hash("_p5"), GetRegisteredType(GetTypeID<T5>()), 1, GetTypeID<T5>());
        GetContext()->AddParameter("_p6", Hash("_p6"), GetRegisteredType(GetTypeID<T6>()), 1, GetTypeID<T6>());
        GetContext()->AddParameter("_p7", Hash("_p7"), GetRegisteredType(GetTypeID<T7>()), 1, GetTypeID<T7>());
        GetContext()->AddParameter("_p8", Hash("_p8"), GetRegisteredType(GetTypeID<T8>()), 1, GetTypeID<T8>());
        GetContext()->AddParameter("_p9", Hash("_p9"), GetRegisteredType(GetTypeID<T9>()), 1, GetTypeID<T9>());
        GetContext()->AddParameter("_p10", Hash("_p10"), GetRegisteredType(GetTypeID<T10>()), 1, GetTypeID<T10>());

        uint32 hash = fe->GetHash();
        tFuncTable* globalfunctable = script_context->FindNamespace(0)->GetFuncTable();
        globalfunctable->AddItem(*fe, hash);
    }

private:
    funcsignature funcptr;
};

// -- class CRegisterFunction<10, void(Args...)> ----------------------------------------

template<typename... Args>
class CRegisterFunction<10, void(Args...)> : public CRegFunctionBase
{
public:

    typedef void (*funcsignature)(Args...);
    using argument_types = std::tuple<Args...>;

    // -- constructor
    CRegisterFunction(const char* _funcname, funcsignature _funcptr)
        : CRegFunctionBase(_funcname)
    {
        funcptr = _funcptr;
    }
    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;
        using T10 = std::tuple_element<9, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);
        CVariableEntry* ve4 = GetContext()->GetParameter(4);
        CVariableEntry* ve5 = GetContext()->GetParameter(5);
        CVariableEntry* ve6 = GetContext()->GetParameter(6);
        CVariableEntry* ve7 = GetContext()->GetParameter(7);
        CVariableEntry* ve8 = GetContext()->GetParameter(8);
        CVariableEntry* ve9 = GetContext()->GetParameter(9);
        CVariableEntry* ve10 = GetContext()->GetParameter(10);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);
        T4 p4 = ConvertVariableForDispatch<T4>(ve4);
        T5 p5 = ConvertVariableForDispatch<T5>(ve5);
        T6 p6 = ConvertVariableForDispatch<T6>(ve6);
        T7 p7 = ConvertVariableForDispatch<T7>(ve7);
        T8 p8 = ConvertVariableForDispatch<T8>(ve8);
        T9 p9 = ConvertVariableForDispatch<T9>(ve9);
        T10 p10 = ConvertVariableForDispatch<T10>(ve10);

        Dispatch(&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10);
    }

    // -- dispatch method
    void Dispatch(void* _p1, void* _p2, void* _p3, void* _p4, void* _p5, void* _p6, void* _p7, void* _p8, void* _p9, void* _p10)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;
        using T10 = std::tuple_element<9, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;
        T4* p4 = (T4*)_p4;
        T5* p5 = (T5*)_p5;
        T6* p6 = (T6*)_p6;
        T7* p7 = (T7*)_p7;
        T8* p8 = (T8*)_p8;
        T9* p9 = (T9*)_p9;
        T10* p10 = (T10*)_p10;

        funcptr(*p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8, *p9, *p10);

    }

    // -- registration method
    virtual void Register(CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;
        using T10 = std::tuple_element<9, argument_types>::type;

        CFunctionEntry* fe = new CFunctionEntry(script_context, 0, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), TYPE_void, 1, 0);
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());
        GetContext()->AddParameter("_p4", Hash("_p4"), GetRegisteredType(GetTypeID<T4>()), 1, GetTypeID<T4>());
        GetContext()->AddParameter("_p5", Hash("_p5"), GetRegisteredType(GetTypeID<T5>()), 1, GetTypeID<T5>());
        GetContext()->AddParameter("_p6", Hash("_p6"), GetRegisteredType(GetTypeID<T6>()), 1, GetTypeID<T6>());
        GetContext()->AddParameter("_p7", Hash("_p7"), GetRegisteredType(GetTypeID<T7>()), 1, GetTypeID<T7>());
        GetContext()->AddParameter("_p8", Hash("_p8"), GetRegisteredType(GetTypeID<T8>()), 1, GetTypeID<T8>());
        GetContext()->AddParameter("_p9", Hash("_p9"), GetRegisteredType(GetTypeID<T9>()), 1, GetTypeID<T9>());
        GetContext()->AddParameter("_p10", Hash("_p10"), GetRegisteredType(GetTypeID<T10>()), 1, GetTypeID<T10>());

        uint32 hash = fe->GetHash();
        tFuncTable* globalfunctable = script_context->FindNamespace(0)->GetFuncTable();
        globalfunctable->AddItem(*fe, hash);
    }

private:
    funcsignature funcptr;
};

// -- class CRegisterMethod<10, R(Args...)> ----------------------------------------

template<typename C, typename R, typename... Args>
class CRegisterMethod<10, C, R(Args...)> : public CRegFunctionBase
{
public:

    using argument_types = std::tuple<Args...>;
    typedef R (C::*methodsignature)(Args...);

    // -- CRegisterMethod
    CRegisterMethod(const char* _methodname, methodsignature _methodptr) :
                  CRegFunctionBase(_methodname) {
        methodptr = _methodptr;
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr)
    {

        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;
        using T10 = std::tuple_element<9, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);
        CVariableEntry* ve4 = GetContext()->GetParameter(4);
        CVariableEntry* ve5 = GetContext()->GetParameter(5);
        CVariableEntry* ve6 = GetContext()->GetParameter(6);
        CVariableEntry* ve7 = GetContext()->GetParameter(7);
        CVariableEntry* ve8 = GetContext()->GetParameter(8);
        CVariableEntry* ve9 = GetContext()->GetParameter(9);
        CVariableEntry* ve10 = GetContext()->GetParameter(10);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);
        T4 p4 = ConvertVariableForDispatch<T4>(ve4);
        T5 p5 = ConvertVariableForDispatch<T5>(ve5);
        T6 p6 = ConvertVariableForDispatch<T6>(ve6);
        T7 p7 = ConvertVariableForDispatch<T7>(ve7);
        T8 p8 = ConvertVariableForDispatch<T8>(ve8);
        T9 p9 = ConvertVariableForDispatch<T9>(ve9);
        T10 p10 = ConvertVariableForDispatch<T10>(ve10);
        Dispatch(objaddr, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10);
    }

    // -- dispatch method
    R Dispatch(void* objaddr, void* _p1, void* _p2, void* _p3, void* _p4, void* _p5, void* _p6, void* _p7, void* _p8, void* _p9, void* _p10)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;
        using T10 = std::tuple_element<9, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;
        T4* p4 = (T4*)_p4;
        T5* p5 = (T5*)_p5;
        T6* p6 = (T6*)_p6;
        T7* p7 = (T7*)_p7;
        T8* p8 = (T8*)_p8;
        T9* p9 = (T9*)_p9;
        T10* p10 = (T10*)_p10;

        C* object = (C*)(objaddr);
        R r = (object->*methodptr)(*p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8, *p9, *p10);

        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = this->GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual void Register(TinScript::CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;
        using T10 = std::tuple_element<9, argument_types>::type;

        uint32 classname_hash = Hash(C::_GetClassName());
        CFunctionEntry* fe = new CFunctionEntry(script_context, classname_hash, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), GetRegisteredType(GetTypeID<R>()), 1, GetTypeID<R>());
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());
        GetContext()->AddParameter("_p4", Hash("_p4"), GetRegisteredType(GetTypeID<T4>()), 1, GetTypeID<T4>());
        GetContext()->AddParameter("_p5", Hash("_p5"), GetRegisteredType(GetTypeID<T5>()), 1, GetTypeID<T5>());
        GetContext()->AddParameter("_p6", Hash("_p6"), GetRegisteredType(GetTypeID<T6>()), 1, GetTypeID<T6>());
        GetContext()->AddParameter("_p7", Hash("_p7"), GetRegisteredType(GetTypeID<T7>()), 1, GetTypeID<T7>());
        GetContext()->AddParameter("_p8", Hash("_p8"), GetRegisteredType(GetTypeID<T8>()), 1, GetTypeID<T8>());
        GetContext()->AddParameter("_p9", Hash("_p9"), GetRegisteredType(GetTypeID<T9>()), 1, GetTypeID<T9>());
        GetContext()->AddParameter("_p10", Hash("_p10"), GetRegisteredType(GetTypeID<T10>()), 1, GetTypeID<T10>());

        uint32 hash = fe->GetHash();
        tFuncTable* methodtable = script_context->FindNamespace(classname_hash)->GetFuncTable();
        methodtable->AddItem(*fe, hash);
    }

private:
    methodsignature methodptr;
};
// -- class CRegisterMethod<10, void(Args...)> ----------------------------------------

template<typename C, typename... Args>
class CRegisterMethod<10, C, void(Args...)> : public CRegFunctionBase
{
public:

    using argument_types = std::tuple<Args...>;
    typedef void (C::*methodsignature)(Args...);

    // -- CRegisterMethod
    CRegisterMethod(const char* _methodname, methodsignature _methodptr) :
                  CRegFunctionBase(_methodname) {
        methodptr = _methodptr;
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr)
    {

        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;
        using T10 = std::tuple_element<9, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);
        CVariableEntry* ve4 = GetContext()->GetParameter(4);
        CVariableEntry* ve5 = GetContext()->GetParameter(5);
        CVariableEntry* ve6 = GetContext()->GetParameter(6);
        CVariableEntry* ve7 = GetContext()->GetParameter(7);
        CVariableEntry* ve8 = GetContext()->GetParameter(8);
        CVariableEntry* ve9 = GetContext()->GetParameter(9);
        CVariableEntry* ve10 = GetContext()->GetParameter(10);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);
        T4 p4 = ConvertVariableForDispatch<T4>(ve4);
        T5 p5 = ConvertVariableForDispatch<T5>(ve5);
        T6 p6 = ConvertVariableForDispatch<T6>(ve6);
        T7 p7 = ConvertVariableForDispatch<T7>(ve7);
        T8 p8 = ConvertVariableForDispatch<T8>(ve8);
        T9 p9 = ConvertVariableForDispatch<T9>(ve9);
        T10 p10 = ConvertVariableForDispatch<T10>(ve10);

        Dispatch(objaddr, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10);
    }

    // -- dispatch method
    void Dispatch(void* objaddr, void* _p1, void* _p2, void* _p3, void* _p4, void* _p5, void* _p6, void* _p7, void* _p8, void* _p9, void* _p10)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;
        using T10 = std::tuple_element<9, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;
        T4* p4 = (T4*)_p4;
        T5* p5 = (T5*)_p5;
        T6* p6 = (T6*)_p6;
        T7* p7 = (T7*)_p7;
        T8* p8 = (T8*)_p8;
        T9* p9 = (T9*)_p9;
        T10* p10 = (T10*)_p10;

        C* object = (C*)(objaddr);
        (object->*methodptr)(*p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8, *p9, *p10);

    }

    // -- registration method
    virtual void Register(TinScript::CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;
        using T10 = std::tuple_element<9, argument_types>::type;


        uint32 classname_hash = Hash(C::_GetClassName());
        CFunctionEntry* fe = new CFunctionEntry(script_context, classname_hash, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), TYPE_void, 1, 0);
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());
        GetContext()->AddParameter("_p4", Hash("_p4"), GetRegisteredType(GetTypeID<T4>()), 1, GetTypeID<T4>());
        GetContext()->AddParameter("_p5", Hash("_p5"), GetRegisteredType(GetTypeID<T5>()), 1, GetTypeID<T5>());
        GetContext()->AddParameter("_p6", Hash("_p6"), GetRegisteredType(GetTypeID<T6>()), 1, GetTypeID<T6>());
        GetContext()->AddParameter("_p7", Hash("_p7"), GetRegisteredType(GetTypeID<T7>()), 1, GetTypeID<T7>());
        GetContext()->AddParameter("_p8", Hash("_p8"), GetRegisteredType(GetTypeID<T8>()), 1, GetTypeID<T8>());
        GetContext()->AddParameter("_p9", Hash("_p9"), GetRegisteredType(GetTypeID<T9>()), 1, GetTypeID<T9>());
        GetContext()->AddParameter("_p10", Hash("_p10"), GetRegisteredType(GetTypeID<T10>()), 1, GetTypeID<T10>());


        uint32 hash = fe->GetHash();
        tFuncTable* methodtable = script_context->FindNamespace(classname_hash)->GetFuncTable();
        methodtable->AddItem(*fe, hash);
    }

private:
    methodsignature methodptr;
};

// -------------------
// Parameter count: 11
// -------------------

// -- class CRegisterFunction<11, R(Args...)> ----------------------------------------

template<typename R, typename... Args>
class CRegisterFunction<11, R(Args...)> : public CRegFunctionBase
{
public:

    typedef R (*funcsignature)(Args...);
    using argument_types = std::tuple<Args...>;

    // -- constructor
    CRegisterFunction(const char* _funcname, funcsignature _funcptr)
        : CRegFunctionBase(_funcname)
    {
        funcptr = _funcptr;
    }
    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;
        using T10 = std::tuple_element<9, argument_types>::type;
        using T11 = std::tuple_element<10, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);
        CVariableEntry* ve4 = GetContext()->GetParameter(4);
        CVariableEntry* ve5 = GetContext()->GetParameter(5);
        CVariableEntry* ve6 = GetContext()->GetParameter(6);
        CVariableEntry* ve7 = GetContext()->GetParameter(7);
        CVariableEntry* ve8 = GetContext()->GetParameter(8);
        CVariableEntry* ve9 = GetContext()->GetParameter(9);
        CVariableEntry* ve10 = GetContext()->GetParameter(10);
        CVariableEntry* ve11 = GetContext()->GetParameter(11);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);
        T4 p4 = ConvertVariableForDispatch<T4>(ve4);
        T5 p5 = ConvertVariableForDispatch<T5>(ve5);
        T6 p6 = ConvertVariableForDispatch<T6>(ve6);
        T7 p7 = ConvertVariableForDispatch<T7>(ve7);
        T8 p8 = ConvertVariableForDispatch<T8>(ve8);
        T9 p9 = ConvertVariableForDispatch<T9>(ve9);
        T10 p10 = ConvertVariableForDispatch<T10>(ve10);
        T11 p11 = ConvertVariableForDispatch<T11>(ve11);

        Dispatch(&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10, &p11);
    }

    // -- dispatch method
    R Dispatch(void* _p1, void* _p2, void* _p3, void* _p4, void* _p5, void* _p6, void* _p7, void* _p8, void* _p9, void* _p10, void* _p11)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;
        using T10 = std::tuple_element<9, argument_types>::type;
        using T11 = std::tuple_element<10, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;
        T4* p4 = (T4*)_p4;
        T5* p5 = (T5*)_p5;
        T6* p6 = (T6*)_p6;
        T7* p7 = (T7*)_p7;
        T8* p8 = (T8*)_p8;
        T9* p9 = (T9*)_p9;
        T10* p10 = (T10*)_p10;
        T11* p11 = (T11*)_p11;

        R r = funcptr(*p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8, *p9, *p10, *p11);

        assert(GetContext()->GetParameter(0));
        CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual void Register(CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;
        using T10 = std::tuple_element<9, argument_types>::type;
        using T11 = std::tuple_element<10, argument_types>::type;

        CFunctionEntry* fe = new CFunctionEntry(script_context, 0, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), GetRegisteredType(GetTypeID<R>()), 1, GetTypeID<R>());
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());
        GetContext()->AddParameter("_p4", Hash("_p4"), GetRegisteredType(GetTypeID<T4>()), 1, GetTypeID<T4>());
        GetContext()->AddParameter("_p5", Hash("_p5"), GetRegisteredType(GetTypeID<T5>()), 1, GetTypeID<T5>());
        GetContext()->AddParameter("_p6", Hash("_p6"), GetRegisteredType(GetTypeID<T6>()), 1, GetTypeID<T6>());
        GetContext()->AddParameter("_p7", Hash("_p7"), GetRegisteredType(GetTypeID<T7>()), 1, GetTypeID<T7>());
        GetContext()->AddParameter("_p8", Hash("_p8"), GetRegisteredType(GetTypeID<T8>()), 1, GetTypeID<T8>());
        GetContext()->AddParameter("_p9", Hash("_p9"), GetRegisteredType(GetTypeID<T9>()), 1, GetTypeID<T9>());
        GetContext()->AddParameter("_p10", Hash("_p10"), GetRegisteredType(GetTypeID<T10>()), 1, GetTypeID<T10>());
        GetContext()->AddParameter("_p11", Hash("_p11"), GetRegisteredType(GetTypeID<T11>()), 1, GetTypeID<T11>());

        uint32 hash = fe->GetHash();
        tFuncTable* globalfunctable = script_context->FindNamespace(0)->GetFuncTable();
        globalfunctable->AddItem(*fe, hash);
    }

private:
    funcsignature funcptr;
};

// -- class CRegisterFunction<11, void(Args...)> ----------------------------------------

template<typename... Args>
class CRegisterFunction<11, void(Args...)> : public CRegFunctionBase
{
public:

    typedef void (*funcsignature)(Args...);
    using argument_types = std::tuple<Args...>;

    // -- constructor
    CRegisterFunction(const char* _funcname, funcsignature _funcptr)
        : CRegFunctionBase(_funcname)
    {
        funcptr = _funcptr;
    }
    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;
        using T10 = std::tuple_element<9, argument_types>::type;
        using T11 = std::tuple_element<10, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);
        CVariableEntry* ve4 = GetContext()->GetParameter(4);
        CVariableEntry* ve5 = GetContext()->GetParameter(5);
        CVariableEntry* ve6 = GetContext()->GetParameter(6);
        CVariableEntry* ve7 = GetContext()->GetParameter(7);
        CVariableEntry* ve8 = GetContext()->GetParameter(8);
        CVariableEntry* ve9 = GetContext()->GetParameter(9);
        CVariableEntry* ve10 = GetContext()->GetParameter(10);
        CVariableEntry* ve11 = GetContext()->GetParameter(11);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);
        T4 p4 = ConvertVariableForDispatch<T4>(ve4);
        T5 p5 = ConvertVariableForDispatch<T5>(ve5);
        T6 p6 = ConvertVariableForDispatch<T6>(ve6);
        T7 p7 = ConvertVariableForDispatch<T7>(ve7);
        T8 p8 = ConvertVariableForDispatch<T8>(ve8);
        T9 p9 = ConvertVariableForDispatch<T9>(ve9);
        T10 p10 = ConvertVariableForDispatch<T10>(ve10);
        T11 p11 = ConvertVariableForDispatch<T11>(ve11);

        Dispatch(&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10, &p11);
    }

    // -- dispatch method
    void Dispatch(void* _p1, void* _p2, void* _p3, void* _p4, void* _p5, void* _p6, void* _p7, void* _p8, void* _p9, void* _p10, void* _p11)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;
        using T10 = std::tuple_element<9, argument_types>::type;
        using T11 = std::tuple_element<10, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;
        T4* p4 = (T4*)_p4;
        T5* p5 = (T5*)_p5;
        T6* p6 = (T6*)_p6;
        T7* p7 = (T7*)_p7;
        T8* p8 = (T8*)_p8;
        T9* p9 = (T9*)_p9;
        T10* p10 = (T10*)_p10;
        T11* p11 = (T11*)_p11;

        funcptr(*p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8, *p9, *p10, *p11);

    }

    // -- registration method
    virtual void Register(CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;
        using T10 = std::tuple_element<9, argument_types>::type;
        using T11 = std::tuple_element<10, argument_types>::type;

        CFunctionEntry* fe = new CFunctionEntry(script_context, 0, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), TYPE_void, 1, 0);
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());
        GetContext()->AddParameter("_p4", Hash("_p4"), GetRegisteredType(GetTypeID<T4>()), 1, GetTypeID<T4>());
        GetContext()->AddParameter("_p5", Hash("_p5"), GetRegisteredType(GetTypeID<T5>()), 1, GetTypeID<T5>());
        GetContext()->AddParameter("_p6", Hash("_p6"), GetRegisteredType(GetTypeID<T6>()), 1, GetTypeID<T6>());
        GetContext()->AddParameter("_p7", Hash("_p7"), GetRegisteredType(GetTypeID<T7>()), 1, GetTypeID<T7>());
        GetContext()->AddParameter("_p8", Hash("_p8"), GetRegisteredType(GetTypeID<T8>()), 1, GetTypeID<T8>());
        GetContext()->AddParameter("_p9", Hash("_p9"), GetRegisteredType(GetTypeID<T9>()), 1, GetTypeID<T9>());
        GetContext()->AddParameter("_p10", Hash("_p10"), GetRegisteredType(GetTypeID<T10>()), 1, GetTypeID<T10>());
        GetContext()->AddParameter("_p11", Hash("_p11"), GetRegisteredType(GetTypeID<T11>()), 1, GetTypeID<T11>());

        uint32 hash = fe->GetHash();
        tFuncTable* globalfunctable = script_context->FindNamespace(0)->GetFuncTable();
        globalfunctable->AddItem(*fe, hash);
    }

private:
    funcsignature funcptr;
};

// -- class CRegisterMethod<11, R(Args...)> ----------------------------------------

template<typename C, typename R, typename... Args>
class CRegisterMethod<11, C, R(Args...)> : public CRegFunctionBase
{
public:

    using argument_types = std::tuple<Args...>;
    typedef R (C::*methodsignature)(Args...);

    // -- CRegisterMethod
    CRegisterMethod(const char* _methodname, methodsignature _methodptr) :
                  CRegFunctionBase(_methodname) {
        methodptr = _methodptr;
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr)
    {

        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;
        using T10 = std::tuple_element<9, argument_types>::type;
        using T11 = std::tuple_element<10, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);
        CVariableEntry* ve4 = GetContext()->GetParameter(4);
        CVariableEntry* ve5 = GetContext()->GetParameter(5);
        CVariableEntry* ve6 = GetContext()->GetParameter(6);
        CVariableEntry* ve7 = GetContext()->GetParameter(7);
        CVariableEntry* ve8 = GetContext()->GetParameter(8);
        CVariableEntry* ve9 = GetContext()->GetParameter(9);
        CVariableEntry* ve10 = GetContext()->GetParameter(10);
        CVariableEntry* ve11 = GetContext()->GetParameter(11);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);
        T4 p4 = ConvertVariableForDispatch<T4>(ve4);
        T5 p5 = ConvertVariableForDispatch<T5>(ve5);
        T6 p6 = ConvertVariableForDispatch<T6>(ve6);
        T7 p7 = ConvertVariableForDispatch<T7>(ve7);
        T8 p8 = ConvertVariableForDispatch<T8>(ve8);
        T9 p9 = ConvertVariableForDispatch<T9>(ve9);
        T10 p10 = ConvertVariableForDispatch<T10>(ve10);
        T11 p11 = ConvertVariableForDispatch<T11>(ve11);
        Dispatch(objaddr, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10, &p11);
    }

    // -- dispatch method
    R Dispatch(void* objaddr, void* _p1, void* _p2, void* _p3, void* _p4, void* _p5, void* _p6, void* _p7, void* _p8, void* _p9, void* _p10, void* _p11)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;
        using T10 = std::tuple_element<9, argument_types>::type;
        using T11 = std::tuple_element<10, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;
        T4* p4 = (T4*)_p4;
        T5* p5 = (T5*)_p5;
        T6* p6 = (T6*)_p6;
        T7* p7 = (T7*)_p7;
        T8* p8 = (T8*)_p8;
        T9* p9 = (T9*)_p9;
        T10* p10 = (T10*)_p10;
        T11* p11 = (T11*)_p11;

        C* object = (C*)(objaddr);
        R r = (object->*methodptr)(*p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8, *p9, *p10, *p11);

        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = this->GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual void Register(TinScript::CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;
        using T10 = std::tuple_element<9, argument_types>::type;
        using T11 = std::tuple_element<10, argument_types>::type;

        uint32 classname_hash = Hash(C::_GetClassName());
        CFunctionEntry* fe = new CFunctionEntry(script_context, classname_hash, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), GetRegisteredType(GetTypeID<R>()), 1, GetTypeID<R>());
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());
        GetContext()->AddParameter("_p4", Hash("_p4"), GetRegisteredType(GetTypeID<T4>()), 1, GetTypeID<T4>());
        GetContext()->AddParameter("_p5", Hash("_p5"), GetRegisteredType(GetTypeID<T5>()), 1, GetTypeID<T5>());
        GetContext()->AddParameter("_p6", Hash("_p6"), GetRegisteredType(GetTypeID<T6>()), 1, GetTypeID<T6>());
        GetContext()->AddParameter("_p7", Hash("_p7"), GetRegisteredType(GetTypeID<T7>()), 1, GetTypeID<T7>());
        GetContext()->AddParameter("_p8", Hash("_p8"), GetRegisteredType(GetTypeID<T8>()), 1, GetTypeID<T8>());
        GetContext()->AddParameter("_p9", Hash("_p9"), GetRegisteredType(GetTypeID<T9>()), 1, GetTypeID<T9>());
        GetContext()->AddParameter("_p10", Hash("_p10"), GetRegisteredType(GetTypeID<T10>()), 1, GetTypeID<T10>());
        GetContext()->AddParameter("_p11", Hash("_p11"), GetRegisteredType(GetTypeID<T11>()), 1, GetTypeID<T11>());

        uint32 hash = fe->GetHash();
        tFuncTable* methodtable = script_context->FindNamespace(classname_hash)->GetFuncTable();
        methodtable->AddItem(*fe, hash);
    }

private:
    methodsignature methodptr;
};
// -- class CRegisterMethod<11, void(Args...)> ----------------------------------------

template<typename C, typename... Args>
class CRegisterMethod<11, C, void(Args...)> : public CRegFunctionBase
{
public:

    using argument_types = std::tuple<Args...>;
    typedef void (C::*methodsignature)(Args...);

    // -- CRegisterMethod
    CRegisterMethod(const char* _methodname, methodsignature _methodptr) :
                  CRegFunctionBase(_methodname) {
        methodptr = _methodptr;
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr)
    {

        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;
        using T10 = std::tuple_element<9, argument_types>::type;
        using T11 = std::tuple_element<10, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);
        CVariableEntry* ve4 = GetContext()->GetParameter(4);
        CVariableEntry* ve5 = GetContext()->GetParameter(5);
        CVariableEntry* ve6 = GetContext()->GetParameter(6);
        CVariableEntry* ve7 = GetContext()->GetParameter(7);
        CVariableEntry* ve8 = GetContext()->GetParameter(8);
        CVariableEntry* ve9 = GetContext()->GetParameter(9);
        CVariableEntry* ve10 = GetContext()->GetParameter(10);
        CVariableEntry* ve11 = GetContext()->GetParameter(11);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);
        T4 p4 = ConvertVariableForDispatch<T4>(ve4);
        T5 p5 = ConvertVariableForDispatch<T5>(ve5);
        T6 p6 = ConvertVariableForDispatch<T6>(ve6);
        T7 p7 = ConvertVariableForDispatch<T7>(ve7);
        T8 p8 = ConvertVariableForDispatch<T8>(ve8);
        T9 p9 = ConvertVariableForDispatch<T9>(ve9);
        T10 p10 = ConvertVariableForDispatch<T10>(ve10);
        T11 p11 = ConvertVariableForDispatch<T11>(ve11);

        Dispatch(objaddr, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10, &p11);
    }

    // -- dispatch method
    void Dispatch(void* objaddr, void* _p1, void* _p2, void* _p3, void* _p4, void* _p5, void* _p6, void* _p7, void* _p8, void* _p9, void* _p10, void* _p11)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;
        using T10 = std::tuple_element<9, argument_types>::type;
        using T11 = std::tuple_element<10, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;
        T4* p4 = (T4*)_p4;
        T5* p5 = (T5*)_p5;
        T6* p6 = (T6*)_p6;
        T7* p7 = (T7*)_p7;
        T8* p8 = (T8*)_p8;
        T9* p9 = (T9*)_p9;
        T10* p10 = (T10*)_p10;
        T11* p11 = (T11*)_p11;

        C* object = (C*)(objaddr);
        (object->*methodptr)(*p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8, *p9, *p10, *p11);

    }

    // -- registration method
    virtual void Register(TinScript::CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;
        using T10 = std::tuple_element<9, argument_types>::type;
        using T11 = std::tuple_element<10, argument_types>::type;


        uint32 classname_hash = Hash(C::_GetClassName());
        CFunctionEntry* fe = new CFunctionEntry(script_context, classname_hash, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), TYPE_void, 1, 0);
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());
        GetContext()->AddParameter("_p4", Hash("_p4"), GetRegisteredType(GetTypeID<T4>()), 1, GetTypeID<T4>());
        GetContext()->AddParameter("_p5", Hash("_p5"), GetRegisteredType(GetTypeID<T5>()), 1, GetTypeID<T5>());
        GetContext()->AddParameter("_p6", Hash("_p6"), GetRegisteredType(GetTypeID<T6>()), 1, GetTypeID<T6>());
        GetContext()->AddParameter("_p7", Hash("_p7"), GetRegisteredType(GetTypeID<T7>()), 1, GetTypeID<T7>());
        GetContext()->AddParameter("_p8", Hash("_p8"), GetRegisteredType(GetTypeID<T8>()), 1, GetTypeID<T8>());
        GetContext()->AddParameter("_p9", Hash("_p9"), GetRegisteredType(GetTypeID<T9>()), 1, GetTypeID<T9>());
        GetContext()->AddParameter("_p10", Hash("_p10"), GetRegisteredType(GetTypeID<T10>()), 1, GetTypeID<T10>());
        GetContext()->AddParameter("_p11", Hash("_p11"), GetRegisteredType(GetTypeID<T11>()), 1, GetTypeID<T11>());


        uint32 hash = fe->GetHash();
        tFuncTable* methodtable = script_context->FindNamespace(classname_hash)->GetFuncTable();
        methodtable->AddItem(*fe, hash);
    }

private:
    methodsignature methodptr;
};

// -------------------
// Parameter count: 12
// -------------------

// -- class CRegisterFunction<12, R(Args...)> ----------------------------------------

template<typename R, typename... Args>
class CRegisterFunction<12, R(Args...)> : public CRegFunctionBase
{
public:

    typedef R (*funcsignature)(Args...);
    using argument_types = std::tuple<Args...>;

    // -- constructor
    CRegisterFunction(const char* _funcname, funcsignature _funcptr)
        : CRegFunctionBase(_funcname)
    {
        funcptr = _funcptr;
    }
    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;
        using T10 = std::tuple_element<9, argument_types>::type;
        using T11 = std::tuple_element<10, argument_types>::type;
        using T12 = std::tuple_element<11, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);
        CVariableEntry* ve4 = GetContext()->GetParameter(4);
        CVariableEntry* ve5 = GetContext()->GetParameter(5);
        CVariableEntry* ve6 = GetContext()->GetParameter(6);
        CVariableEntry* ve7 = GetContext()->GetParameter(7);
        CVariableEntry* ve8 = GetContext()->GetParameter(8);
        CVariableEntry* ve9 = GetContext()->GetParameter(9);
        CVariableEntry* ve10 = GetContext()->GetParameter(10);
        CVariableEntry* ve11 = GetContext()->GetParameter(11);
        CVariableEntry* ve12 = GetContext()->GetParameter(12);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);
        T4 p4 = ConvertVariableForDispatch<T4>(ve4);
        T5 p5 = ConvertVariableForDispatch<T5>(ve5);
        T6 p6 = ConvertVariableForDispatch<T6>(ve6);
        T7 p7 = ConvertVariableForDispatch<T7>(ve7);
        T8 p8 = ConvertVariableForDispatch<T8>(ve8);
        T9 p9 = ConvertVariableForDispatch<T9>(ve9);
        T10 p10 = ConvertVariableForDispatch<T10>(ve10);
        T11 p11 = ConvertVariableForDispatch<T11>(ve11);
        T12 p12 = ConvertVariableForDispatch<T12>(ve12);

        Dispatch(&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10, &p11, &p12);
    }

    // -- dispatch method
    R Dispatch(void* _p1, void* _p2, void* _p3, void* _p4, void* _p5, void* _p6, void* _p7, void* _p8, void* _p9, void* _p10, void* _p11, void* _p12)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;
        using T10 = std::tuple_element<9, argument_types>::type;
        using T11 = std::tuple_element<10, argument_types>::type;
        using T12 = std::tuple_element<11, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;
        T4* p4 = (T4*)_p4;
        T5* p5 = (T5*)_p5;
        T6* p6 = (T6*)_p6;
        T7* p7 = (T7*)_p7;
        T8* p8 = (T8*)_p8;
        T9* p9 = (T9*)_p9;
        T10* p10 = (T10*)_p10;
        T11* p11 = (T11*)_p11;
        T12* p12 = (T12*)_p12;

        R r = funcptr(*p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8, *p9, *p10, *p11, *p12);

        assert(GetContext()->GetParameter(0));
        CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual void Register(CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;
        using T10 = std::tuple_element<9, argument_types>::type;
        using T11 = std::tuple_element<10, argument_types>::type;
        using T12 = std::tuple_element<11, argument_types>::type;

        CFunctionEntry* fe = new CFunctionEntry(script_context, 0, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), GetRegisteredType(GetTypeID<R>()), 1, GetTypeID<R>());
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());
        GetContext()->AddParameter("_p4", Hash("_p4"), GetRegisteredType(GetTypeID<T4>()), 1, GetTypeID<T4>());
        GetContext()->AddParameter("_p5", Hash("_p5"), GetRegisteredType(GetTypeID<T5>()), 1, GetTypeID<T5>());
        GetContext()->AddParameter("_p6", Hash("_p6"), GetRegisteredType(GetTypeID<T6>()), 1, GetTypeID<T6>());
        GetContext()->AddParameter("_p7", Hash("_p7"), GetRegisteredType(GetTypeID<T7>()), 1, GetTypeID<T7>());
        GetContext()->AddParameter("_p8", Hash("_p8"), GetRegisteredType(GetTypeID<T8>()), 1, GetTypeID<T8>());
        GetContext()->AddParameter("_p9", Hash("_p9"), GetRegisteredType(GetTypeID<T9>()), 1, GetTypeID<T9>());
        GetContext()->AddParameter("_p10", Hash("_p10"), GetRegisteredType(GetTypeID<T10>()), 1, GetTypeID<T10>());
        GetContext()->AddParameter("_p11", Hash("_p11"), GetRegisteredType(GetTypeID<T11>()), 1, GetTypeID<T11>());
        GetContext()->AddParameter("_p12", Hash("_p12"), GetRegisteredType(GetTypeID<T12>()), 1, GetTypeID<T12>());

        uint32 hash = fe->GetHash();
        tFuncTable* globalfunctable = script_context->FindNamespace(0)->GetFuncTable();
        globalfunctable->AddItem(*fe, hash);
    }

private:
    funcsignature funcptr;
};

// -- class CRegisterFunction<12, void(Args...)> ----------------------------------------

template<typename... Args>
class CRegisterFunction<12, void(Args...)> : public CRegFunctionBase
{
public:

    typedef void (*funcsignature)(Args...);
    using argument_types = std::tuple<Args...>;

    // -- constructor
    CRegisterFunction(const char* _funcname, funcsignature _funcptr)
        : CRegFunctionBase(_funcname)
    {
        funcptr = _funcptr;
    }
    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;
        using T10 = std::tuple_element<9, argument_types>::type;
        using T11 = std::tuple_element<10, argument_types>::type;
        using T12 = std::tuple_element<11, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);
        CVariableEntry* ve4 = GetContext()->GetParameter(4);
        CVariableEntry* ve5 = GetContext()->GetParameter(5);
        CVariableEntry* ve6 = GetContext()->GetParameter(6);
        CVariableEntry* ve7 = GetContext()->GetParameter(7);
        CVariableEntry* ve8 = GetContext()->GetParameter(8);
        CVariableEntry* ve9 = GetContext()->GetParameter(9);
        CVariableEntry* ve10 = GetContext()->GetParameter(10);
        CVariableEntry* ve11 = GetContext()->GetParameter(11);
        CVariableEntry* ve12 = GetContext()->GetParameter(12);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);
        T4 p4 = ConvertVariableForDispatch<T4>(ve4);
        T5 p5 = ConvertVariableForDispatch<T5>(ve5);
        T6 p6 = ConvertVariableForDispatch<T6>(ve6);
        T7 p7 = ConvertVariableForDispatch<T7>(ve7);
        T8 p8 = ConvertVariableForDispatch<T8>(ve8);
        T9 p9 = ConvertVariableForDispatch<T9>(ve9);
        T10 p10 = ConvertVariableForDispatch<T10>(ve10);
        T11 p11 = ConvertVariableForDispatch<T11>(ve11);
        T12 p12 = ConvertVariableForDispatch<T12>(ve12);

        Dispatch(&p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10, &p11, &p12);
    }

    // -- dispatch method
    void Dispatch(void* _p1, void* _p2, void* _p3, void* _p4, void* _p5, void* _p6, void* _p7, void* _p8, void* _p9, void* _p10, void* _p11, void* _p12)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;
        using T10 = std::tuple_element<9, argument_types>::type;
        using T11 = std::tuple_element<10, argument_types>::type;
        using T12 = std::tuple_element<11, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;
        T4* p4 = (T4*)_p4;
        T5* p5 = (T5*)_p5;
        T6* p6 = (T6*)_p6;
        T7* p7 = (T7*)_p7;
        T8* p8 = (T8*)_p8;
        T9* p9 = (T9*)_p9;
        T10* p10 = (T10*)_p10;
        T11* p11 = (T11*)_p11;
        T12* p12 = (T12*)_p12;

        funcptr(*p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8, *p9, *p10, *p11, *p12);

    }

    // -- registration method
    virtual void Register(CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;
        using T10 = std::tuple_element<9, argument_types>::type;
        using T11 = std::tuple_element<10, argument_types>::type;
        using T12 = std::tuple_element<11, argument_types>::type;

        CFunctionEntry* fe = new CFunctionEntry(script_context, 0, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), TYPE_void, 1, 0);
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());
        GetContext()->AddParameter("_p4", Hash("_p4"), GetRegisteredType(GetTypeID<T4>()), 1, GetTypeID<T4>());
        GetContext()->AddParameter("_p5", Hash("_p5"), GetRegisteredType(GetTypeID<T5>()), 1, GetTypeID<T5>());
        GetContext()->AddParameter("_p6", Hash("_p6"), GetRegisteredType(GetTypeID<T6>()), 1, GetTypeID<T6>());
        GetContext()->AddParameter("_p7", Hash("_p7"), GetRegisteredType(GetTypeID<T7>()), 1, GetTypeID<T7>());
        GetContext()->AddParameter("_p8", Hash("_p8"), GetRegisteredType(GetTypeID<T8>()), 1, GetTypeID<T8>());
        GetContext()->AddParameter("_p9", Hash("_p9"), GetRegisteredType(GetTypeID<T9>()), 1, GetTypeID<T9>());
        GetContext()->AddParameter("_p10", Hash("_p10"), GetRegisteredType(GetTypeID<T10>()), 1, GetTypeID<T10>());
        GetContext()->AddParameter("_p11", Hash("_p11"), GetRegisteredType(GetTypeID<T11>()), 1, GetTypeID<T11>());
        GetContext()->AddParameter("_p12", Hash("_p12"), GetRegisteredType(GetTypeID<T12>()), 1, GetTypeID<T12>());

        uint32 hash = fe->GetHash();
        tFuncTable* globalfunctable = script_context->FindNamespace(0)->GetFuncTable();
        globalfunctable->AddItem(*fe, hash);
    }

private:
    funcsignature funcptr;
};

// -- class CRegisterMethod<12, R(Args...)> ----------------------------------------

template<typename C, typename R, typename... Args>
class CRegisterMethod<12, C, R(Args...)> : public CRegFunctionBase
{
public:

    using argument_types = std::tuple<Args...>;
    typedef R (C::*methodsignature)(Args...);

    // -- CRegisterMethod
    CRegisterMethod(const char* _methodname, methodsignature _methodptr) :
                  CRegFunctionBase(_methodname) {
        methodptr = _methodptr;
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr)
    {

        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;
        using T10 = std::tuple_element<9, argument_types>::type;
        using T11 = std::tuple_element<10, argument_types>::type;
        using T12 = std::tuple_element<11, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);
        CVariableEntry* ve4 = GetContext()->GetParameter(4);
        CVariableEntry* ve5 = GetContext()->GetParameter(5);
        CVariableEntry* ve6 = GetContext()->GetParameter(6);
        CVariableEntry* ve7 = GetContext()->GetParameter(7);
        CVariableEntry* ve8 = GetContext()->GetParameter(8);
        CVariableEntry* ve9 = GetContext()->GetParameter(9);
        CVariableEntry* ve10 = GetContext()->GetParameter(10);
        CVariableEntry* ve11 = GetContext()->GetParameter(11);
        CVariableEntry* ve12 = GetContext()->GetParameter(12);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);
        T4 p4 = ConvertVariableForDispatch<T4>(ve4);
        T5 p5 = ConvertVariableForDispatch<T5>(ve5);
        T6 p6 = ConvertVariableForDispatch<T6>(ve6);
        T7 p7 = ConvertVariableForDispatch<T7>(ve7);
        T8 p8 = ConvertVariableForDispatch<T8>(ve8);
        T9 p9 = ConvertVariableForDispatch<T9>(ve9);
        T10 p10 = ConvertVariableForDispatch<T10>(ve10);
        T11 p11 = ConvertVariableForDispatch<T11>(ve11);
        T12 p12 = ConvertVariableForDispatch<T12>(ve12);
        Dispatch(objaddr, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10, &p11, &p12);
    }

    // -- dispatch method
    R Dispatch(void* objaddr, void* _p1, void* _p2, void* _p3, void* _p4, void* _p5, void* _p6, void* _p7, void* _p8, void* _p9, void* _p10, void* _p11, void* _p12)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;
        using T10 = std::tuple_element<9, argument_types>::type;
        using T11 = std::tuple_element<10, argument_types>::type;
        using T12 = std::tuple_element<11, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;
        T4* p4 = (T4*)_p4;
        T5* p5 = (T5*)_p5;
        T6* p6 = (T6*)_p6;
        T7* p7 = (T7*)_p7;
        T8* p8 = (T8*)_p8;
        T9* p9 = (T9*)_p9;
        T10* p10 = (T10*)_p10;
        T11* p11 = (T11*)_p11;
        T12* p12 = (T12*)_p12;

        C* object = (C*)(objaddr);
        R r = (object->*methodptr)(*p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8, *p9, *p10, *p11, *p12);

        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = this->GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual void Register(TinScript::CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;
        using T10 = std::tuple_element<9, argument_types>::type;
        using T11 = std::tuple_element<10, argument_types>::type;
        using T12 = std::tuple_element<11, argument_types>::type;

        uint32 classname_hash = Hash(C::_GetClassName());
        CFunctionEntry* fe = new CFunctionEntry(script_context, classname_hash, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), GetRegisteredType(GetTypeID<R>()), 1, GetTypeID<R>());
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());
        GetContext()->AddParameter("_p4", Hash("_p4"), GetRegisteredType(GetTypeID<T4>()), 1, GetTypeID<T4>());
        GetContext()->AddParameter("_p5", Hash("_p5"), GetRegisteredType(GetTypeID<T5>()), 1, GetTypeID<T5>());
        GetContext()->AddParameter("_p6", Hash("_p6"), GetRegisteredType(GetTypeID<T6>()), 1, GetTypeID<T6>());
        GetContext()->AddParameter("_p7", Hash("_p7"), GetRegisteredType(GetTypeID<T7>()), 1, GetTypeID<T7>());
        GetContext()->AddParameter("_p8", Hash("_p8"), GetRegisteredType(GetTypeID<T8>()), 1, GetTypeID<T8>());
        GetContext()->AddParameter("_p9", Hash("_p9"), GetRegisteredType(GetTypeID<T9>()), 1, GetTypeID<T9>());
        GetContext()->AddParameter("_p10", Hash("_p10"), GetRegisteredType(GetTypeID<T10>()), 1, GetTypeID<T10>());
        GetContext()->AddParameter("_p11", Hash("_p11"), GetRegisteredType(GetTypeID<T11>()), 1, GetTypeID<T11>());
        GetContext()->AddParameter("_p12", Hash("_p12"), GetRegisteredType(GetTypeID<T12>()), 1, GetTypeID<T12>());

        uint32 hash = fe->GetHash();
        tFuncTable* methodtable = script_context->FindNamespace(classname_hash)->GetFuncTable();
        methodtable->AddItem(*fe, hash);
    }

private:
    methodsignature methodptr;
};
// -- class CRegisterMethod<12, void(Args...)> ----------------------------------------

template<typename C, typename... Args>
class CRegisterMethod<12, C, void(Args...)> : public CRegFunctionBase
{
public:

    using argument_types = std::tuple<Args...>;
    typedef void (C::*methodsignature)(Args...);

    // -- CRegisterMethod
    CRegisterMethod(const char* _methodname, methodsignature _methodptr) :
                  CRegFunctionBase(_methodname) {
        methodptr = _methodptr;
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr)
    {

        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;
        using T10 = std::tuple_element<9, argument_types>::type;
        using T11 = std::tuple_element<10, argument_types>::type;
        using T12 = std::tuple_element<11, argument_types>::type;

        CVariableEntry* ve1 = GetContext()->GetParameter(1);
        CVariableEntry* ve2 = GetContext()->GetParameter(2);
        CVariableEntry* ve3 = GetContext()->GetParameter(3);
        CVariableEntry* ve4 = GetContext()->GetParameter(4);
        CVariableEntry* ve5 = GetContext()->GetParameter(5);
        CVariableEntry* ve6 = GetContext()->GetParameter(6);
        CVariableEntry* ve7 = GetContext()->GetParameter(7);
        CVariableEntry* ve8 = GetContext()->GetParameter(8);
        CVariableEntry* ve9 = GetContext()->GetParameter(9);
        CVariableEntry* ve10 = GetContext()->GetParameter(10);
        CVariableEntry* ve11 = GetContext()->GetParameter(11);
        CVariableEntry* ve12 = GetContext()->GetParameter(12);

        T1 p1 = ConvertVariableForDispatch<T1>(ve1);
        T2 p2 = ConvertVariableForDispatch<T2>(ve2);
        T3 p3 = ConvertVariableForDispatch<T3>(ve3);
        T4 p4 = ConvertVariableForDispatch<T4>(ve4);
        T5 p5 = ConvertVariableForDispatch<T5>(ve5);
        T6 p6 = ConvertVariableForDispatch<T6>(ve6);
        T7 p7 = ConvertVariableForDispatch<T7>(ve7);
        T8 p8 = ConvertVariableForDispatch<T8>(ve8);
        T9 p9 = ConvertVariableForDispatch<T9>(ve9);
        T10 p10 = ConvertVariableForDispatch<T10>(ve10);
        T11 p11 = ConvertVariableForDispatch<T11>(ve11);
        T12 p12 = ConvertVariableForDispatch<T12>(ve12);

        Dispatch(objaddr, &p1, &p2, &p3, &p4, &p5, &p6, &p7, &p8, &p9, &p10, &p11, &p12);
    }

    // -- dispatch method
    void Dispatch(void* objaddr, void* _p1, void* _p2, void* _p3, void* _p4, void* _p5, void* _p6, void* _p7, void* _p8, void* _p9, void* _p10, void* _p11, void* _p12)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;
        using T10 = std::tuple_element<9, argument_types>::type;
        using T11 = std::tuple_element<10, argument_types>::type;
        using T12 = std::tuple_element<11, argument_types>::type;

        T1* p1 = (T1*)_p1;
        T2* p2 = (T2*)_p2;
        T3* p3 = (T3*)_p3;
        T4* p4 = (T4*)_p4;
        T5* p5 = (T5*)_p5;
        T6* p6 = (T6*)_p6;
        T7* p7 = (T7*)_p7;
        T8* p8 = (T8*)_p8;
        T9* p9 = (T9*)_p9;
        T10* p10 = (T10*)_p10;
        T11* p11 = (T11*)_p11;
        T12* p12 = (T12*)_p12;

        C* object = (C*)(objaddr);
        (object->*methodptr)(*p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8, *p9, *p10, *p11, *p12);

    }

    // -- registration method
    virtual void Register(TinScript::CScriptContext* script_context)
    {
        using T1 = std::tuple_element<0, argument_types>::type;
        using T2 = std::tuple_element<1, argument_types>::type;
        using T3 = std::tuple_element<2, argument_types>::type;
        using T4 = std::tuple_element<3, argument_types>::type;
        using T5 = std::tuple_element<4, argument_types>::type;
        using T6 = std::tuple_element<5, argument_types>::type;
        using T7 = std::tuple_element<6, argument_types>::type;
        using T8 = std::tuple_element<7, argument_types>::type;
        using T9 = std::tuple_element<8, argument_types>::type;
        using T10 = std::tuple_element<9, argument_types>::type;
        using T11 = std::tuple_element<10, argument_types>::type;
        using T12 = std::tuple_element<11, argument_types>::type;


        uint32 classname_hash = Hash(C::_GetClassName());
        CFunctionEntry* fe = new CFunctionEntry(script_context, classname_hash, GetName(), Hash(GetName()), eFuncTypeGlobal, this);
        SetScriptContext(script_context);
        SetContext(fe->GetContext());
        GetContext()->AddParameter("__return", Hash("__return"), TYPE_void, 1, 0);
        GetContext()->AddParameter("_p1", Hash("_p1"), GetRegisteredType(GetTypeID<T1>()), 1, GetTypeID<T1>());
        GetContext()->AddParameter("_p2", Hash("_p2"), GetRegisteredType(GetTypeID<T2>()), 1, GetTypeID<T2>());
        GetContext()->AddParameter("_p3", Hash("_p3"), GetRegisteredType(GetTypeID<T3>()), 1, GetTypeID<T3>());
        GetContext()->AddParameter("_p4", Hash("_p4"), GetRegisteredType(GetTypeID<T4>()), 1, GetTypeID<T4>());
        GetContext()->AddParameter("_p5", Hash("_p5"), GetRegisteredType(GetTypeID<T5>()), 1, GetTypeID<T5>());
        GetContext()->AddParameter("_p6", Hash("_p6"), GetRegisteredType(GetTypeID<T6>()), 1, GetTypeID<T6>());
        GetContext()->AddParameter("_p7", Hash("_p7"), GetRegisteredType(GetTypeID<T7>()), 1, GetTypeID<T7>());
        GetContext()->AddParameter("_p8", Hash("_p8"), GetRegisteredType(GetTypeID<T8>()), 1, GetTypeID<T8>());
        GetContext()->AddParameter("_p9", Hash("_p9"), GetRegisteredType(GetTypeID<T9>()), 1, GetTypeID<T9>());
        GetContext()->AddParameter("_p10", Hash("_p10"), GetRegisteredType(GetTypeID<T10>()), 1, GetTypeID<T10>());
        GetContext()->AddParameter("_p11", Hash("_p11"), GetRegisteredType(GetTypeID<T11>()), 1, GetTypeID<T11>());
        GetContext()->AddParameter("_p12", Hash("_p12"), GetRegisteredType(GetTypeID<T12>()), 1, GetTypeID<T12>());


        uint32 hash = fe->GetHash();
        tFuncTable* methodtable = script_context->FindNamespace(classname_hash)->GetFuncTable();
        methodtable->AddItem(*fe, hash);
    }

private:
    methodsignature methodptr;
};
