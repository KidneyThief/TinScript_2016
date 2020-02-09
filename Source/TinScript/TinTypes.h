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
// TinTypes.h
// ====================================================================================================================

#ifndef __TINTYPES_H
#define __TINTYPES_H

// -- includes
#include "integration.h"
#include "TinHash.h"

// == namespace TinScript =============================================================================================

namespace TinScript
{

// --------------------------------------------------------------------------------------------------------------------
// -- constants
const int32 kMaxNameLength = 256;
const int32 kMaxTokenLength = 2048;

// -- current largest var type is a hashtable entry, 16 bytes
const int32 kMaxTypeSize = 16;

// --------------------------------------------------------------------------------------------------------------------
// -- for all non-first class types, declare a struct so GetTypeID<type> will be unique
struct sPODMember
{
    typedef uint32 type;
};

struct sMember
{
    typedef uint32 type;
};

struct sHashTable
{
    typedef uint32 type;
};

struct sHashVarIndex
{
    typedef uint32 type;
};

// == templated TypeID functions ======================================================================================
// ghetto type manipulation templates

// ====================================================================================================================
// GetTypeID():  Returns a unique ID for a given type.
// ====================================================================================================================
template <typename T>
inline uint32 GetTypeID()
{
    static T* t = NULL;
    void* ptr = (void*)&t;
    return (kPointerToUInt32(ptr));
}

// ====================================================================================================================
// GetTypeID():  Returns a unique ID for the type of argument passed.
// ====================================================================================================================
template <typename T>
uint32 GetTypeID(T&)
{
    return GetTypeID<T>();
}

// ====================================================================================================================
// -- overloaded GetTypeID() implementations for each registered type.
inline uint32 GetTypeID(sHashTable*)
{
    return GetTypeID<sHashTable>();
}

inline uint32 GetTypeID(uint32*)
{
    return GetTypeID<uint32>();
}

inline uint32 GetTypeID(const char*)
{
    return GetTypeID<const char*>();
}

inline uint32 GetTypeID(const char**)
{
    return GetTypeID<const char*>();
}

inline uint32 GetTypeID(float32*)
{
    return GetTypeID<float32>();
}

inline uint32 GetTypeID(int32*)
{
    return GetTypeID<int32>();
}

inline uint32 GetTypeID(bool8*)
{
    return GetTypeID<bool8>();
}

/*
inline uint32 GetTypeID(CVector3f*)
{
    return GetTypeID<CVector3f>();
}
*/

// ====================================================================================================================
// IsArray():  Returns a true if the argument passed is actually an array.
// ====================================================================================================================
template <typename T>
inline bool8 IsArray(T&)
{
    return (false);
}

// ====================================================================================================================
// -- overloaded IsArray() implementations for each registered type.
inline uint32 IsArray(sHashTable*)
{
    return (true);
}

inline uint32 IsArray(uint32*)
{
    return (true);
}

// -- const char* is still considered a single value, but an array of strings is
inline uint32 IsArray(const char*)
{
    return (false);
}

inline uint32 IsArray(const char**)
{
    return (true);
}

inline uint32 IsArray(float32*)
{
    return (true);
}

inline uint32 IsArray(int32*)
{
    return (true);
}

inline uint32 IsArray(bool8*)
{
    return (true);
}

/*
inline uint32 IsArray(CVector3f*)
{
    return (true);
}
*/

// ====================================================================================================================
// GetTypeSize():  Returns the byte size for the given type.
// ====================================================================================================================
template <typename T>
inline int32 GetTypeSize(T&)
{
    return (sizeof(T));
}

// ====================================================================================================================
// GetTypeSize():  Returns the byte size for the given type, for when an array of that type is passed.
// ====================================================================================================================
template <typename T>
inline int32 GetTypeSize(T*)
{
    return (sizeof(T));
}

// ====================================================================================================================
// CompareTypes():  Returns true if the two types are the same.
// ====================================================================================================================
template <typename T0, typename T1>
bool8 CompareTypes()
{
    return GetTypeID<T0>() == GetTypeID<T1>();
}

// ====================================================================================================================
// is_pointer():  Returns true if the type is actually a pointer.
// ====================================================================================================================
template<typename T>
struct is_pointer
{
    static const bool8 value = false;
};

template<typename T>
struct is_pointer<T*> {
    static const bool8 value = true;
};

// ====================================================================================================================
// remove_ptr():  Returns the actual type, whether the type, or a pointer is used.
// ====================================================================================================================
template<typename T>
struct remove_ptr
{
   typedef T type;
};

template<typename T>
struct remove_ptr<T*>
{
   typedef T type;
};

template <>
struct remove_ptr<const char*>
{
    typedef const char* type;
};

// ====================================================================================================================
// convert_from_void_ptr():  Convert a void pointer to the requested type.
// ====================================================================================================================
template<typename T>
struct convert_from_void_ptr
{
    static T Convert(void* addr)
    {
        return *reinterpret_cast<T*>(addr);
    }
};

template<typename T>
struct convert_from_void_ptr<T*>
{
    static T* Convert(void* addr)
    {
        return reinterpret_cast<T*>(addr);
    }
};

// ====================================================================================================================
// convert_to_void_ptr():  Converts a given value to a void*.
// ====================================================================================================================
template<typename T>
struct convert_to_void_ptr
{
    static void* Convert(T& t)
    {
        return reinterpret_cast<void*>(&t);
    }
};

template<typename T>
struct convert_to_void_ptr<const T*>
{
    static void* Convert(const T* t)
    {
        return reinterpret_cast<void*>(const_cast<T*>(t));
    }
};

// ====================================================================================================================
// -- typedefs for integrating the registered types
enum eVarType;
typedef bool8 (*TypeToString)(TinScript::CScriptContext* script_context, void* value, char* buf, int32 bufsize);
typedef bool8(*StringToType)(TinScript::CScriptContext* script_context, void* addr, char* value);

// -- an extra configuration function provided for non-standard types
// -- (e.g.  vector3f requires more initialization than a bool or float)
typedef bool8 (*TypeConfiguration)(eVarType, bool);

// ====================================================================================================================
// struct tPODTypeMember: For POD types, we need a hash table to contain the member hash, offset, and type.
// ====================================================================================================================
struct tPODTypeMember
{
    tPODTypeMember(eVarType _type, uint32 _offset)
    {
        type = _type;
        offset = _offset;
    }

    eVarType type;
    uint32 offset;
};

typedef CHashTable<tPODTypeMember> tPODTypeTable;

// ====================================================================================================================
// -- String conversion prototypes for standard types
bool8 VoidToString(TinScript::CScriptContext* script_context, void* value, char* buf, int32 bufsize);
bool8 StringToVoid(TinScript::CScriptContext* script_context, void* addr, char* value);
bool8 STEToString(TinScript::CScriptContext* script_context, void* value, char* buf, int32 bufsize);
bool8 StringToSTE(TinScript::CScriptContext* script_context, void* addr, char* value);
bool8 IntToString(TinScript::CScriptContext* script_context, void* value, char* buf, int32 bufsize);
bool8 StringToInt(TinScript::CScriptContext* script_context, void* addr, char* value);
bool8 BoolToString(TinScript::CScriptContext* script_context, void* value, char* buf, int32 bufsize);
bool8 StringToBool(TinScript::CScriptContext* script_context, void* addr, char* value);
bool8 FloatToString(TinScript::CScriptContext* script_context, void* value, char* buf, int32 bufsize);
bool8 StringToFloat(TinScript::CScriptContext* script_context, void* addr, char* value);

// ====================================================================================================================
// -- Configuration functions for standard types
bool8 ObjectConfig(eVarType var_type, bool8 onInit);
bool8 StringConfig(eVarType var_type, bool8 onInit);
bool8 FloatConfig(eVarType var_type, bool8 onInit);
bool8 IntegerConfig(eVarType var_type, bool8 onInit);
bool8 BoolConfig(eVarType var_type, bool8 onInit);

// -- external type configuration
bool8 Vector3fToString(TinScript::CScriptContext* script_context, void* value, char* buf, int32 bufsize);
bool8 StringToVector3f(TinScript::CScriptContext* script_context, void* addr, char* value);
bool8 Vector3fConfig(eVarType var_type, bool8 onInit);

// ====================================================================================================================
// -- operation and conversion type functions
enum eOpCode;
typedef bool8 (*TypeOpOverride)(CScriptContext* script_context, eOpCode op, eVarType& result_type, void* result_addr,
                                eVarType v0_type, void* val0, eVarType val1_type, void* val1);

typedef void* (*TypeConvertFunction)(CScriptContext* script_context, eVarType from_type, void* from_val,
                                     void* to_buffer);

// ====================================================================================================================
// -- use a tuple to define the token types:
// -- type, byte size, type-to-string, string-to-type, registered C++ equivalent, custom config function
// -- FIRST_VALID_TYPE is defined to identify the first type valid for use with a registered C++ method
// -- e.g.  CVector3f GetPosition(uint32 object_id)...  whereas no C++ function can return a  Type__stackvar...
// -- the custom config function is used to, say, create and register a POD member hashtable

// -- the ORDER in which the types are registered is valid, when looking up operation overrides
// -- for example if one of the values is a float, and one is an int, the float version of the operation
// -- will be chosen.  E.g. (3.5f * 10) is 35 using a float op, whereas (3.5f * 10) is 30 in integer math

#define FIRST_VALID_TYPE TYPE_hashtable
#define LAST_VALID_TYPE TYPE_vector3f
#define VarTypeTuple \
	VarTypeEntry(NULL,		    0,		VoidToString,		StringToVoid,       uint8,          NULL)               \
	VarTypeEntry(void,		    0,		VoidToString,		StringToVoid,       uint8,          NULL)   	        \
	VarTypeEntry(_resolve,	    16,		VoidToString,		StringToVoid,       uint8,          NULL)   	        \
	VarTypeEntry(_stackvar,     12,		IntToString,		StringToInt,        uint8,          NULL)   	        \
	VarTypeEntry(_var,          12,		IntToString,		StringToInt,        uint8,          NULL)   	        \
	VarTypeEntry(_member,       8,		IntToString,		StringToInt,        sMember,        NULL)           	\
	VarTypeEntry(_podmember,    8,		IntToString,		StringToInt,        sPODMember,     NULL)           	\
	VarTypeEntry(_hashvarindex, 16,		IntToString,		StringToInt,        sHashVarIndex,  NULL)           	\
    VarTypeEntry(hashtable,     4,      IntToString,        StringToInt,        sHashTable,     NULL)               \
	VarTypeEntry(object,        4,		IntToString,		StringToInt,        uint32,         ObjectConfig)       \
    VarTypeEntry(string,        4,      STEToString,        StringToSTE,        const char*,    StringConfig)       \
	VarTypeEntry(float,		    4,		FloatToString,		StringToFloat,      float32,        FloatConfig)        \
	VarTypeEntry(int,		    4,		IntToString,		StringToInt,        int32,          IntegerConfig)      \
	VarTypeEntry(bool,		    1,		BoolToString,		StringToBool,       bool8,          BoolConfig)         \
	VarTypeEntry(vector3f,	   12,		Vector3fToString,   StringToVector3f,   CVector3f,      Vector3fConfig)		\

// -- 4x words actually, 16x bytes, the size of a HashVar
#define MAX_TYPE_SIZE 4

enum eVarType : int16
{
	#define VarTypeEntry(a, b, c, d, e, f) TYPE_##a,
	VarTypeTuple
	#undef VarTypeEntry

	TYPE_COUNT
};

// ====================================================================================================================
// interface
void InitializeTypes();
void ShutdownTypes();

// -- manual registration of a POD table
void RegisterPODTypeTable(eVarType var_type, tPODTypeTable* pod_table);

// -- manual registration of an operation override for a registered types
void RegisterTypeOpOverride(eOpCode op, eVarType var_type, TypeOpOverride op_override);

// -- manual registration of the conversion to a type
void RegisterTypeConvert(eVarType to_type, eVarType from_type, TypeConvertFunction type_convert);

// ====================================================================================================================
void* TypeConvert(CScriptContext* script_context, eVarType fromtype, void* fromaddr, eVarType totype);
const char* DebugPrintVar(void* addr, eVarType vartype);

// ====================================================================================================================
// -- three types of registered functions: script, global, and method
#define FunctionTypeTuple \
	FunctionTypeEntry(NULL)		    	\
	FunctionTypeEntry(Script)			\
	FunctionTypeEntry(Global)			\
	FunctionTypeEntry(Method)			\

enum EFunctionType {
	#define FunctionTypeEntry(a) eFuncType##a,
	FunctionTypeTuple
	#undef FunctionTypeEntry
	eFuncTypeCount
};

// ====================================================================================================================
const char* GetRegisteredTypeName(eVarType vartype);
eVarType GetRegisteredType(const char* token, int32 length);
eVarType GetRegisteredType(uint32 id);

bool8 GetRegisteredPODMember(eVarType type_id, void* var_addr, uint32 member_hash, eVarType& out_member_type,
                             void*& out_member_addr);

TypeOpOverride GetTypeOpOverride(eOpCode op, eVarType var_type);


bool8 SafeStrcpy(char* dest, size_t dest_size, const char* src, size_t length = 0);
const char* SafeStrStr(const char* str, const char* partial, bool case_sensitive = false);
int32 Atoi(const char* src, int32 length = -1);

// ====================================================================================================================
// -- any registered type that implements a conver to bool can use this method to perform BooleanAnd and BooleanOr
bool8 BooleanBinaryOp(CScriptContext* script_context, eOpCode op, eVarType& result_type, void* result_addr,
                      eVarType val0_type, void* val0, eVarType val1_type, void* val1);

// ====================================================================================================================
// -- externs
extern const char* gRegisteredTypeNames[TYPE_COUNT];
extern int32 gRegisteredTypeSize[TYPE_COUNT];
extern TypeToString gRegisteredTypeToString[TYPE_COUNT];
extern StringToType gRegisteredStringToType[TYPE_COUNT];

} // TinScript

#endif // __TINTYPES_H

// ====================================================================================================================
// EOF
// ====================================================================================================================
