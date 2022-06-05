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
// TinTypeVector3f.cpp : The registered type vector3f required methods
// ------------------------------------------------------------------------------------------------

// -- lib includes
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

#include "mathutil.h"

#include "integration.h"
#include "TinHash.h"
#include "TinCompile.h"
#include "TinTypes.h"
#include "TinScript.h"
#include "TinStringTable.h"
#include "TinOpExecFunctions.h"
#include "TinRegBinding.h"

#if PLATFORM_UE4
	#include "math/Vector.h"
#endif

namespace TinScript {

// --------------------------------------------------------------------------------------------------------------------
// -- vector3f POD table
tPODTypeTable* gVector3fTable = nullptr;

// --------------------------------------------------------------------------------------------------------------------
// External type - CVector3f is a POD type (aggregate, but no user-defined constructors)
bool8 Vector3fToString(TinScript::CScriptContext* script_context, void* value, char* buf, int32 bufsize)
{
	if(value && buf && bufsize > 0)
    {
        Vector3fClass* c3vector = (Vector3fClass*)value;
		float x = (float)c3vector->v3f_X;
		float y = (float)c3vector->v3f_Y;
		float z = (float)c3vector->v3f_Z;
        snprintf(buf, bufsize, "%.4f %.4f %.4f", x, y, z);
		return (true);
	}
	return (false);
}

bool8 StringToVector3f(TinScript::CScriptContext* script_context, void* addr, char* value)
{
	if (addr && value)
    {
        Vector3fClass* varaddr = (Vector3fClass*)addr;

		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;

		// -- if the string is non-empty, ensure we can read in either space or comman delineated format
		bool success = true;
        if (value && value[0])
        {
			success = sscanf_s(value, "%f %f %f", &x, &y, &z) == 3;
			success = success || (sscanf_s(value, "%f, %f, %f", &x, &y, &z) == 3);
		}

		if (success)
		{
			*varaddr = Vector3fClass(x, y, z);
			return (true);
        }
	}
	return (false);
}

// --------------------------------------------------------------------------------------------------------------------
// -- Vector3f only supports specific operations
bool8 Vector3fOpOverrides(CScriptContext* script_context, eOpCode op, eVarType& result_type, void* result_addr,
                          eVarType val0_type, void* val0, eVarType val1_type, void* val1)
{
    // -- sanity check
    if (!script_context || !result_addr || !val0 || !val1)
        return (false);

    // -- ensure the types are converted to vector3f
    void* val0addr = TypeConvert(script_context, val0_type, val0, TYPE_vector3f);
    void* val1addr = TypeConvert(script_context, val1_type, val1, TYPE_vector3f);
    if (!val0addr || !val1addr)
        return (false);

    Vector3fClass* v0 = (Vector3fClass*)val0addr;
    Vector3fClass* v1 = (Vector3fClass*)val1addr;
    Vector3fClass* result = (Vector3fClass*)result_addr;
    result_type = TYPE_vector3f;
    int32* int_result = (int32*)result_addr;

    // -- perform the operation
    switch (op)
    {
        case OP_Add:
            *result = *v0 + *v1;
            return (true);

        case OP_Sub:
            *result = *v0 - *v1;
            return (true);

        // -- comparison operations (push a -1, 0, 1) for less than, equal, greater than
        case OP_CompareEqual:
            result_type = TYPE_int;
            *int_result = (*v0 == *v1) ? 0 : 1;
            return (true);

        case OP_CompareNotEqual:
            result_type = TYPE_int;
            *int_result = (*v0 != *v1) ? 0 : 1;
            return (true);
    }

    // -- fail
    return (false);
}

bool8 Vector3fScale(CScriptContext* script_context, eOpCode op, eVarType& result_type, void* result_addr,
                    eVarType val0_type, void* val0, eVarType val1_type, void* val1)
{
    // -- one of the types must be a vector3f, the other must be able to be converted to a float
    // -- sanity check
    if (!script_context || !result_addr || !val0 || !val1)
        return (false);

    // -- division is a scalar, but the order is relevent
    Vector3fClass* v = (Vector3fClass*)TypeConvert(script_context, val0_type, val0, TYPE_vector3f);
    float32* scalar = (float32*)TypeConvert(script_context, val1_type, val1, TYPE_float);
    if (!v && op != OP_Div)
    {
        v = (Vector3fClass*)TypeConvert(script_context, val1_type, val1, TYPE_vector3f);
        scalar = (float32*)TypeConvert(script_context, val0_type, val0, TYPE_float);
    }

    // -- ensure we found valid types
    if (!v || !scalar)
        return (false);

    // -- set up the result
    Vector3fClass* result = (Vector3fClass*)result_addr;
    result_type = TYPE_vector3f;
    
    // -- perform the operation
    switch (op)
    {
        case OP_Mult:
            *result = *v * *scalar;
            return (true);

        case OP_Div:
            *result = *v / *scalar;
            return (true);
    }

    // -- fail
    return (false);
}

// --------------------------------------------------------------------------------------------------------------------
// -- Type conversion functions
void* Vector3fToBoolConvert(CScriptContext* script_context, eVarType from_type, void* from_val, void* to_buffer)
{
    // -- sanity check
    if (!from_val || !to_buffer)
        return (NULL);

    // -- only one conversion viable here - a non-zero vector3f is true, false otherwise
    if (from_type == TYPE_vector3f)
    {
		Vector3fClass* v3 = (Vector3fClass*)from_val;
		*(bool*)to_buffer = (*v3 == Vector3fClass(0.0f, 0.0f, 0.0f)) ? 0 : 1;
		return (to_buffer);
	}

    // -- no registered conversion
    return (NULL);
}

// -- PLATFORM UE4 FVector functions ----------------------------------------------------------------------------------

bool8 FVectorToString(TinScript::CScriptContext* script_context, void* value, char* buf, int32 bufsize)
{
#if PLATFORM_UE4
	if (value && buf && bufsize > 0)
	{
		FVector* ueVector = (FVector*)value;
		double x = (float)ueVector->X;
		double y = (float)ueVector->Y;
		double z = (float)ueVector->Z;
        snprintf(buf, bufsize, "%.4lf %.4lf %.4lf", x, y, z);
		return (true);
	}
#endif
	return (false);
}

bool8 StringToFVector(TinScript::CScriptContext* script_context, void* addr, char* value)
{
#if PLATFORM_UE4
	if (addr && value)
	{
		FVector* varaddr = (FVector*)addr;

		double x = 0.0f;
		double y = 0.0f;
		double z = 0.0f;

		// -- if the string is non-empty, ensure we can read in either space or comma delineated format
		bool success = true;
		if (value && value[0])
		{
			success = sscanf_s(value, "%lf %lf %lf", &x, &y, &z) == 3;
			success = success || (sscanf_s(value, "%lf, %lf, %lf", &x, &y, &z) == 3);
		}

		if (success)
		{
			*varaddr = FVector(x, y, z);
			return (true);
		}
	}
#endif
	return (false);
}

void* Vector3fToFVectorConvert(CScriptContext* script_context, eVarType from_type, void* from_val, void* to_buffer)
{

#if PLATFORM_UE4
	// -- sanity check
	if (!from_val || !to_buffer)
		return (NULL);

	// -- only one conversion viable here - a non-zero vector3f is true, false otherwise
	if (from_type == TYPE_vector3f)
	{
		Vector3fClass* v3 = (Vector3fClass*)from_val;
		FVector* out_FVector = (FVector*)to_buffer;
		out_FVector->Set(v3->v3f_X, v3->v3f_Y, v3->v3f_Z);
		return (to_buffer);
	}
#endif

	// -- no registered conversion
	return (NULL);
}

void* FVectorToVector3fConvert(CScriptContext* script_context, eVarType from_type, void* from_val, void* to_buffer)
{

#if PLATFORM_UE4
	// -- sanity check
	if (!from_val || !to_buffer)
		return (NULL);

	// -- only one conversion viable here - a non-zero vector3f is true, false otherwise
	if (from_type == TYPE_ue_vector)
	{
		FVector* in_FVector = (FVector*)(from_val);
		Vector3fClass* out_Vector3f = (Vector3fClass*)to_buffer;
		*out_Vector3f = Vector3fClass(in_FVector->X, in_FVector->Y, in_FVector->Z);
		return (to_buffer);
	}
#endif

	// -- no registered conversion
	return (NULL);
}

// --------------------------------------------------------------------------------------------------------------------
bool TypeVector3f_Contains(CVariableEntry* ve, CVector3f v3f_val)
{
    int32 count = ve != nullptr ? ve->GetArraySize() : 0;
    for (int i = 0; i < count; ++i)
    {
        void* array_val = ve->GetArrayVarAddr(nullptr, i);
        if (array_val != nullptr && *(CVector3f*)array_val == v3f_val)
            return true;
    }

    return false;
}

// --------------------------------------------------------------------------------------------------------------------
CVector3f TypeVector3f_Set(CVariableEntry* ve_src, float _x, float _y, float _z)
{
    // -- sanity check
    if (ve_src == nullptr || ve_src->GetType() != TYPE_vector3f)
        return CVector3f::zero;

    // -- resolve the storage for the variable
    CVector3f* value = nullptr;

    //-- the ref addr already calculates the array and/or stack offsets
    if (ve_src->GetRefAddr() != nullptr)
    {
        value = (CVector3f*)ve_src->GetRefAddr();
    }

    // -- if the ref address (which has already been calculated for array offsets, etc...)
    // hasn't been provided, use the actual VE address
    else
    {
        value = (CVector3f*)ve_src->GetAddr(nullptr);
    }

    if (value == nullptr)
        return (CVector3f::zero);

    // -- modify the given variable, initializing to the new value
    value->Set(_x, _y, _z);

    // -- this POD modifies the variable - notify the debugger
    int32 stack_var_offset = 0;
    CExecStack* execstack = nullptr;
    CFunctionCallStack* funccallstack = CFunctionCallStack::GetExecutionStackAtDepth(0, execstack, stack_var_offset);
    ve_src->NotifyWrite(TinScript::GetContext(), execstack, funccallstack);

    return *value;
}

// --------------------------------------------------------------------------------------------------------------------
CVector3f TypeVector3f_Normalized(CVariableEntry* ve_src)
{
    // -- sanity check
    if (ve_src == nullptr || ve_src->GetType() != TYPE_vector3f)
        return CVector3f::zero;

    // -- resolve the storage for the variable
    CVector3f* value = nullptr;

    //-- the ref addr already calculates the array and/or stack offsets
    if (ve_src->GetRefAddr() != nullptr)
    {
        value = (CVector3f*)ve_src->GetRefAddr();
    }

    // -- if the ref address (which has already been calculated for array offsets, etc...)
    // hasn't been provided, use the actual VE address
    else
    {
        value = (CVector3f*)ve_src->GetAddr(nullptr);
    }

    if (value == nullptr)
        return (CVector3f::zero);

    // -- this POD modifies the variable - notify the debugger
    int32 stack_var_offset = 0;
    CExecStack* execstack = nullptr;
    CFunctionCallStack* funccallstack = CFunctionCallStack::GetExecutionStackAtDepth(0, execstack, stack_var_offset);
    ve_src->NotifyWrite(TinScript::GetContext(), execstack, funccallstack);

    // -- returns the normalized vector, without modifying the original
    CVector3f result = CVector3f::Normalized(*value);
    return result;
}

// --------------------------------------------------------------------------------------------------------------------
float TypeVector3f_Normalize(CVariableEntry* ve_src)
{
    // -- sanity check
    if (ve_src == nullptr || ve_src->GetType() != TYPE_vector3f)
        return (0.0f);

    // -- resolve the storage for the variable
    CVector3f* value = nullptr;

    //-- the ref addr already calculates the array and/or stack offsets
    if (ve_src->GetRefAddr() != nullptr)
    {
        value = (CVector3f*)ve_src->GetRefAddr();
    }

    // -- if the ref address (which has already been calculated for array offsets, etc...)
    // hasn't been provided, use the actual VE address
    else
    {
        value = (CVector3f*)ve_src->GetAddr(nullptr);
    }

    if (value == nullptr)
        return (0.0f);

    // -- modify the given var by normalizing, return the length
    float length = value->Normalize();
    return length;
}

// --------------------------------------------------------------------------------------------------------------------
float TypeVector3f_Length(CVariableEntry* ve_src)
{
    // -- sanity check
    if (ve_src == nullptr || ve_src->GetType() != TYPE_vector3f)
        return 0.0f;

    // -- resolve the storage for the variable
    CVector3f* value = nullptr;

    //-- the ref addr already calculates the array and/or stack offsets
    if (ve_src->GetRefAddr() != nullptr)
    {
        value = (CVector3f*)ve_src->GetRefAddr();
    }

    // -- if the ref address (which has already been calculated for array offsets, etc...)
    // hasn't been provided, use the actual VE address
    else
    {
        value = (CVector3f*)ve_src->GetAddr(nullptr);
    }

    if (value == nullptr)
        return (0.0f);

    return (value->Length());
}

// --------------------------------------------------------------------------------------------------------------------
float TypeVector3f_Dot(CVariableEntry* ve_src, CVector3f v)
{
    // -- sanity check
    if (ve_src == nullptr || ve_src->GetType() != TYPE_vector3f)
        return (0.0f);

    // -- resolve the storage for the variable
    CVector3f* value = nullptr;

    //-- the ref addr already calculates the array and/or stack offsets
    if (ve_src->GetRefAddr() != nullptr)
    {
        value = (CVector3f*)ve_src->GetRefAddr();
    }

    // -- if the ref address (which has already been calculated for array offsets, etc...)
    // hasn't been provided, use the actual VE address
    else
    {
        value = (CVector3f*)ve_src->GetAddr(nullptr);
    }

    if (value == nullptr)
        return (0.0f);

    float result = CVector3f::Dot(*value, v);
    return result;
}

// --------------------------------------------------------------------------------------------------------------------
CVector3f TypeVector3f_Cross(CVariableEntry* ve_src, CVector3f v)
{
    // -- sanity check
    if (ve_src == nullptr || ve_src->GetType() != TYPE_vector3f)
        return 0.0f;

    // -- resolve the storage for the variable
    CVector3f* value = nullptr;

    //-- the ref addr already calculates the array and/or stack offsets
    if (ve_src->GetRefAddr() != nullptr)
    {
        value = (CVector3f*)ve_src->GetRefAddr();
    }

    // -- if the ref address (which has already been calculated for array offsets, etc...)
    // hasn't been provided, use the actual VE address
    else
    {
        value = (CVector3f*)ve_src->GetAddr(nullptr);
    }

    if (value == nullptr)
        return (CVector3f::zero);

    CVector3f result = CVector3f::Cross(*value, v);
    return result;
}

// --------------------------------------------------------------------------------------------------------------------
// -- Configure the registered Vector3f type, by registering the POD table and op functions
bool8 Vector3fConfig(eVarType var_type, bool8 onInit)
{
    // -- see if this is the initialization or the shutdown
    if (onInit)
    {
        // -- create the vector3f member lookup table (size 3 for 3x members)
        if (gVector3fTable == nullptr)
        {
            gVector3fTable = TinAlloc(ALLOC_HashTable, CHashTable<tPODTypeMember>, 3);
			size_t unit_size = sizeof(Vector3fClass) / 3;
            tPODTypeMember* member_x = new tPODTypeMember(TYPE_float, 0);
            tPODTypeMember* member_y = new tPODTypeMember(TYPE_float, unit_size);
            tPODTypeMember* member_z = new tPODTypeMember(TYPE_float, unit_size * 2);
            gVector3fTable->AddItem(*member_x, Hash("x"));
            gVector3fTable->AddItem(*member_y, Hash("y"));
            gVector3fTable->AddItem(*member_z, Hash("z"));
        }

        // -- now register the hashtable
        RegisterPODTypeTable(var_type, gVector3fTable);

        // -- register the operation overrides
        RegisterTypeOpOverride(OP_Add, TYPE_vector3f, Vector3fOpOverrides);
        RegisterTypeOpOverride(OP_Sub, TYPE_vector3f, Vector3fOpOverrides);

        RegisterTypeOpOverride(OP_CompareEqual, TYPE_vector3f, Vector3fOpOverrides);
        RegisterTypeOpOverride(OP_CompareNotEqual, TYPE_vector3f, Vector3fOpOverrides);

        // -- boolean operations - let type bool handle them
        RegisterTypeOpOverride(OP_BooleanAnd, TYPE_vector3f, BooleanBinaryOp);
        RegisterTypeOpOverride(OP_BooleanOr, TYPE_vector3f, BooleanBinaryOp);

        // -- scalar operations use both a vector3f and a float
        RegisterTypeOpOverride(OP_Mult, TYPE_vector3f, Vector3fScale);
        RegisterTypeOpOverride(OP_Div, TYPE_vector3f, Vector3fScale);

        // -- register the conversion from Vector3f to bool - note the first arg is Type_bool
        RegisterTypeConvert(TYPE_bool, TYPE_vector3f, Vector3fToBoolConvert);

		// -- special conversion for FVector - as a UE4 type, this type cannot be used by script,
		// but we add a conversion method so C++ registered parameters of type FVector can be passed
		// a CVector3f successfully
		RegisterTypeConvert(TYPE_ue_vector, TYPE_vector3f, Vector3fToFVectorConvert);
		RegisterTypeConvert(TYPE_vector3f, TYPE_ue_vector, FVectorToVector3fConvert);

        // -- in addition to POD type member initialization, we want to have types have non-hierarchical methods
        // e.g.  float:isNan(), or vector3f:normalize(), etc...
        if (TinScript::GetContext()->FindNamespace(Hash("TYPE_vector3f")) == nullptr)
        {
            // -- register methods for this type
            // note:  this will actually create a Namespace named "Type_vector3f"
            // note: the last param is reassign... it requires the function to return
			// a value or the same type, to be reassigned back to the calling POD var
            REGISTER_TYPE_METHOD(TYPE_vector3f, set, TypeVector3f_Set);
            REGISTER_TYPE_METHOD(TYPE_vector3f, normalized, TypeVector3f_Normalized);
            REGISTER_TYPE_METHOD(TYPE_vector3f, normalize, TypeVector3f_Normalize);
            REGISTER_TYPE_METHOD(TYPE_vector3f, length, TypeVector3f_Length);
            REGISTER_TYPE_METHOD(TYPE_vector3f, dot, TypeVector3f_Dot);
            REGISTER_TYPE_METHOD(TYPE_vector3f, cross, TypeVector3f_Cross);

            REGISTER_TYPE_METHOD(TYPE_vector3f, initialized, TypeVariable_IsSet);
            REGISTER_TYPE_METHOD(TYPE_vector3f, count, TypeVariable_Count);
            REGISTER_TYPE_METHOD(TYPE_vector3f, contains, TypeVector3f_Contains);
            REGISTER_TYPE_METHOD(TYPE_vector3f, copy, TypeVariableArray_Copy);
            REGISTER_TYPE_METHOD(TYPE_vector3f, resize, TypeVariableArray_Resize);
        }
    }

    // -- shutdown
    else
    {
        // -- memory cleanup for the vector3f lookup table
        if (gVector3fTable != nullptr)
        {
            gVector3fTable->DestroyAll();
            TinFree(gVector3fTable);
            gVector3fTable = nullptr;
        }
    }

    // -- success
    return (true);
}

} // TinScript

// ------------------------------------------------------------------------------------------------
// eof
// ------------------------------------------------------------------------------------------------
