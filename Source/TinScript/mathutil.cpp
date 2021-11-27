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

// -- system includes
#include <cstdlib>
#include <functional>

// -- includes
#include "mathutil.h"

// -- includes required by any system wanting access to TinScript
#include "integration.h"
#include "TinScript.h"
#include "TinRegistration.h"

// -- use the DECLARE_FILE/REGISTER_FILE macros to prevent deadstripping
DECLARE_FILE(mathutil_cpp);

// == CVector3f =======================================================================================================

const CVector3f CVector3f::zero(0.0f, 0.0f, 0.0f);
const CVector3f CVector3f::realmax(1e8f, 1e8f, 1e8f);

// ====================================================================================================================
// Set():  Sets the x,y,z values
// ====================================================================================================================
void CVector3f::Set(float32 _x, float32 _y, float32 _z)
{
    x = _x;
    y = _y;
    z = _z;
}

// ====================================================================================================================
// Length():  Returns the length of the vector
// ====================================================================================================================
float32 CVector3f::Length()
{
    float32 length = (float32)sqrt(float32(x*x + y*y + z*z));
    return (length);
}

// ====================================================================================================================
// Normalize():  Normalizes the vector, returns the length
// ====================================================================================================================
float32 CVector3f::Normalize()
{
    float32 length = (float32)sqrt(x*x + y*y + z*z);
    if (length > 0.0f)
    {
        x /= length;
        y /= length;
        z /= length;
    }

    // -- return the length
    return (length);
}

// ====================================================================================================================
// Cross():  Returns the cross product of two vectors
// ====================================================================================================================
CVector3f CVector3f::Cross(CVector3f v0, CVector3f v1)
{
    CVector3f result(v0.y * v1.z - v0.z * v1.y,
                     v0.z * v1.x - v0.x * v1.z,
                     v0.x * v1.y - v0.y * v1.x);
    return (result);
}

// ====================================================================================================================
// Dot():  Returns the dot product of two vectors
// ====================================================================================================================
float32 CVector3f::Dot(CVector3f v0, CVector3f v1)
{
    float dot = v0.x * v1.x + v0.y * v1.y + v0.z * v1.z;
    return dot;
}

// ====================================================================================================================
// Normalized():  Returns the value of the input vector normalized
// ====================================================================================================================
CVector3f CVector3f::Normalized(CVector3f v0)
{
    v0.Normalize();
    return (v0);
}

// ====================================================================================================================
// Length():  Returns the length of the input vector
// ====================================================================================================================
float32 CVector3f::V3fLength(CVector3f v0)
{
    return (v0.Length());
}

// --------------------------------------------------------------------------------------------------------------------
// -- registered functions
bool8 TS_Cross(CVector3f* result, CVector3f* v0, CVector3f* v1)
{
    if (!result || !v0 || !v1)
    {
        ScriptAssert_(::TinScript::GetContext(), result && v0 && v1, "<internal>", -1,
                      "Error - Cross():  Unable to find the result/v0/v1 objects\n");
        return (false);
    }

    // -- cross the vectors, assign the result
    *result = CVector3f::Cross(*v0, *v1);

    // -- success
    return (true);
}

// --------------------------------------------------------------------------------------------------------------------
// -- registered functions
float32 TS_Dot(CVector3f* v0, CVector3f* v1)
{
    if (!v0 || !v1)
    {
        ScriptAssert_(::TinScript::GetContext(), v0 && v1, "<internal>", -1,
                      "Error - Dot():  Unable to find the v0/v1 objects\n");
        return (0.0f);
    }

    // -- return the dot product
    float dot = CVector3f::Dot(*v0, *v1);
    return (dot);
}

// --------------------------------------------------------------------------------------------------------------------
// -- registered functions
float32 TS_Normalized(CVector3f* result, CVector3f* v0)
{
    if (!result || !v0)
    {
        ScriptAssert_(::TinScript::GetContext(), result && v0, "<internal>", -1,
                      "Error - Normalized():  Unable to find the result/v0 objects\n");
        return (0.0f);
    }

    // -- return the dot product
    *result = *v0;
    float32 length = result->Normalize();
    return (length);
}

// --------------------------------------------------------------------------------------------------------------------
// -- registration - CVector3f as a object, using create, destroy, etc..
REGISTER_SCRIPT_CLASS_BEGIN(CVector3f, VOID)
    REGISTER_MEMBER(CVector3f, x, x);
    REGISTER_MEMBER(CVector3f, y, y);
    REGISTER_MEMBER(CVector3f, z, z);
REGISTER_SCRIPT_CLASS_END()

REGISTER_METHOD(CVector3f, Set, Set);
REGISTER_METHOD(CVector3f, Length, Length);
REGISTER_METHOD(CVector3f, Normalize, Normalize);

REGISTER_FUNCTION(ObjCross, TS_Cross);
REGISTER_FUNCTION(ObjDot, TS_Dot);
REGISTER_FUNCTION(ObjNormalized, TS_Normalized);

// --------------------------------------------------------------------------------------------------------------------
// -- Re-registered using the registered type, instead of having to find an object
REGISTER_CLASS_FUNCTION(CVector3f, V3fLength, V3fLength);
REGISTER_CLASS_FUNCTION(CVector3f, V3fCross, Cross);
REGISTER_CLASS_FUNCTION(CVector3f, V3fDot, Dot);
REGISTER_CLASS_FUNCTION(CVector3f, V3fNormalized, Normalized);

// ====================================================================================================================
// Random Numbers
// ====================================================================================================================
float32 Random()
{
    float32 rand_num = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    return (rand_num);
}

float32 RandomRange(float32 low, float32 high)
{
    // -- sanity check
    if (high < low)
        high = low;

    float32 rand_num = low + Random() * (high - low);
    return (rand_num);
}

int32 RandomInt(int32 exclusiveMax)
{
    float32 rand_num = Random() * static_cast<float32>(exclusiveMax);
    int32 result = static_cast<int32>(rand_num);
    return (result);
}

REGISTER_FUNCTION(Random, Random);
REGISTER_FUNCTION(RandomRange, RandomRange);
REGISTER_FUNCTION(RandomInt, RandomInt);

// ====================================================================================================================
// Trigonometry
// ====================================================================================================================
static const float32 kPI = 3.1415926535f;
float32 Cos(float32 degrees)
{
    return (cosf(degrees * kPI / 180.0f));
}

float32 Sin(float32 degrees)
{
    return (sinf(degrees * kPI / 180.0f));
}

float32 Atan2(float32 y, float32 x)
{
    float32 angle = (float32)atan2(y, x);

    // -- convert the angle back to degrees
    angle *= (180.0f / kPI);
    return (angle);
}

REGISTER_FUNCTION(Cos, Cos);
REGISTER_FUNCTION(Sin, Sin);
REGISTER_FUNCTION(Atan2, Atan2);

// ====================================================================================================================
// EOF
// ====================================================================================================================
