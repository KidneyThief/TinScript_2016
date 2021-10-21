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

// ====================================================================================================================
// TinTypes.cpp : Registered types for use with TinScript.
// ====================================================================================================================

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

#pragma optimize("", off)

// == namespace TinScript =============================================================================================

namespace TinScript
{

// ====================================================================================================================
// GetRegisteredTypeName(): Return the string name for a registered type.
// ====================================================================================================================
const char* gRegisteredTypeNames[TYPE_COUNT] =
{

	#define VarTypeEntry(a, b, c, d, e, f) #a,
	VarTypeTuple
	#undef VarTypeEntry
};

const char* GetRegisteredTypeName(eVarType vartype)
{
	return gRegisteredTypeNames[vartype];
}

// ====================================================================================================================
// GetRegisteredType():  Returns the enum type for an inplace string segment.
// ====================================================================================================================
eVarType GetRegisteredType(const char* token, int32 length)
{
    if (!token)
        return TYPE_NULL;
	for (eVarType i = TYPE_void; i < TYPE_COUNT; i = eVarType(i + 1))
    {
		int32 comparelength = int32(strlen(gRegisteredTypeNames[i])) > length
                              ? (int32)strlen(gRegisteredTypeNames[i])
                              : length;
		if (!Strncmp_(token, gRegisteredTypeNames[i], comparelength))
        {
			return i;
		}
	}

	return TYPE_NULL;
}

int32 gRegisteredTypeSize[TYPE_COUNT] =
{
	#define VarTypeEntry(a, b, c, d, e, f) b,
	VarTypeTuple
	#undef VarTypeEntry
};

TypeToString gRegisteredTypeToString[TYPE_COUNT] =
{
	#define VarTypeEntry(a, b, c, d, e, f) c,
	VarTypeTuple
	#undef VarTypeEntry
};

StringToType gRegisteredStringToType[TYPE_COUNT] =
{
	#define VarTypeEntry(a, b, c, d, e, f) d,
	VarTypeTuple
	#undef VarTypeEntry
};

TypeConfiguration gRegisteredTypeConfig[TYPE_COUNT] = {
	#define VarTypeEntry(a, b, c, d, e, f) f,
	VarTypeTuple
	#undef VarTypeEntry
};

// -- this table must be manually populated, as POD tables require memory allocations
tPODTypeTable* gRegisteredPODTypeTable[TYPE_COUNT];

// -- this table must be manually populated
TypeOpOverride gRegisteredTypeOpTable[OP_COUNT][TYPE_COUNT];

// -- this table must be manually populated
TypeConvertFunction gRegisteredTypeConvertTable[TYPE_COUNT][TYPE_COUNT];

// ====================================================================================================================
// GetRegsiteredType():  Returns the enum type, given the ID returned from GetTypeID().
// ====================================================================================================================
eVarType GetRegisteredType(uint32 id)
{
    // -- if this array is declared in the global scope, GetTypeID<>() initializes
    // -- the entire array to 0s...
    static uint32 gRegisteredTypeID[TYPE_COUNT] =
    {
	    #define VarTypeEntry(a, b, c, d, e, f) GetTypeID<e>(),
	    VarTypeTuple
	    #undef VarTypeEntry
    };

	for (eVarType i = FIRST_VALID_TYPE; i < TYPE_COUNT; i = eVarType(i + 1))
    {
        if (id == gRegisteredTypeID[i])
        {
			return i;
		}
	}

    // -- not found - see if this type is a registered class
    CScriptContext* script_context = GetContext();
    if (script_context)
    {
        CHashTable<CNamespace>* ns_dictionary = script_context->GetNamespaceDictionary();
        CNamespace* ns_entry = ns_dictionary->First();
        while (ns_entry)
        {
            if (ns_entry->GetTypeID() == id)
            {
                return (TYPE_object);
            }
            ns_entry = ns_dictionary->Next();
        }
    }
    
	return TYPE_NULL;
}

// == Type Registration Functions =====================================================================================

// ====================================================================================================================
// -- type void
bool8 VoidToString(TinScript::CScriptContext* script_context, void*, char* buf, int32 bufsize)
{
	if (buf && bufsize > 0)
    {
		*buf = '\0';
		return (true);
	}
	return (false);
}

bool8 StringToVoid(TinScript::CScriptContext* script_context, void*, char*)
{
	return (true);
}

// ====================================================================================================================
// -- type string table entry
bool8 STEToString(TinScript::CScriptContext* script_context, void* value, char* buf, int32 bufsize)
{
	if (value && buf && bufsize > 0)
    {
        sprintf_s(buf, bufsize, "%s", script_context->GetStringTable()->FindString(*(uint32*)value));
		return (true);
	}
	return (false);
}

bool8 StringToSTE(TinScript::CScriptContext* script_context, void* addr, char* value)
{
    // -- an STE is simply an address, copy the 4x bytes verbatim
	if (addr && value)
    {
		uint32* varaddr = (uint32*)addr;
		*varaddr = Hash(value, -1, false);
		return (true);
	}
	return (false);
}

// ====================================================================================================================
// -- type int
bool8 IntToString(TinScript::CScriptContext* script_context, void* value, char* buf, int32 bufsize)
{
	if (value && buf && bufsize > 0)
    {
		sprintf_s(buf, bufsize, "%d", *(int32*)(value));
		return (true);
	}
	return (false);
}

bool8 StringToInt(TinScript::CScriptContext* script_context, void* addr, char* value)
{
	if (addr && value)
    {
		int32* varaddr = (int32*)addr;
		*varaddr = Atoi(value);
		return (true);
	}
	return (false);
}

// ====================================================================================================================
// -- type bool
bool8 BoolToString(TinScript::CScriptContext* script_context, void* value, char* buf, int32 bufsize)
{
	if (value && buf && bufsize > 0)
    {
		sprintf_s(buf, bufsize, "%s", *(bool8*)(value) ? "true" : "false");
		return (true);
	}
	return (false);
}

bool8 StringToBool(TinScript::CScriptContext* script_context, void* addr, char* value)
{
	if (addr && value)
    {
		bool8* varaddr = (bool8*)addr;
		if (!Strncmp_(value, "false", 6) || !Strncmp_(value, "0", 2) ||
					!Strncmp_(value, "0.0", 4) || !Strncmp_(value, "0.0f", 5) ||
                    !Strncmp_(value, "", 1))
        {
			*varaddr = false;
			return (true);
		}
		else
        {
			*varaddr = true;
			return (true);
		}
	}
	return (false);
}

// ====================================================================================================================
// -- type float
bool8 FloatToString(TinScript::CScriptContext* script_context, void* value, char* buf, int32 bufsize)
{
	if (value && buf && bufsize > 0)
    {
		sprintf_s(buf, bufsize, "%.4f", *(float32*)(value));
		return (true);
	}
	return (false);
}

bool8 StringToFloat(TinScript::CScriptContext* script_context, void* addr, char* value)
{
	if (addr && value)
    {
		float32* varaddr = (float32*)addr;
		*varaddr = float32(atof(value));
		return (true);
	}
	return (false);
}

// ====================================================================================================================
// GetRegisteredPODMember():  Given a type, returns the address of the POD member of that type
// ====================================================================================================================
bool8 GetRegisteredPODMember(eVarType type_id, void* var_addr, uint32 member_hash, eVarType& out_member_type,
                             void*& out_member_addr)
{
    // -- ensure this type has a hashtable of it's pod members
    if (gRegisteredPODTypeTable[type_id] == NULL || !var_addr)
        return (false);

    // -- see if the hash table contains a specific entry for the member
    tPODTypeMember* member = gRegisteredPODTypeTable[type_id]->FindItem(member_hash);
    if (!member)
        return (false);

    // -- get the member type and address
    out_member_type = member->type;
    out_member_addr = (void*)(((char*)var_addr) + member->offset);

    // -- success
    return (true);
}

// ====================================================================================================================
// GetTypeOpOverride():  Given an operation and a type, see if there's an override function to perform the op
// ====================================================================================================================
TypeOpOverride GetTypeOpOverride(eOpCode op, eVarType var0_type)
{
    return (gRegisteredTypeOpTable[op][var0_type]);
}

// ====================================================================================================================
// InitializeTypes():  Perform any initialization for registered types.
// ====================================================================================================================
void InitializeTypes()
{
    // -- initialize the manually populated tables
	for (eVarType i = TYPE_void; i < TYPE_COUNT; i = eVarType(i + 1))
    {
        gRegisteredPODTypeTable[i] = NULL;

        // -- initialize the op override table
    	for (eOpCode op = OP_NULL; op < OP_COUNT; op = eOpCode(op + 1))
        {
            gRegisteredTypeOpTable[op][i] = NULL;
        }

        // -- initialize the type convert function table
    	for (eVarType j = TYPE_void; j < TYPE_COUNT; j = eVarType(j + 1))
        {
            gRegisteredTypeConvertTable[i][j] = NULL;
        }
    }

    // -- call the config function for each type (if one is given), with a "true" init flag
	for (eVarType i = TYPE_void; i < TYPE_COUNT; i = eVarType(i + 1))
    {
        if (gRegisteredTypeConfig[i])
            gRegisteredTypeConfig[i](i, true);
    }
}

// ====================================================================================================================
// ShutdownTypes():  Perform any shutdown required for registered types.
// ====================================================================================================================
void ShutdownTypes()
{
    // -- call the config function for each type (if one is given), with a "false" shutdown flag
	for(eVarType i = TYPE_void; i < TYPE_COUNT; i = eVarType(i + 1))
    {
        if (gRegisteredTypeConfig[i])
            gRegisteredTypeConfig[i](i, false);
    }

    // -- clear the manaully populated tables
	for(eVarType i = TYPE_void; i < TYPE_COUNT; i = eVarType(i + 1))
    {
        gRegisteredPODTypeTable[i] = NULL;
    }
}

// ====================================================================================================================
// RegisterPODTypeTable(): Register the hashtable of the POD members of a custom type
// ====================================================================================================================
void RegisterPODTypeTable(eVarType var_type, tPODTypeTable* pod_table)
{
    gRegisteredPODTypeTable[var_type] = pod_table;
}

// ====================================================================================================================
// RegisterTypeOpOverride():  Register the method used to perform a numerical operation between specified types
// ====================================================================================================================
void RegisterTypeOpOverride(eOpCode op, eVarType var_type, TypeOpOverride op_override)
{
    gRegisteredTypeOpTable[op][var_type] = op_override;
}

// ====================================================================================================================
// RegisterTypeConvert():  Register the method used to convert to a given type
// ====================================================================================================================
void RegisterTypeConvert(eVarType to_type, eVarType from_type, TypeConvertFunction type_convert)
{
    gRegisteredTypeConvertTable[to_type][from_type] = type_convert;
}

// ====================================================================================================================
// TypeConvert():  Convert between types, returns a void* to a result of the requested type
// ====================================================================================================================
void* TypeConvert(CScriptContext* script_context, eVarType fromtype, void* fromaddr, eVarType totype)
{
    // -- sanity check
    if (! fromaddr)
        return (NULL);

    // -- each script context maintains an array of scratch buffers...
    // -- this allows us to have a place to do conversions, without memory management
    char* bufferptr = script_context->GetScratchBuffer();
    
    // -- if the type remains the same, no conversion is necessary, except in the case of objects
    // from code, an object could be the physical address - we want to ensure what we pass to script
    // is the object *ID*!
    if (fromtype == totype || !fromaddr)
    {
        // -- note:  if we're passing a type object, and the address is an actual
        // object address - we need to convert it to the ID
        if (fromaddr && fromtype == TYPE_object)
        {
            void** obj_addr_ptr = (void**)fromaddr;
            void* actual_obj_addr = *obj_addr_ptr;
            CObjectEntry* found = script_context->FindObjectByAddress(actual_obj_addr);
            if (found != nullptr)
            {
                uint32* stringbuf = (uint32*)script_context->GetScratchBuffer();
                *stringbuf = found->GetID();
                return (stringbuf);
            }
        }

        // -- otherwise, return with no conversion - fromaddr is the addr of
        // an uint32, already containing the ID
        return fromaddr;
    }

    // -- if the types we're converting between are variable or hashtable, they're interchangeable by definition
    //if ((totype == TYPE__var || totype == TYPE_hashtable) && (fromtype == TYPE__var || fromtype == TYPE_hashtable))
    //    return fromaddr;

    // -- if the "to type" is a string, use the registered string conversion function
    if (totype == TYPE_string)
    {
        bool8 success = gRegisteredTypeToString[fromtype](script_context, fromaddr, bufferptr, kMaxTokenLength);
        if (!success)
        {
            ScriptAssert_(TinScript::GetContext(), false, "<internal>", -1,
                          "Error - failed to convert to string from type %s\n", GetRegisteredTypeName(fromtype));
            return ((void*)"");
        }

        // -- Type_string actually stores an STE value, so we must convert the char* bufferptr to an STE
        char* stebuf = script_context->GetScratchBuffer();
        success = gRegisteredStringToType[TYPE_string](script_context, (void*)stebuf, (char*)bufferptr);
        if (!success)
        {
            ScriptAssert_(TinScript::GetContext(), false, "<internal>", -1,
                          "Error - Bad StringTableEntry value\n");
            return ((void*)"");
        }

        // -- return the converted value
        return ((void*)stebuf);
    }

    // -- else if we're converting from a string...
    else if (fromtype == TYPE_string)
    {
        // -- note:  string variables are actually STE's, and need to be converted to char*
        // -- before converting to another type
        char* stringbuf = script_context->GetScratchBuffer();
        bool8 success = gRegisteredTypeToString[TYPE_string](script_context, fromaddr, stringbuf, kMaxTokenLength);
        if (!success)
        {
            ScriptAssert_(TinScript::GetContext(), false, "<internal>", -1,
                          "Error - Bad StringTableEntry value\n");
            return ((void*)"");
        }

        // -- now convert from the actual char* to the requested type
        success = gRegisteredStringToType[totype](script_context, (void*)bufferptr, (char*)stringbuf);
        if (!success)
        {
            ScriptAssert_(TinScript::GetContext(), false, "<internal>", -1,
                "Error - failed to convert string to type %s\n", GetRegisteredTypeName(totype));
            return ((void*)"");
        }

        // -- return the converted value
        return (bufferptr);
    }

    // -- see if we have a registered conversion function
    else if (gRegisteredTypeConvertTable[totype][fromtype])
    {
        // -- note:  bufferptr is a static buffer within this function, and is obviously
        // -- larger than the memsize of any of the registered types
        void* result = gRegisteredTypeConvertTable[totype][fromtype](script_context, fromtype, fromaddr, bufferptr);
        if (result)
        {
            return (result);
        }
    }

    // -- no conversion found
    return (NULL);
}

// ====================================================================================================================
// DebugPrintVar():  Convert the given address to a variable type, and print the value to std out.
// ====================================================================================================================
const char* DebugPrintVar(void* addr, eVarType vartype)
{
    if (!CScriptContext::gDebugTrace)
        return ("");

    static int32 bufferindex = 0;
    static char buffers[8][kMaxTokenLength];

    if (!addr)
        return "";
    char* convertbuf = TinScript::GetContext()->GetScratchBuffer();
    char* destbuf = TinScript::GetContext()->GetScratchBuffer();
	gRegisteredTypeToString[vartype](TinScript::GetContext(), addr, convertbuf, kMaxTokenLength);
    sprintf_s(destbuf, kMaxTokenLength, "[%s] %s", GetRegisteredTypeName(vartype), convertbuf);
    return convertbuf;
}


// ====================================================================================================================
// SafeStrcpy(): Safe version of strcpy(), checks nullptrs and returns a null-terminated destination.
// ====================================================================================================================
bool8 SafeStrcpy(char* dest, size_t dest_size, const char* src, size_t length)
{
	// terminate the dest pointer, in case we copy a zero length string
	if (dest)
		*dest = '\0';

	// -- sanity check
	if (! dest || dest_size == 0 || ! src)
		return false;

    // -- a lenth of 0 means copy the complete string (up to dest_size)
    if (length == 0)
        length = dest_size;

	char* destptr = dest;
	const char* srcptr = src;
	int32 count = length - 1;
    if (dest_size < length)
        count = dest_size - 1;
	while (*srcptr != '\0' && count > 0)
    {
		*destptr++ = *srcptr++;
		--count;
	}
	*destptr = '\0';
	return true;
}

// ====================================================================================================================
// SafeStrCaseStr(): pre-Cx11 strcasestr
// ====================================================================================================================
const char* SafeStrStr(const char* str, const char* partial, bool case_sensitive)
{
    // -- sanity checks
    if (str && (!partial || !partial[0]))
    {
        return str;
    }
    else if (!str || strlen(partial) > strlen(str))
    {
        return nullptr;
    }

    #define IS_ALPHA(c) (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z'))
    #define TO_UPPER(c) ((c) & 0xDF)

    const char* loop_iter = str;
    while (*loop_iter)
    {
        bool8 diff = false;
        const char* str_iter = loop_iter;
        const char* partial_iter = partial;
        while (*partial_iter)
        {
            char test_str = case_sensitive ? *str_iter
                                           : (!IS_ALPHA(*str_iter) ? *str_iter : TO_UPPER(*str_iter));
            char test_partial = case_sensitive ? *partial_iter
                                               : (!IS_ALPHA(*partial_iter) ? *partial_iter : TO_UPPER(*partial_iter));
            if (test_str == test_partial)
            {
                ++str_iter;
                ++partial_iter;
            }
            else
            {
                diff = true;
                break;
            }
        }

        // -- if we completed the loop without finding a difference, we're done
        if (!*partial_iter && !diff)
        {
            return str_iter;
        }
        else
        {
            ++loop_iter;
        }
    }

    // -- no match found
    return nullptr;
}

// ====================================================================================================================
// Atoi():  Converts an ascii string segment to an integer value, including binary and hex string formats.
// ====================================================================================================================
int32 Atoi(const char* src, int32 length)
{
    int32 result = 0;
    if (!src || (length == 0))
        return 0;

    // -- see if we need to negate
    int signchange = 1;
    if (src[0] == '-')
    {
        signchange = -1;

        // -- next character
        ++src;
        --length;
    }

    // see if we're converting from hex
    if (src[0] == '0' && (src[1] == 'x' || src[1] == 'X'))
    {
        src += 2;
        length -= 2;
        while (*src != '\0' && length != 0)
        {
            result = result * 16;
            if (*src >= '0' && *src <= '9')
                result += (*src - '0');
            else if (*src >= 'a' && *src <= 'f')
                result += 10 + (*src - 'a');
            else if (*src >= 'A' && *src <= 'F')
                result += 10 + (*src - 'A');
            else
                return (signchange * result);

            // -- next character
            ++src;
            --length;
        }
    }

    // see if we're converting from binary
    if (src[0] == '0' && (src[1] == 'b' || src[1] == 'B'))
    {
        src += 2;
        length -= 2;
        while (*src != '\0' && length != 0)
        {
            result = result << 1;
            if (*src == '1')
                ++result;
            else if (*src != '0')
                return (signchange * result);

            // -- next character
            ++src;
            --length;
        }
    }

    else
    {
        // -- if length was given as -1, we process the entire null-terminated string
        while(*src != '\0' && length != 0)
        {
            // -- validate the character
            if (*src < '0' || *src > '9')
                return (signchange * result);
            result = (result * 10) + (int32)(*src - '0');

            // -- next character
            ++src;
            --length;
        }
    }

    return (signchange * result);
}

// == Type Conversion functions =======================================================================================

// ====================================================================================================================
// FloatConvert():  Convert the given value to a float, for valid source types.
// ====================================================================================================================
void* FloatConvert(CScriptContext* script_context, eVarType from_type, void* from_val, void* to_buffer)
{
    // -- sanity check
    if (!from_val || !to_buffer)
        return (NULL);

    switch (from_type)
    {
        case TYPE_int:
            *(float32*)(to_buffer) = (float32)*(int32*)(from_val);
             return (void*)(to_buffer);

        case TYPE_bool:
            *(float32*)(to_buffer) = *(bool8*)from_val ? 1.0f : 0.0f;
            return (void*)(to_buffer);
    }

    // -- no registered conversion
    return (NULL);
}

// ====================================================================================================================
// IntegerConvert():  Convert the given value to an integer, for valid source types.
// ====================================================================================================================
void* IntegerConvert(CScriptContext* script_context, eVarType from_type, void* from_val, void* to_buffer)
{
    // -- sanity check
    if (!from_val || !to_buffer)
        return (NULL);

    switch (from_type)
    {
        case TYPE_bool:
            *(int32*)(to_buffer) = *(bool8*)from_val ? 1 : 0;
            return (void*)(to_buffer);

        case TYPE_float:
            *(int32*)(to_buffer) = (int32)*(float32*)(from_val);
            return (void*)(to_buffer);

        // -- since objects are referred to by their IDs, we need to determine if the
        // from_val is the physical address of an object, or the ID
        case TYPE_object:
        {
            CObjectEntry* found = nullptr;
            if (from_val != nullptr)
            {
                void** object_addr_ptr = (void**)from_val;
                void* obj_addr = *object_addr_ptr;
                found = script_context->FindObjectByAddress(obj_addr);
                if (found != nullptr)
                {
                    *(int32*)(to_buffer) = (int32)found->GetID();
                    return (void*)(to_buffer);
                }
            }

            // -- otherwise, the value is already the ID - simply verify it exists, and return
            if (script_context->FindObjectEntry(*(uint32*)from_val))
                return (from_val);
            else
            {
                *(int32*)(to_buffer) = 0;
                return (void*)(to_buffer);
            }
        }
    }

    // -- no registered conversion
    return (NULL);
}

// ====================================================================================================================
// BoolConvert():  Convert the given value to a bool, for valid source types.
// ====================================================================================================================
void* BoolConvert(CScriptContext* script_context, eVarType from_type, void* from_val, void* to_buffer)
{
    // -- sanity check
    if (!from_val || !to_buffer)
        return (NULL);

    switch (from_type)
    {
        case TYPE_int:
            *(bool8*)(to_buffer) = (*(int32*)(from_val) != 0);
            return (void*)(to_buffer);

        case TYPE_float:
            *(bool8*)(to_buffer) = (*(float32*)(from_val) != 0.0f);
            return (void*)(to_buffer);

        case TYPE_object:
        {
            CObjectEntry* oe = script_context->FindObjectEntry(*(uint32*)from_val);
            *(bool8*)(to_buffer) = (oe != NULL);
            return (void*)(to_buffer);
        }
    }

    // -- no registered conversion
    return (NULL);
}

// ====================================================================================================================
// ObjectConvert():  Convert the given value to an object, for valid source types.  Also validates the object exists.
// ====================================================================================================================
void* ObjectConvert(CScriptContext* script_context, eVarType from_type, void* from_val, void* to_buffer)
{
    // -- sanity check
    if (!from_val || !to_buffer)
        return (NULL);


    // -- since objects are actually uint32 IDs, no conversion from int is necessary
    // -- if the object exists, retain the value, otherwise set to 0
    if (from_type == TYPE_int)
    {
        if (script_context->FindObjectEntry(*(uint32*)from_val) != NULL)
            return (from_val);
        else
        {
            *(uint32*)to_buffer = 0;
            return (to_buffer);
        }
    }

    // -- no registered conversion
    return (NULL);
}

// ====================================================================================================================
// -- Forward declarations for operation overrides
bool8 ObjectBinaryOp(CScriptContext* script_context, eOpCode op, eVarType& result_type, void* result_addr,
                     eVarType val0_type, void* val0, eVarType val1_type, void* val1);
bool8 StringBinaryOp(CScriptContext* script_context, eOpCode op, eVarType& result_type, void* result_addr,
                     eVarType val0_type, void* val0, eVarType val1_type, void* val1);
bool8 FloatBinaryOp(CScriptContext* script_context, eOpCode op, eVarType& result_type, void* result_addr,
                    eVarType val0_type, void* val0, eVarType val1_type, void* val1);
bool8 IntegerBinaryOp(CScriptContext* script_context, eOpCode op, eVarType& result_type, void* result_addr,
                      eVarType val0_type, void* val0, eVarType val1_type, void* val1);

// --------------------------------------------------------------------------------------------------------------------
// ObjectBinaryOp():  Registered to perform all operations involving objectc only values
// --------------------------------------------------------------------------------------------------------------------
bool8 ObjectBinaryOp(CScriptContext* script_context, eOpCode op, eVarType& result_type, void* result_addr,
                      eVarType val0_type, void* val0, eVarType val1_type, void* val1)
{
    // -- sanity check
    if (!result_addr || !val0 || !val1)
        return (false);

    // -- ensure the types are converted to Type_object
    void* val0addr = TypeConvert(script_context, val0_type, val0, TYPE_object);
    void* val1addr = TypeConvert(script_context, val1_type, val1, TYPE_object);
    if (!val0addr || !val1addr)
        return (false);

    uint32 v0 = *(uint32*)val0addr;
    uint32 v1 = *(uint32*)val1addr;
    int32* result = (int32*)result_addr;
    result_type = TYPE_int;

    // -- perform the operation - note, both object handle values
    // -- can differ, but if neither object actually exists, they are equal
    // -- note: all comparisons follow the same result as, say, strcmp()...
    // -- '0' is equal, -1 is (a < b), '1' is (a > b)
    switch (op)
    {
        case OP_CompareNotEqual:
        case OP_CompareEqual:
        {
            CObjectEntry* oe0 = script_context->FindObjectEntry(v0);
            CObjectEntry* oe1 = script_context->FindObjectEntry(v1);
            *result = (oe0 == oe1) ? 0 : 1;
            return (true);
        }
    }

    // -- fail
    return (false);
}

// --------------------------------------------------------------------------------------------------------------------
// StringBinaryOp():  Registered to perform all numerical operations using a string that represents a float or an int
// --------------------------------------------------------------------------------------------------------------------
bool8 StringBinaryOp(CScriptContext* script_context, eOpCode op, eVarType& result_type, void* result_addr,
                     eVarType val0_type, void* val0, eVarType val1_type, void* val1)
{
    // -- sanity check
    if (!script_context || !result_addr || !val0 || !val1)
        return (false);

    // -- if we're comparing, use the StringCmp() fuunctions
    if (val0_type == TYPE_string && val1_type == TYPE_string)
    {
        if (op == OP_CompareEqual || op == OP_CompareNotEqual)
        {
            uint32 str_0_hash = *(uint32*)TypeConvert(script_context, val0_type, val0, TYPE_string);
            uint32 str_1_hash = *(uint32*)TypeConvert(script_context, val1_type, val1, TYPE_string);
            int32* result = (int32*)result_addr;
            result_type = TYPE_int;
            *result = (str_0_hash == str_1_hash) ? 0 : 1;
            return (true);
        }
    }

    // -- if we cannot convert the string to a float non-zero value, convert it as an integer
    bool val0_float = true;
    void* val0addr = TypeConvert(script_context, val0_type, val0, TYPE_float);
    if (!val0addr || *(float32*)(val0addr) == 0.0f)
    {
        val0addr = TypeConvert(script_context, val0_type, val0, TYPE_int);
        val0_float = false;
    }

    bool val1_float = true;
    void* val1addr = TypeConvert(script_context, val1_type, val1, TYPE_float);
    if (!val1addr || *(float32*)(val1addr) == 0.0f)
    {
        val1addr = TypeConvert(script_context, val1_type, val1, TYPE_int);
        val1_float = false;
    }

    // -- ensure we at least got two numerical types
    if (!val0addr || !val1addr)
        return (false);

    // -- if either is a float, perform a float operation
    if (val0_float || val1_float)
    {
        return (FloatBinaryOp(script_context, op, result_type, result_addr,
                              val0_float ? TYPE_float : TYPE_int, val0addr,
                              val1_float ? TYPE_float : TYPE_int, val1addr));
    }

    // -- else perform an integer op
    else
    {
        return (IntegerBinaryOp(script_context, op, result_type, result_addr, TYPE_int, val0addr, TYPE_int, val1addr));
    }
}

// --------------------------------------------------------------------------------------------------------------------
// FloatBinaryOp():  Registered to perform all numerical operations involving a float, except vector3f scaling
// --------------------------------------------------------------------------------------------------------------------
bool8 FloatBinaryOp(CScriptContext* script_context, eOpCode op, eVarType& result_type, void* result_addr,
                    eVarType val0_type, void* val0, eVarType val1_type, void* val1)
{
    // -- sanity check
    if (!script_context || !result_addr || !val0 || !val1)
        return (false);

    // -- ensure the types are converted to vector3f
    void* val0addr = TypeConvert(script_context, val0_type, val0, TYPE_float);
    void* val1addr = TypeConvert(script_context, val1_type, val1, TYPE_float);
    if (!val0addr || !val1addr)
        return (false);

    float32* v0 = (float32*)val0addr;
    float32* v1 = (float32*)val1addr;
    float32* result = (float32*)result_addr;
    result_type = TYPE_float;

    // -- perform the operation
    switch (op)
    {
        case OP_Add:
            *result = *v0 + *v1;
            return (true);

        case OP_Sub:
            *result = *v0 - *v1;
            return (true);

        case OP_Mult:
            *result = *v0 * *v1;
            return (true);

        case OP_Div:
            if (*v1 == 0.0f)
            {
                ScriptAssert_(script_context, false, "<internal>", -1, "Error - OP_Mod division by 0.0f\n");
                *result = 0.0f;
                return (false);
            }
            *result = *v0 / *v1;
            return (true);

        case OP_Mod:
            if (*v1 == 0.0f)
            {
                ScriptAssert_(script_context, false, "<internal>", -1, "Error - OP_Mod division by 0.0f\n");
                *result = 0.0f;
                return (false);
            }

            *result = *v0 - (float32)((int32)(*v0 / *v1) * *v1);
            return (true);

        // -- comparison operations (push a -1, 0, 1) for less than, equal, greater than
        case OP_CompareEqual:
        case OP_CompareNotEqual:
        case OP_CompareLess:
        case OP_CompareLessEqual:
        case OP_CompareGreater:
        case OP_CompareGreaterEqual:
            *result = (*v0 - *v1) < 0.0f ? -1.0f : (*v0 - *v1) == 0.0f ? 0.0f : 1.0f;
            return (true);

        default:
            return false;
    }
}

// --------------------------------------------------------------------------------------------------------------------
// IntegerBinaryOp():  Registered to perform all numerical operations involving integer only values
// --------------------------------------------------------------------------------------------------------------------
bool8 IntegerBinaryOp(CScriptContext* script_context, eOpCode op, eVarType& result_type, void* result_addr,
                      eVarType val0_type, void* val0, eVarType val1_type, void* val1)
{
    // -- sanity check
    if (!result_addr || !val0 || !val1)
        return (false);

    // -- ensure the types are converted to vector3f
    void* val0addr = TypeConvert(script_context, val0_type, val0, TYPE_int);
    void* val1addr = TypeConvert(script_context, val1_type, val1, TYPE_int);
    if (!val0addr || !val1addr)
        return (false);

    int32* v0 = (int32*)val0addr;
    int32* v1 = (int32*)val1addr;
    int32* result = (int32*)result_addr;
    result_type = TYPE_int;

    // -- perform the operation
    switch (op)
    {
        case OP_Add:
            *result = *v0 + *v1;
            return (true);

        case OP_Sub:
            *result = *v0 - *v1;
            return (true);

        case OP_Mult:
            *result = *v0 * *v1;
            return (true);

        case OP_Div:
            if (*v1 == 0)
            {
                ScriptAssert_(script_context, false, "<internal>", -1, "Error - OP_Mod division by 0\n");
                *result = 0;
                return (false);
            }
            *result = *v0 / *v1;
            return (true);

        case OP_Mod:
            if (*v1 == 0)
            {
                ScriptAssert_(script_context, false, "<internal>", -1, "Error - OP_Mod division by 0\n");
                *result = 0;
                return (false);
            }

            *result = *v0 - ((*v0 / *v1) * *v1);
            return (true);

        // -- comparison operations (push a -1, 0, 1) for less than, equal, greater than
        case OP_CompareEqual:
        case OP_CompareNotEqual:
        case OP_CompareLess:
        case OP_CompareLessEqual:
        case OP_CompareGreater:
        case OP_CompareGreaterEqual:
            *result = *v0 - *v1;
            return (true);

        // -- Bit operations
        case OP_BitLeftShift:
            *result = *v0 << *v1;
            return (true);

        case OP_BitRightShift:
            *result = *v0 >> *v1;
            return (true);

        case OP_BitAnd:
            *result = *v0 & *v1;
            return (true);

        case OP_BitOr:
            *result = *v0 | *v1;
            return (true);

        case OP_BitXor:
            *result = *v0 ^ *v1;
            return (true);

        default:
            return false;
    }
}

// --------------------------------------------------------------------------------------------------------------------
// BooleanBinaryOp():  Registered to perform all boolean operations using type bool
// --------------------------------------------------------------------------------------------------------------------
bool8 BooleanBinaryOp(CScriptContext* script_context, eOpCode op, eVarType& result_type, void* result_addr,
                      eVarType val0_type, void* val0, eVarType val1_type, void* val1)
{
    // -- sanity check
    if (!script_context || !result_addr || !val0 || !val1)
        return (false);

    // -- ensure the types are converted to vector3f
    void* val0addr = TypeConvert(script_context, val0_type, val0, TYPE_bool);
    void* val1addr = TypeConvert(script_context, val1_type, val1, TYPE_bool);
    if (!val0addr || !val1addr)
        return (false);

    bool8* v0 = (bool8*)val0addr;
    bool8* v1 = (bool8*)val1addr;
    int32* result = (int32*)result_addr;
    result_type = TYPE_int;

    // -- the result for boolean operators is numerical... 1 == true, 0 == false
    // -- the result for comparison operators is numerical... (-1, 0, 1) for lt, eq, gt
    switch (op)
    {
        case OP_BooleanAnd:
            *result = *v0 && *v1 ? 1 : 0;
            return (true);

        case OP_BooleanOr:
            *result = *v0 || *v1 ? 1 : 0;
            return (true);

        // -- note: all comparisons follow the same result as, say, strcmp()...
        // -- '0' is equal, -1 is (a < b), '1' is (a > b)
        case OP_CompareEqual:
        case OP_CompareNotEqual:
            *result = *v0 == *v1 ? 0 : 1;
            return (true);
    }

    // -- fail
    return (false);
}

// ====================================================================================================================
// ObjectConfig():  Called from InitializeTypes() to register object operations and conversions
// ====================================================================================================================
bool8 ObjectConfig(eVarType var_type, bool8 onInit)
{
    // -- see if this is the initialization or the shutdown
    if (onInit)
    {
        // -- The only valid conversion (other than string) to object, is from an int
        RegisterTypeConvert(TYPE_object, TYPE_int, ObjectConvert);

        // -- comparison operations
        RegisterTypeOpOverride(OP_CompareEqual, TYPE_object, ObjectBinaryOp);
        RegisterTypeOpOverride(OP_CompareNotEqual, TYPE_object, ObjectBinaryOp);

        // -- allow BooleanBinaryOp to perform the operation
        RegisterTypeOpOverride(OP_BooleanAnd, TYPE_object, BooleanBinaryOp);
        RegisterTypeOpOverride(OP_BooleanOr, TYPE_object, BooleanBinaryOp);
    }

    // -- success
    return (true);
}

// ====================================================================================================================
// StringConfig():  Called from InitializeTypes() to populate the override table to perform numerical ops
// where one or both arguments are represented as strings
// ====================================================================================================================
bool8 StringConfig(eVarType var_type, bool8 onInit)
{
    // -- see if this is the initialization or the shutdown
    if (onInit)
    {
        // -- register the operation overrides
        RegisterTypeOpOverride(OP_Add, TYPE_string, StringBinaryOp);
        RegisterTypeOpOverride(OP_Sub, TYPE_string, StringBinaryOp);
        RegisterTypeOpOverride(OP_Mult, TYPE_string, StringBinaryOp);
        RegisterTypeOpOverride(OP_Div, TYPE_string, StringBinaryOp);
        RegisterTypeOpOverride(OP_Mod, TYPE_string, StringBinaryOp);

        RegisterTypeOpOverride(OP_CompareEqual, TYPE_string, StringBinaryOp);
        RegisterTypeOpOverride(OP_CompareNotEqual, TYPE_string, StringBinaryOp);
    }

    // -- success
    return (true);
}

// ====================================================================================================================
// FloatConfig():  Called from InitializeTypes() to register float operations and conversions
// ====================================================================================================================
bool8 FloatConfig(eVarType var_type, bool8 onInit)
{
    // -- see if this is the initialization or the shutdown
    if (onInit)
    {
        // -- register the operation overrides
        RegisterTypeOpOverride(OP_Add, TYPE_float, FloatBinaryOp);
        RegisterTypeOpOverride(OP_Sub, TYPE_float, FloatBinaryOp);
        RegisterTypeOpOverride(OP_Mult, TYPE_float, FloatBinaryOp);
        RegisterTypeOpOverride(OP_Div, TYPE_float, FloatBinaryOp);
        RegisterTypeOpOverride(OP_Mod, TYPE_float, FloatBinaryOp);

        // -- comparison operations
        RegisterTypeOpOverride(OP_CompareEqual, TYPE_float, FloatBinaryOp);
        RegisterTypeOpOverride(OP_CompareNotEqual, TYPE_float, FloatBinaryOp);
        RegisterTypeOpOverride(OP_CompareLess, TYPE_float, FloatBinaryOp);
        RegisterTypeOpOverride(OP_CompareLessEqual, TYPE_float, FloatBinaryOp);
        RegisterTypeOpOverride(OP_CompareGreater, TYPE_float, FloatBinaryOp);
        RegisterTypeOpOverride(OP_CompareGreaterEqual, TYPE_float, FloatBinaryOp);

        // -- boolean operations - let type bool handle them
        RegisterTypeOpOverride(OP_BooleanAnd, TYPE_float, BooleanBinaryOp);
        RegisterTypeOpOverride(OP_BooleanOr, TYPE_float, BooleanBinaryOp);

        // -- register the conversion methods
        RegisterTypeConvert(TYPE_float, TYPE_int, FloatConvert);
        RegisterTypeConvert(TYPE_float, TYPE_bool, FloatConvert);
    }

    // -- success
    return (true);
}

// ====================================================================================================================
// IntegerConfig():  Called from InitializeTypes() to register integer operations and conversions
// ====================================================================================================================
bool8 IntegerConfig(eVarType var_type, bool8 onInit)
{
    // -- see if this is the initialization or the shutdown
    if (onInit)
    {
        // -- register the operation overrides
        RegisterTypeOpOverride(OP_Add, TYPE_int, IntegerBinaryOp);
        RegisterTypeOpOverride(OP_Sub, TYPE_int, IntegerBinaryOp);
        RegisterTypeOpOverride(OP_Mult, TYPE_int, IntegerBinaryOp);
        RegisterTypeOpOverride(OP_Div, TYPE_int, IntegerBinaryOp);
        RegisterTypeOpOverride(OP_Mod, TYPE_int, IntegerBinaryOp);

        // -- comparison operations
        RegisterTypeOpOverride(OP_CompareEqual, TYPE_int, IntegerBinaryOp);
        RegisterTypeOpOverride(OP_CompareNotEqual, TYPE_int, IntegerBinaryOp);
        RegisterTypeOpOverride(OP_CompareLess, TYPE_int, IntegerBinaryOp);
        RegisterTypeOpOverride(OP_CompareLessEqual, TYPE_int, IntegerBinaryOp);
        RegisterTypeOpOverride(OP_CompareGreater, TYPE_int, IntegerBinaryOp);
        RegisterTypeOpOverride(OP_CompareGreaterEqual, TYPE_int, IntegerBinaryOp);

        // -- boolean operations - let type bool handle them
        RegisterTypeOpOverride(OP_BooleanAnd, TYPE_int, BooleanBinaryOp);
        RegisterTypeOpOverride(OP_BooleanOr, TYPE_int, BooleanBinaryOp);

        // -- bit operations
        RegisterTypeOpOverride(OP_BitLeftShift, TYPE_int, IntegerBinaryOp);
        RegisterTypeOpOverride(OP_BitRightShift, TYPE_int, IntegerBinaryOp);
        RegisterTypeOpOverride(OP_BitAnd, TYPE_int, IntegerBinaryOp);
        RegisterTypeOpOverride(OP_BitOr, TYPE_int, IntegerBinaryOp);
        RegisterTypeOpOverride(OP_BitXor, TYPE_int, IntegerBinaryOp);

        // -- register the conversion methods
        RegisterTypeConvert(TYPE_int, TYPE_float, IntegerConvert);
        RegisterTypeConvert(TYPE_int, TYPE_bool, IntegerConvert);
        RegisterTypeConvert(TYPE_int, TYPE_object, IntegerConvert);
    }

    // -- success
    return (true);
}

// ====================================================================================================================
// BoolConfig():  Called from InitializeTypes() to register bool operations and conversions
// ====================================================================================================================
bool8 BoolConfig(eVarType var_type, bool8 onInit)
{
    // -- see if this is the initialization or the shutdown
    if (onInit)
    {
        // -- comparison operations
        RegisterTypeOpOverride(OP_CompareEqual, TYPE_bool, BooleanBinaryOp);
        RegisterTypeOpOverride(OP_CompareNotEqual, TYPE_bool, BooleanBinaryOp);
        RegisterTypeOpOverride(OP_BooleanAnd, TYPE_bool, BooleanBinaryOp);
        RegisterTypeOpOverride(OP_BooleanOr, TYPE_bool, BooleanBinaryOp);

        // -- register the conversion methods
        RegisterTypeConvert(TYPE_bool, TYPE_float, BoolConvert);
        RegisterTypeConvert(TYPE_bool, TYPE_int, BoolConvert);
        RegisterTypeConvert(TYPE_bool, TYPE_object, BoolConvert);
    }

    // -- success
    return (true);
}

} // TinScript

// ====================================================================================================================
// EOF
// ====================================================================================================================
