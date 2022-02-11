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

#ifndef __MATHUTIL_H
#define __MATHUTIL_H

// -- lib includes
#include "math.h"

// -- includes required by any system wanting access to TinScript
#include "TinScript.h"
#include "TinRegistration.h"

#if PLATFORM_UE4
	struct FVector;
#endif

// ====================================================================================================================
// class CVector3f
// Simple implementation of a 3D float class
// ====================================================================================================================
class CVector3f {

public:

    CVector3f(float32 _x = 0.0f, float32 _y = 0.0f, float32 _z = 0.0f)
    {
        x = _x; y = _y; z = _z;
    }


#if PLATFORM_UE4
	// -- if we're using an Unreal FVector type (which could be doubles for x, y, z), we need a conversion operator
	CVector3f operator=(const FVector& inValue);
	explicit operator FVector();
#endif

    CVector3f& operator=(const CVector3f& rhs)
    {
        x = rhs.x; y = rhs.y; z = rhs.z;
        return *this;
    }

    CVector3f& operator+(const CVector3f& rhs)
    {
        x += rhs.x; y += rhs.y; z += rhs.z;
        return *this;
    }

    CVector3f& operator-(const CVector3f& rhs)
    {
        x -= rhs.x; y -= rhs.y; z -= rhs.z;
        return *this;
    }

    CVector3f& operator*(const float32 s)
    {
        x *= s; y *= s; z *= s;
        return *this;
    }

    CVector3f& operator/(const float32 s)
    {
        x /= s; y /= s; z /= s;
        return *this;
    }

    bool8 operator==(const CVector3f& rhs)
    {
        return (x == rhs.x && y == rhs.y && z == rhs.z);
    }

    bool8 operator!=(const CVector3f& rhs)
    {
        return (x != rhs.x || y != rhs.y || z != rhs.z);
    }

    // -- registered methods
    void Set(float32 _x, float32 _y, float32 _z);
    float32 Length();
    float32 Normalize();

    static CVector3f Cross(CVector3f v0, CVector3f v1);
    static float32 Dot(CVector3f v0, CVector3f v1);
    static float32 V3fLength(CVector3f v0);
    static CVector3f Normalized(CVector3f v0);

    static const CVector3f zero;
    static const CVector3f realmax;

    float32 x;
    float32 y;
    float32 z;
};

// ====================================================================================================================
// Random Numbers
// ====================================================================================================================
float32 Random();
float32 RandomRange(float32 low, float32 high);
int32 RandomInt(int32 exclusiveMax);

// -- platform specific Vector3f wrappers -----------------------------------------------------------------------------
// 
// -- used by unit tests, we need platform specific wrappers
// obviously for *real* code, nothing should ever call this wrappers
inline float TS_V3fLength(CVector3f v0)
{
    return CVector3f::V3fLength(v0);
}

inline CVector3f TS_V3fCrossProduct(CVector3f v0, CVector3f v1)
{
    return CVector3f::Cross(v0, v1);
}

inline float TS_V3fDotProduct(CVector3f v0, CVector3f v1)
{
    return CVector3f::Dot(v0, v1);
}

inline CVector3f TS_V3fNormalized(CVector3f v0)
{
    return CVector3f::Normalized(v0);
}

#endif // __MATHUTIL_H

// ====================================================================================================================
// EOF
// ====================================================================================================================
