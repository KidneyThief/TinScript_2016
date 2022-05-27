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
// single include file for allowing any .cpp file to register classes / functions / methods, etc...
// ------------------------------------------------------------------------------------------------

#pragma once

#include "TinParse.h"

#include "registrationexecs.h"
#include "registrationclasses.h"
#include "registrationmacros.h"
#include "variadicclasses.h"
#include "registrationdefaultargs.h"

// ====================================================================================================================
// -- Registration macros

#define REGISTER_SCRIPT_CLASS_NO_CONSTRUCT_BEGIN(classname, parentname)                                             \
    static classname* __##classname##_Create() {                                                                    \
        assert(0 && #classname " cannot be constructed from script");                                               \
        return nullptr;                                                                                             \
    }                                                                                                               \
    static void __##classname##_Destroy(void* addr) {                                                               \
        if (addr) {                                                                                                 \
            assert(0 && #classname " cannot be destructed from script");                                            \
	    }                                                                                                           \
    }                                                                                                               \
    void __##classname##_Register(::TinScript::CScriptContext* script_context,                                      \
                                  ::TinScript::CNamespace* classnamespace);                                         \
    ::TinScript::CNamespaceReg reg_##classname(#classname, #parentname, ::TinScript::GetTypeID<classname*>(),       \
                                               (void*)__##classname##_Create, (void*)__##classname##_Destroy,       \
                                               (void*)__##classname##_Register);                                    \
    REGISTER_DEFAULT_METHODS(classname);                                                                            \
    void __##classname##_Register(::TinScript::CScriptContext* script_context,                                      \
                                  ::TinScript::CNamespace* classnamespace)                                          \
    {                                                                                                               \
        Unused_(script_context);                                                                                    \
        Unused_(classnamespace);

#define REGISTER_SCRIPT_CLASS_BEGIN(classname, parentname)                                                          \
    static classname* __##classname##_Create() {                                                                    \
        classname* newobj = TinAlloc(ALLOC_CreateObj, classname);                                                   \
        return newobj;                                                                                              \
    }                                                                                                               \
    static void __##classname##_Destroy(void* addr) {                                                               \
        if (addr) {                                                                                                 \
            classname* obj = static_cast<classname*>(addr);                                                         \
            TinFree(obj);                                                                                           \
	    }                                                                                                           \
    }                                                                                                               \
    void __##classname##_Register(::TinScript::CScriptContext* script_context,                                      \
                                  ::TinScript::CNamespace* classnamespace);                                         \
    ::TinScript::CNamespaceReg reg_##classname(#classname, #parentname, ::TinScript::GetTypeID<classname*>(),       \
                                               (void*)__##classname##_Create, (void*)__##classname##_Destroy,       \
                                               (void*)__##classname##_Register);                                    \
    REGISTER_DEFAULT_METHODS(classname);                                                                            \
    void __##classname##_Register(::TinScript::CScriptContext* script_context,                                      \
                                  ::TinScript::CNamespace* classnamespace)                                          \
    {                                                                                                               \
        Unused_(script_context);                                                                                    \
        Unused_(classnamespace);

#define REGISTER_SCRIPT_CLASS_END() \
    }

#define REGISTER_MEMBER(classname, scriptname, membername)                                              \
    {                                                                                                   \
        classname* classptr = reinterpret_cast<classname*>(0);                                          \
        uint32 varhash = ::TinScript::Hash(#scriptname);                                                \
        ::TinScript::CVariableEntry* ve =                                                               \
            TinAlloc(ALLOC_VarEntry, ::TinScript::CVariableEntry, script_context, #scriptname, varhash, \
            ::TinScript::GetRegisteredType(::TinScript::GetTypeID(classptr->membername)),               \
            ::TinScript::IsArray(classptr->membername) ?                                                \
            (sizeof(classptr->membername) / ::TinScript::GetTypeSize(classptr->membername)) : 1, true,  \
            Offsetof_(classname, membername));                                                          \
        classnamespace->GetVarTable()->AddItem(*ve, varhash);                                           \
    }

#define REGISTER_GLOBAL_VAR(scriptname, var)                                                        \
    ::TinScript::CRegisterGlobal _reg_gv_##scriptname(#scriptname,                                  \
        ::TinScript::GetRegisteredType(::TinScript::GetTypeID(var)), (void*)&var,                   \
        ::TinScript::IsArray(var) ? (sizeof(var) / ::TinScript::GetTypeSize(var)) : 1)

#define DECLARE_FILE(filename) \
    bool8 g_##filename##_registered = false;

#define REGISTER_FILE(filename) \
    extern bool8 g_##filename##_registered; \
    g_##filename##_registered = true;

// -- internal macros to register an enum
#define MAKE_ENUM(table, var, value) var = value,
#define MAKE_STRING(table, var, value) #var,
#define REG_ENUM_GLOBAL(table, var, value)	static int32 table##_##var = value; \
	    REGISTER_GLOBAL_VAR(table##_##var, table##_##var);

// -- individual macros to register just the enum table, the string table, or to register int32 globals with TinScript
#define CREATE_ENUM_CLASS(enum_name, enum_list)		\
	enum class enum_name {							\
		enum_list(enum_name, MAKE_ENUM)				\
	};

#define CREATE_ENUM_STRINGS(enum_name, enum_list)	\
	const char* enum_name##Strings[] = {			\
		enum_list(enum_name, MAKE_STRING)			\
	};												\

#define REGISTER_ENUM_CLASS(enum_name, enum_list)	\
	enum_list(enum_name, REG_ENUM_GLOBAL)			\

// -- a single macro to register the entire macro
#define REGISTER_SCRIPT_ENUM(enum_name, enum_list)	\
	enum class enum_name {							\
		enum_list(enum_name, MAKE_ENUM)				\
	};												\
	const char* enum_name##Strings[] = {			\
		enum_list(enum_name, MAKE_STRING)			\
	};												\
	enum_list(enum_name, REG_ENUM_GLOBAL)

// ====================================================================================================================
// -- These macros are included by REGISTER_SCRIPT_CLASS...
// -- not to be used independently (or publicly unless you know what you're doing!)
#define SCRIPT_DEFAULT_METHODS(classname)                                                           \
    static uint32 classname##GetObjectID(classname* obj);                                           \
    static const char* classname##GetObjectName(classname* obj);                                    \
    static uint32 classname##GetGroupID(classname* obj);                                            \
    static void classname##ListMembers(classname* obj, const char* partial = nullptr);              \
    static void classname##ListMethods(classname* obj, const char* partial = nullptr);              \
    static bool classname##HasMethod(classname* obj, const char* name);                             \
    static bool classname##HasMember(classname* obj, const char* name);                             \
    static bool classname##HasNamespace(classname* obj, const char* name);                          \

#define REGISTER_DEFAULT_METHODS(classname)                                                         \
    static uint32 classname##GetObjectID(classname* obj) {                                          \
        return ::TinScript::GetContext()->FindIDByAddress((void*)obj);                              \
    }                                                                                               \
    static CRegMethodP0<classname, uint32>                                                          \
        _reg_##classname##GetObjectID                                                               \
        ("GetObjectID", classname##GetObjectID);                                                    \
                                                                                                    \
    static const char* classname##GetObjectName(classname* obj) {                                   \
        ::TinScript::CObjectEntry* oe =                                                             \
            ::TinScript::GetContext()->FindObjectByAddress((void*)obj);                             \
        return oe ? oe->GetName() : "";                                                             \
    }                                                                                               \
    static CRegMethodP0<classname, const char*>                                                     \
        _reg_##classname##GetObjectName                                                             \
        ("GetObjectName", classname##GetObjectName);                                                \
                                                                                                    \
    static uint32 classname##GetGroupID(classname* obj) {                                           \
        ::TinScript::CScriptContext* script_context = ::TinScript::GetContext();                    \
        ::TinScript::CObjectEntry* oe =                                                             \
            script_context->FindObjectByAddress((void*)obj);                                        \
        ::TinScript::CObjectEntry* group_oe =                                                       \
            oe ? script_context->FindObjectByAddress((void*)oe->GetObjectGroup()) : NULL;           \
        return (group_oe ? group_oe->GetID() : 0);                                                  \
    }                                                                                               \
    static CRegMethodP0<classname, uint32>                                                          \
        _reg_##classname##GetGroupID                                                                \
        ("GetGroupID", classname##GetGroupID);                                                      \
                                                                                                    \
    static void classname##ListMembers(classname* obj, const char* partial = nullptr) {             \
        ::TinScript::CObjectEntry* oe =                                                             \
            ::TinScript::GetContext()->FindObjectByAddress((void*)obj);                             \
        ::TinScript::DumpVarTable(oe, partial);                                                     \
    }                                                                                               \
    static CRegMethodP1<classname, void, const char*>                                               \
        _reg_##classname##ListMembers                                                               \
        ("ListMembers", classname##ListMembers);                                                    \
                                                                                                    \
    static void classname##ListMethods(classname* obj, const char* partial = nullptr) {             \
        ::TinScript::CObjectEntry* oe =                                                             \
            ::TinScript::GetContext()->FindObjectByAddress((void*)obj);                             \
        ::TinScript::DumpFuncTable(oe, partial);                                                    \
    }                                                                                               \
    static CRegMethodP1<classname, void, const char*>                                               \
        _reg_##classname##ListMethods                                                               \
        ("ListMethods", classname##ListMethods);                                                    \
                                                                                                    \
    static bool classname##HasMember(classname* obj, const char* name) {                            \
        ::TinScript::CObjectEntry* oe =                                                             \
            ::TinScript::GetContext()->FindObjectByAddress((void*)obj);                             \
        return (::TinScript::GetContext()->HasMember(oe->GetID(), name));                           \
    }                                                                                               \
    static CRegMethodP1<classname, bool, const char*>                                               \
        _reg_##classname##HasMember                                                                 \
        ("HasMember", classname##HasMember);                                                        \
                                                                                                    \
    static bool classname##HasMethod(classname* obj, const char* name) {                            \
        ::TinScript::CObjectEntry* oe =                                                             \
            ::TinScript::GetContext()->FindObjectByAddress((void*)obj);                             \
        return (::TinScript::GetContext()->HasMethod(oe->GetID(), name));                           \
    }                                                                                               \
    static CRegMethodP1<classname, bool, const char*>                                               \
        _reg_##classname##HasMethod                                                                 \
        ("HasMethod", classname##HasMethod);                                                        \
                                                                                                    \
    static bool classname##HasNamespace(classname* obj, const char* name) {                         \
        ::TinScript::CObjectEntry* oe =                                                             \
            ::TinScript::GetContext()->FindObjectByAddress((void*)obj);                             \
        return (::TinScript::GetContext()->FindObject(oe->GetID(), name) != nullptr);               \
    }                                                                                               \
    static CRegMethodP1<classname, bool, const char*>                                               \
        _reg_##classname##HasNamespace                                                              \
        ("HasNamespace", classname##HasNamespace);                                                  \
                                                                                                    \

// -- eof -----------------------------------------------------------------------------------------
