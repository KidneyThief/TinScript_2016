// ------------------------------------------------------------------------------------------------
//  The MIT License
//  
//  Copyright (c) 2021 Tim Andersen
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
// Generated classes for function registration
// ------------------------------------------------------------------------------------------------


#pragma once

#include "TinVariableEntry.h"
#include "TinFunctionEntry.h"
#include "TinRegistration.h"

// -------------------
// Parameter count: 0
// -------------------

template<typename R>
class CRegFunctionP0 : public TinScript::CRegFunctionBase {
public:

    typedef R (*funcsignature)();

    // -- CRegisterFunctionP0
    CRegFunctionP0(const char* _funcname, funcsignature _funcptr) :
                    CRegFunctionBase("", _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegFunctionP0() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*) {
        Dispatch();
    }

    // -- dispatch method
    R Dispatch() {
        R r = funcptr();
        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, TinScript::convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::GetRegisteredType(TinScript::GetTypeID<R>()), 1, TinScript::GetTypeID<R>()) && success;
        return (success);
    }

private:
    funcsignature funcptr;
};

template<>
class CRegFunctionP0<void> : public TinScript::CRegFunctionBase {
public:

    typedef void (*funcsignature)();

    // -- CRegisterFunctionP0
    CRegFunctionP0(const char* _funcname, funcsignature _funcptr) :
                    CRegFunctionBase("", _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegFunctionP0() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*) {
        Dispatch();
    }

    // -- dispatch method
    void Dispatch() {
        funcptr();
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::TYPE_void, 1, 0) && success;

        return (success);
    }

private:
    funcsignature funcptr;
};

template<typename C, typename R>
class CRegMethodP0 : public TinScript::CRegFunctionBase {
public:

    typedef R (*methodsignature)(C* c);

    // -- CRegisterMethodP0
    CRegMethodP0(const char* _funcname, methodsignature _funcptr) :
                  CRegFunctionBase(__GetClassName<C>(), _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegMethodP0() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr) {
        Dispatch(objaddr);
    }

    // -- dispatch method
    R Dispatch(void* objaddr) {
        C* objptr = (C*)objaddr;
        R r = funcptr(objptr);
        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, TinScript::convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::GetRegisteredType(TinScript::GetTypeID<R>()), 1, TinScript::GetTypeID<R>()) && success;

        return (success);
    }

private:
    methodsignature funcptr;
};

template<typename C>
class CRegMethodP0<C, void> : public TinScript::CRegFunctionBase {
public:

    typedef void (*methodsignature)(C* c);

    // -- CRegisterMethodP0
    CRegMethodP0(const char* _funcname, methodsignature _funcptr) :
                  CRegFunctionBase(__GetClassName<C>(), _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegMethodP0() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr) {
        Dispatch(objaddr);
    }

    // -- dispatch method
    void Dispatch(void* objaddr) {
        C* objptr = (C*)objaddr;
        funcptr(objptr);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::TYPE_void, 1, 0) && success;

        return (success);
    }

private:
    methodsignature funcptr;
};


// -------------------
// Parameter count: 1
// -------------------

template<typename R, typename T1>
class CRegFunctionP1 : public TinScript::CRegFunctionBase {
public:

    typedef R (*funcsignature)(T1 p1);

    // -- CRegisterFunctionP1
    CRegFunctionP1(const char* _funcname, funcsignature _funcptr) :
                    CRegFunctionBase("", _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegFunctionP1() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        Dispatch(TinScript::ConvertVariableForDispatch<T1>(ve1));
    }

    // -- dispatch method
    R Dispatch(T1 p1) {
        R r = funcptr(p1);
        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, TinScript::convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::GetRegisteredType(TinScript::GetTypeID<R>()), 1, TinScript::GetTypeID<R>()) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        return (success);
    }

private:
    funcsignature funcptr;
};

template<typename T1>
class CRegFunctionP1<void, T1> : public TinScript::CRegFunctionBase {
public:

    typedef void (*funcsignature)(T1 p1);

    // -- CRegisterFunctionP1
    CRegFunctionP1(const char* _funcname, funcsignature _funcptr) :
                    CRegFunctionBase("", _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegFunctionP1() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        Dispatch(TinScript::ConvertVariableForDispatch<T1>(ve1));
    }

    // -- dispatch method
    void Dispatch(T1 p1) {
        funcptr(p1);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::TYPE_void, 1, 0) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;

        return (success);
    }

private:
    funcsignature funcptr;
};

template<typename C, typename R, typename T1>
class CRegMethodP1 : public TinScript::CRegFunctionBase {
public:

    typedef R (*methodsignature)(C* c, T1 p1);

    // -- CRegisterMethodP1
    CRegMethodP1(const char* _funcname, methodsignature _funcptr) :
                  CRegFunctionBase(__GetClassName<C>(), _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegMethodP1() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        Dispatch(objaddr,
                 TinScript::ConvertVariableForDispatch<T1>(ve1));
    }

    // -- dispatch method
    R Dispatch(void* objaddr, T1 p1) {
        C* objptr = (C*)objaddr;
        R r = funcptr(objptr, p1);
        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, TinScript::convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::GetRegisteredType(TinScript::GetTypeID<R>()), 1, TinScript::GetTypeID<R>()) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;

        return (success);
    }

private:
    methodsignature funcptr;
};

template<typename C, typename T1>
class CRegMethodP1<C, void, T1> : public TinScript::CRegFunctionBase {
public:

    typedef void (*methodsignature)(C* c, T1 p1);

    // -- CRegisterMethodP1
    CRegMethodP1(const char* _funcname, methodsignature _funcptr) :
                  CRegFunctionBase(__GetClassName<C>(), _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegMethodP1() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        Dispatch(objaddr,
                 TinScript::ConvertVariableForDispatch<T1>(ve1));
    }

    // -- dispatch method
    void Dispatch(void* objaddr, T1 p1) {
        C* objptr = (C*)objaddr;
        funcptr(objptr, p1);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::TYPE_void, 1, 0) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;

        return (success);
    }

private:
    methodsignature funcptr;
};


// -------------------
// Parameter count: 2
// -------------------

template<typename R, typename T1, typename T2>
class CRegFunctionP2 : public TinScript::CRegFunctionBase {
public:

    typedef R (*funcsignature)(T1 p1, T2 p2);

    // -- CRegisterFunctionP2
    CRegFunctionP2(const char* _funcname, funcsignature _funcptr) :
                    CRegFunctionBase("", _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegFunctionP2() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        Dispatch(TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2));
    }

    // -- dispatch method
    R Dispatch(T1 p1, T2 p2) {
        R r = funcptr(p1, p2);
        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, TinScript::convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::GetRegisteredType(TinScript::GetTypeID<R>()), 1, TinScript::GetTypeID<R>()) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        return (success);
    }

private:
    funcsignature funcptr;
};

template<typename T1, typename T2>
class CRegFunctionP2<void, T1, T2> : public TinScript::CRegFunctionBase {
public:

    typedef void (*funcsignature)(T1 p1, T2 p2);

    // -- CRegisterFunctionP2
    CRegFunctionP2(const char* _funcname, funcsignature _funcptr) :
                    CRegFunctionBase("", _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegFunctionP2() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        Dispatch(TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2));
    }

    // -- dispatch method
    void Dispatch(T1 p1, T2 p2) {
        funcptr(p1, p2);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::TYPE_void, 1, 0) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;

        return (success);
    }

private:
    funcsignature funcptr;
};

template<typename C, typename R, typename T1, typename T2>
class CRegMethodP2 : public TinScript::CRegFunctionBase {
public:

    typedef R (*methodsignature)(C* c, T1 p1, T2 p2);

    // -- CRegisterMethodP2
    CRegMethodP2(const char* _funcname, methodsignature _funcptr) :
                  CRegFunctionBase(__GetClassName<C>(), _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegMethodP2() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        Dispatch(objaddr,
                 TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2));
    }

    // -- dispatch method
    R Dispatch(void* objaddr, T1 p1, T2 p2) {
        C* objptr = (C*)objaddr;
        R r = funcptr(objptr, p1, p2);
        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, TinScript::convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::GetRegisteredType(TinScript::GetTypeID<R>()), 1, TinScript::GetTypeID<R>()) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;

        return (success);
    }

private:
    methodsignature funcptr;
};

template<typename C, typename T1, typename T2>
class CRegMethodP2<C, void, T1, T2> : public TinScript::CRegFunctionBase {
public:

    typedef void (*methodsignature)(C* c, T1 p1, T2 p2);

    // -- CRegisterMethodP2
    CRegMethodP2(const char* _funcname, methodsignature _funcptr) :
                  CRegFunctionBase(__GetClassName<C>(), _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegMethodP2() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        Dispatch(objaddr,
                 TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2));
    }

    // -- dispatch method
    void Dispatch(void* objaddr, T1 p1, T2 p2) {
        C* objptr = (C*)objaddr;
        funcptr(objptr, p1, p2);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::TYPE_void, 1, 0) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;

        return (success);
    }

private:
    methodsignature funcptr;
};


// -------------------
// Parameter count: 3
// -------------------

template<typename R, typename T1, typename T2, typename T3>
class CRegFunctionP3 : public TinScript::CRegFunctionBase {
public:

    typedef R (*funcsignature)(T1 p1, T2 p2, T3 p3);

    // -- CRegisterFunctionP3
    CRegFunctionP3(const char* _funcname, funcsignature _funcptr) :
                    CRegFunctionBase("", _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegFunctionP3() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        Dispatch(TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3));
    }

    // -- dispatch method
    R Dispatch(T1 p1, T2 p2, T3 p3) {
        R r = funcptr(p1, p2, p3);
        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, TinScript::convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::GetRegisteredType(TinScript::GetTypeID<R>()), 1, TinScript::GetTypeID<R>()) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;
        return (success);
    }

private:
    funcsignature funcptr;
};

template<typename T1, typename T2, typename T3>
class CRegFunctionP3<void, T1, T2, T3> : public TinScript::CRegFunctionBase {
public:

    typedef void (*funcsignature)(T1 p1, T2 p2, T3 p3);

    // -- CRegisterFunctionP3
    CRegFunctionP3(const char* _funcname, funcsignature _funcptr) :
                    CRegFunctionBase("", _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegFunctionP3() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        Dispatch(TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3));
    }

    // -- dispatch method
    void Dispatch(T1 p1, T2 p2, T3 p3) {
        funcptr(p1, p2, p3);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::TYPE_void, 1, 0) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;

        return (success);
    }

private:
    funcsignature funcptr;
};

template<typename C, typename R, typename T1, typename T2, typename T3>
class CRegMethodP3 : public TinScript::CRegFunctionBase {
public:

    typedef R (*methodsignature)(C* c, T1 p1, T2 p2, T3 p3);

    // -- CRegisterMethodP3
    CRegMethodP3(const char* _funcname, methodsignature _funcptr) :
                  CRegFunctionBase(__GetClassName<C>(), _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegMethodP3() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        Dispatch(objaddr,
                 TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3));
    }

    // -- dispatch method
    R Dispatch(void* objaddr, T1 p1, T2 p2, T3 p3) {
        C* objptr = (C*)objaddr;
        R r = funcptr(objptr, p1, p2, p3);
        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, TinScript::convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::GetRegisteredType(TinScript::GetTypeID<R>()), 1, TinScript::GetTypeID<R>()) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;

        return (success);
    }

private:
    methodsignature funcptr;
};

template<typename C, typename T1, typename T2, typename T3>
class CRegMethodP3<C, void, T1, T2, T3> : public TinScript::CRegFunctionBase {
public:

    typedef void (*methodsignature)(C* c, T1 p1, T2 p2, T3 p3);

    // -- CRegisterMethodP3
    CRegMethodP3(const char* _funcname, methodsignature _funcptr) :
                  CRegFunctionBase(__GetClassName<C>(), _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegMethodP3() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        Dispatch(objaddr,
                 TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3));
    }

    // -- dispatch method
    void Dispatch(void* objaddr, T1 p1, T2 p2, T3 p3) {
        C* objptr = (C*)objaddr;
        funcptr(objptr, p1, p2, p3);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::TYPE_void, 1, 0) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;

        return (success);
    }

private:
    methodsignature funcptr;
};


// -------------------
// Parameter count: 4
// -------------------

template<typename R, typename T1, typename T2, typename T3, typename T4>
class CRegFunctionP4 : public TinScript::CRegFunctionBase {
public:

    typedef R (*funcsignature)(T1 p1, T2 p2, T3 p3, T4 p4);

    // -- CRegisterFunctionP4
    CRegFunctionP4(const char* _funcname, funcsignature _funcptr) :
                    CRegFunctionBase("", _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegFunctionP4() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        TinScript::CVariableEntry* ve4 = GetContext()->GetParameter(4);
        Dispatch(TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3),
                 TinScript::ConvertVariableForDispatch<T4>(ve4));
    }

    // -- dispatch method
    R Dispatch(T1 p1, T2 p2, T3 p3, T4 p4) {
        R r = funcptr(p1, p2, p3, p4);
        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, TinScript::convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::GetRegisteredType(TinScript::GetTypeID<R>()), 1, TinScript::GetTypeID<R>()) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;
        success = fc->AddParameter("_p4", TinScript::Hash("_p4"), TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), 1, TinScript::GetTypeID<T4>()) && success;
        return (success);
    }

private:
    funcsignature funcptr;
};

template<typename T1, typename T2, typename T3, typename T4>
class CRegFunctionP4<void, T1, T2, T3, T4> : public TinScript::CRegFunctionBase {
public:

    typedef void (*funcsignature)(T1 p1, T2 p2, T3 p3, T4 p4);

    // -- CRegisterFunctionP4
    CRegFunctionP4(const char* _funcname, funcsignature _funcptr) :
                    CRegFunctionBase("", _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegFunctionP4() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        TinScript::CVariableEntry* ve4 = GetContext()->GetParameter(4);
        Dispatch(TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3),
                 TinScript::ConvertVariableForDispatch<T4>(ve4));
    }

    // -- dispatch method
    void Dispatch(T1 p1, T2 p2, T3 p3, T4 p4) {
        funcptr(p1, p2, p3, p4);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::TYPE_void, 1, 0) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;
        success = fc->AddParameter("_p4", TinScript::Hash("_p4"), TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), 1, TinScript::GetTypeID<T4>()) && success;

        return (success);
    }

private:
    funcsignature funcptr;
};

template<typename C, typename R, typename T1, typename T2, typename T3, typename T4>
class CRegMethodP4 : public TinScript::CRegFunctionBase {
public:

    typedef R (*methodsignature)(C* c, T1 p1, T2 p2, T3 p3, T4 p4);

    // -- CRegisterMethodP4
    CRegMethodP4(const char* _funcname, methodsignature _funcptr) :
                  CRegFunctionBase(__GetClassName<C>(), _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegMethodP4() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        TinScript::CVariableEntry* ve4 = GetContext()->GetParameter(4);
        Dispatch(objaddr,
                 TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3),
                 TinScript::ConvertVariableForDispatch<T4>(ve4));
    }

    // -- dispatch method
    R Dispatch(void* objaddr, T1 p1, T2 p2, T3 p3, T4 p4) {
        C* objptr = (C*)objaddr;
        R r = funcptr(objptr, p1, p2, p3, p4);
        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, TinScript::convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::GetRegisteredType(TinScript::GetTypeID<R>()), 1, TinScript::GetTypeID<R>()) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;
        success = fc->AddParameter("_p4", TinScript::Hash("_p4"), TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), 1, TinScript::GetTypeID<T4>()) && success;

        return (success);
    }

private:
    methodsignature funcptr;
};

template<typename C, typename T1, typename T2, typename T3, typename T4>
class CRegMethodP4<C, void, T1, T2, T3, T4> : public TinScript::CRegFunctionBase {
public:

    typedef void (*methodsignature)(C* c, T1 p1, T2 p2, T3 p3, T4 p4);

    // -- CRegisterMethodP4
    CRegMethodP4(const char* _funcname, methodsignature _funcptr) :
                  CRegFunctionBase(__GetClassName<C>(), _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegMethodP4() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        TinScript::CVariableEntry* ve4 = GetContext()->GetParameter(4);
        Dispatch(objaddr,
                 TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3),
                 TinScript::ConvertVariableForDispatch<T4>(ve4));
    }

    // -- dispatch method
    void Dispatch(void* objaddr, T1 p1, T2 p2, T3 p3, T4 p4) {
        C* objptr = (C*)objaddr;
        funcptr(objptr, p1, p2, p3, p4);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::TYPE_void, 1, 0) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;
        success = fc->AddParameter("_p4", TinScript::Hash("_p4"), TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), 1, TinScript::GetTypeID<T4>()) && success;

        return (success);
    }

private:
    methodsignature funcptr;
};


// -------------------
// Parameter count: 5
// -------------------

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5>
class CRegFunctionP5 : public TinScript::CRegFunctionBase {
public:

    typedef R (*funcsignature)(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5);

    // -- CRegisterFunctionP5
    CRegFunctionP5(const char* _funcname, funcsignature _funcptr) :
                    CRegFunctionBase("", _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegFunctionP5() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        TinScript::CVariableEntry* ve4 = GetContext()->GetParameter(4);
        TinScript::CVariableEntry* ve5 = GetContext()->GetParameter(5);
        Dispatch(TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3),
                 TinScript::ConvertVariableForDispatch<T4>(ve4),
                 TinScript::ConvertVariableForDispatch<T5>(ve5));
    }

    // -- dispatch method
    R Dispatch(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5) {
        R r = funcptr(p1, p2, p3, p4, p5);
        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, TinScript::convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::GetRegisteredType(TinScript::GetTypeID<R>()), 1, TinScript::GetTypeID<R>()) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;
        success = fc->AddParameter("_p4", TinScript::Hash("_p4"), TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), 1, TinScript::GetTypeID<T4>()) && success;
        success = fc->AddParameter("_p5", TinScript::Hash("_p5"), TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), 1, TinScript::GetTypeID<T5>()) && success;
        return (success);
    }

private:
    funcsignature funcptr;
};

template<typename T1, typename T2, typename T3, typename T4, typename T5>
class CRegFunctionP5<void, T1, T2, T3, T4, T5> : public TinScript::CRegFunctionBase {
public:

    typedef void (*funcsignature)(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5);

    // -- CRegisterFunctionP5
    CRegFunctionP5(const char* _funcname, funcsignature _funcptr) :
                    CRegFunctionBase("", _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegFunctionP5() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        TinScript::CVariableEntry* ve4 = GetContext()->GetParameter(4);
        TinScript::CVariableEntry* ve5 = GetContext()->GetParameter(5);
        Dispatch(TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3),
                 TinScript::ConvertVariableForDispatch<T4>(ve4),
                 TinScript::ConvertVariableForDispatch<T5>(ve5));
    }

    // -- dispatch method
    void Dispatch(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5) {
        funcptr(p1, p2, p3, p4, p5);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::TYPE_void, 1, 0) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;
        success = fc->AddParameter("_p4", TinScript::Hash("_p4"), TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), 1, TinScript::GetTypeID<T4>()) && success;
        success = fc->AddParameter("_p5", TinScript::Hash("_p5"), TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), 1, TinScript::GetTypeID<T5>()) && success;

        return (success);
    }

private:
    funcsignature funcptr;
};

template<typename C, typename R, typename T1, typename T2, typename T3, typename T4, typename T5>
class CRegMethodP5 : public TinScript::CRegFunctionBase {
public:

    typedef R (*methodsignature)(C* c, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5);

    // -- CRegisterMethodP5
    CRegMethodP5(const char* _funcname, methodsignature _funcptr) :
                  CRegFunctionBase(__GetClassName<C>(), _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegMethodP5() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        TinScript::CVariableEntry* ve4 = GetContext()->GetParameter(4);
        TinScript::CVariableEntry* ve5 = GetContext()->GetParameter(5);
        Dispatch(objaddr,
                 TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3),
                 TinScript::ConvertVariableForDispatch<T4>(ve4),
                 TinScript::ConvertVariableForDispatch<T5>(ve5));
    }

    // -- dispatch method
    R Dispatch(void* objaddr, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5) {
        C* objptr = (C*)objaddr;
        R r = funcptr(objptr, p1, p2, p3, p4, p5);
        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, TinScript::convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::GetRegisteredType(TinScript::GetTypeID<R>()), 1, TinScript::GetTypeID<R>()) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;
        success = fc->AddParameter("_p4", TinScript::Hash("_p4"), TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), 1, TinScript::GetTypeID<T4>()) && success;
        success = fc->AddParameter("_p5", TinScript::Hash("_p5"), TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), 1, TinScript::GetTypeID<T5>()) && success;

        return (success);
    }

private:
    methodsignature funcptr;
};

template<typename C, typename T1, typename T2, typename T3, typename T4, typename T5>
class CRegMethodP5<C, void, T1, T2, T3, T4, T5> : public TinScript::CRegFunctionBase {
public:

    typedef void (*methodsignature)(C* c, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5);

    // -- CRegisterMethodP5
    CRegMethodP5(const char* _funcname, methodsignature _funcptr) :
                  CRegFunctionBase(__GetClassName<C>(), _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegMethodP5() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        TinScript::CVariableEntry* ve4 = GetContext()->GetParameter(4);
        TinScript::CVariableEntry* ve5 = GetContext()->GetParameter(5);
        Dispatch(objaddr,
                 TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3),
                 TinScript::ConvertVariableForDispatch<T4>(ve4),
                 TinScript::ConvertVariableForDispatch<T5>(ve5));
    }

    // -- dispatch method
    void Dispatch(void* objaddr, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5) {
        C* objptr = (C*)objaddr;
        funcptr(objptr, p1, p2, p3, p4, p5);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::TYPE_void, 1, 0) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;
        success = fc->AddParameter("_p4", TinScript::Hash("_p4"), TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), 1, TinScript::GetTypeID<T4>()) && success;
        success = fc->AddParameter("_p5", TinScript::Hash("_p5"), TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), 1, TinScript::GetTypeID<T5>()) && success;

        return (success);
    }

private:
    methodsignature funcptr;
};


// -------------------
// Parameter count: 6
// -------------------

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
class CRegFunctionP6 : public TinScript::CRegFunctionBase {
public:

    typedef R (*funcsignature)(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6);

    // -- CRegisterFunctionP6
    CRegFunctionP6(const char* _funcname, funcsignature _funcptr) :
                    CRegFunctionBase("", _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegFunctionP6() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        TinScript::CVariableEntry* ve4 = GetContext()->GetParameter(4);
        TinScript::CVariableEntry* ve5 = GetContext()->GetParameter(5);
        TinScript::CVariableEntry* ve6 = GetContext()->GetParameter(6);
        Dispatch(TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3),
                 TinScript::ConvertVariableForDispatch<T4>(ve4),
                 TinScript::ConvertVariableForDispatch<T5>(ve5),
                 TinScript::ConvertVariableForDispatch<T6>(ve6));
    }

    // -- dispatch method
    R Dispatch(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6) {
        R r = funcptr(p1, p2, p3, p4, p5, p6);
        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, TinScript::convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::GetRegisteredType(TinScript::GetTypeID<R>()), 1, TinScript::GetTypeID<R>()) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;
        success = fc->AddParameter("_p4", TinScript::Hash("_p4"), TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), 1, TinScript::GetTypeID<T4>()) && success;
        success = fc->AddParameter("_p5", TinScript::Hash("_p5"), TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), 1, TinScript::GetTypeID<T5>()) && success;
        success = fc->AddParameter("_p6", TinScript::Hash("_p6"), TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()), 1, TinScript::GetTypeID<T6>()) && success;
        return (success);
    }

private:
    funcsignature funcptr;
};

template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
class CRegFunctionP6<void, T1, T2, T3, T4, T5, T6> : public TinScript::CRegFunctionBase {
public:

    typedef void (*funcsignature)(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6);

    // -- CRegisterFunctionP6
    CRegFunctionP6(const char* _funcname, funcsignature _funcptr) :
                    CRegFunctionBase("", _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegFunctionP6() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        TinScript::CVariableEntry* ve4 = GetContext()->GetParameter(4);
        TinScript::CVariableEntry* ve5 = GetContext()->GetParameter(5);
        TinScript::CVariableEntry* ve6 = GetContext()->GetParameter(6);
        Dispatch(TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3),
                 TinScript::ConvertVariableForDispatch<T4>(ve4),
                 TinScript::ConvertVariableForDispatch<T5>(ve5),
                 TinScript::ConvertVariableForDispatch<T6>(ve6));
    }

    // -- dispatch method
    void Dispatch(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6) {
        funcptr(p1, p2, p3, p4, p5, p6);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::TYPE_void, 1, 0) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;
        success = fc->AddParameter("_p4", TinScript::Hash("_p4"), TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), 1, TinScript::GetTypeID<T4>()) && success;
        success = fc->AddParameter("_p5", TinScript::Hash("_p5"), TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), 1, TinScript::GetTypeID<T5>()) && success;
        success = fc->AddParameter("_p6", TinScript::Hash("_p6"), TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()), 1, TinScript::GetTypeID<T6>()) && success;

        return (success);
    }

private:
    funcsignature funcptr;
};

template<typename C, typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
class CRegMethodP6 : public TinScript::CRegFunctionBase {
public:

    typedef R (*methodsignature)(C* c, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6);

    // -- CRegisterMethodP6
    CRegMethodP6(const char* _funcname, methodsignature _funcptr) :
                  CRegFunctionBase(__GetClassName<C>(), _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegMethodP6() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        TinScript::CVariableEntry* ve4 = GetContext()->GetParameter(4);
        TinScript::CVariableEntry* ve5 = GetContext()->GetParameter(5);
        TinScript::CVariableEntry* ve6 = GetContext()->GetParameter(6);
        Dispatch(objaddr,
                 TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3),
                 TinScript::ConvertVariableForDispatch<T4>(ve4),
                 TinScript::ConvertVariableForDispatch<T5>(ve5),
                 TinScript::ConvertVariableForDispatch<T6>(ve6));
    }

    // -- dispatch method
    R Dispatch(void* objaddr, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6) {
        C* objptr = (C*)objaddr;
        R r = funcptr(objptr, p1, p2, p3, p4, p5, p6);
        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, TinScript::convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::GetRegisteredType(TinScript::GetTypeID<R>()), 1, TinScript::GetTypeID<R>()) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;
        success = fc->AddParameter("_p4", TinScript::Hash("_p4"), TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), 1, TinScript::GetTypeID<T4>()) && success;
        success = fc->AddParameter("_p5", TinScript::Hash("_p5"), TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), 1, TinScript::GetTypeID<T5>()) && success;
        success = fc->AddParameter("_p6", TinScript::Hash("_p6"), TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()), 1, TinScript::GetTypeID<T6>()) && success;

        return (success);
    }

private:
    methodsignature funcptr;
};

template<typename C, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
class CRegMethodP6<C, void, T1, T2, T3, T4, T5, T6> : public TinScript::CRegFunctionBase {
public:

    typedef void (*methodsignature)(C* c, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6);

    // -- CRegisterMethodP6
    CRegMethodP6(const char* _funcname, methodsignature _funcptr) :
                  CRegFunctionBase(__GetClassName<C>(), _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegMethodP6() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        TinScript::CVariableEntry* ve4 = GetContext()->GetParameter(4);
        TinScript::CVariableEntry* ve5 = GetContext()->GetParameter(5);
        TinScript::CVariableEntry* ve6 = GetContext()->GetParameter(6);
        Dispatch(objaddr,
                 TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3),
                 TinScript::ConvertVariableForDispatch<T4>(ve4),
                 TinScript::ConvertVariableForDispatch<T5>(ve5),
                 TinScript::ConvertVariableForDispatch<T6>(ve6));
    }

    // -- dispatch method
    void Dispatch(void* objaddr, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6) {
        C* objptr = (C*)objaddr;
        funcptr(objptr, p1, p2, p3, p4, p5, p6);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::TYPE_void, 1, 0) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;
        success = fc->AddParameter("_p4", TinScript::Hash("_p4"), TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), 1, TinScript::GetTypeID<T4>()) && success;
        success = fc->AddParameter("_p5", TinScript::Hash("_p5"), TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), 1, TinScript::GetTypeID<T5>()) && success;
        success = fc->AddParameter("_p6", TinScript::Hash("_p6"), TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()), 1, TinScript::GetTypeID<T6>()) && success;

        return (success);
    }

private:
    methodsignature funcptr;
};


// -------------------
// Parameter count: 7
// -------------------

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
class CRegFunctionP7 : public TinScript::CRegFunctionBase {
public:

    typedef R (*funcsignature)(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7);

    // -- CRegisterFunctionP7
    CRegFunctionP7(const char* _funcname, funcsignature _funcptr) :
                    CRegFunctionBase("", _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegFunctionP7() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        TinScript::CVariableEntry* ve4 = GetContext()->GetParameter(4);
        TinScript::CVariableEntry* ve5 = GetContext()->GetParameter(5);
        TinScript::CVariableEntry* ve6 = GetContext()->GetParameter(6);
        TinScript::CVariableEntry* ve7 = GetContext()->GetParameter(7);
        Dispatch(TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3),
                 TinScript::ConvertVariableForDispatch<T4>(ve4),
                 TinScript::ConvertVariableForDispatch<T5>(ve5),
                 TinScript::ConvertVariableForDispatch<T6>(ve6),
                 TinScript::ConvertVariableForDispatch<T7>(ve7));
    }

    // -- dispatch method
    R Dispatch(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7) {
        R r = funcptr(p1, p2, p3, p4, p5, p6, p7);
        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, TinScript::convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::GetRegisteredType(TinScript::GetTypeID<R>()), 1, TinScript::GetTypeID<R>()) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;
        success = fc->AddParameter("_p4", TinScript::Hash("_p4"), TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), 1, TinScript::GetTypeID<T4>()) && success;
        success = fc->AddParameter("_p5", TinScript::Hash("_p5"), TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), 1, TinScript::GetTypeID<T5>()) && success;
        success = fc->AddParameter("_p6", TinScript::Hash("_p6"), TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()), 1, TinScript::GetTypeID<T6>()) && success;
        success = fc->AddParameter("_p7", TinScript::Hash("_p7"), TinScript::GetRegisteredType(TinScript::GetTypeID<T7>()), 1, TinScript::GetTypeID<T7>()) && success;
        return (success);
    }

private:
    funcsignature funcptr;
};

template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
class CRegFunctionP7<void, T1, T2, T3, T4, T5, T6, T7> : public TinScript::CRegFunctionBase {
public:

    typedef void (*funcsignature)(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7);

    // -- CRegisterFunctionP7
    CRegFunctionP7(const char* _funcname, funcsignature _funcptr) :
                    CRegFunctionBase("", _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegFunctionP7() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        TinScript::CVariableEntry* ve4 = GetContext()->GetParameter(4);
        TinScript::CVariableEntry* ve5 = GetContext()->GetParameter(5);
        TinScript::CVariableEntry* ve6 = GetContext()->GetParameter(6);
        TinScript::CVariableEntry* ve7 = GetContext()->GetParameter(7);
        Dispatch(TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3),
                 TinScript::ConvertVariableForDispatch<T4>(ve4),
                 TinScript::ConvertVariableForDispatch<T5>(ve5),
                 TinScript::ConvertVariableForDispatch<T6>(ve6),
                 TinScript::ConvertVariableForDispatch<T7>(ve7));
    }

    // -- dispatch method
    void Dispatch(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7) {
        funcptr(p1, p2, p3, p4, p5, p6, p7);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::TYPE_void, 1, 0) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;
        success = fc->AddParameter("_p4", TinScript::Hash("_p4"), TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), 1, TinScript::GetTypeID<T4>()) && success;
        success = fc->AddParameter("_p5", TinScript::Hash("_p5"), TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), 1, TinScript::GetTypeID<T5>()) && success;
        success = fc->AddParameter("_p6", TinScript::Hash("_p6"), TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()), 1, TinScript::GetTypeID<T6>()) && success;
        success = fc->AddParameter("_p7", TinScript::Hash("_p7"), TinScript::GetRegisteredType(TinScript::GetTypeID<T7>()), 1, TinScript::GetTypeID<T7>()) && success;

        return (success);
    }

private:
    funcsignature funcptr;
};

template<typename C, typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
class CRegMethodP7 : public TinScript::CRegFunctionBase {
public:

    typedef R (*methodsignature)(C* c, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7);

    // -- CRegisterMethodP7
    CRegMethodP7(const char* _funcname, methodsignature _funcptr) :
                  CRegFunctionBase(__GetClassName<C>(), _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegMethodP7() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        TinScript::CVariableEntry* ve4 = GetContext()->GetParameter(4);
        TinScript::CVariableEntry* ve5 = GetContext()->GetParameter(5);
        TinScript::CVariableEntry* ve6 = GetContext()->GetParameter(6);
        TinScript::CVariableEntry* ve7 = GetContext()->GetParameter(7);
        Dispatch(objaddr,
                 TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3),
                 TinScript::ConvertVariableForDispatch<T4>(ve4),
                 TinScript::ConvertVariableForDispatch<T5>(ve5),
                 TinScript::ConvertVariableForDispatch<T6>(ve6),
                 TinScript::ConvertVariableForDispatch<T7>(ve7));
    }

    // -- dispatch method
    R Dispatch(void* objaddr, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7) {
        C* objptr = (C*)objaddr;
        R r = funcptr(objptr, p1, p2, p3, p4, p5, p6, p7);
        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, TinScript::convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::GetRegisteredType(TinScript::GetTypeID<R>()), 1, TinScript::GetTypeID<R>()) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;
        success = fc->AddParameter("_p4", TinScript::Hash("_p4"), TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), 1, TinScript::GetTypeID<T4>()) && success;
        success = fc->AddParameter("_p5", TinScript::Hash("_p5"), TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), 1, TinScript::GetTypeID<T5>()) && success;
        success = fc->AddParameter("_p6", TinScript::Hash("_p6"), TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()), 1, TinScript::GetTypeID<T6>()) && success;
        success = fc->AddParameter("_p7", TinScript::Hash("_p7"), TinScript::GetRegisteredType(TinScript::GetTypeID<T7>()), 1, TinScript::GetTypeID<T7>()) && success;

        return (success);
    }

private:
    methodsignature funcptr;
};

template<typename C, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
class CRegMethodP7<C, void, T1, T2, T3, T4, T5, T6, T7> : public TinScript::CRegFunctionBase {
public:

    typedef void (*methodsignature)(C* c, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7);

    // -- CRegisterMethodP7
    CRegMethodP7(const char* _funcname, methodsignature _funcptr) :
                  CRegFunctionBase(__GetClassName<C>(), _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegMethodP7() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        TinScript::CVariableEntry* ve4 = GetContext()->GetParameter(4);
        TinScript::CVariableEntry* ve5 = GetContext()->GetParameter(5);
        TinScript::CVariableEntry* ve6 = GetContext()->GetParameter(6);
        TinScript::CVariableEntry* ve7 = GetContext()->GetParameter(7);
        Dispatch(objaddr,
                 TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3),
                 TinScript::ConvertVariableForDispatch<T4>(ve4),
                 TinScript::ConvertVariableForDispatch<T5>(ve5),
                 TinScript::ConvertVariableForDispatch<T6>(ve6),
                 TinScript::ConvertVariableForDispatch<T7>(ve7));
    }

    // -- dispatch method
    void Dispatch(void* objaddr, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7) {
        C* objptr = (C*)objaddr;
        funcptr(objptr, p1, p2, p3, p4, p5, p6, p7);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::TYPE_void, 1, 0) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;
        success = fc->AddParameter("_p4", TinScript::Hash("_p4"), TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), 1, TinScript::GetTypeID<T4>()) && success;
        success = fc->AddParameter("_p5", TinScript::Hash("_p5"), TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), 1, TinScript::GetTypeID<T5>()) && success;
        success = fc->AddParameter("_p6", TinScript::Hash("_p6"), TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()), 1, TinScript::GetTypeID<T6>()) && success;
        success = fc->AddParameter("_p7", TinScript::Hash("_p7"), TinScript::GetRegisteredType(TinScript::GetTypeID<T7>()), 1, TinScript::GetTypeID<T7>()) && success;

        return (success);
    }

private:
    methodsignature funcptr;
};


// -------------------
// Parameter count: 8
// -------------------

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
class CRegFunctionP8 : public TinScript::CRegFunctionBase {
public:

    typedef R (*funcsignature)(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8);

    // -- CRegisterFunctionP8
    CRegFunctionP8(const char* _funcname, funcsignature _funcptr) :
                    CRegFunctionBase("", _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegFunctionP8() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        TinScript::CVariableEntry* ve4 = GetContext()->GetParameter(4);
        TinScript::CVariableEntry* ve5 = GetContext()->GetParameter(5);
        TinScript::CVariableEntry* ve6 = GetContext()->GetParameter(6);
        TinScript::CVariableEntry* ve7 = GetContext()->GetParameter(7);
        TinScript::CVariableEntry* ve8 = GetContext()->GetParameter(8);
        Dispatch(TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3),
                 TinScript::ConvertVariableForDispatch<T4>(ve4),
                 TinScript::ConvertVariableForDispatch<T5>(ve5),
                 TinScript::ConvertVariableForDispatch<T6>(ve6),
                 TinScript::ConvertVariableForDispatch<T7>(ve7),
                 TinScript::ConvertVariableForDispatch<T8>(ve8));
    }

    // -- dispatch method
    R Dispatch(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8) {
        R r = funcptr(p1, p2, p3, p4, p5, p6, p7, p8);
        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, TinScript::convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::GetRegisteredType(TinScript::GetTypeID<R>()), 1, TinScript::GetTypeID<R>()) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;
        success = fc->AddParameter("_p4", TinScript::Hash("_p4"), TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), 1, TinScript::GetTypeID<T4>()) && success;
        success = fc->AddParameter("_p5", TinScript::Hash("_p5"), TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), 1, TinScript::GetTypeID<T5>()) && success;
        success = fc->AddParameter("_p6", TinScript::Hash("_p6"), TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()), 1, TinScript::GetTypeID<T6>()) && success;
        success = fc->AddParameter("_p7", TinScript::Hash("_p7"), TinScript::GetRegisteredType(TinScript::GetTypeID<T7>()), 1, TinScript::GetTypeID<T7>()) && success;
        success = fc->AddParameter("_p8", TinScript::Hash("_p8"), TinScript::GetRegisteredType(TinScript::GetTypeID<T8>()), 1, TinScript::GetTypeID<T8>()) && success;
        return (success);
    }

private:
    funcsignature funcptr;
};

template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
class CRegFunctionP8<void, T1, T2, T3, T4, T5, T6, T7, T8> : public TinScript::CRegFunctionBase {
public:

    typedef void (*funcsignature)(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8);

    // -- CRegisterFunctionP8
    CRegFunctionP8(const char* _funcname, funcsignature _funcptr) :
                    CRegFunctionBase("", _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegFunctionP8() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        TinScript::CVariableEntry* ve4 = GetContext()->GetParameter(4);
        TinScript::CVariableEntry* ve5 = GetContext()->GetParameter(5);
        TinScript::CVariableEntry* ve6 = GetContext()->GetParameter(6);
        TinScript::CVariableEntry* ve7 = GetContext()->GetParameter(7);
        TinScript::CVariableEntry* ve8 = GetContext()->GetParameter(8);
        Dispatch(TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3),
                 TinScript::ConvertVariableForDispatch<T4>(ve4),
                 TinScript::ConvertVariableForDispatch<T5>(ve5),
                 TinScript::ConvertVariableForDispatch<T6>(ve6),
                 TinScript::ConvertVariableForDispatch<T7>(ve7),
                 TinScript::ConvertVariableForDispatch<T8>(ve8));
    }

    // -- dispatch method
    void Dispatch(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8) {
        funcptr(p1, p2, p3, p4, p5, p6, p7, p8);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::TYPE_void, 1, 0) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;
        success = fc->AddParameter("_p4", TinScript::Hash("_p4"), TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), 1, TinScript::GetTypeID<T4>()) && success;
        success = fc->AddParameter("_p5", TinScript::Hash("_p5"), TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), 1, TinScript::GetTypeID<T5>()) && success;
        success = fc->AddParameter("_p6", TinScript::Hash("_p6"), TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()), 1, TinScript::GetTypeID<T6>()) && success;
        success = fc->AddParameter("_p7", TinScript::Hash("_p7"), TinScript::GetRegisteredType(TinScript::GetTypeID<T7>()), 1, TinScript::GetTypeID<T7>()) && success;
        success = fc->AddParameter("_p8", TinScript::Hash("_p8"), TinScript::GetRegisteredType(TinScript::GetTypeID<T8>()), 1, TinScript::GetTypeID<T8>()) && success;

        return (success);
    }

private:
    funcsignature funcptr;
};

template<typename C, typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
class CRegMethodP8 : public TinScript::CRegFunctionBase {
public:

    typedef R (*methodsignature)(C* c, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8);

    // -- CRegisterMethodP8
    CRegMethodP8(const char* _funcname, methodsignature _funcptr) :
                  CRegFunctionBase(__GetClassName<C>(), _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegMethodP8() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        TinScript::CVariableEntry* ve4 = GetContext()->GetParameter(4);
        TinScript::CVariableEntry* ve5 = GetContext()->GetParameter(5);
        TinScript::CVariableEntry* ve6 = GetContext()->GetParameter(6);
        TinScript::CVariableEntry* ve7 = GetContext()->GetParameter(7);
        TinScript::CVariableEntry* ve8 = GetContext()->GetParameter(8);
        Dispatch(objaddr,
                 TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3),
                 TinScript::ConvertVariableForDispatch<T4>(ve4),
                 TinScript::ConvertVariableForDispatch<T5>(ve5),
                 TinScript::ConvertVariableForDispatch<T6>(ve6),
                 TinScript::ConvertVariableForDispatch<T7>(ve7),
                 TinScript::ConvertVariableForDispatch<T8>(ve8));
    }

    // -- dispatch method
    R Dispatch(void* objaddr, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8) {
        C* objptr = (C*)objaddr;
        R r = funcptr(objptr, p1, p2, p3, p4, p5, p6, p7, p8);
        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, TinScript::convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::GetRegisteredType(TinScript::GetTypeID<R>()), 1, TinScript::GetTypeID<R>()) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;
        success = fc->AddParameter("_p4", TinScript::Hash("_p4"), TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), 1, TinScript::GetTypeID<T4>()) && success;
        success = fc->AddParameter("_p5", TinScript::Hash("_p5"), TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), 1, TinScript::GetTypeID<T5>()) && success;
        success = fc->AddParameter("_p6", TinScript::Hash("_p6"), TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()), 1, TinScript::GetTypeID<T6>()) && success;
        success = fc->AddParameter("_p7", TinScript::Hash("_p7"), TinScript::GetRegisteredType(TinScript::GetTypeID<T7>()), 1, TinScript::GetTypeID<T7>()) && success;
        success = fc->AddParameter("_p8", TinScript::Hash("_p8"), TinScript::GetRegisteredType(TinScript::GetTypeID<T8>()), 1, TinScript::GetTypeID<T8>()) && success;

        return (success);
    }

private:
    methodsignature funcptr;
};

template<typename C, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
class CRegMethodP8<C, void, T1, T2, T3, T4, T5, T6, T7, T8> : public TinScript::CRegFunctionBase {
public:

    typedef void (*methodsignature)(C* c, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8);

    // -- CRegisterMethodP8
    CRegMethodP8(const char* _funcname, methodsignature _funcptr) :
                  CRegFunctionBase(__GetClassName<C>(), _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegMethodP8() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        TinScript::CVariableEntry* ve4 = GetContext()->GetParameter(4);
        TinScript::CVariableEntry* ve5 = GetContext()->GetParameter(5);
        TinScript::CVariableEntry* ve6 = GetContext()->GetParameter(6);
        TinScript::CVariableEntry* ve7 = GetContext()->GetParameter(7);
        TinScript::CVariableEntry* ve8 = GetContext()->GetParameter(8);
        Dispatch(objaddr,
                 TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3),
                 TinScript::ConvertVariableForDispatch<T4>(ve4),
                 TinScript::ConvertVariableForDispatch<T5>(ve5),
                 TinScript::ConvertVariableForDispatch<T6>(ve6),
                 TinScript::ConvertVariableForDispatch<T7>(ve7),
                 TinScript::ConvertVariableForDispatch<T8>(ve8));
    }

    // -- dispatch method
    void Dispatch(void* objaddr, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8) {
        C* objptr = (C*)objaddr;
        funcptr(objptr, p1, p2, p3, p4, p5, p6, p7, p8);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::TYPE_void, 1, 0) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;
        success = fc->AddParameter("_p4", TinScript::Hash("_p4"), TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), 1, TinScript::GetTypeID<T4>()) && success;
        success = fc->AddParameter("_p5", TinScript::Hash("_p5"), TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), 1, TinScript::GetTypeID<T5>()) && success;
        success = fc->AddParameter("_p6", TinScript::Hash("_p6"), TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()), 1, TinScript::GetTypeID<T6>()) && success;
        success = fc->AddParameter("_p7", TinScript::Hash("_p7"), TinScript::GetRegisteredType(TinScript::GetTypeID<T7>()), 1, TinScript::GetTypeID<T7>()) && success;
        success = fc->AddParameter("_p8", TinScript::Hash("_p8"), TinScript::GetRegisteredType(TinScript::GetTypeID<T8>()), 1, TinScript::GetTypeID<T8>()) && success;

        return (success);
    }

private:
    methodsignature funcptr;
};


// -------------------
// Parameter count: 9
// -------------------

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
class CRegFunctionP9 : public TinScript::CRegFunctionBase {
public:

    typedef R (*funcsignature)(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9);

    // -- CRegisterFunctionP9
    CRegFunctionP9(const char* _funcname, funcsignature _funcptr) :
                    CRegFunctionBase("", _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegFunctionP9() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        TinScript::CVariableEntry* ve4 = GetContext()->GetParameter(4);
        TinScript::CVariableEntry* ve5 = GetContext()->GetParameter(5);
        TinScript::CVariableEntry* ve6 = GetContext()->GetParameter(6);
        TinScript::CVariableEntry* ve7 = GetContext()->GetParameter(7);
        TinScript::CVariableEntry* ve8 = GetContext()->GetParameter(8);
        TinScript::CVariableEntry* ve9 = GetContext()->GetParameter(9);
        Dispatch(TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3),
                 TinScript::ConvertVariableForDispatch<T4>(ve4),
                 TinScript::ConvertVariableForDispatch<T5>(ve5),
                 TinScript::ConvertVariableForDispatch<T6>(ve6),
                 TinScript::ConvertVariableForDispatch<T7>(ve7),
                 TinScript::ConvertVariableForDispatch<T8>(ve8),
                 TinScript::ConvertVariableForDispatch<T9>(ve9));
    }

    // -- dispatch method
    R Dispatch(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9) {
        R r = funcptr(p1, p2, p3, p4, p5, p6, p7, p8, p9);
        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, TinScript::convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::GetRegisteredType(TinScript::GetTypeID<R>()), 1, TinScript::GetTypeID<R>()) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;
        success = fc->AddParameter("_p4", TinScript::Hash("_p4"), TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), 1, TinScript::GetTypeID<T4>()) && success;
        success = fc->AddParameter("_p5", TinScript::Hash("_p5"), TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), 1, TinScript::GetTypeID<T5>()) && success;
        success = fc->AddParameter("_p6", TinScript::Hash("_p6"), TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()), 1, TinScript::GetTypeID<T6>()) && success;
        success = fc->AddParameter("_p7", TinScript::Hash("_p7"), TinScript::GetRegisteredType(TinScript::GetTypeID<T7>()), 1, TinScript::GetTypeID<T7>()) && success;
        success = fc->AddParameter("_p8", TinScript::Hash("_p8"), TinScript::GetRegisteredType(TinScript::GetTypeID<T8>()), 1, TinScript::GetTypeID<T8>()) && success;
        success = fc->AddParameter("_p9", TinScript::Hash("_p9"), TinScript::GetRegisteredType(TinScript::GetTypeID<T9>()), 1, TinScript::GetTypeID<T9>()) && success;
        return (success);
    }

private:
    funcsignature funcptr;
};

template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
class CRegFunctionP9<void, T1, T2, T3, T4, T5, T6, T7, T8, T9> : public TinScript::CRegFunctionBase {
public:

    typedef void (*funcsignature)(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9);

    // -- CRegisterFunctionP9
    CRegFunctionP9(const char* _funcname, funcsignature _funcptr) :
                    CRegFunctionBase("", _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegFunctionP9() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        TinScript::CVariableEntry* ve4 = GetContext()->GetParameter(4);
        TinScript::CVariableEntry* ve5 = GetContext()->GetParameter(5);
        TinScript::CVariableEntry* ve6 = GetContext()->GetParameter(6);
        TinScript::CVariableEntry* ve7 = GetContext()->GetParameter(7);
        TinScript::CVariableEntry* ve8 = GetContext()->GetParameter(8);
        TinScript::CVariableEntry* ve9 = GetContext()->GetParameter(9);
        Dispatch(TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3),
                 TinScript::ConvertVariableForDispatch<T4>(ve4),
                 TinScript::ConvertVariableForDispatch<T5>(ve5),
                 TinScript::ConvertVariableForDispatch<T6>(ve6),
                 TinScript::ConvertVariableForDispatch<T7>(ve7),
                 TinScript::ConvertVariableForDispatch<T8>(ve8),
                 TinScript::ConvertVariableForDispatch<T9>(ve9));
    }

    // -- dispatch method
    void Dispatch(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9) {
        funcptr(p1, p2, p3, p4, p5, p6, p7, p8, p9);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::TYPE_void, 1, 0) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;
        success = fc->AddParameter("_p4", TinScript::Hash("_p4"), TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), 1, TinScript::GetTypeID<T4>()) && success;
        success = fc->AddParameter("_p5", TinScript::Hash("_p5"), TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), 1, TinScript::GetTypeID<T5>()) && success;
        success = fc->AddParameter("_p6", TinScript::Hash("_p6"), TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()), 1, TinScript::GetTypeID<T6>()) && success;
        success = fc->AddParameter("_p7", TinScript::Hash("_p7"), TinScript::GetRegisteredType(TinScript::GetTypeID<T7>()), 1, TinScript::GetTypeID<T7>()) && success;
        success = fc->AddParameter("_p8", TinScript::Hash("_p8"), TinScript::GetRegisteredType(TinScript::GetTypeID<T8>()), 1, TinScript::GetTypeID<T8>()) && success;
        success = fc->AddParameter("_p9", TinScript::Hash("_p9"), TinScript::GetRegisteredType(TinScript::GetTypeID<T9>()), 1, TinScript::GetTypeID<T9>()) && success;

        return (success);
    }

private:
    funcsignature funcptr;
};

template<typename C, typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
class CRegMethodP9 : public TinScript::CRegFunctionBase {
public:

    typedef R (*methodsignature)(C* c, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9);

    // -- CRegisterMethodP9
    CRegMethodP9(const char* _funcname, methodsignature _funcptr) :
                  CRegFunctionBase(__GetClassName<C>(), _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegMethodP9() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        TinScript::CVariableEntry* ve4 = GetContext()->GetParameter(4);
        TinScript::CVariableEntry* ve5 = GetContext()->GetParameter(5);
        TinScript::CVariableEntry* ve6 = GetContext()->GetParameter(6);
        TinScript::CVariableEntry* ve7 = GetContext()->GetParameter(7);
        TinScript::CVariableEntry* ve8 = GetContext()->GetParameter(8);
        TinScript::CVariableEntry* ve9 = GetContext()->GetParameter(9);
        Dispatch(objaddr,
                 TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3),
                 TinScript::ConvertVariableForDispatch<T4>(ve4),
                 TinScript::ConvertVariableForDispatch<T5>(ve5),
                 TinScript::ConvertVariableForDispatch<T6>(ve6),
                 TinScript::ConvertVariableForDispatch<T7>(ve7),
                 TinScript::ConvertVariableForDispatch<T8>(ve8),
                 TinScript::ConvertVariableForDispatch<T9>(ve9));
    }

    // -- dispatch method
    R Dispatch(void* objaddr, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9) {
        C* objptr = (C*)objaddr;
        R r = funcptr(objptr, p1, p2, p3, p4, p5, p6, p7, p8, p9);
        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, TinScript::convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::GetRegisteredType(TinScript::GetTypeID<R>()), 1, TinScript::GetTypeID<R>()) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;
        success = fc->AddParameter("_p4", TinScript::Hash("_p4"), TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), 1, TinScript::GetTypeID<T4>()) && success;
        success = fc->AddParameter("_p5", TinScript::Hash("_p5"), TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), 1, TinScript::GetTypeID<T5>()) && success;
        success = fc->AddParameter("_p6", TinScript::Hash("_p6"), TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()), 1, TinScript::GetTypeID<T6>()) && success;
        success = fc->AddParameter("_p7", TinScript::Hash("_p7"), TinScript::GetRegisteredType(TinScript::GetTypeID<T7>()), 1, TinScript::GetTypeID<T7>()) && success;
        success = fc->AddParameter("_p8", TinScript::Hash("_p8"), TinScript::GetRegisteredType(TinScript::GetTypeID<T8>()), 1, TinScript::GetTypeID<T8>()) && success;
        success = fc->AddParameter("_p9", TinScript::Hash("_p9"), TinScript::GetRegisteredType(TinScript::GetTypeID<T9>()), 1, TinScript::GetTypeID<T9>()) && success;

        return (success);
    }

private:
    methodsignature funcptr;
};

template<typename C, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
class CRegMethodP9<C, void, T1, T2, T3, T4, T5, T6, T7, T8, T9> : public TinScript::CRegFunctionBase {
public:

    typedef void (*methodsignature)(C* c, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9);

    // -- CRegisterMethodP9
    CRegMethodP9(const char* _funcname, methodsignature _funcptr) :
                  CRegFunctionBase(__GetClassName<C>(), _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegMethodP9() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        TinScript::CVariableEntry* ve4 = GetContext()->GetParameter(4);
        TinScript::CVariableEntry* ve5 = GetContext()->GetParameter(5);
        TinScript::CVariableEntry* ve6 = GetContext()->GetParameter(6);
        TinScript::CVariableEntry* ve7 = GetContext()->GetParameter(7);
        TinScript::CVariableEntry* ve8 = GetContext()->GetParameter(8);
        TinScript::CVariableEntry* ve9 = GetContext()->GetParameter(9);
        Dispatch(objaddr,
                 TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3),
                 TinScript::ConvertVariableForDispatch<T4>(ve4),
                 TinScript::ConvertVariableForDispatch<T5>(ve5),
                 TinScript::ConvertVariableForDispatch<T6>(ve6),
                 TinScript::ConvertVariableForDispatch<T7>(ve7),
                 TinScript::ConvertVariableForDispatch<T8>(ve8),
                 TinScript::ConvertVariableForDispatch<T9>(ve9));
    }

    // -- dispatch method
    void Dispatch(void* objaddr, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9) {
        C* objptr = (C*)objaddr;
        funcptr(objptr, p1, p2, p3, p4, p5, p6, p7, p8, p9);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::TYPE_void, 1, 0) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;
        success = fc->AddParameter("_p4", TinScript::Hash("_p4"), TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), 1, TinScript::GetTypeID<T4>()) && success;
        success = fc->AddParameter("_p5", TinScript::Hash("_p5"), TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), 1, TinScript::GetTypeID<T5>()) && success;
        success = fc->AddParameter("_p6", TinScript::Hash("_p6"), TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()), 1, TinScript::GetTypeID<T6>()) && success;
        success = fc->AddParameter("_p7", TinScript::Hash("_p7"), TinScript::GetRegisteredType(TinScript::GetTypeID<T7>()), 1, TinScript::GetTypeID<T7>()) && success;
        success = fc->AddParameter("_p8", TinScript::Hash("_p8"), TinScript::GetRegisteredType(TinScript::GetTypeID<T8>()), 1, TinScript::GetTypeID<T8>()) && success;
        success = fc->AddParameter("_p9", TinScript::Hash("_p9"), TinScript::GetRegisteredType(TinScript::GetTypeID<T9>()), 1, TinScript::GetTypeID<T9>()) && success;

        return (success);
    }

private:
    methodsignature funcptr;
};


// -------------------
// Parameter count: 10
// -------------------

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10>
class CRegFunctionP10 : public TinScript::CRegFunctionBase {
public:

    typedef R (*funcsignature)(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10);

    // -- CRegisterFunctionP10
    CRegFunctionP10(const char* _funcname, funcsignature _funcptr) :
                    CRegFunctionBase("", _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegFunctionP10() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        TinScript::CVariableEntry* ve4 = GetContext()->GetParameter(4);
        TinScript::CVariableEntry* ve5 = GetContext()->GetParameter(5);
        TinScript::CVariableEntry* ve6 = GetContext()->GetParameter(6);
        TinScript::CVariableEntry* ve7 = GetContext()->GetParameter(7);
        TinScript::CVariableEntry* ve8 = GetContext()->GetParameter(8);
        TinScript::CVariableEntry* ve9 = GetContext()->GetParameter(9);
        TinScript::CVariableEntry* ve10 = GetContext()->GetParameter(10);
        Dispatch(TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3),
                 TinScript::ConvertVariableForDispatch<T4>(ve4),
                 TinScript::ConvertVariableForDispatch<T5>(ve5),
                 TinScript::ConvertVariableForDispatch<T6>(ve6),
                 TinScript::ConvertVariableForDispatch<T7>(ve7),
                 TinScript::ConvertVariableForDispatch<T8>(ve8),
                 TinScript::ConvertVariableForDispatch<T9>(ve9),
                 TinScript::ConvertVariableForDispatch<T10>(ve10));
    }

    // -- dispatch method
    R Dispatch(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10) {
        R r = funcptr(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, TinScript::convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::GetRegisteredType(TinScript::GetTypeID<R>()), 1, TinScript::GetTypeID<R>()) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;
        success = fc->AddParameter("_p4", TinScript::Hash("_p4"), TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), 1, TinScript::GetTypeID<T4>()) && success;
        success = fc->AddParameter("_p5", TinScript::Hash("_p5"), TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), 1, TinScript::GetTypeID<T5>()) && success;
        success = fc->AddParameter("_p6", TinScript::Hash("_p6"), TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()), 1, TinScript::GetTypeID<T6>()) && success;
        success = fc->AddParameter("_p7", TinScript::Hash("_p7"), TinScript::GetRegisteredType(TinScript::GetTypeID<T7>()), 1, TinScript::GetTypeID<T7>()) && success;
        success = fc->AddParameter("_p8", TinScript::Hash("_p8"), TinScript::GetRegisteredType(TinScript::GetTypeID<T8>()), 1, TinScript::GetTypeID<T8>()) && success;
        success = fc->AddParameter("_p9", TinScript::Hash("_p9"), TinScript::GetRegisteredType(TinScript::GetTypeID<T9>()), 1, TinScript::GetTypeID<T9>()) && success;
        success = fc->AddParameter("_p10", TinScript::Hash("_p10"), TinScript::GetRegisteredType(TinScript::GetTypeID<T10>()), 1, TinScript::GetTypeID<T10>()) && success;
        return (success);
    }

private:
    funcsignature funcptr;
};

template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10>
class CRegFunctionP10<void, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10> : public TinScript::CRegFunctionBase {
public:

    typedef void (*funcsignature)(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10);

    // -- CRegisterFunctionP10
    CRegFunctionP10(const char* _funcname, funcsignature _funcptr) :
                    CRegFunctionBase("", _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegFunctionP10() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        TinScript::CVariableEntry* ve4 = GetContext()->GetParameter(4);
        TinScript::CVariableEntry* ve5 = GetContext()->GetParameter(5);
        TinScript::CVariableEntry* ve6 = GetContext()->GetParameter(6);
        TinScript::CVariableEntry* ve7 = GetContext()->GetParameter(7);
        TinScript::CVariableEntry* ve8 = GetContext()->GetParameter(8);
        TinScript::CVariableEntry* ve9 = GetContext()->GetParameter(9);
        TinScript::CVariableEntry* ve10 = GetContext()->GetParameter(10);
        Dispatch(TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3),
                 TinScript::ConvertVariableForDispatch<T4>(ve4),
                 TinScript::ConvertVariableForDispatch<T5>(ve5),
                 TinScript::ConvertVariableForDispatch<T6>(ve6),
                 TinScript::ConvertVariableForDispatch<T7>(ve7),
                 TinScript::ConvertVariableForDispatch<T8>(ve8),
                 TinScript::ConvertVariableForDispatch<T9>(ve9),
                 TinScript::ConvertVariableForDispatch<T10>(ve10));
    }

    // -- dispatch method
    void Dispatch(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10) {
        funcptr(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::TYPE_void, 1, 0) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;
        success = fc->AddParameter("_p4", TinScript::Hash("_p4"), TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), 1, TinScript::GetTypeID<T4>()) && success;
        success = fc->AddParameter("_p5", TinScript::Hash("_p5"), TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), 1, TinScript::GetTypeID<T5>()) && success;
        success = fc->AddParameter("_p6", TinScript::Hash("_p6"), TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()), 1, TinScript::GetTypeID<T6>()) && success;
        success = fc->AddParameter("_p7", TinScript::Hash("_p7"), TinScript::GetRegisteredType(TinScript::GetTypeID<T7>()), 1, TinScript::GetTypeID<T7>()) && success;
        success = fc->AddParameter("_p8", TinScript::Hash("_p8"), TinScript::GetRegisteredType(TinScript::GetTypeID<T8>()), 1, TinScript::GetTypeID<T8>()) && success;
        success = fc->AddParameter("_p9", TinScript::Hash("_p9"), TinScript::GetRegisteredType(TinScript::GetTypeID<T9>()), 1, TinScript::GetTypeID<T9>()) && success;
        success = fc->AddParameter("_p10", TinScript::Hash("_p10"), TinScript::GetRegisteredType(TinScript::GetTypeID<T10>()), 1, TinScript::GetTypeID<T10>()) && success;

        return (success);
    }

private:
    funcsignature funcptr;
};

template<typename C, typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10>
class CRegMethodP10 : public TinScript::CRegFunctionBase {
public:

    typedef R (*methodsignature)(C* c, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10);

    // -- CRegisterMethodP10
    CRegMethodP10(const char* _funcname, methodsignature _funcptr) :
                  CRegFunctionBase(__GetClassName<C>(), _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegMethodP10() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        TinScript::CVariableEntry* ve4 = GetContext()->GetParameter(4);
        TinScript::CVariableEntry* ve5 = GetContext()->GetParameter(5);
        TinScript::CVariableEntry* ve6 = GetContext()->GetParameter(6);
        TinScript::CVariableEntry* ve7 = GetContext()->GetParameter(7);
        TinScript::CVariableEntry* ve8 = GetContext()->GetParameter(8);
        TinScript::CVariableEntry* ve9 = GetContext()->GetParameter(9);
        TinScript::CVariableEntry* ve10 = GetContext()->GetParameter(10);
        Dispatch(objaddr,
                 TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3),
                 TinScript::ConvertVariableForDispatch<T4>(ve4),
                 TinScript::ConvertVariableForDispatch<T5>(ve5),
                 TinScript::ConvertVariableForDispatch<T6>(ve6),
                 TinScript::ConvertVariableForDispatch<T7>(ve7),
                 TinScript::ConvertVariableForDispatch<T8>(ve8),
                 TinScript::ConvertVariableForDispatch<T9>(ve9),
                 TinScript::ConvertVariableForDispatch<T10>(ve10));
    }

    // -- dispatch method
    R Dispatch(void* objaddr, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10) {
        C* objptr = (C*)objaddr;
        R r = funcptr(objptr, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, TinScript::convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::GetRegisteredType(TinScript::GetTypeID<R>()), 1, TinScript::GetTypeID<R>()) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;
        success = fc->AddParameter("_p4", TinScript::Hash("_p4"), TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), 1, TinScript::GetTypeID<T4>()) && success;
        success = fc->AddParameter("_p5", TinScript::Hash("_p5"), TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), 1, TinScript::GetTypeID<T5>()) && success;
        success = fc->AddParameter("_p6", TinScript::Hash("_p6"), TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()), 1, TinScript::GetTypeID<T6>()) && success;
        success = fc->AddParameter("_p7", TinScript::Hash("_p7"), TinScript::GetRegisteredType(TinScript::GetTypeID<T7>()), 1, TinScript::GetTypeID<T7>()) && success;
        success = fc->AddParameter("_p8", TinScript::Hash("_p8"), TinScript::GetRegisteredType(TinScript::GetTypeID<T8>()), 1, TinScript::GetTypeID<T8>()) && success;
        success = fc->AddParameter("_p9", TinScript::Hash("_p9"), TinScript::GetRegisteredType(TinScript::GetTypeID<T9>()), 1, TinScript::GetTypeID<T9>()) && success;
        success = fc->AddParameter("_p10", TinScript::Hash("_p10"), TinScript::GetRegisteredType(TinScript::GetTypeID<T10>()), 1, TinScript::GetTypeID<T10>()) && success;

        return (success);
    }

private:
    methodsignature funcptr;
};

template<typename C, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10>
class CRegMethodP10<C, void, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10> : public TinScript::CRegFunctionBase {
public:

    typedef void (*methodsignature)(C* c, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10);

    // -- CRegisterMethodP10
    CRegMethodP10(const char* _funcname, methodsignature _funcptr) :
                  CRegFunctionBase(__GetClassName<C>(), _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegMethodP10() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        TinScript::CVariableEntry* ve4 = GetContext()->GetParameter(4);
        TinScript::CVariableEntry* ve5 = GetContext()->GetParameter(5);
        TinScript::CVariableEntry* ve6 = GetContext()->GetParameter(6);
        TinScript::CVariableEntry* ve7 = GetContext()->GetParameter(7);
        TinScript::CVariableEntry* ve8 = GetContext()->GetParameter(8);
        TinScript::CVariableEntry* ve9 = GetContext()->GetParameter(9);
        TinScript::CVariableEntry* ve10 = GetContext()->GetParameter(10);
        Dispatch(objaddr,
                 TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3),
                 TinScript::ConvertVariableForDispatch<T4>(ve4),
                 TinScript::ConvertVariableForDispatch<T5>(ve5),
                 TinScript::ConvertVariableForDispatch<T6>(ve6),
                 TinScript::ConvertVariableForDispatch<T7>(ve7),
                 TinScript::ConvertVariableForDispatch<T8>(ve8),
                 TinScript::ConvertVariableForDispatch<T9>(ve9),
                 TinScript::ConvertVariableForDispatch<T10>(ve10));
    }

    // -- dispatch method
    void Dispatch(void* objaddr, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10) {
        C* objptr = (C*)objaddr;
        funcptr(objptr, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::TYPE_void, 1, 0) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;
        success = fc->AddParameter("_p4", TinScript::Hash("_p4"), TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), 1, TinScript::GetTypeID<T4>()) && success;
        success = fc->AddParameter("_p5", TinScript::Hash("_p5"), TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), 1, TinScript::GetTypeID<T5>()) && success;
        success = fc->AddParameter("_p6", TinScript::Hash("_p6"), TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()), 1, TinScript::GetTypeID<T6>()) && success;
        success = fc->AddParameter("_p7", TinScript::Hash("_p7"), TinScript::GetRegisteredType(TinScript::GetTypeID<T7>()), 1, TinScript::GetTypeID<T7>()) && success;
        success = fc->AddParameter("_p8", TinScript::Hash("_p8"), TinScript::GetRegisteredType(TinScript::GetTypeID<T8>()), 1, TinScript::GetTypeID<T8>()) && success;
        success = fc->AddParameter("_p9", TinScript::Hash("_p9"), TinScript::GetRegisteredType(TinScript::GetTypeID<T9>()), 1, TinScript::GetTypeID<T9>()) && success;
        success = fc->AddParameter("_p10", TinScript::Hash("_p10"), TinScript::GetRegisteredType(TinScript::GetTypeID<T10>()), 1, TinScript::GetTypeID<T10>()) && success;

        return (success);
    }

private:
    methodsignature funcptr;
};


// -------------------
// Parameter count: 11
// -------------------

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11>
class CRegFunctionP11 : public TinScript::CRegFunctionBase {
public:

    typedef R (*funcsignature)(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11);

    // -- CRegisterFunctionP11
    CRegFunctionP11(const char* _funcname, funcsignature _funcptr) :
                    CRegFunctionBase("", _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegFunctionP11() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        TinScript::CVariableEntry* ve4 = GetContext()->GetParameter(4);
        TinScript::CVariableEntry* ve5 = GetContext()->GetParameter(5);
        TinScript::CVariableEntry* ve6 = GetContext()->GetParameter(6);
        TinScript::CVariableEntry* ve7 = GetContext()->GetParameter(7);
        TinScript::CVariableEntry* ve8 = GetContext()->GetParameter(8);
        TinScript::CVariableEntry* ve9 = GetContext()->GetParameter(9);
        TinScript::CVariableEntry* ve10 = GetContext()->GetParameter(10);
        TinScript::CVariableEntry* ve11 = GetContext()->GetParameter(11);
        Dispatch(TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3),
                 TinScript::ConvertVariableForDispatch<T4>(ve4),
                 TinScript::ConvertVariableForDispatch<T5>(ve5),
                 TinScript::ConvertVariableForDispatch<T6>(ve6),
                 TinScript::ConvertVariableForDispatch<T7>(ve7),
                 TinScript::ConvertVariableForDispatch<T8>(ve8),
                 TinScript::ConvertVariableForDispatch<T9>(ve9),
                 TinScript::ConvertVariableForDispatch<T10>(ve10),
                 TinScript::ConvertVariableForDispatch<T11>(ve11));
    }

    // -- dispatch method
    R Dispatch(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11) {
        R r = funcptr(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11);
        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, TinScript::convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::GetRegisteredType(TinScript::GetTypeID<R>()), 1, TinScript::GetTypeID<R>()) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;
        success = fc->AddParameter("_p4", TinScript::Hash("_p4"), TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), 1, TinScript::GetTypeID<T4>()) && success;
        success = fc->AddParameter("_p5", TinScript::Hash("_p5"), TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), 1, TinScript::GetTypeID<T5>()) && success;
        success = fc->AddParameter("_p6", TinScript::Hash("_p6"), TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()), 1, TinScript::GetTypeID<T6>()) && success;
        success = fc->AddParameter("_p7", TinScript::Hash("_p7"), TinScript::GetRegisteredType(TinScript::GetTypeID<T7>()), 1, TinScript::GetTypeID<T7>()) && success;
        success = fc->AddParameter("_p8", TinScript::Hash("_p8"), TinScript::GetRegisteredType(TinScript::GetTypeID<T8>()), 1, TinScript::GetTypeID<T8>()) && success;
        success = fc->AddParameter("_p9", TinScript::Hash("_p9"), TinScript::GetRegisteredType(TinScript::GetTypeID<T9>()), 1, TinScript::GetTypeID<T9>()) && success;
        success = fc->AddParameter("_p10", TinScript::Hash("_p10"), TinScript::GetRegisteredType(TinScript::GetTypeID<T10>()), 1, TinScript::GetTypeID<T10>()) && success;
        success = fc->AddParameter("_p11", TinScript::Hash("_p11"), TinScript::GetRegisteredType(TinScript::GetTypeID<T11>()), 1, TinScript::GetTypeID<T11>()) && success;
        return (success);
    }

private:
    funcsignature funcptr;
};

template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11>
class CRegFunctionP11<void, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11> : public TinScript::CRegFunctionBase {
public:

    typedef void (*funcsignature)(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11);

    // -- CRegisterFunctionP11
    CRegFunctionP11(const char* _funcname, funcsignature _funcptr) :
                    CRegFunctionBase("", _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegFunctionP11() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        TinScript::CVariableEntry* ve4 = GetContext()->GetParameter(4);
        TinScript::CVariableEntry* ve5 = GetContext()->GetParameter(5);
        TinScript::CVariableEntry* ve6 = GetContext()->GetParameter(6);
        TinScript::CVariableEntry* ve7 = GetContext()->GetParameter(7);
        TinScript::CVariableEntry* ve8 = GetContext()->GetParameter(8);
        TinScript::CVariableEntry* ve9 = GetContext()->GetParameter(9);
        TinScript::CVariableEntry* ve10 = GetContext()->GetParameter(10);
        TinScript::CVariableEntry* ve11 = GetContext()->GetParameter(11);
        Dispatch(TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3),
                 TinScript::ConvertVariableForDispatch<T4>(ve4),
                 TinScript::ConvertVariableForDispatch<T5>(ve5),
                 TinScript::ConvertVariableForDispatch<T6>(ve6),
                 TinScript::ConvertVariableForDispatch<T7>(ve7),
                 TinScript::ConvertVariableForDispatch<T8>(ve8),
                 TinScript::ConvertVariableForDispatch<T9>(ve9),
                 TinScript::ConvertVariableForDispatch<T10>(ve10),
                 TinScript::ConvertVariableForDispatch<T11>(ve11));
    }

    // -- dispatch method
    void Dispatch(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11) {
        funcptr(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::TYPE_void, 1, 0) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;
        success = fc->AddParameter("_p4", TinScript::Hash("_p4"), TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), 1, TinScript::GetTypeID<T4>()) && success;
        success = fc->AddParameter("_p5", TinScript::Hash("_p5"), TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), 1, TinScript::GetTypeID<T5>()) && success;
        success = fc->AddParameter("_p6", TinScript::Hash("_p6"), TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()), 1, TinScript::GetTypeID<T6>()) && success;
        success = fc->AddParameter("_p7", TinScript::Hash("_p7"), TinScript::GetRegisteredType(TinScript::GetTypeID<T7>()), 1, TinScript::GetTypeID<T7>()) && success;
        success = fc->AddParameter("_p8", TinScript::Hash("_p8"), TinScript::GetRegisteredType(TinScript::GetTypeID<T8>()), 1, TinScript::GetTypeID<T8>()) && success;
        success = fc->AddParameter("_p9", TinScript::Hash("_p9"), TinScript::GetRegisteredType(TinScript::GetTypeID<T9>()), 1, TinScript::GetTypeID<T9>()) && success;
        success = fc->AddParameter("_p10", TinScript::Hash("_p10"), TinScript::GetRegisteredType(TinScript::GetTypeID<T10>()), 1, TinScript::GetTypeID<T10>()) && success;
        success = fc->AddParameter("_p11", TinScript::Hash("_p11"), TinScript::GetRegisteredType(TinScript::GetTypeID<T11>()), 1, TinScript::GetTypeID<T11>()) && success;

        return (success);
    }

private:
    funcsignature funcptr;
};

template<typename C, typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11>
class CRegMethodP11 : public TinScript::CRegFunctionBase {
public:

    typedef R (*methodsignature)(C* c, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11);

    // -- CRegisterMethodP11
    CRegMethodP11(const char* _funcname, methodsignature _funcptr) :
                  CRegFunctionBase(__GetClassName<C>(), _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegMethodP11() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        TinScript::CVariableEntry* ve4 = GetContext()->GetParameter(4);
        TinScript::CVariableEntry* ve5 = GetContext()->GetParameter(5);
        TinScript::CVariableEntry* ve6 = GetContext()->GetParameter(6);
        TinScript::CVariableEntry* ve7 = GetContext()->GetParameter(7);
        TinScript::CVariableEntry* ve8 = GetContext()->GetParameter(8);
        TinScript::CVariableEntry* ve9 = GetContext()->GetParameter(9);
        TinScript::CVariableEntry* ve10 = GetContext()->GetParameter(10);
        TinScript::CVariableEntry* ve11 = GetContext()->GetParameter(11);
        Dispatch(objaddr,
                 TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3),
                 TinScript::ConvertVariableForDispatch<T4>(ve4),
                 TinScript::ConvertVariableForDispatch<T5>(ve5),
                 TinScript::ConvertVariableForDispatch<T6>(ve6),
                 TinScript::ConvertVariableForDispatch<T7>(ve7),
                 TinScript::ConvertVariableForDispatch<T8>(ve8),
                 TinScript::ConvertVariableForDispatch<T9>(ve9),
                 TinScript::ConvertVariableForDispatch<T10>(ve10),
                 TinScript::ConvertVariableForDispatch<T11>(ve11));
    }

    // -- dispatch method
    R Dispatch(void* objaddr, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11) {
        C* objptr = (C*)objaddr;
        R r = funcptr(objptr, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11);
        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, TinScript::convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::GetRegisteredType(TinScript::GetTypeID<R>()), 1, TinScript::GetTypeID<R>()) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;
        success = fc->AddParameter("_p4", TinScript::Hash("_p4"), TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), 1, TinScript::GetTypeID<T4>()) && success;
        success = fc->AddParameter("_p5", TinScript::Hash("_p5"), TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), 1, TinScript::GetTypeID<T5>()) && success;
        success = fc->AddParameter("_p6", TinScript::Hash("_p6"), TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()), 1, TinScript::GetTypeID<T6>()) && success;
        success = fc->AddParameter("_p7", TinScript::Hash("_p7"), TinScript::GetRegisteredType(TinScript::GetTypeID<T7>()), 1, TinScript::GetTypeID<T7>()) && success;
        success = fc->AddParameter("_p8", TinScript::Hash("_p8"), TinScript::GetRegisteredType(TinScript::GetTypeID<T8>()), 1, TinScript::GetTypeID<T8>()) && success;
        success = fc->AddParameter("_p9", TinScript::Hash("_p9"), TinScript::GetRegisteredType(TinScript::GetTypeID<T9>()), 1, TinScript::GetTypeID<T9>()) && success;
        success = fc->AddParameter("_p10", TinScript::Hash("_p10"), TinScript::GetRegisteredType(TinScript::GetTypeID<T10>()), 1, TinScript::GetTypeID<T10>()) && success;
        success = fc->AddParameter("_p11", TinScript::Hash("_p11"), TinScript::GetRegisteredType(TinScript::GetTypeID<T11>()), 1, TinScript::GetTypeID<T11>()) && success;

        return (success);
    }

private:
    methodsignature funcptr;
};

template<typename C, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11>
class CRegMethodP11<C, void, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11> : public TinScript::CRegFunctionBase {
public:

    typedef void (*methodsignature)(C* c, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11);

    // -- CRegisterMethodP11
    CRegMethodP11(const char* _funcname, methodsignature _funcptr) :
                  CRegFunctionBase(__GetClassName<C>(), _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegMethodP11() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        TinScript::CVariableEntry* ve4 = GetContext()->GetParameter(4);
        TinScript::CVariableEntry* ve5 = GetContext()->GetParameter(5);
        TinScript::CVariableEntry* ve6 = GetContext()->GetParameter(6);
        TinScript::CVariableEntry* ve7 = GetContext()->GetParameter(7);
        TinScript::CVariableEntry* ve8 = GetContext()->GetParameter(8);
        TinScript::CVariableEntry* ve9 = GetContext()->GetParameter(9);
        TinScript::CVariableEntry* ve10 = GetContext()->GetParameter(10);
        TinScript::CVariableEntry* ve11 = GetContext()->GetParameter(11);
        Dispatch(objaddr,
                 TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3),
                 TinScript::ConvertVariableForDispatch<T4>(ve4),
                 TinScript::ConvertVariableForDispatch<T5>(ve5),
                 TinScript::ConvertVariableForDispatch<T6>(ve6),
                 TinScript::ConvertVariableForDispatch<T7>(ve7),
                 TinScript::ConvertVariableForDispatch<T8>(ve8),
                 TinScript::ConvertVariableForDispatch<T9>(ve9),
                 TinScript::ConvertVariableForDispatch<T10>(ve10),
                 TinScript::ConvertVariableForDispatch<T11>(ve11));
    }

    // -- dispatch method
    void Dispatch(void* objaddr, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11) {
        C* objptr = (C*)objaddr;
        funcptr(objptr, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::TYPE_void, 1, 0) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;
        success = fc->AddParameter("_p4", TinScript::Hash("_p4"), TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), 1, TinScript::GetTypeID<T4>()) && success;
        success = fc->AddParameter("_p5", TinScript::Hash("_p5"), TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), 1, TinScript::GetTypeID<T5>()) && success;
        success = fc->AddParameter("_p6", TinScript::Hash("_p6"), TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()), 1, TinScript::GetTypeID<T6>()) && success;
        success = fc->AddParameter("_p7", TinScript::Hash("_p7"), TinScript::GetRegisteredType(TinScript::GetTypeID<T7>()), 1, TinScript::GetTypeID<T7>()) && success;
        success = fc->AddParameter("_p8", TinScript::Hash("_p8"), TinScript::GetRegisteredType(TinScript::GetTypeID<T8>()), 1, TinScript::GetTypeID<T8>()) && success;
        success = fc->AddParameter("_p9", TinScript::Hash("_p9"), TinScript::GetRegisteredType(TinScript::GetTypeID<T9>()), 1, TinScript::GetTypeID<T9>()) && success;
        success = fc->AddParameter("_p10", TinScript::Hash("_p10"), TinScript::GetRegisteredType(TinScript::GetTypeID<T10>()), 1, TinScript::GetTypeID<T10>()) && success;
        success = fc->AddParameter("_p11", TinScript::Hash("_p11"), TinScript::GetRegisteredType(TinScript::GetTypeID<T11>()), 1, TinScript::GetTypeID<T11>()) && success;

        return (success);
    }

private:
    methodsignature funcptr;
};


// -------------------
// Parameter count: 12
// -------------------

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12>
class CRegFunctionP12 : public TinScript::CRegFunctionBase {
public:

    typedef R (*funcsignature)(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11, T12 p12);

    // -- CRegisterFunctionP12
    CRegFunctionP12(const char* _funcname, funcsignature _funcptr) :
                    CRegFunctionBase("", _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegFunctionP12() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        TinScript::CVariableEntry* ve4 = GetContext()->GetParameter(4);
        TinScript::CVariableEntry* ve5 = GetContext()->GetParameter(5);
        TinScript::CVariableEntry* ve6 = GetContext()->GetParameter(6);
        TinScript::CVariableEntry* ve7 = GetContext()->GetParameter(7);
        TinScript::CVariableEntry* ve8 = GetContext()->GetParameter(8);
        TinScript::CVariableEntry* ve9 = GetContext()->GetParameter(9);
        TinScript::CVariableEntry* ve10 = GetContext()->GetParameter(10);
        TinScript::CVariableEntry* ve11 = GetContext()->GetParameter(11);
        TinScript::CVariableEntry* ve12 = GetContext()->GetParameter(12);
        Dispatch(TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3),
                 TinScript::ConvertVariableForDispatch<T4>(ve4),
                 TinScript::ConvertVariableForDispatch<T5>(ve5),
                 TinScript::ConvertVariableForDispatch<T6>(ve6),
                 TinScript::ConvertVariableForDispatch<T7>(ve7),
                 TinScript::ConvertVariableForDispatch<T8>(ve8),
                 TinScript::ConvertVariableForDispatch<T9>(ve9),
                 TinScript::ConvertVariableForDispatch<T10>(ve10),
                 TinScript::ConvertVariableForDispatch<T11>(ve11),
                 TinScript::ConvertVariableForDispatch<T12>(ve12));
    }

    // -- dispatch method
    R Dispatch(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11, T12 p12) {
        R r = funcptr(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12);
        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, TinScript::convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::GetRegisteredType(TinScript::GetTypeID<R>()), 1, TinScript::GetTypeID<R>()) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;
        success = fc->AddParameter("_p4", TinScript::Hash("_p4"), TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), 1, TinScript::GetTypeID<T4>()) && success;
        success = fc->AddParameter("_p5", TinScript::Hash("_p5"), TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), 1, TinScript::GetTypeID<T5>()) && success;
        success = fc->AddParameter("_p6", TinScript::Hash("_p6"), TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()), 1, TinScript::GetTypeID<T6>()) && success;
        success = fc->AddParameter("_p7", TinScript::Hash("_p7"), TinScript::GetRegisteredType(TinScript::GetTypeID<T7>()), 1, TinScript::GetTypeID<T7>()) && success;
        success = fc->AddParameter("_p8", TinScript::Hash("_p8"), TinScript::GetRegisteredType(TinScript::GetTypeID<T8>()), 1, TinScript::GetTypeID<T8>()) && success;
        success = fc->AddParameter("_p9", TinScript::Hash("_p9"), TinScript::GetRegisteredType(TinScript::GetTypeID<T9>()), 1, TinScript::GetTypeID<T9>()) && success;
        success = fc->AddParameter("_p10", TinScript::Hash("_p10"), TinScript::GetRegisteredType(TinScript::GetTypeID<T10>()), 1, TinScript::GetTypeID<T10>()) && success;
        success = fc->AddParameter("_p11", TinScript::Hash("_p11"), TinScript::GetRegisteredType(TinScript::GetTypeID<T11>()), 1, TinScript::GetTypeID<T11>()) && success;
        success = fc->AddParameter("_p12", TinScript::Hash("_p12"), TinScript::GetRegisteredType(TinScript::GetTypeID<T12>()), 1, TinScript::GetTypeID<T12>()) && success;
        return (success);
    }

private:
    funcsignature funcptr;
};

template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12>
class CRegFunctionP12<void, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12> : public TinScript::CRegFunctionBase {
public:

    typedef void (*funcsignature)(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11, T12 p12);

    // -- CRegisterFunctionP12
    CRegFunctionP12(const char* _funcname, funcsignature _funcptr) :
                    CRegFunctionBase("", _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegFunctionP12() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void*) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        TinScript::CVariableEntry* ve4 = GetContext()->GetParameter(4);
        TinScript::CVariableEntry* ve5 = GetContext()->GetParameter(5);
        TinScript::CVariableEntry* ve6 = GetContext()->GetParameter(6);
        TinScript::CVariableEntry* ve7 = GetContext()->GetParameter(7);
        TinScript::CVariableEntry* ve8 = GetContext()->GetParameter(8);
        TinScript::CVariableEntry* ve9 = GetContext()->GetParameter(9);
        TinScript::CVariableEntry* ve10 = GetContext()->GetParameter(10);
        TinScript::CVariableEntry* ve11 = GetContext()->GetParameter(11);
        TinScript::CVariableEntry* ve12 = GetContext()->GetParameter(12);
        Dispatch(TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3),
                 TinScript::ConvertVariableForDispatch<T4>(ve4),
                 TinScript::ConvertVariableForDispatch<T5>(ve5),
                 TinScript::ConvertVariableForDispatch<T6>(ve6),
                 TinScript::ConvertVariableForDispatch<T7>(ve7),
                 TinScript::ConvertVariableForDispatch<T8>(ve8),
                 TinScript::ConvertVariableForDispatch<T9>(ve9),
                 TinScript::ConvertVariableForDispatch<T10>(ve10),
                 TinScript::ConvertVariableForDispatch<T11>(ve11),
                 TinScript::ConvertVariableForDispatch<T12>(ve12));
    }

    // -- dispatch method
    void Dispatch(T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11, T12 p12) {
        funcptr(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::TYPE_void, 1, 0) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;
        success = fc->AddParameter("_p4", TinScript::Hash("_p4"), TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), 1, TinScript::GetTypeID<T4>()) && success;
        success = fc->AddParameter("_p5", TinScript::Hash("_p5"), TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), 1, TinScript::GetTypeID<T5>()) && success;
        success = fc->AddParameter("_p6", TinScript::Hash("_p6"), TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()), 1, TinScript::GetTypeID<T6>()) && success;
        success = fc->AddParameter("_p7", TinScript::Hash("_p7"), TinScript::GetRegisteredType(TinScript::GetTypeID<T7>()), 1, TinScript::GetTypeID<T7>()) && success;
        success = fc->AddParameter("_p8", TinScript::Hash("_p8"), TinScript::GetRegisteredType(TinScript::GetTypeID<T8>()), 1, TinScript::GetTypeID<T8>()) && success;
        success = fc->AddParameter("_p9", TinScript::Hash("_p9"), TinScript::GetRegisteredType(TinScript::GetTypeID<T9>()), 1, TinScript::GetTypeID<T9>()) && success;
        success = fc->AddParameter("_p10", TinScript::Hash("_p10"), TinScript::GetRegisteredType(TinScript::GetTypeID<T10>()), 1, TinScript::GetTypeID<T10>()) && success;
        success = fc->AddParameter("_p11", TinScript::Hash("_p11"), TinScript::GetRegisteredType(TinScript::GetTypeID<T11>()), 1, TinScript::GetTypeID<T11>()) && success;
        success = fc->AddParameter("_p12", TinScript::Hash("_p12"), TinScript::GetRegisteredType(TinScript::GetTypeID<T12>()), 1, TinScript::GetTypeID<T12>()) && success;

        return (success);
    }

private:
    funcsignature funcptr;
};

template<typename C, typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12>
class CRegMethodP12 : public TinScript::CRegFunctionBase {
public:

    typedef R (*methodsignature)(C* c, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11, T12 p12);

    // -- CRegisterMethodP12
    CRegMethodP12(const char* _funcname, methodsignature _funcptr) :
                  CRegFunctionBase(__GetClassName<C>(), _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegMethodP12() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        TinScript::CVariableEntry* ve4 = GetContext()->GetParameter(4);
        TinScript::CVariableEntry* ve5 = GetContext()->GetParameter(5);
        TinScript::CVariableEntry* ve6 = GetContext()->GetParameter(6);
        TinScript::CVariableEntry* ve7 = GetContext()->GetParameter(7);
        TinScript::CVariableEntry* ve8 = GetContext()->GetParameter(8);
        TinScript::CVariableEntry* ve9 = GetContext()->GetParameter(9);
        TinScript::CVariableEntry* ve10 = GetContext()->GetParameter(10);
        TinScript::CVariableEntry* ve11 = GetContext()->GetParameter(11);
        TinScript::CVariableEntry* ve12 = GetContext()->GetParameter(12);
        Dispatch(objaddr,
                 TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3),
                 TinScript::ConvertVariableForDispatch<T4>(ve4),
                 TinScript::ConvertVariableForDispatch<T5>(ve5),
                 TinScript::ConvertVariableForDispatch<T6>(ve6),
                 TinScript::ConvertVariableForDispatch<T7>(ve7),
                 TinScript::ConvertVariableForDispatch<T8>(ve8),
                 TinScript::ConvertVariableForDispatch<T9>(ve9),
                 TinScript::ConvertVariableForDispatch<T10>(ve10),
                 TinScript::ConvertVariableForDispatch<T11>(ve11),
                 TinScript::ConvertVariableForDispatch<T12>(ve12));
    }

    // -- dispatch method
    R Dispatch(void* objaddr, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11, T12 p12) {
        C* objptr = (C*)objaddr;
        R r = funcptr(objptr, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12);
        assert(GetContext()->GetParameter(0));
        TinScript::CVariableEntry* returnval = GetContext()->GetParameter(0);
        returnval->SetValueAddr(NULL, TinScript::convert_to_void_ptr<R>::Convert(r));
        return (r);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::GetRegisteredType(TinScript::GetTypeID<R>()), 1, TinScript::GetTypeID<R>()) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;
        success = fc->AddParameter("_p4", TinScript::Hash("_p4"), TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), 1, TinScript::GetTypeID<T4>()) && success;
        success = fc->AddParameter("_p5", TinScript::Hash("_p5"), TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), 1, TinScript::GetTypeID<T5>()) && success;
        success = fc->AddParameter("_p6", TinScript::Hash("_p6"), TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()), 1, TinScript::GetTypeID<T6>()) && success;
        success = fc->AddParameter("_p7", TinScript::Hash("_p7"), TinScript::GetRegisteredType(TinScript::GetTypeID<T7>()), 1, TinScript::GetTypeID<T7>()) && success;
        success = fc->AddParameter("_p8", TinScript::Hash("_p8"), TinScript::GetRegisteredType(TinScript::GetTypeID<T8>()), 1, TinScript::GetTypeID<T8>()) && success;
        success = fc->AddParameter("_p9", TinScript::Hash("_p9"), TinScript::GetRegisteredType(TinScript::GetTypeID<T9>()), 1, TinScript::GetTypeID<T9>()) && success;
        success = fc->AddParameter("_p10", TinScript::Hash("_p10"), TinScript::GetRegisteredType(TinScript::GetTypeID<T10>()), 1, TinScript::GetTypeID<T10>()) && success;
        success = fc->AddParameter("_p11", TinScript::Hash("_p11"), TinScript::GetRegisteredType(TinScript::GetTypeID<T11>()), 1, TinScript::GetTypeID<T11>()) && success;
        success = fc->AddParameter("_p12", TinScript::Hash("_p12"), TinScript::GetRegisteredType(TinScript::GetTypeID<T12>()), 1, TinScript::GetTypeID<T12>()) && success;

        return (success);
    }

private:
    methodsignature funcptr;
};

template<typename C, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12>
class CRegMethodP12<C, void, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12> : public TinScript::CRegFunctionBase {
public:

    typedef void (*methodsignature)(C* c, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11, T12 p12);

    // -- CRegisterMethodP12
    CRegMethodP12(const char* _funcname, methodsignature _funcptr) :
                  CRegFunctionBase(__GetClassName<C>(), _funcname) {
        funcptr = _funcptr;
    }

    // -- destructor
    virtual ~CRegMethodP12() {
    }

    // -- virtual DispatchFunction wrapper
    virtual void DispatchFunction(void* objaddr) {
        TinScript::CVariableEntry* ve1 = GetContext()->GetParameter(1);
        TinScript::CVariableEntry* ve2 = GetContext()->GetParameter(2);
        TinScript::CVariableEntry* ve3 = GetContext()->GetParameter(3);
        TinScript::CVariableEntry* ve4 = GetContext()->GetParameter(4);
        TinScript::CVariableEntry* ve5 = GetContext()->GetParameter(5);
        TinScript::CVariableEntry* ve6 = GetContext()->GetParameter(6);
        TinScript::CVariableEntry* ve7 = GetContext()->GetParameter(7);
        TinScript::CVariableEntry* ve8 = GetContext()->GetParameter(8);
        TinScript::CVariableEntry* ve9 = GetContext()->GetParameter(9);
        TinScript::CVariableEntry* ve10 = GetContext()->GetParameter(10);
        TinScript::CVariableEntry* ve11 = GetContext()->GetParameter(11);
        TinScript::CVariableEntry* ve12 = GetContext()->GetParameter(12);
        Dispatch(objaddr,
                 TinScript::ConvertVariableForDispatch<T1>(ve1),
                 TinScript::ConvertVariableForDispatch<T2>(ve2),
                 TinScript::ConvertVariableForDispatch<T3>(ve3),
                 TinScript::ConvertVariableForDispatch<T4>(ve4),
                 TinScript::ConvertVariableForDispatch<T5>(ve5),
                 TinScript::ConvertVariableForDispatch<T6>(ve6),
                 TinScript::ConvertVariableForDispatch<T7>(ve7),
                 TinScript::ConvertVariableForDispatch<T8>(ve8),
                 TinScript::ConvertVariableForDispatch<T9>(ve9),
                 TinScript::ConvertVariableForDispatch<T10>(ve10),
                 TinScript::ConvertVariableForDispatch<T11>(ve11),
                 TinScript::ConvertVariableForDispatch<T12>(ve12));
    }

    // -- dispatch method
    void Dispatch(void* objaddr, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11, T12 p12) {
        C* objptr = (C*)objaddr;
        funcptr(objptr, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12);
    }

    // -- registration method
    virtual bool Register() {
        TinScript::CFunctionContext* fc = CreateContext();
        bool success = true;
        success = fc->AddParameter("__return", TinScript::Hash("__return"), TinScript::TYPE_void, 1, 0) && success;
        success = fc->AddParameter("_p1", TinScript::Hash("_p1"), TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), 1, TinScript::GetTypeID<T1>()) && success;
        success = fc->AddParameter("_p2", TinScript::Hash("_p2"), TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), 1, TinScript::GetTypeID<T2>()) && success;
        success = fc->AddParameter("_p3", TinScript::Hash("_p3"), TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), 1, TinScript::GetTypeID<T3>()) && success;
        success = fc->AddParameter("_p4", TinScript::Hash("_p4"), TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), 1, TinScript::GetTypeID<T4>()) && success;
        success = fc->AddParameter("_p5", TinScript::Hash("_p5"), TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), 1, TinScript::GetTypeID<T5>()) && success;
        success = fc->AddParameter("_p6", TinScript::Hash("_p6"), TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()), 1, TinScript::GetTypeID<T6>()) && success;
        success = fc->AddParameter("_p7", TinScript::Hash("_p7"), TinScript::GetRegisteredType(TinScript::GetTypeID<T7>()), 1, TinScript::GetTypeID<T7>()) && success;
        success = fc->AddParameter("_p8", TinScript::Hash("_p8"), TinScript::GetRegisteredType(TinScript::GetTypeID<T8>()), 1, TinScript::GetTypeID<T8>()) && success;
        success = fc->AddParameter("_p9", TinScript::Hash("_p9"), TinScript::GetRegisteredType(TinScript::GetTypeID<T9>()), 1, TinScript::GetTypeID<T9>()) && success;
        success = fc->AddParameter("_p10", TinScript::Hash("_p10"), TinScript::GetRegisteredType(TinScript::GetTypeID<T10>()), 1, TinScript::GetTypeID<T10>()) && success;
        success = fc->AddParameter("_p11", TinScript::Hash("_p11"), TinScript::GetRegisteredType(TinScript::GetTypeID<T11>()), 1, TinScript::GetTypeID<T11>()) && success;
        success = fc->AddParameter("_p12", TinScript::Hash("_p12"), TinScript::GetRegisteredType(TinScript::GetTypeID<T12>()), 1, TinScript::GetTypeID<T12>()) && success;

        return (success);
    }

private:
    methodsignature funcptr;
};

