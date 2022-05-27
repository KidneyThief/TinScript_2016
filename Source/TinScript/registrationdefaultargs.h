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

class CRegisterDefaultArgsP0 : public TinScript::CRegDefaultArgValues {
public:

    CRegisterDefaultArgsP0(::TinScript::CRegFunctionBase* reg_object, const char* _r_name,
                            const char* _help_str = "")
        : CRegDefaultArgValues(reg_object, 0, _help_str)
    {
        mDefaultValues[0].mName = _r_name;
    }

    virtual int32 GetDefaultArgStorage(tDefaultValue*& out_storage)
    {
        out_storage = mDefaultValues;
        return 1;
    }

private:
    tDefaultValue mDefaultValues[1];

};

// -------------------
// Parameter count: 1
// -------------------

template<typename T1>
class CRegisterDefaultArgsP1 : public TinScript::CRegDefaultArgValues {
public:

    CRegisterDefaultArgsP1(::TinScript::CRegFunctionBase* reg_object, const char* _r_name,
                            const char* _p1_name, T1 _p1_value,
                            const char* _help_str = "")
        : CRegDefaultArgValues(reg_object, 1, _help_str)
    {
        mDefaultValues[0].mName = _r_name;
        mDefaultValues[1].mName = _p1_name;
        mDefaultValues[1].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T1>());
        memcpy(mDefaultValues[1].mValue, &_p1_value, sizeof(uint32) * MAX_TYPE_SIZE);

    }

    virtual int32 GetDefaultArgStorage(tDefaultValue*& out_storage)
    {
        out_storage = mDefaultValues;
        return 2;
    }

private:
    tDefaultValue mDefaultValues[2];

};

// -------------------
// Parameter count: 2
// -------------------

template<typename T1, typename T2>
class CRegisterDefaultArgsP2 : public TinScript::CRegDefaultArgValues {
public:

    CRegisterDefaultArgsP2(::TinScript::CRegFunctionBase* reg_object, const char* _r_name,
                            const char* _p1_name, T1 _p1_value,
                            const char* _p2_name, T2 _p2_value,
                            const char* _help_str = "")
        : CRegDefaultArgValues(reg_object, 2, _help_str)
    {
        mDefaultValues[0].mName = _r_name;
        mDefaultValues[1].mName = _p1_name;
        mDefaultValues[1].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T1>());
        memcpy(mDefaultValues[1].mValue, &_p1_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[2].mName = _p2_name;
        mDefaultValues[2].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T2>());
        memcpy(mDefaultValues[2].mValue, &_p2_value, sizeof(uint32) * MAX_TYPE_SIZE);

    }

    virtual int32 GetDefaultArgStorage(tDefaultValue*& out_storage)
    {
        out_storage = mDefaultValues;
        return 3;
    }

private:
    tDefaultValue mDefaultValues[3];

};

// -------------------
// Parameter count: 3
// -------------------

template<typename T1, typename T2, typename T3>
class CRegisterDefaultArgsP3 : public TinScript::CRegDefaultArgValues {
public:

    CRegisterDefaultArgsP3(::TinScript::CRegFunctionBase* reg_object, const char* _r_name,
                            const char* _p1_name, T1 _p1_value,
                            const char* _p2_name, T2 _p2_value,
                            const char* _p3_name, T3 _p3_value,
                            const char* _help_str = "")
        : CRegDefaultArgValues(reg_object, 3, _help_str)
    {
        mDefaultValues[0].mName = _r_name;
        mDefaultValues[1].mName = _p1_name;
        mDefaultValues[1].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T1>());
        memcpy(mDefaultValues[1].mValue, &_p1_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[2].mName = _p2_name;
        mDefaultValues[2].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T2>());
        memcpy(mDefaultValues[2].mValue, &_p2_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[3].mName = _p3_name;
        mDefaultValues[3].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T3>());
        memcpy(mDefaultValues[3].mValue, &_p3_value, sizeof(uint32) * MAX_TYPE_SIZE);

    }

    virtual int32 GetDefaultArgStorage(tDefaultValue*& out_storage)
    {
        out_storage = mDefaultValues;
        return 4;
    }

private:
    tDefaultValue mDefaultValues[4];

};

// -------------------
// Parameter count: 4
// -------------------

template<typename T1, typename T2, typename T3, typename T4>
class CRegisterDefaultArgsP4 : public TinScript::CRegDefaultArgValues {
public:

    CRegisterDefaultArgsP4(::TinScript::CRegFunctionBase* reg_object, const char* _r_name,
                            const char* _p1_name, T1 _p1_value,
                            const char* _p2_name, T2 _p2_value,
                            const char* _p3_name, T3 _p3_value,
                            const char* _p4_name, T4 _p4_value,
                            const char* _help_str = "")
        : CRegDefaultArgValues(reg_object, 4, _help_str)
    {
        mDefaultValues[0].mName = _r_name;
        mDefaultValues[1].mName = _p1_name;
        mDefaultValues[1].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T1>());
        memcpy(mDefaultValues[1].mValue, &_p1_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[2].mName = _p2_name;
        mDefaultValues[2].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T2>());
        memcpy(mDefaultValues[2].mValue, &_p2_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[3].mName = _p3_name;
        mDefaultValues[3].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T3>());
        memcpy(mDefaultValues[3].mValue, &_p3_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[4].mName = _p4_name;
        mDefaultValues[4].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T4>());
        memcpy(mDefaultValues[4].mValue, &_p4_value, sizeof(uint32) * MAX_TYPE_SIZE);

    }

    virtual int32 GetDefaultArgStorage(tDefaultValue*& out_storage)
    {
        out_storage = mDefaultValues;
        return 5;
    }

private:
    tDefaultValue mDefaultValues[5];

};

// -------------------
// Parameter count: 5
// -------------------

template<typename T1, typename T2, typename T3, typename T4, typename T5>
class CRegisterDefaultArgsP5 : public TinScript::CRegDefaultArgValues {
public:

    CRegisterDefaultArgsP5(::TinScript::CRegFunctionBase* reg_object, const char* _r_name,
                            const char* _p1_name, T1 _p1_value,
                            const char* _p2_name, T2 _p2_value,
                            const char* _p3_name, T3 _p3_value,
                            const char* _p4_name, T4 _p4_value,
                            const char* _p5_name, T5 _p5_value,
                            const char* _help_str = "")
        : CRegDefaultArgValues(reg_object, 5, _help_str)
    {
        mDefaultValues[0].mName = _r_name;
        mDefaultValues[1].mName = _p1_name;
        mDefaultValues[1].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T1>());
        memcpy(mDefaultValues[1].mValue, &_p1_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[2].mName = _p2_name;
        mDefaultValues[2].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T2>());
        memcpy(mDefaultValues[2].mValue, &_p2_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[3].mName = _p3_name;
        mDefaultValues[3].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T3>());
        memcpy(mDefaultValues[3].mValue, &_p3_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[4].mName = _p4_name;
        mDefaultValues[4].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T4>());
        memcpy(mDefaultValues[4].mValue, &_p4_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[5].mName = _p5_name;
        mDefaultValues[5].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T5>());
        memcpy(mDefaultValues[5].mValue, &_p5_value, sizeof(uint32) * MAX_TYPE_SIZE);

    }

    virtual int32 GetDefaultArgStorage(tDefaultValue*& out_storage)
    {
        out_storage = mDefaultValues;
        return 6;
    }

private:
    tDefaultValue mDefaultValues[6];

};

// -------------------
// Parameter count: 6
// -------------------

template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
class CRegisterDefaultArgsP6 : public TinScript::CRegDefaultArgValues {
public:

    CRegisterDefaultArgsP6(::TinScript::CRegFunctionBase* reg_object, const char* _r_name,
                            const char* _p1_name, T1 _p1_value,
                            const char* _p2_name, T2 _p2_value,
                            const char* _p3_name, T3 _p3_value,
                            const char* _p4_name, T4 _p4_value,
                            const char* _p5_name, T5 _p5_value,
                            const char* _p6_name, T6 _p6_value,
                            const char* _help_str = "")
        : CRegDefaultArgValues(reg_object, 6, _help_str)
    {
        mDefaultValues[0].mName = _r_name;
        mDefaultValues[1].mName = _p1_name;
        mDefaultValues[1].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T1>());
        memcpy(mDefaultValues[1].mValue, &_p1_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[2].mName = _p2_name;
        mDefaultValues[2].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T2>());
        memcpy(mDefaultValues[2].mValue, &_p2_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[3].mName = _p3_name;
        mDefaultValues[3].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T3>());
        memcpy(mDefaultValues[3].mValue, &_p3_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[4].mName = _p4_name;
        mDefaultValues[4].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T4>());
        memcpy(mDefaultValues[4].mValue, &_p4_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[5].mName = _p5_name;
        mDefaultValues[5].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T5>());
        memcpy(mDefaultValues[5].mValue, &_p5_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[6].mName = _p6_name;
        mDefaultValues[6].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T6>());
        memcpy(mDefaultValues[6].mValue, &_p6_value, sizeof(uint32) * MAX_TYPE_SIZE);

    }

    virtual int32 GetDefaultArgStorage(tDefaultValue*& out_storage)
    {
        out_storage = mDefaultValues;
        return 7;
    }

private:
    tDefaultValue mDefaultValues[7];

};

// -------------------
// Parameter count: 7
// -------------------

template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
class CRegisterDefaultArgsP7 : public TinScript::CRegDefaultArgValues {
public:

    CRegisterDefaultArgsP7(::TinScript::CRegFunctionBase* reg_object, const char* _r_name,
                            const char* _p1_name, T1 _p1_value,
                            const char* _p2_name, T2 _p2_value,
                            const char* _p3_name, T3 _p3_value,
                            const char* _p4_name, T4 _p4_value,
                            const char* _p5_name, T5 _p5_value,
                            const char* _p6_name, T6 _p6_value,
                            const char* _p7_name, T7 _p7_value,
                            const char* _help_str = "")
        : CRegDefaultArgValues(reg_object, 7, _help_str)
    {
        mDefaultValues[0].mName = _r_name;
        mDefaultValues[1].mName = _p1_name;
        mDefaultValues[1].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T1>());
        memcpy(mDefaultValues[1].mValue, &_p1_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[2].mName = _p2_name;
        mDefaultValues[2].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T2>());
        memcpy(mDefaultValues[2].mValue, &_p2_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[3].mName = _p3_name;
        mDefaultValues[3].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T3>());
        memcpy(mDefaultValues[3].mValue, &_p3_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[4].mName = _p4_name;
        mDefaultValues[4].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T4>());
        memcpy(mDefaultValues[4].mValue, &_p4_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[5].mName = _p5_name;
        mDefaultValues[5].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T5>());
        memcpy(mDefaultValues[5].mValue, &_p5_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[6].mName = _p6_name;
        mDefaultValues[6].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T6>());
        memcpy(mDefaultValues[6].mValue, &_p6_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[7].mName = _p7_name;
        mDefaultValues[7].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T7>());
        memcpy(mDefaultValues[7].mValue, &_p7_value, sizeof(uint32) * MAX_TYPE_SIZE);

    }

    virtual int32 GetDefaultArgStorage(tDefaultValue*& out_storage)
    {
        out_storage = mDefaultValues;
        return 8;
    }

private:
    tDefaultValue mDefaultValues[8];

};

// -------------------
// Parameter count: 8
// -------------------

template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
class CRegisterDefaultArgsP8 : public TinScript::CRegDefaultArgValues {
public:

    CRegisterDefaultArgsP8(::TinScript::CRegFunctionBase* reg_object, const char* _r_name,
                            const char* _p1_name, T1 _p1_value,
                            const char* _p2_name, T2 _p2_value,
                            const char* _p3_name, T3 _p3_value,
                            const char* _p4_name, T4 _p4_value,
                            const char* _p5_name, T5 _p5_value,
                            const char* _p6_name, T6 _p6_value,
                            const char* _p7_name, T7 _p7_value,
                            const char* _p8_name, T8 _p8_value,
                            const char* _help_str = "")
        : CRegDefaultArgValues(reg_object, 8, _help_str)
    {
        mDefaultValues[0].mName = _r_name;
        mDefaultValues[1].mName = _p1_name;
        mDefaultValues[1].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T1>());
        memcpy(mDefaultValues[1].mValue, &_p1_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[2].mName = _p2_name;
        mDefaultValues[2].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T2>());
        memcpy(mDefaultValues[2].mValue, &_p2_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[3].mName = _p3_name;
        mDefaultValues[3].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T3>());
        memcpy(mDefaultValues[3].mValue, &_p3_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[4].mName = _p4_name;
        mDefaultValues[4].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T4>());
        memcpy(mDefaultValues[4].mValue, &_p4_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[5].mName = _p5_name;
        mDefaultValues[5].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T5>());
        memcpy(mDefaultValues[5].mValue, &_p5_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[6].mName = _p6_name;
        mDefaultValues[6].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T6>());
        memcpy(mDefaultValues[6].mValue, &_p6_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[7].mName = _p7_name;
        mDefaultValues[7].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T7>());
        memcpy(mDefaultValues[7].mValue, &_p7_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[8].mName = _p8_name;
        mDefaultValues[8].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T8>());
        memcpy(mDefaultValues[8].mValue, &_p8_value, sizeof(uint32) * MAX_TYPE_SIZE);

    }

    virtual int32 GetDefaultArgStorage(tDefaultValue*& out_storage)
    {
        out_storage = mDefaultValues;
        return 9;
    }

private:
    tDefaultValue mDefaultValues[9];

};

// -------------------
// Parameter count: 9
// -------------------

template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
class CRegisterDefaultArgsP9 : public TinScript::CRegDefaultArgValues {
public:

    CRegisterDefaultArgsP9(::TinScript::CRegFunctionBase* reg_object, const char* _r_name,
                            const char* _p1_name, T1 _p1_value,
                            const char* _p2_name, T2 _p2_value,
                            const char* _p3_name, T3 _p3_value,
                            const char* _p4_name, T4 _p4_value,
                            const char* _p5_name, T5 _p5_value,
                            const char* _p6_name, T6 _p6_value,
                            const char* _p7_name, T7 _p7_value,
                            const char* _p8_name, T8 _p8_value,
                            const char* _p9_name, T9 _p9_value,
                            const char* _help_str = "")
        : CRegDefaultArgValues(reg_object, 9, _help_str)
    {
        mDefaultValues[0].mName = _r_name;
        mDefaultValues[1].mName = _p1_name;
        mDefaultValues[1].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T1>());
        memcpy(mDefaultValues[1].mValue, &_p1_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[2].mName = _p2_name;
        mDefaultValues[2].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T2>());
        memcpy(mDefaultValues[2].mValue, &_p2_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[3].mName = _p3_name;
        mDefaultValues[3].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T3>());
        memcpy(mDefaultValues[3].mValue, &_p3_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[4].mName = _p4_name;
        mDefaultValues[4].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T4>());
        memcpy(mDefaultValues[4].mValue, &_p4_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[5].mName = _p5_name;
        mDefaultValues[5].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T5>());
        memcpy(mDefaultValues[5].mValue, &_p5_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[6].mName = _p6_name;
        mDefaultValues[6].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T6>());
        memcpy(mDefaultValues[6].mValue, &_p6_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[7].mName = _p7_name;
        mDefaultValues[7].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T7>());
        memcpy(mDefaultValues[7].mValue, &_p7_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[8].mName = _p8_name;
        mDefaultValues[8].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T8>());
        memcpy(mDefaultValues[8].mValue, &_p8_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[9].mName = _p9_name;
        mDefaultValues[9].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T9>());
        memcpy(mDefaultValues[9].mValue, &_p9_value, sizeof(uint32) * MAX_TYPE_SIZE);

    }

    virtual int32 GetDefaultArgStorage(tDefaultValue*& out_storage)
    {
        out_storage = mDefaultValues;
        return 10;
    }

private:
    tDefaultValue mDefaultValues[10];

};

// -------------------
// Parameter count: 10
// -------------------

template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10>
class CRegisterDefaultArgsP10 : public TinScript::CRegDefaultArgValues {
public:

    CRegisterDefaultArgsP10(::TinScript::CRegFunctionBase* reg_object, const char* _r_name,
                            const char* _p1_name, T1 _p1_value,
                            const char* _p2_name, T2 _p2_value,
                            const char* _p3_name, T3 _p3_value,
                            const char* _p4_name, T4 _p4_value,
                            const char* _p5_name, T5 _p5_value,
                            const char* _p6_name, T6 _p6_value,
                            const char* _p7_name, T7 _p7_value,
                            const char* _p8_name, T8 _p8_value,
                            const char* _p9_name, T9 _p9_value,
                            const char* _p10_name, T10 _p10_value,
                            const char* _help_str = "")
        : CRegDefaultArgValues(reg_object, 10, _help_str)
    {
        mDefaultValues[0].mName = _r_name;
        mDefaultValues[1].mName = _p1_name;
        mDefaultValues[1].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T1>());
        memcpy(mDefaultValues[1].mValue, &_p1_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[2].mName = _p2_name;
        mDefaultValues[2].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T2>());
        memcpy(mDefaultValues[2].mValue, &_p2_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[3].mName = _p3_name;
        mDefaultValues[3].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T3>());
        memcpy(mDefaultValues[3].mValue, &_p3_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[4].mName = _p4_name;
        mDefaultValues[4].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T4>());
        memcpy(mDefaultValues[4].mValue, &_p4_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[5].mName = _p5_name;
        mDefaultValues[5].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T5>());
        memcpy(mDefaultValues[5].mValue, &_p5_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[6].mName = _p6_name;
        mDefaultValues[6].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T6>());
        memcpy(mDefaultValues[6].mValue, &_p6_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[7].mName = _p7_name;
        mDefaultValues[7].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T7>());
        memcpy(mDefaultValues[7].mValue, &_p7_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[8].mName = _p8_name;
        mDefaultValues[8].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T8>());
        memcpy(mDefaultValues[8].mValue, &_p8_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[9].mName = _p9_name;
        mDefaultValues[9].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T9>());
        memcpy(mDefaultValues[9].mValue, &_p9_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[10].mName = _p10_name;
        mDefaultValues[10].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T10>());
        memcpy(mDefaultValues[10].mValue, &_p10_value, sizeof(uint32) * MAX_TYPE_SIZE);

    }

    virtual int32 GetDefaultArgStorage(tDefaultValue*& out_storage)
    {
        out_storage = mDefaultValues;
        return 11;
    }

private:
    tDefaultValue mDefaultValues[11];

};

// -------------------
// Parameter count: 11
// -------------------

template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11>
class CRegisterDefaultArgsP11 : public TinScript::CRegDefaultArgValues {
public:

    CRegisterDefaultArgsP11(::TinScript::CRegFunctionBase* reg_object, const char* _r_name,
                            const char* _p1_name, T1 _p1_value,
                            const char* _p2_name, T2 _p2_value,
                            const char* _p3_name, T3 _p3_value,
                            const char* _p4_name, T4 _p4_value,
                            const char* _p5_name, T5 _p5_value,
                            const char* _p6_name, T6 _p6_value,
                            const char* _p7_name, T7 _p7_value,
                            const char* _p8_name, T8 _p8_value,
                            const char* _p9_name, T9 _p9_value,
                            const char* _p10_name, T10 _p10_value,
                            const char* _p11_name, T11 _p11_value,
                            const char* _help_str = "")
        : CRegDefaultArgValues(reg_object, 11, _help_str)
    {
        mDefaultValues[0].mName = _r_name;
        mDefaultValues[1].mName = _p1_name;
        mDefaultValues[1].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T1>());
        memcpy(mDefaultValues[1].mValue, &_p1_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[2].mName = _p2_name;
        mDefaultValues[2].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T2>());
        memcpy(mDefaultValues[2].mValue, &_p2_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[3].mName = _p3_name;
        mDefaultValues[3].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T3>());
        memcpy(mDefaultValues[3].mValue, &_p3_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[4].mName = _p4_name;
        mDefaultValues[4].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T4>());
        memcpy(mDefaultValues[4].mValue, &_p4_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[5].mName = _p5_name;
        mDefaultValues[5].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T5>());
        memcpy(mDefaultValues[5].mValue, &_p5_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[6].mName = _p6_name;
        mDefaultValues[6].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T6>());
        memcpy(mDefaultValues[6].mValue, &_p6_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[7].mName = _p7_name;
        mDefaultValues[7].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T7>());
        memcpy(mDefaultValues[7].mValue, &_p7_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[8].mName = _p8_name;
        mDefaultValues[8].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T8>());
        memcpy(mDefaultValues[8].mValue, &_p8_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[9].mName = _p9_name;
        mDefaultValues[9].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T9>());
        memcpy(mDefaultValues[9].mValue, &_p9_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[10].mName = _p10_name;
        mDefaultValues[10].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T10>());
        memcpy(mDefaultValues[10].mValue, &_p10_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[11].mName = _p11_name;
        mDefaultValues[11].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T11>());
        memcpy(mDefaultValues[11].mValue, &_p11_value, sizeof(uint32) * MAX_TYPE_SIZE);

    }

    virtual int32 GetDefaultArgStorage(tDefaultValue*& out_storage)
    {
        out_storage = mDefaultValues;
        return 12;
    }

private:
    tDefaultValue mDefaultValues[12];

};

// -------------------
// Parameter count: 12
// -------------------

template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12>
class CRegisterDefaultArgsP12 : public TinScript::CRegDefaultArgValues {
public:

    CRegisterDefaultArgsP12(::TinScript::CRegFunctionBase* reg_object, const char* _r_name,
                            const char* _p1_name, T1 _p1_value,
                            const char* _p2_name, T2 _p2_value,
                            const char* _p3_name, T3 _p3_value,
                            const char* _p4_name, T4 _p4_value,
                            const char* _p5_name, T5 _p5_value,
                            const char* _p6_name, T6 _p6_value,
                            const char* _p7_name, T7 _p7_value,
                            const char* _p8_name, T8 _p8_value,
                            const char* _p9_name, T9 _p9_value,
                            const char* _p10_name, T10 _p10_value,
                            const char* _p11_name, T11 _p11_value,
                            const char* _p12_name, T12 _p12_value,
                            const char* _help_str = "")
        : CRegDefaultArgValues(reg_object, 12, _help_str)
    {
        mDefaultValues[0].mName = _r_name;
        mDefaultValues[1].mName = _p1_name;
        mDefaultValues[1].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T1>());
        memcpy(mDefaultValues[1].mValue, &_p1_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[2].mName = _p2_name;
        mDefaultValues[2].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T2>());
        memcpy(mDefaultValues[2].mValue, &_p2_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[3].mName = _p3_name;
        mDefaultValues[3].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T3>());
        memcpy(mDefaultValues[3].mValue, &_p3_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[4].mName = _p4_name;
        mDefaultValues[4].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T4>());
        memcpy(mDefaultValues[4].mValue, &_p4_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[5].mName = _p5_name;
        mDefaultValues[5].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T5>());
        memcpy(mDefaultValues[5].mValue, &_p5_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[6].mName = _p6_name;
        mDefaultValues[6].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T6>());
        memcpy(mDefaultValues[6].mValue, &_p6_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[7].mName = _p7_name;
        mDefaultValues[7].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T7>());
        memcpy(mDefaultValues[7].mValue, &_p7_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[8].mName = _p8_name;
        mDefaultValues[8].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T8>());
        memcpy(mDefaultValues[8].mValue, &_p8_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[9].mName = _p9_name;
        mDefaultValues[9].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T9>());
        memcpy(mDefaultValues[9].mValue, &_p9_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[10].mName = _p10_name;
        mDefaultValues[10].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T10>());
        memcpy(mDefaultValues[10].mValue, &_p10_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[11].mName = _p11_name;
        mDefaultValues[11].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T11>());
        memcpy(mDefaultValues[11].mValue, &_p11_value, sizeof(uint32) * MAX_TYPE_SIZE);

        mDefaultValues[12].mName = _p12_name;
        mDefaultValues[12].mType = TinScript::GetRegisteredType(TinScript::GetTypeID<T12>());
        memcpy(mDefaultValues[12].mValue, &_p12_value, sizeof(uint32) * MAX_TYPE_SIZE);

    }

    virtual int32 GetDefaultArgStorage(tDefaultValue*& out_storage)
    {
        out_storage = mDefaultValues;
        return 13;
    }

private:
    tDefaultValue mDefaultValues[13];

};
