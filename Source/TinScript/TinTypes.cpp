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

// -- class include
#include "TinTypes.h"

// -- lib includes
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

#include "mathutil.h"

#include "TinHash.h"
#include "TinCompile.h"
#include "TinScript.h"
#include "TinStringTable.h"
#include "TinRegBinding.h"

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
// GetRegisteredTypeName(): Return the string name for a registered type.
// note:  this is the hash of the string TYPE_xxx, not just xxx
// ====================================================================================================================
uint32 gRegisteredTypeHash[TYPE_COUNT] =
{
    #define VarTypeEntry(a, b, c, d, e, f) TinScript::Hash("TYPE_" #a),
    VarTypeTuple
    #undef VarTypeEntry
};

uint32 GetRegisteredTypeHash(eVarType vartype)
{
    return gRegisteredTypeHash[vartype];
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

    // -- see if this is explicitly a CVariableEntry* or a tVarTable*
    // -- TYPE_hashtable "values" are tVarTable...  this is only used OP_PODCallArgs, which can resolve
    if (id == GetTypeID<CVariableEntry*>() || id == GetTypeID<tVarTable*>())
        return TYPE__var;

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
        snprintf(buf, bufsize, "%s", script_context->GetStringTable()->FindString(*(uint32*)value));
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
        snprintf(buf, bufsize, "%d", *(int32*)(value));
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
        snprintf(buf, bufsize, "%s", *(bool8*)(value) ? "true" : "false");
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
        snprintf(buf, bufsize, "%.4f", *(float32*)(value));
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
// GetPODMemberTable():  return the registered table of members, for a given type
// ====================================================================================================================
tPODTypeTable* GetPODMemberTable(eVarType type_id)
{
    return (gRegisteredPODTypeTable[type_id]);
}

// ====================================================================================================================
// GetPODMethodTable():  return the function table registered for a given type
// ====================================================================================================================
CHashTable<CFunctionEntry>* GetPODMethodTable(eVarType type_id)
{
    uint32 ns_hash = GetRegisteredTypeHash(type_id);
    CNamespace* type_ns = TinScript::GetContext()->FindNamespace(ns_hash);
    return (type_ns != nullptr ? type_ns->GetFuncTable() : nullptr);
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
        gRegisteredPODTypeTable[i] = nullptr;

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
        gRegisteredPODTypeTable[i] = nullptr;
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

    // -- a length of 0 means copy the complete string (up to dest_size)
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
                TinError(script_context, "Error - OP_Div division by 0.0f\n");
                *result = 0.0f;
                return (false);
            }
            *result = *v0 / *v1;
            return (true);

        case OP_Mod:
            if (*v1 == 0.0f)
            {
                TinError(script_context, "Error - OP_Mod division by 0.0f\n");
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
                TinError(script_context, "Error - OP_Div division by 0\n");
                *result = 0;
                return (false);
            }
            *result = *v0 / *v1;
            return (true);

        case OP_Mod:
            if (*v1 == 0)
            {
                TinError(script_context, "Error - OP_Mod division by 0\n");
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

// -- POD method(s) for all first-class types ---------------------------------------------------------------------------

bool TypeVariable_IsSet(CVariableEntry* ve)
{
    if (ve == nullptr)
    {
        TinPrint(TinScript::GetContext(), "Error - TypeVariable_IsSet(): invalid variable");
        return 0;
    }

    // -- return if the variable has ever been assigned
    return ve->IsSet();
}

int32 TypeVariable_Count(CVariableEntry* ve)
{
    if (ve == nullptr)
    {
        TinPrint(TinScript::GetContext(), "Error - TypeVariable_Count(): invalid variable");
        return 0;
    }

    int32 count = ve != nullptr ? ve->GetArraySize() : 0;
    if (count == -1)
    {
        TinWarning(TinScript::GetContext(), "Warning - array variable `%s` has not been initialized - \n"
                                          "you must (e.g.) array:copy(%s) from a valid array before derferrencing\n",
                                          UnHash(ve->GetHash()), UnHash(ve->GetHash()));
    }
    return count;
}

bool TypeVariableArray_Copy(CVariableEntry* ve_src, CVariableEntry* ve_dst)
{
    // -- the source and dest must be an arrays of the same type
    if (ve_src == nullptr || ve_dst == nullptr || !ve_src->IsArray() || !ve_dst->IsArray() ||
        ve_src->GetArraySize() < 1 || ve_src->GetType() != ve_dst->GetType())
    {
        TinPrint(TinScript::GetContext(), "Error - array:copy() failed from copying %s to %s\n",
                                           (ve_src ? UnHash(ve_src->GetHash()) : "<unknown>"),
                                           (ve_dst ? UnHash(ve_dst->GetHash()) : "<unknown>"));
        return false;
    }

    // -- an extra check to ensure we're not trying to copy to a C++ registered member
    // (resizing and moving address pointers around - is not yet supported...  likely
    // we'll allow a direct copy if'f the dst address is already the right type and size)
    if (!ve_src->IsScriptVar() || !ve_dst->IsScriptVar())
    {
        TinPrint(TinScript::GetContext(), "Error - array:copy() failed from '%s' to '%s'-\n"
                                          "only script variables are currently supported\n",
                                          UnHash(ve_src->GetHash()), UnHash(ve_dst->GetHash()));
        return (false);
    }

    // -- if array is already the right type/size to copy to, we don't need to free/resize
    int32 count = ve_src->GetArraySize();
    if (ve_dst->GetArraySize() != count)
    {
        // -- try to free the memory of the dest
        if (!ve_dst->TryFreeAddrMem())
            return (false);

        if (!ve_dst->ConvertToArray(count))
            return false;
    }

    // -- simple memcpy
    void* src_addr = ve_src->GetAddr(nullptr);
    void* dst_addr = ve_dst->GetAddr(nullptr);
    if (src_addr == nullptr || dst_addr == nullptr)
    {
        TinPrint(TinScript::GetContext(), "Error - array:copy() null address copying %s to %s\n",
                                           (ve_src ? UnHash(ve_src->GetHash()) : "<unknown>"),
                                           (ve_dst ? UnHash(ve_dst->GetHash()) : "<unknown>"));
         return (false);
    }

    // -- copy the memory
    // -- note:  for Type_string, GetAddr() returns the GetStringHashArray() addr
    memcpy(dst_addr, src_addr, gRegisteredTypeSize[ve_src->GetType()] * count);

    // -- return success
    return (true);
}

bool TypeVariableArray_Resize(CVariableEntry* ve_src, int32 new_size)
{
    // -- make sure we've got a script array
    if (ve_src == nullptr || !ve_src->IsArray() || !ve_src->IsScriptVar() ||  ve_src->IsReference())
    {
        TinPrint(TinScript::GetContext(), "Error - array:resize() var: %s, :resize() only supports script array vars\n",
                                           (ve_src ? UnHash(ve_src->GetHash()) : "<unknown>"));
        return (false);
    }

    if (ve_src->IsParameter() || ve_src->GetFunctionEntry() != nullptr)
    {
        TinPrint(TinScript::GetContext(), "Error - array:resize() var: %s, :resize() cannot resize local (e.g. stack) variables\n",
                                           (ve_src ? UnHash(ve_src->GetHash()) : "<unknown>"));
        return (false);
    }

    // -- valid size
    // $$$TZA max size?
    if (new_size <= 1)
    {
        TinPrint(TinScript::GetContext(), "Error - array:resize() invalid size %d for var: %s\n",
                                           new_size, UnHash(ve_src->GetHash()));
        return (false);
    }

    // -- no point in resizing to a smaller size (TinScript doesn't yet require manual memory optimizations)
    // note:  we'll return true, since technically the array is large enough for the size requested
    if (new_size <= ve_src->GetArraySize())
    {
        TinPrint(TinScript::GetContext(), "array:resize() from %d to smaller size %d for var: %s skipped\n",
                                          ve_src->GetArraySize(), new_size, UnHash(ve_src->GetHash()));
        return (true);
    }

    // -- resize is already an expensive operation and shouldn't be used if possible...
    // for now, we'll block copy, rather than hooking into the ve, and deleting the original storage after a copy
    int32 orig_size = ve_src->GetArraySize();
    int32 byte_count = gRegisteredTypeSize[ve_src->GetType()] * orig_size;
    char* orig_value = nullptr;
    if (orig_size >= 1)
    {
        orig_value = TinAllocArray(ALLOC_VarStorage, char, byte_count);
        memcpy(orig_value, ve_src->GetAddr(nullptr), byte_count);
    }

    // -- try to free the memory for our array
    if (!ve_src->TryFreeAddrMem())
        return (false);

    // -- try to convert back to an array, of the new size
    if (!ve_src->ConvertToArray(new_size))
        return false;

    // -- copy the contents back to our array (the original count obviously)
    if (orig_size >= 1)
    {
        memcpy(ve_src->GetAddr(nullptr), orig_value, byte_count);
    }

    // -- return success
    return (true);
}

// ====================================================================================================================
// ObjectConfig():  Called from InitializeTypes() to register object operations and conversions
// ====================================================================================================================
bool TypeObject_Contains(CVariableEntry* ve, uint32 obj_id)
{
    int32 count = ve != nullptr ? ve->GetArraySize() : 0;
    for (int i = 0; i < count; ++i)
    {
        void* array_val = ve->GetArrayVarAddr(nullptr, i);
        if (array_val != nullptr && *(uint32*)array_val == obj_id)
            return true;
    }

    return false;
}

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

        // -- register the POD methods
        REGISTER_TYPE_METHOD(TYPE_object, initialized, TypeVariable_IsSet);
        REGISTER_TYPE_METHOD(TYPE_object, count, TypeVariable_Count);
        REGISTER_TYPE_METHOD(TYPE_object, contains, TypeObject_Contains);
        REGISTER_TYPE_METHOD(TYPE_object, copy, TypeVariableArray_Copy);
        REGISTER_TYPE_METHOD(TYPE_object, resize, TypeVariableArray_Resize);
    }

    // -- success
    return (true);
}

// ====================================================================================================================
// StringConfig():  Called from InitializeTypes() to populate the override table to perform numerical ops
// where one or both arguments are represented as strings
// ====================================================================================================================
bool TypeString_Contains(CVariableEntry* ve, const char* string_val)
{
    if (string_val == nullptr)
        return false;

    uint32 string_val_hash = TinScript::Hash(string_val, -1, false);
    int32 count = ve != nullptr ? ve->GetArraySize() : 0;
    for (int i = 0; i < count; ++i)
    {
        // -- string "values" are stored by hash
        void* array_val = ve->GetStringArrayHashAddr(nullptr, i);
        if (array_val != nullptr && *(uint32*)array_val == string_val_hash)
            return true;
    }

    return false;
}

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

        // -- register the POD methods
        REGISTER_TYPE_METHOD(TYPE_string, initialized, TypeVariable_IsSet);
        REGISTER_TYPE_METHOD(TYPE_string, count, TypeVariable_Count);
        REGISTER_TYPE_METHOD(TYPE_string, contains, TypeString_Contains);
        REGISTER_TYPE_METHOD(TYPE_string, copy, TypeVariableArray_Copy);
        REGISTER_TYPE_METHOD(TYPE_string, resize, TypeVariableArray_Resize);
    }

    // -- success
    return (true);
}

// ====================================================================================================================
// FloatConfig():  Called from InitializeTypes() to register float operations and conversions
// ====================================================================================================================
bool TypeFloat_Contains(CVariableEntry* ve, float float_val)
{
    int32 count = ve != nullptr ? ve->GetArraySize() : 0;
    for (int i = 0; i < count; ++i)
    {
        void* array_val = ve->GetArrayVarAddr(nullptr, i);
        if (array_val != nullptr && *(float*)array_val == float_val)
            return true;
    }

    return false;
}

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

        // -- register the POD methods
        REGISTER_TYPE_METHOD(TYPE_float, initialized, TypeVariable_IsSet);
        REGISTER_TYPE_METHOD(TYPE_float, count, TypeVariable_Count);
        REGISTER_TYPE_METHOD(TYPE_float, contains, TypeFloat_Contains);
        REGISTER_TYPE_METHOD(TYPE_float, copy, TypeVariableArray_Copy);
        REGISTER_TYPE_METHOD(TYPE_float, resize, TypeVariableArray_Resize);
    }

    // -- success
    return (true);
}

// ====================================================================================================================
// IntegerConfig():  Called from InitializeTypes() to register integer operations and conversions
// ====================================================================================================================
bool TypeInt_Contains(CVariableEntry* ve, int32 int_val)
{
    int32 count = ve != nullptr ? ve->GetArraySize() : 0;
    for (int i = 0; i < count; ++i)
    {
        void* array_val = ve->GetArrayVarAddr(nullptr, i);
        if (array_val != nullptr && *(int32*)array_val == int_val)
            return true;
    }

    return false;
}

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

        // -- register the POD methods
        REGISTER_TYPE_METHOD(TYPE_int, initialized, TypeVariable_IsSet);
        REGISTER_TYPE_METHOD(TYPE_int, count, TypeVariable_Count);
        REGISTER_TYPE_METHOD(TYPE_int, contains, TypeInt_Contains);
        REGISTER_TYPE_METHOD(TYPE_int, copy, TypeVariableArray_Copy);
        REGISTER_TYPE_METHOD(TYPE_int, resize, TypeVariableArray_Resize);
    }

    // -- success
    return (true);
}

// ====================================================================================================================
// BoolConfig():  Called from InitializeTypes() to register bool operations and conversions
// ====================================================================================================================
bool TypeBool_Contains(CVariableEntry* ve, bool bool_val)
{
    int32 count = ve != nullptr ? ve->GetArraySize() : 0;
    for (int i = 0; i < count; ++i)
    {
        void* array_val = ve->GetArrayVarAddr(nullptr, i);
        if (array_val != nullptr && *(bool*)array_val == bool_val)
            return true;
    }

    return false;
}

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

        // -- register the POD methods
        REGISTER_TYPE_METHOD(TYPE_bool, initialized, TypeVariable_IsSet);
        REGISTER_TYPE_METHOD(TYPE_bool, count, TypeVariable_Count);
        REGISTER_TYPE_METHOD(TYPE_bool, contains, TypeBool_Contains);
        REGISTER_TYPE_METHOD(TYPE_bool, copy, TypeVariableArray_Copy);
        REGISTER_TYPE_METHOD(TYPE_bool, resize, TypeVariableArray_Resize);
    }

    // -- success
    return (true);
}

// --------------------------------------------------------------------------------------------------------------------
// -- hashtable POD methods (not actually a POD) are more complicated, in that they don't have a "value"

void TypeHashtable_Clear(CVariableEntry* ht_ve)
{
    tVarTable* ht_vartable = ht_ve != nullptr ? (tVarTable*)ht_ve->GetAddr(nullptr) : nullptr;
    if (ht_vartable != nullptr)
    {
        ht_vartable->DestroyAll();
    }
}

int32 TypeHashtable_Count(CVariableEntry* ht_ve)
{
    tVarTable* ht_vartable = ht_ve != nullptr ? (tVarTable*)ht_ve->GetAddr(nullptr) : nullptr;
    int32 count = ht_vartable != nullptr ? ht_vartable->Used() : 0;
    return count;
}

bool TypeHashtable_HasKey(CVariableEntry* ht_ve, const char* key0, const char* key1, const char* key2,
                          const char* key3, const char* key4, const char* key5, const char* key6, const char* key7)
{
    // -- the hashtable key is an appended string, pushed in reverse order (since we use a stack)
    tVarTable* ht_vartable = ht_ve != nullptr ? (tVarTable*)ht_ve->GetAddr(nullptr) : nullptr;
    const char* key_table[8] =
    {
        key7 ? key7 : "",
        key6 ? key6 : "",
        key5 ? key5 : "",
        key4 ? key4 : "",
        key3 ? key3 : "",
        key2 ? key2 : "",
        key1 ? key1 : "",
        key0 ? key0 : "",
    };

    // -- append all of our keys together
    uint32 key_hash = 0;
    for (int i = 0; i < 8; ++i)
    {
        if (key_table[i][0] == '\0')
            continue;
        if (key_hash != 0)
        {
            key_hash = HashAppend(key_hash, "_");
            key_hash = HashAppend(key_hash, key_table[i]);
        }
        else
        {
            key_hash = Hash(key_table[i], -1, false);
        }
    }

    if (ht_vartable != nullptr)
    {
        return (ht_vartable->FindItem(key_hash) != nullptr);
    }

    // -- not found
    return (false);
}

// -- $$$TZA fixme - need a way to compare values of different types, without converting to a string
bool TypeHashtable_Contains(CVariableEntry* ht_ve, const char* value)
{
    tVarTable* ht_vartable = ht_ve != nullptr ? (tVarTable*)ht_ve->GetAddr(nullptr) : nullptr;
    if (ht_vartable != nullptr)
    {
        uint32 in_value_hash = Hash(value, -1, false);

        // -- see if we've got a matching string, converting the hashtable entries
        CVariableEntry* ht_ve = ht_vartable->First();
        while (ht_ve != nullptr)
        {
            // -- this is horrible - if we're looking to see if, e.g., a float is contains, and we give the string "3.14",
            // our comparison won't match, since converting the contained float to a string is "3.1400", (not identical)
            //  -- since our comparison is non-templated, we're stuck with an enum type, and a void*
            // -- the only way to make this work, is to convert the value to the ht type, and then back to a string
            // note:  conversions take the address of a hash value, when converting strings
            void* convert_val_to_ht_type = TypeConvert(TinScript::GetContext(), TYPE_string, (void*)&in_value_hash, ht_ve->GetType());
            if (convert_val_to_ht_type == nullptr)
            {
                continue;
            }

            // -- convert the ht version of "value" back to a string (which will turn "3.14" to "3.1400")...
            void* compare_val = TypeConvert(TinScript::GetContext(), ht_ve->GetType(), convert_val_to_ht_type, TYPE_string);

            // -- see if we can convert the hashtable value to a string
            void* converted_val = TypeConvert(TinScript::GetContext(), ht_ve->GetType(), ht_ve->GetAddr(nullptr),
                                               TYPE_string);
            if (converted_val != nullptr && compare_val != nullptr)
            {
                // -- get the hash for each of the converted strings
                uint32 compare_hash = *(uint32*)compare_val;
                uint32 val_hash = *(uint32*)converted_val;
                if (compare_hash == val_hash)
                {
                    // -- found
                    return true;
                }
            }

            ht_ve = ht_vartable->Next();
        }
    }

    // -- not found
    return (false);
}

bool TypeHashtable_Keys(CVariableEntry* ht_ve, CVariableEntry* ve_keys_array)
{
    // -- the source and dest must be an arrays of the same type
    tVarTable* ht_vartable = ht_ve != nullptr ? (tVarTable*)ht_ve->GetAddr(nullptr) : nullptr;
    if (ht_vartable == nullptr || ve_keys_array == nullptr || !ve_keys_array->IsArray() ||
        !ve_keys_array->IsScriptVar() || ve_keys_array->GetType() != TYPE_string)
    {
        if (ve_keys_array != nullptr)
        {
            TinPrint(TinScript::GetContext(), "Error - hashtable:keys(`%s`) failed\n"
                                              "Be sure %s is a script variable, an array of type string\n",
                                               UnHash(ve_keys_array->GetHash()), UnHash(ve_keys_array->GetHash()));
        }
        else
        {
            TinPrint(TinScript::GetContext(), "Error - hashtable:keys() failed\n");
        }
        return false;
    }

    // -- get the count of how many keys we'll need - resize the keys array if required
    int count = ht_vartable->Used();
    if (ve_keys_array->GetArraySize() != count)
    {
        // -- try to free the memory of the dest
        if (!ve_keys_array->TryFreeAddrMem())
            return (false);

        if (!ve_keys_array->ConvertToArray(count))
            return false;
    }
    
    // -- using raw entries, because we need the hash key (to unhash into a string)
    TinScript::CHashTable<CVariableEntry>::CHashTableEntry* hte = nullptr;
    for (int i = 0; i < count; ++i)
    {
        hte = ht_vartable->FindRawEntryByIndex(i, hte);
        if (hte != nullptr)
        {
            ve_keys_array->SetStringArrayHashValue(nullptr, &hte->hash, nullptr, nullptr, i);
        }
    }

    // -- return success
    return (true);
}

// ====================================================================================================================
// HashtableConfig():  Called from InitializeTypes(), in this case to add type methods
// ====================================================================================================================
bool8 HashtableConfig(eVarType var_type, bool8 onInit)
{
    // -- see if this is the initialization or the shutdown
    if (onInit)
    {
        // -- register the POD methods
        REGISTER_TYPE_METHOD(TYPE_hashtable, clear, TypeHashtable_Clear);
        REGISTER_TYPE_METHOD(TYPE_hashtable, count, TypeHashtable_Count);
        REGISTER_TYPE_METHOD(TYPE_hashtable, haskey, TypeHashtable_HasKey);
        REGISTER_TYPE_METHOD(TYPE_hashtable, contains, TypeHashtable_Contains);
        REGISTER_TYPE_METHOD(TYPE_hashtable, keys, TypeHashtable_Keys);
    }

    // -- success
    return (true);
}

} // TinScript

// ====================================================================================================================
// EOF
// ====================================================================================================================
