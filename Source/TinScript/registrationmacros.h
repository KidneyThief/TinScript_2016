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
// Generated macros for function registration
// ------------------------------------------------------------------------------------------------


// -- Parameter count: 0
#define REGISTER_FUNCTION_P0(scriptname, funcname, R) \
    static TinScript::CRegFunctionP0<R> _reg_##scriptname(#scriptname, funcname);

#define REGISTER_METHOD_P0(classname, scriptname, methodname, R)    \
    static R classname##methodname(classname* obj) {    \
        return obj->methodname();    \
    }    \
    static TinScript::CRegMethodP0<classname, R> _reg_##classname##methodname(#scriptname, classname##methodname);

// -- Parameter count: 1
#define REGISTER_FUNCTION_P1(scriptname, funcname, R, T1) \
    static TinScript::CRegFunctionP1<R, T1> _reg_##scriptname(#scriptname, funcname);

#define REGISTER_METHOD_P1(classname, scriptname, methodname, R, T1)    \
    static R classname##methodname(classname* obj, T1 t1) {    \
        return obj->methodname(t1);    \
    }    \
    static TinScript::CRegMethodP1<classname, R, T1> _reg_##classname##methodname(#scriptname, classname##methodname);

// -- Parameter count: 2
#define REGISTER_FUNCTION_P2(scriptname, funcname, R, T1, T2) \
    static TinScript::CRegFunctionP2<R, T1, T2> _reg_##scriptname(#scriptname, funcname);

#define REGISTER_METHOD_P2(classname, scriptname, methodname, R, T1, T2)    \
    static R classname##methodname(classname* obj, T1 t1, T2 t2) {    \
        return obj->methodname(t1, t2);    \
    }    \
    static TinScript::CRegMethodP2<classname, R, T1, T2> _reg_##classname##methodname(#scriptname, classname##methodname);

// -- Parameter count: 3
#define REGISTER_FUNCTION_P3(scriptname, funcname, R, T1, T2, T3) \
    static TinScript::CRegFunctionP3<R, T1, T2, T3> _reg_##scriptname(#scriptname, funcname);

#define REGISTER_METHOD_P3(classname, scriptname, methodname, R, T1, T2, T3)    \
    static R classname##methodname(classname* obj, T1 t1, T2 t2, T3 t3) {    \
        return obj->methodname(t1, t2, t3);    \
    }    \
    static TinScript::CRegMethodP3<classname, R, T1, T2, T3> _reg_##classname##methodname(#scriptname, classname##methodname);

// -- Parameter count: 4
#define REGISTER_FUNCTION_P4(scriptname, funcname, R, T1, T2, T3, T4) \
    static TinScript::CRegFunctionP4<R, T1, T2, T3, T4> _reg_##scriptname(#scriptname, funcname);

#define REGISTER_METHOD_P4(classname, scriptname, methodname, R, T1, T2, T3, T4)    \
    static R classname##methodname(classname* obj, T1 t1, T2 t2, T3 t3, T4 t4) {    \
        return obj->methodname(t1, t2, t3, t4);    \
    }    \
    static TinScript::CRegMethodP4<classname, R, T1, T2, T3, T4> _reg_##classname##methodname(#scriptname, classname##methodname);

// -- Parameter count: 5
#define REGISTER_FUNCTION_P5(scriptname, funcname, R, T1, T2, T3, T4, T5) \
    static TinScript::CRegFunctionP5<R, T1, T2, T3, T4, T5> _reg_##scriptname(#scriptname, funcname);

#define REGISTER_METHOD_P5(classname, scriptname, methodname, R, T1, T2, T3, T4, T5)    \
    static R classname##methodname(classname* obj, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5) {    \
        return obj->methodname(t1, t2, t3, t4, t5);    \
    }    \
    static TinScript::CRegMethodP5<classname, R, T1, T2, T3, T4, T5> _reg_##classname##methodname(#scriptname, classname##methodname);

// -- Parameter count: 6
#define REGISTER_FUNCTION_P6(scriptname, funcname, R, T1, T2, T3, T4, T5, T6) \
    static TinScript::CRegFunctionP6<R, T1, T2, T3, T4, T5, T6> _reg_##scriptname(#scriptname, funcname);

#define REGISTER_METHOD_P6(classname, scriptname, methodname, R, T1, T2, T3, T4, T5, T6)    \
    static R classname##methodname(classname* obj, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6) {    \
        return obj->methodname(t1, t2, t3, t4, t5, t6);    \
    }    \
    static TinScript::CRegMethodP6<classname, R, T1, T2, T3, T4, T5, T6> _reg_##classname##methodname(#scriptname, classname##methodname);

// -- Parameter count: 7
#define REGISTER_FUNCTION_P7(scriptname, funcname, R, T1, T2, T3, T4, T5, T6, T7) \
    static TinScript::CRegFunctionP7<R, T1, T2, T3, T4, T5, T6, T7> _reg_##scriptname(#scriptname, funcname);

#define REGISTER_METHOD_P7(classname, scriptname, methodname, R, T1, T2, T3, T4, T5, T6, T7)    \
    static R classname##methodname(classname* obj, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7) {    \
        return obj->methodname(t1, t2, t3, t4, t5, t6, t7);    \
    }    \
    static TinScript::CRegMethodP7<classname, R, T1, T2, T3, T4, T5, T6, T7> _reg_##classname##methodname(#scriptname, classname##methodname);

// -- Parameter count: 8
#define REGISTER_FUNCTION_P8(scriptname, funcname, R, T1, T2, T3, T4, T5, T6, T7, T8) \
    static TinScript::CRegFunctionP8<R, T1, T2, T3, T4, T5, T6, T7, T8> _reg_##scriptname(#scriptname, funcname);

#define REGISTER_METHOD_P8(classname, scriptname, methodname, R, T1, T2, T3, T4, T5, T6, T7, T8)    \
    static R classname##methodname(classname* obj, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8) {    \
        return obj->methodname(t1, t2, t3, t4, t5, t6, t7, t8);    \
    }    \
    static TinScript::CRegMethodP8<classname, R, T1, T2, T3, T4, T5, T6, T7, T8> _reg_##classname##methodname(#scriptname, classname##methodname);

// -- Parameter count: 9
#define REGISTER_FUNCTION_P9(scriptname, funcname, R, T1, T2, T3, T4, T5, T6, T7, T8, T9) \
    static TinScript::CRegFunctionP9<R, T1, T2, T3, T4, T5, T6, T7, T8, T9> _reg_##scriptname(#scriptname, funcname);

#define REGISTER_METHOD_P9(classname, scriptname, methodname, R, T1, T2, T3, T4, T5, T6, T7, T8, T9)    \
    static R classname##methodname(classname* obj, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8, T9 t9) {    \
        return obj->methodname(t1, t2, t3, t4, t5, t6, t7, t8, t9);    \
    }    \
    static TinScript::CRegMethodP9<classname, R, T1, T2, T3, T4, T5, T6, T7, T8, T9> _reg_##classname##methodname(#scriptname, classname##methodname);

// -- Parameter count: 10
#define REGISTER_FUNCTION_P10(scriptname, funcname, R, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10) \
    static TinScript::CRegFunctionP10<R, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10> _reg_##scriptname(#scriptname, funcname);

#define REGISTER_METHOD_P10(classname, scriptname, methodname, R, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10)    \
    static R classname##methodname(classname* obj, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8, T9 t9, T10 t10) {    \
        return obj->methodname(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10);    \
    }    \
    static TinScript::CRegMethodP10<classname, R, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10> _reg_##classname##methodname(#scriptname, classname##methodname);

// -- Parameter count: 11
#define REGISTER_FUNCTION_P11(scriptname, funcname, R, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11) \
    static TinScript::CRegFunctionP11<R, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11> _reg_##scriptname(#scriptname, funcname);

#define REGISTER_METHOD_P11(classname, scriptname, methodname, R, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11)    \
    static R classname##methodname(classname* obj, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8, T9 t9, T10 t10, T11 t11) {    \
        return obj->methodname(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11);    \
    }    \
    static TinScript::CRegMethodP11<classname, R, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11> _reg_##classname##methodname(#scriptname, classname##methodname);

// -- Parameter count: 12
#define REGISTER_FUNCTION_P12(scriptname, funcname, R, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12) \
    static TinScript::CRegFunctionP12<R, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12> _reg_##scriptname(#scriptname, funcname);

#define REGISTER_METHOD_P12(classname, scriptname, methodname, R, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12)    \
    static R classname##methodname(classname* obj, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8, T9 t9, T10 t10, T11 t11, T12 t12) {    \
        return obj->methodname(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12);    \
    }    \
    static TinScript::CRegMethodP12<classname, R, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12> _reg_##classname##methodname(#scriptname, classname##methodname);
