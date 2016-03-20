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

#include "TinHash.h"
#include "TinCompile.h"
#include "TinTypes.h"
#include "TinScript.h"
#include "TinStringTable.h"

namespace TinScript {

// --------------------------------------------------------------------------------------------------------------------
// -- vector3f POD table
tPODTypeTable* gVector3fTable = NULL;

// --------------------------------------------------------------------------------------------------------------------
// External type - CVector3f is a POD type (aggregate, but no user-defined constructors)
bool8 Vector3fToString(void* value, char* buf, int32 bufsize) {
	if(value && buf && bufsize > 0) {
        CVector3f* c3vector = (CVector3f*)value;
		sprintf_s(buf, bufsize, "%.4f %.4f %.4f", c3vector->x, c3vector->y, c3vector->z);
		return true;
	}
	return false;
}

bool8 StringToVector3f(void* addr, char* value) {
	if(addr && value) {
		CVector3f* varaddr = (CVector3f*)addr;

        // -- handle an empty string
        if (!value || !value[0])
        {
            *varaddr = CVector3f(0.0f, 0.0f, 0.0f);
            return (true);
        }

        else if(sscanf_s(value, "%f %f %f", &varaddr->x, &varaddr->y, &varaddr->z) == 3) {
		    return (true);
        }

        else if(sscanf_s(value, "%f, %f, %f", &varaddr->x, &varaddr->y, &varaddr->z) == 3) {
		    return (true);
        }
	}
	return false;
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

    CVector3f* v0 = (CVector3f*)val0addr;
    CVector3f* v1 = (CVector3f*)val1addr;
    CVector3f* result = (CVector3f*)result_addr;
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
    CVector3f* v = (CVector3f*)TypeConvert(script_context, val0_type, val0, TYPE_vector3f);
    float32* scalar = (float32*)TypeConvert(script_context, val1_type, val1, TYPE_float);
    if (!v && op != OP_Div)
    {
        v = (CVector3f*)TypeConvert(script_context, val1_type, val1, TYPE_vector3f);
        scalar = (float32*)TypeConvert(script_context, val0_type, val0, TYPE_float);
    }

    // -- ensure we found valid types
    if (!v || !scalar)
        return (false);

    // -- set up the result
    CVector3f* result = (CVector3f*)result_addr;
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
        CVector3f* v3 = (CVector3f*)from_val;
        *(bool*)to_buffer = (*v3 == CVector3f::zero) ? 0 : 1;
        return (void*)(to_buffer);
    }

    // -- no registered conversion
    return (NULL);
}

// --------------------------------------------------------------------------------------------------------------------
// -- Configure the registered Vector3f type, by registering the POD table and op functions
bool8 Vector3fConfig(eVarType var_type, bool8 onInit)
{
    // -- see if this is the initialization or the shutdown
    if (onInit)
    {
        // -- create the vector3f member lookup table (size 3 for 3x members)
        if (gVector3fTable == NULL)
        {
            gVector3fTable = TinAlloc(ALLOC_HashTable, CHashTable<tPODTypeMember>, 3);
            tPODTypeMember* member_x = new tPODTypeMember(TYPE_float, 0);
            tPODTypeMember* member_y = new tPODTypeMember(TYPE_float, 4);
            tPODTypeMember* member_z = new tPODTypeMember(TYPE_float, 8);
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
    }

    // -- shutdown
    else
    {
        // -- memory cleanup for the vector3f lookup table
        if (gVector3fTable != NULL)
        {
            gVector3fTable->DestroyAll();
            TinFree(gVector3fTable);
            gVector3fTable = NULL;
        }
    }

    // -- success
    return (true);
}

} // TinScript

// ------------------------------------------------------------------------------------------------
// eof
// ------------------------------------------------------------------------------------------------
