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
// Generated interface for calling scripted functions from code
// ------------------------------------------------------------------------------------------------

#ifndef __REGISTRATIONEXECS_H
#define __REGISTRATIONEXECS_H

#include "TinVariableEntry.h"
#include "TinFunctionEntry.h"
#include "TinExecute.h"

namespace TinScript
{

// -- the object must exist
inline bool8 ObjHasMethod(void* obj_addr, int32 method_hash, int32& out_param_count)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (script_context == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "TinScript context does not exist!\n");
        return false;
    }

    CObjectEntry* oe = script_context->FindObjectByAddress(obj_addr);
    if (oe == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not found\n");
        return false;
    }

    TinScript::CFunctionEntry* fe = oe->GetFunctionEntry(0, method_hash);
    out_param_count = fe && fe->GetContext() ? fe->GetContext()->GetParameterCount() : 0;
    return (fe != nullptr);
}

// -- the object must exist
inline bool8 ObjHasMethod(uint32 obj_id, int32 method_hash, int32& out_param_count)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (script_context == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "TinScript context does not exist!\n");
        return false;
    }

    CObjectEntry* oe = script_context->FindObjectEntry(obj_id);
    if (oe == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not found\n");
        return false;
    }

    TinScript::CFunctionEntry* fe = oe->GetFunctionEntry(0, method_hash);
    out_param_count = fe && fe->GetContext() ? fe->GetContext()->GetParameterCount() : 0;
    return (fe != nullptr);
}



// -- Parameter count: 0
template<typename R>
inline bool8 ExecFunctionImpl(R& return_value, uint32 object_id, uint32 ns_hash, uint32 func_hash)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    // -- get the object, if one was required
    CObjectEntry* oe = object_id > 0 ? script_context->FindObjectEntry(object_id) : NULL;
    if (!oe && object_id > 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object %d not found\n", object_id);
        return false;
    }

    TinScript::CFunctionEntry* fe = oe ? oe->GetFunctionEntry(ns_hash, func_hash)
                            : script_context->GetGlobalNamespace()->GetFuncTable()->FindItem(func_hash);
    TinScript::CVariableEntry* return_ve = fe ? fe->GetContext()->GetParameter(0) : NULL;
    if (!fe || !return_ve)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() not found\n", UnHash(func_hash));
        return false;
    }

    // -- see if we can recognize an appropriate type
    eVarType returntype = TinScript::GetRegisteredType(TinScript::GetTypeID<R>());
    if (returntype == TYPE_NULL)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - invalid return type (use an int32 if void)\n");
        return false;
    }

    // -- execute the function
    if (!ExecuteScheduledFunction(GetContext(), object_id, ns_hash, func_hash, fe->GetContext()))
    {
        TinPrint(script_context, "Error - unable to exec function %s()\n", UnHash(func_hash));
        return false;
    }

    // -- return true if we're able to convert to the return type requested
    return (ReturnExecfResult(script_context, return_value));
}

template<typename R>
inline bool8 ExecFunction(R& return_value, const char* func_name)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !func_name || !func_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, TinScript::Hash(func_name)));
}

template<typename R>
inline bool8 ExecFunction(R& return_value, uint32 func_hash)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, func_hash));
}

template<typename R>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, const char* method_name)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, TinScript::Hash(method_name)));
}

template<typename R>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, uint32 method_hash)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, method_hash));
}

template<typename R>
inline bool8 ObjExecNSMethod(uint32 object_id, R& return_value, uint32 ns_hash, uint32 method_hash)
{
    return (ExecFunctionImpl<R>(return_value, object_id, ns_hash, method_hash));
}

template<typename R>
inline bool8 ObjExecMethod(uint32 object_id, R& return_value, const char* method_name)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, object_id, 0, TinScript::Hash(method_name)));
}



// -- Parameter count: 1
template<typename R, typename T1>
inline bool8 ExecFunctionImpl(R& return_value, uint32 object_id, uint32 ns_hash, uint32 func_hash, T1 p1)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    // -- get the object, if one was required
    CObjectEntry* oe = object_id > 0 ? script_context->FindObjectEntry(object_id) : NULL;
    if (!oe && object_id > 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object %d not found\n", object_id);
        return false;
    }

    TinScript::CFunctionEntry* fe = oe ? oe->GetFunctionEntry(ns_hash, func_hash)
                            : script_context->GetGlobalNamespace()->GetFuncTable()->FindItem(func_hash);
    TinScript::CVariableEntry* return_ve = fe ? fe->GetContext()->GetParameter(0) : NULL;
    if (!fe || !return_ve)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() not found\n", UnHash(func_hash));
        return false;
    }

    // -- see if we can recognize an appropriate type
    eVarType returntype = TinScript::GetRegisteredType(TinScript::GetTypeID<R>());
    if (returntype == TYPE_NULL)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - invalid return type (use an int32 if void)\n");
        return false;
    }

    TinScript::CVariableEntry* ve_p1 = fe->GetContext()->GetParameter(1);
    if (ve_p1 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p1_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p1_ptr_ptr = (void**)(&p1);
        void* p1_ptr = (void*)(*p1_ptr_ptr);
        p1_convert_addr = TypeConvert(script_context, TYPE_string, p1_ptr, ve_p1->GetType());
    }
    else
        p1_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), (void*)&p1, ve_p1->GetType());
    if (!p1_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 1\n", UnHash(func_hash));
        return false;
    }

    ve_p1->SetValueAddr(NULL, p1_convert_addr);

    // -- execute the function
    if (!ExecuteScheduledFunction(GetContext(), object_id, ns_hash, func_hash, fe->GetContext()))
    {
        TinPrint(script_context, "Error - unable to exec function %s()\n", UnHash(func_hash));
        return false;
    }

    // -- return true if we're able to convert to the return type requested
    return (ReturnExecfResult(script_context, return_value));
}

template<typename R, typename T1>
inline bool8 ExecFunction(R& return_value, const char* func_name, T1 p1)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !func_name || !func_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, TinScript::Hash(func_name), p1));
}

template<typename R, typename T1>
inline bool8 ExecFunction(R& return_value, uint32 func_hash, T1 p1)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, func_hash, p1));
}

template<typename R, typename T1>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, const char* method_name, T1 p1)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, TinScript::Hash(method_name), p1));
}

template<typename R, typename T1>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, uint32 method_hash, T1 p1)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, method_hash, p1));
}

template<typename R, typename T1>
inline bool8 ObjExecNSMethod(uint32 object_id, R& return_value, uint32 ns_hash, uint32 method_hash, T1 p1)
{
    return (ExecFunctionImpl<R>(return_value, object_id, ns_hash, method_hash, p1));
}

template<typename R, typename T1>
inline bool8 ObjExecMethod(uint32 object_id, R& return_value, const char* method_name, T1 p1)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, object_id, 0, TinScript::Hash(method_name), p1));
}



// -- Parameter count: 2
template<typename R, typename T1, typename T2>
inline bool8 ExecFunctionImpl(R& return_value, uint32 object_id, uint32 ns_hash, uint32 func_hash, T1 p1, T2 p2)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    // -- get the object, if one was required
    CObjectEntry* oe = object_id > 0 ? script_context->FindObjectEntry(object_id) : NULL;
    if (!oe && object_id > 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object %d not found\n", object_id);
        return false;
    }

    TinScript::CFunctionEntry* fe = oe ? oe->GetFunctionEntry(ns_hash, func_hash)
                            : script_context->GetGlobalNamespace()->GetFuncTable()->FindItem(func_hash);
    TinScript::CVariableEntry* return_ve = fe ? fe->GetContext()->GetParameter(0) : NULL;
    if (!fe || !return_ve)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() not found\n", UnHash(func_hash));
        return false;
    }

    // -- see if we can recognize an appropriate type
    eVarType returntype = TinScript::GetRegisteredType(TinScript::GetTypeID<R>());
    if (returntype == TYPE_NULL)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - invalid return type (use an int32 if void)\n");
        return false;
    }

    TinScript::CVariableEntry* ve_p1 = fe->GetContext()->GetParameter(1);
    if (ve_p1 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p1_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p1_ptr_ptr = (void**)(&p1);
        void* p1_ptr = (void*)(*p1_ptr_ptr);
        p1_convert_addr = TypeConvert(script_context, TYPE_string, p1_ptr, ve_p1->GetType());
    }
    else
        p1_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), (void*)&p1, ve_p1->GetType());
    if (!p1_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 1\n", UnHash(func_hash));
        return false;
    }

    ve_p1->SetValueAddr(NULL, p1_convert_addr);

    TinScript::CVariableEntry* ve_p2 = fe->GetContext()->GetParameter(2);
    if (ve_p2 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p2_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p2_ptr_ptr = (void**)(&p2);
        void* p2_ptr = (void*)(*p2_ptr_ptr);
        p2_convert_addr = TypeConvert(script_context, TYPE_string, p2_ptr, ve_p2->GetType());
    }
    else
        p2_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), (void*)&p2, ve_p2->GetType());
    if (!p2_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 2\n", UnHash(func_hash));
        return false;
    }

    ve_p2->SetValueAddr(NULL, p2_convert_addr);

    // -- execute the function
    if (!ExecuteScheduledFunction(GetContext(), object_id, ns_hash, func_hash, fe->GetContext()))
    {
        TinPrint(script_context, "Error - unable to exec function %s()\n", UnHash(func_hash));
        return false;
    }

    // -- return true if we're able to convert to the return type requested
    return (ReturnExecfResult(script_context, return_value));
}

template<typename R, typename T1, typename T2>
inline bool8 ExecFunction(R& return_value, const char* func_name, T1 p1, T2 p2)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !func_name || !func_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, TinScript::Hash(func_name), p1, p2));
}

template<typename R, typename T1, typename T2>
inline bool8 ExecFunction(R& return_value, uint32 func_hash, T1 p1, T2 p2)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, func_hash, p1, p2));
}

template<typename R, typename T1, typename T2>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, const char* method_name, T1 p1, T2 p2)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, TinScript::Hash(method_name), p1, p2));
}

template<typename R, typename T1, typename T2>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, uint32 method_hash, T1 p1, T2 p2)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, method_hash, p1, p2));
}

template<typename R, typename T1, typename T2>
inline bool8 ObjExecNSMethod(uint32 object_id, R& return_value, uint32 ns_hash, uint32 method_hash, T1 p1, T2 p2)
{
    return (ExecFunctionImpl<R>(return_value, object_id, ns_hash, method_hash, p1, p2));
}

template<typename R, typename T1, typename T2>
inline bool8 ObjExecMethod(uint32 object_id, R& return_value, const char* method_name, T1 p1, T2 p2)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, object_id, 0, TinScript::Hash(method_name), p1, p2));
}



// -- Parameter count: 3
template<typename R, typename T1, typename T2, typename T3>
inline bool8 ExecFunctionImpl(R& return_value, uint32 object_id, uint32 ns_hash, uint32 func_hash, T1 p1, T2 p2, T3 p3)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    // -- get the object, if one was required
    CObjectEntry* oe = object_id > 0 ? script_context->FindObjectEntry(object_id) : NULL;
    if (!oe && object_id > 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object %d not found\n", object_id);
        return false;
    }

    TinScript::CFunctionEntry* fe = oe ? oe->GetFunctionEntry(ns_hash, func_hash)
                            : script_context->GetGlobalNamespace()->GetFuncTable()->FindItem(func_hash);
    TinScript::CVariableEntry* return_ve = fe ? fe->GetContext()->GetParameter(0) : NULL;
    if (!fe || !return_ve)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() not found\n", UnHash(func_hash));
        return false;
    }

    // -- see if we can recognize an appropriate type
    eVarType returntype = TinScript::GetRegisteredType(TinScript::GetTypeID<R>());
    if (returntype == TYPE_NULL)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - invalid return type (use an int32 if void)\n");
        return false;
    }

    TinScript::CVariableEntry* ve_p1 = fe->GetContext()->GetParameter(1);
    if (ve_p1 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p1_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p1_ptr_ptr = (void**)(&p1);
        void* p1_ptr = (void*)(*p1_ptr_ptr);
        p1_convert_addr = TypeConvert(script_context, TYPE_string, p1_ptr, ve_p1->GetType());
    }
    else
        p1_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), (void*)&p1, ve_p1->GetType());
    if (!p1_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 1\n", UnHash(func_hash));
        return false;
    }

    ve_p1->SetValueAddr(NULL, p1_convert_addr);

    TinScript::CVariableEntry* ve_p2 = fe->GetContext()->GetParameter(2);
    if (ve_p2 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p2_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p2_ptr_ptr = (void**)(&p2);
        void* p2_ptr = (void*)(*p2_ptr_ptr);
        p2_convert_addr = TypeConvert(script_context, TYPE_string, p2_ptr, ve_p2->GetType());
    }
    else
        p2_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), (void*)&p2, ve_p2->GetType());
    if (!p2_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 2\n", UnHash(func_hash));
        return false;
    }

    ve_p2->SetValueAddr(NULL, p2_convert_addr);

    TinScript::CVariableEntry* ve_p3 = fe->GetContext()->GetParameter(3);
    if (ve_p3 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p3_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p3_ptr_ptr = (void**)(&p3);
        void* p3_ptr = (void*)(*p3_ptr_ptr);
        p3_convert_addr = TypeConvert(script_context, TYPE_string, p3_ptr, ve_p3->GetType());
    }
    else
        p3_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), (void*)&p3, ve_p3->GetType());
    if (!p3_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 3\n", UnHash(func_hash));
        return false;
    }

    ve_p3->SetValueAddr(NULL, p3_convert_addr);

    // -- execute the function
    if (!ExecuteScheduledFunction(GetContext(), object_id, ns_hash, func_hash, fe->GetContext()))
    {
        TinPrint(script_context, "Error - unable to exec function %s()\n", UnHash(func_hash));
        return false;
    }

    // -- return true if we're able to convert to the return type requested
    return (ReturnExecfResult(script_context, return_value));
}

template<typename R, typename T1, typename T2, typename T3>
inline bool8 ExecFunction(R& return_value, const char* func_name, T1 p1, T2 p2, T3 p3)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !func_name || !func_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, TinScript::Hash(func_name), p1, p2, p3));
}

template<typename R, typename T1, typename T2, typename T3>
inline bool8 ExecFunction(R& return_value, uint32 func_hash, T1 p1, T2 p2, T3 p3)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, func_hash, p1, p2, p3));
}

template<typename R, typename T1, typename T2, typename T3>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, const char* method_name, T1 p1, T2 p2, T3 p3)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, TinScript::Hash(method_name), p1, p2, p3));
}

template<typename R, typename T1, typename T2, typename T3>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, uint32 method_hash, T1 p1, T2 p2, T3 p3)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, method_hash, p1, p2, p3));
}

template<typename R, typename T1, typename T2, typename T3>
inline bool8 ObjExecNSMethod(uint32 object_id, R& return_value, uint32 ns_hash, uint32 method_hash, T1 p1, T2 p2, T3 p3)
{
    return (ExecFunctionImpl<R>(return_value, object_id, ns_hash, method_hash, p1, p2, p3));
}

template<typename R, typename T1, typename T2, typename T3>
inline bool8 ObjExecMethod(uint32 object_id, R& return_value, const char* method_name, T1 p1, T2 p2, T3 p3)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, object_id, 0, TinScript::Hash(method_name), p1, p2, p3));
}



// -- Parameter count: 4
template<typename R, typename T1, typename T2, typename T3, typename T4>
inline bool8 ExecFunctionImpl(R& return_value, uint32 object_id, uint32 ns_hash, uint32 func_hash, T1 p1, T2 p2, T3 p3, T4 p4)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    // -- get the object, if one was required
    CObjectEntry* oe = object_id > 0 ? script_context->FindObjectEntry(object_id) : NULL;
    if (!oe && object_id > 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object %d not found\n", object_id);
        return false;
    }

    TinScript::CFunctionEntry* fe = oe ? oe->GetFunctionEntry(ns_hash, func_hash)
                            : script_context->GetGlobalNamespace()->GetFuncTable()->FindItem(func_hash);
    TinScript::CVariableEntry* return_ve = fe ? fe->GetContext()->GetParameter(0) : NULL;
    if (!fe || !return_ve)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() not found\n", UnHash(func_hash));
        return false;
    }

    // -- see if we can recognize an appropriate type
    eVarType returntype = TinScript::GetRegisteredType(TinScript::GetTypeID<R>());
    if (returntype == TYPE_NULL)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - invalid return type (use an int32 if void)\n");
        return false;
    }

    TinScript::CVariableEntry* ve_p1 = fe->GetContext()->GetParameter(1);
    if (ve_p1 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p1_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p1_ptr_ptr = (void**)(&p1);
        void* p1_ptr = (void*)(*p1_ptr_ptr);
        p1_convert_addr = TypeConvert(script_context, TYPE_string, p1_ptr, ve_p1->GetType());
    }
    else
        p1_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), (void*)&p1, ve_p1->GetType());
    if (!p1_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 1\n", UnHash(func_hash));
        return false;
    }

    ve_p1->SetValueAddr(NULL, p1_convert_addr);

    TinScript::CVariableEntry* ve_p2 = fe->GetContext()->GetParameter(2);
    if (ve_p2 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p2_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p2_ptr_ptr = (void**)(&p2);
        void* p2_ptr = (void*)(*p2_ptr_ptr);
        p2_convert_addr = TypeConvert(script_context, TYPE_string, p2_ptr, ve_p2->GetType());
    }
    else
        p2_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), (void*)&p2, ve_p2->GetType());
    if (!p2_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 2\n", UnHash(func_hash));
        return false;
    }

    ve_p2->SetValueAddr(NULL, p2_convert_addr);

    TinScript::CVariableEntry* ve_p3 = fe->GetContext()->GetParameter(3);
    if (ve_p3 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p3_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p3_ptr_ptr = (void**)(&p3);
        void* p3_ptr = (void*)(*p3_ptr_ptr);
        p3_convert_addr = TypeConvert(script_context, TYPE_string, p3_ptr, ve_p3->GetType());
    }
    else
        p3_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), (void*)&p3, ve_p3->GetType());
    if (!p3_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 3\n", UnHash(func_hash));
        return false;
    }

    ve_p3->SetValueAddr(NULL, p3_convert_addr);

    TinScript::CVariableEntry* ve_p4 = fe->GetContext()->GetParameter(4);
    if (ve_p4 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p4_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p4_ptr_ptr = (void**)(&p4);
        void* p4_ptr = (void*)(*p4_ptr_ptr);
        p4_convert_addr = TypeConvert(script_context, TYPE_string, p4_ptr, ve_p4->GetType());
    }
    else
        p4_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), (void*)&p4, ve_p4->GetType());
    if (!p4_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 4\n", UnHash(func_hash));
        return false;
    }

    ve_p4->SetValueAddr(NULL, p4_convert_addr);

    // -- execute the function
    if (!ExecuteScheduledFunction(GetContext(), object_id, ns_hash, func_hash, fe->GetContext()))
    {
        TinPrint(script_context, "Error - unable to exec function %s()\n", UnHash(func_hash));
        return false;
    }

    // -- return true if we're able to convert to the return type requested
    return (ReturnExecfResult(script_context, return_value));
}

template<typename R, typename T1, typename T2, typename T3, typename T4>
inline bool8 ExecFunction(R& return_value, const char* func_name, T1 p1, T2 p2, T3 p3, T4 p4)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !func_name || !func_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, TinScript::Hash(func_name), p1, p2, p3, p4));
}

template<typename R, typename T1, typename T2, typename T3, typename T4>
inline bool8 ExecFunction(R& return_value, uint32 func_hash, T1 p1, T2 p2, T3 p3, T4 p4)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, func_hash, p1, p2, p3, p4));
}

template<typename R, typename T1, typename T2, typename T3, typename T4>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, const char* method_name, T1 p1, T2 p2, T3 p3, T4 p4)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, TinScript::Hash(method_name), p1, p2, p3, p4));
}

template<typename R, typename T1, typename T2, typename T3, typename T4>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, uint32 method_hash, T1 p1, T2 p2, T3 p3, T4 p4)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, method_hash, p1, p2, p3, p4));
}

template<typename R, typename T1, typename T2, typename T3, typename T4>
inline bool8 ObjExecNSMethod(uint32 object_id, R& return_value, uint32 ns_hash, uint32 method_hash, T1 p1, T2 p2, T3 p3, T4 p4)
{
    return (ExecFunctionImpl<R>(return_value, object_id, ns_hash, method_hash, p1, p2, p3, p4));
}

template<typename R, typename T1, typename T2, typename T3, typename T4>
inline bool8 ObjExecMethod(uint32 object_id, R& return_value, const char* method_name, T1 p1, T2 p2, T3 p3, T4 p4)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, object_id, 0, TinScript::Hash(method_name), p1, p2, p3, p4));
}



// -- Parameter count: 5
template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5>
inline bool8 ExecFunctionImpl(R& return_value, uint32 object_id, uint32 ns_hash, uint32 func_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    // -- get the object, if one was required
    CObjectEntry* oe = object_id > 0 ? script_context->FindObjectEntry(object_id) : NULL;
    if (!oe && object_id > 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object %d not found\n", object_id);
        return false;
    }

    TinScript::CFunctionEntry* fe = oe ? oe->GetFunctionEntry(ns_hash, func_hash)
                            : script_context->GetGlobalNamespace()->GetFuncTable()->FindItem(func_hash);
    TinScript::CVariableEntry* return_ve = fe ? fe->GetContext()->GetParameter(0) : NULL;
    if (!fe || !return_ve)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() not found\n", UnHash(func_hash));
        return false;
    }

    // -- see if we can recognize an appropriate type
    eVarType returntype = TinScript::GetRegisteredType(TinScript::GetTypeID<R>());
    if (returntype == TYPE_NULL)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - invalid return type (use an int32 if void)\n");
        return false;
    }

    TinScript::CVariableEntry* ve_p1 = fe->GetContext()->GetParameter(1);
    if (ve_p1 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p1_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p1_ptr_ptr = (void**)(&p1);
        void* p1_ptr = (void*)(*p1_ptr_ptr);
        p1_convert_addr = TypeConvert(script_context, TYPE_string, p1_ptr, ve_p1->GetType());
    }
    else
        p1_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), (void*)&p1, ve_p1->GetType());
    if (!p1_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 1\n", UnHash(func_hash));
        return false;
    }

    ve_p1->SetValueAddr(NULL, p1_convert_addr);

    TinScript::CVariableEntry* ve_p2 = fe->GetContext()->GetParameter(2);
    if (ve_p2 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p2_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p2_ptr_ptr = (void**)(&p2);
        void* p2_ptr = (void*)(*p2_ptr_ptr);
        p2_convert_addr = TypeConvert(script_context, TYPE_string, p2_ptr, ve_p2->GetType());
    }
    else
        p2_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), (void*)&p2, ve_p2->GetType());
    if (!p2_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 2\n", UnHash(func_hash));
        return false;
    }

    ve_p2->SetValueAddr(NULL, p2_convert_addr);

    TinScript::CVariableEntry* ve_p3 = fe->GetContext()->GetParameter(3);
    if (ve_p3 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p3_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p3_ptr_ptr = (void**)(&p3);
        void* p3_ptr = (void*)(*p3_ptr_ptr);
        p3_convert_addr = TypeConvert(script_context, TYPE_string, p3_ptr, ve_p3->GetType());
    }
    else
        p3_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), (void*)&p3, ve_p3->GetType());
    if (!p3_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 3\n", UnHash(func_hash));
        return false;
    }

    ve_p3->SetValueAddr(NULL, p3_convert_addr);

    TinScript::CVariableEntry* ve_p4 = fe->GetContext()->GetParameter(4);
    if (ve_p4 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p4_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p4_ptr_ptr = (void**)(&p4);
        void* p4_ptr = (void*)(*p4_ptr_ptr);
        p4_convert_addr = TypeConvert(script_context, TYPE_string, p4_ptr, ve_p4->GetType());
    }
    else
        p4_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), (void*)&p4, ve_p4->GetType());
    if (!p4_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 4\n", UnHash(func_hash));
        return false;
    }

    ve_p4->SetValueAddr(NULL, p4_convert_addr);

    TinScript::CVariableEntry* ve_p5 = fe->GetContext()->GetParameter(5);
    if (ve_p5 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p5_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p5_ptr_ptr = (void**)(&p5);
        void* p5_ptr = (void*)(*p5_ptr_ptr);
        p5_convert_addr = TypeConvert(script_context, TYPE_string, p5_ptr, ve_p5->GetType());
    }
    else
        p5_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), (void*)&p5, ve_p5->GetType());
    if (!p5_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 5\n", UnHash(func_hash));
        return false;
    }

    ve_p5->SetValueAddr(NULL, p5_convert_addr);

    // -- execute the function
    if (!ExecuteScheduledFunction(GetContext(), object_id, ns_hash, func_hash, fe->GetContext()))
    {
        TinPrint(script_context, "Error - unable to exec function %s()\n", UnHash(func_hash));
        return false;
    }

    // -- return true if we're able to convert to the return type requested
    return (ReturnExecfResult(script_context, return_value));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5>
inline bool8 ExecFunction(R& return_value, const char* func_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !func_name || !func_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, TinScript::Hash(func_name), p1, p2, p3, p4, p5));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5>
inline bool8 ExecFunction(R& return_value, uint32 func_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, func_hash, p1, p2, p3, p4, p5));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, const char* method_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, TinScript::Hash(method_name), p1, p2, p3, p4, p5));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, uint32 method_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, method_hash, p1, p2, p3, p4, p5));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5>
inline bool8 ObjExecNSMethod(uint32 object_id, R& return_value, uint32 ns_hash, uint32 method_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5)
{
    return (ExecFunctionImpl<R>(return_value, object_id, ns_hash, method_hash, p1, p2, p3, p4, p5));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5>
inline bool8 ObjExecMethod(uint32 object_id, R& return_value, const char* method_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, object_id, 0, TinScript::Hash(method_name), p1, p2, p3, p4, p5));
}



// -- Parameter count: 6
template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
inline bool8 ExecFunctionImpl(R& return_value, uint32 object_id, uint32 ns_hash, uint32 func_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    // -- get the object, if one was required
    CObjectEntry* oe = object_id > 0 ? script_context->FindObjectEntry(object_id) : NULL;
    if (!oe && object_id > 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object %d not found\n", object_id);
        return false;
    }

    TinScript::CFunctionEntry* fe = oe ? oe->GetFunctionEntry(ns_hash, func_hash)
                            : script_context->GetGlobalNamespace()->GetFuncTable()->FindItem(func_hash);
    TinScript::CVariableEntry* return_ve = fe ? fe->GetContext()->GetParameter(0) : NULL;
    if (!fe || !return_ve)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() not found\n", UnHash(func_hash));
        return false;
    }

    // -- see if we can recognize an appropriate type
    eVarType returntype = TinScript::GetRegisteredType(TinScript::GetTypeID<R>());
    if (returntype == TYPE_NULL)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - invalid return type (use an int32 if void)\n");
        return false;
    }

    TinScript::CVariableEntry* ve_p1 = fe->GetContext()->GetParameter(1);
    if (ve_p1 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p1_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p1_ptr_ptr = (void**)(&p1);
        void* p1_ptr = (void*)(*p1_ptr_ptr);
        p1_convert_addr = TypeConvert(script_context, TYPE_string, p1_ptr, ve_p1->GetType());
    }
    else
        p1_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), (void*)&p1, ve_p1->GetType());
    if (!p1_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 1\n", UnHash(func_hash));
        return false;
    }

    ve_p1->SetValueAddr(NULL, p1_convert_addr);

    TinScript::CVariableEntry* ve_p2 = fe->GetContext()->GetParameter(2);
    if (ve_p2 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p2_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p2_ptr_ptr = (void**)(&p2);
        void* p2_ptr = (void*)(*p2_ptr_ptr);
        p2_convert_addr = TypeConvert(script_context, TYPE_string, p2_ptr, ve_p2->GetType());
    }
    else
        p2_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), (void*)&p2, ve_p2->GetType());
    if (!p2_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 2\n", UnHash(func_hash));
        return false;
    }

    ve_p2->SetValueAddr(NULL, p2_convert_addr);

    TinScript::CVariableEntry* ve_p3 = fe->GetContext()->GetParameter(3);
    if (ve_p3 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p3_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p3_ptr_ptr = (void**)(&p3);
        void* p3_ptr = (void*)(*p3_ptr_ptr);
        p3_convert_addr = TypeConvert(script_context, TYPE_string, p3_ptr, ve_p3->GetType());
    }
    else
        p3_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), (void*)&p3, ve_p3->GetType());
    if (!p3_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 3\n", UnHash(func_hash));
        return false;
    }

    ve_p3->SetValueAddr(NULL, p3_convert_addr);

    TinScript::CVariableEntry* ve_p4 = fe->GetContext()->GetParameter(4);
    if (ve_p4 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p4_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p4_ptr_ptr = (void**)(&p4);
        void* p4_ptr = (void*)(*p4_ptr_ptr);
        p4_convert_addr = TypeConvert(script_context, TYPE_string, p4_ptr, ve_p4->GetType());
    }
    else
        p4_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), (void*)&p4, ve_p4->GetType());
    if (!p4_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 4\n", UnHash(func_hash));
        return false;
    }

    ve_p4->SetValueAddr(NULL, p4_convert_addr);

    TinScript::CVariableEntry* ve_p5 = fe->GetContext()->GetParameter(5);
    if (ve_p5 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p5_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p5_ptr_ptr = (void**)(&p5);
        void* p5_ptr = (void*)(*p5_ptr_ptr);
        p5_convert_addr = TypeConvert(script_context, TYPE_string, p5_ptr, ve_p5->GetType());
    }
    else
        p5_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), (void*)&p5, ve_p5->GetType());
    if (!p5_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 5\n", UnHash(func_hash));
        return false;
    }

    ve_p5->SetValueAddr(NULL, p5_convert_addr);

    TinScript::CVariableEntry* ve_p6 = fe->GetContext()->GetParameter(6);
    if (ve_p6 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p6_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p6_ptr_ptr = (void**)(&p6);
        void* p6_ptr = (void*)(*p6_ptr_ptr);
        p6_convert_addr = TypeConvert(script_context, TYPE_string, p6_ptr, ve_p6->GetType());
    }
    else
        p6_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()), (void*)&p6, ve_p6->GetType());
    if (!p6_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 6\n", UnHash(func_hash));
        return false;
    }

    ve_p6->SetValueAddr(NULL, p6_convert_addr);

    // -- execute the function
    if (!ExecuteScheduledFunction(GetContext(), object_id, ns_hash, func_hash, fe->GetContext()))
    {
        TinPrint(script_context, "Error - unable to exec function %s()\n", UnHash(func_hash));
        return false;
    }

    // -- return true if we're able to convert to the return type requested
    return (ReturnExecfResult(script_context, return_value));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
inline bool8 ExecFunction(R& return_value, const char* func_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !func_name || !func_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, TinScript::Hash(func_name), p1, p2, p3, p4, p5, p6));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
inline bool8 ExecFunction(R& return_value, uint32 func_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, func_hash, p1, p2, p3, p4, p5, p6));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, const char* method_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, TinScript::Hash(method_name), p1, p2, p3, p4, p5, p6));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, uint32 method_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, method_hash, p1, p2, p3, p4, p5, p6));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
inline bool8 ObjExecNSMethod(uint32 object_id, R& return_value, uint32 ns_hash, uint32 method_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6)
{
    return (ExecFunctionImpl<R>(return_value, object_id, ns_hash, method_hash, p1, p2, p3, p4, p5, p6));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
inline bool8 ObjExecMethod(uint32 object_id, R& return_value, const char* method_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, object_id, 0, TinScript::Hash(method_name), p1, p2, p3, p4, p5, p6));
}



// -- Parameter count: 7
template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
inline bool8 ExecFunctionImpl(R& return_value, uint32 object_id, uint32 ns_hash, uint32 func_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    // -- get the object, if one was required
    CObjectEntry* oe = object_id > 0 ? script_context->FindObjectEntry(object_id) : NULL;
    if (!oe && object_id > 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object %d not found\n", object_id);
        return false;
    }

    TinScript::CFunctionEntry* fe = oe ? oe->GetFunctionEntry(ns_hash, func_hash)
                            : script_context->GetGlobalNamespace()->GetFuncTable()->FindItem(func_hash);
    TinScript::CVariableEntry* return_ve = fe ? fe->GetContext()->GetParameter(0) : NULL;
    if (!fe || !return_ve)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() not found\n", UnHash(func_hash));
        return false;
    }

    // -- see if we can recognize an appropriate type
    eVarType returntype = TinScript::GetRegisteredType(TinScript::GetTypeID<R>());
    if (returntype == TYPE_NULL)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - invalid return type (use an int32 if void)\n");
        return false;
    }

    TinScript::CVariableEntry* ve_p1 = fe->GetContext()->GetParameter(1);
    if (ve_p1 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p1_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p1_ptr_ptr = (void**)(&p1);
        void* p1_ptr = (void*)(*p1_ptr_ptr);
        p1_convert_addr = TypeConvert(script_context, TYPE_string, p1_ptr, ve_p1->GetType());
    }
    else
        p1_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), (void*)&p1, ve_p1->GetType());
    if (!p1_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 1\n", UnHash(func_hash));
        return false;
    }

    ve_p1->SetValueAddr(NULL, p1_convert_addr);

    TinScript::CVariableEntry* ve_p2 = fe->GetContext()->GetParameter(2);
    if (ve_p2 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p2_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p2_ptr_ptr = (void**)(&p2);
        void* p2_ptr = (void*)(*p2_ptr_ptr);
        p2_convert_addr = TypeConvert(script_context, TYPE_string, p2_ptr, ve_p2->GetType());
    }
    else
        p2_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), (void*)&p2, ve_p2->GetType());
    if (!p2_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 2\n", UnHash(func_hash));
        return false;
    }

    ve_p2->SetValueAddr(NULL, p2_convert_addr);

    TinScript::CVariableEntry* ve_p3 = fe->GetContext()->GetParameter(3);
    if (ve_p3 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p3_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p3_ptr_ptr = (void**)(&p3);
        void* p3_ptr = (void*)(*p3_ptr_ptr);
        p3_convert_addr = TypeConvert(script_context, TYPE_string, p3_ptr, ve_p3->GetType());
    }
    else
        p3_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), (void*)&p3, ve_p3->GetType());
    if (!p3_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 3\n", UnHash(func_hash));
        return false;
    }

    ve_p3->SetValueAddr(NULL, p3_convert_addr);

    TinScript::CVariableEntry* ve_p4 = fe->GetContext()->GetParameter(4);
    if (ve_p4 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p4_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p4_ptr_ptr = (void**)(&p4);
        void* p4_ptr = (void*)(*p4_ptr_ptr);
        p4_convert_addr = TypeConvert(script_context, TYPE_string, p4_ptr, ve_p4->GetType());
    }
    else
        p4_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), (void*)&p4, ve_p4->GetType());
    if (!p4_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 4\n", UnHash(func_hash));
        return false;
    }

    ve_p4->SetValueAddr(NULL, p4_convert_addr);

    TinScript::CVariableEntry* ve_p5 = fe->GetContext()->GetParameter(5);
    if (ve_p5 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p5_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p5_ptr_ptr = (void**)(&p5);
        void* p5_ptr = (void*)(*p5_ptr_ptr);
        p5_convert_addr = TypeConvert(script_context, TYPE_string, p5_ptr, ve_p5->GetType());
    }
    else
        p5_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), (void*)&p5, ve_p5->GetType());
    if (!p5_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 5\n", UnHash(func_hash));
        return false;
    }

    ve_p5->SetValueAddr(NULL, p5_convert_addr);

    TinScript::CVariableEntry* ve_p6 = fe->GetContext()->GetParameter(6);
    if (ve_p6 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p6_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p6_ptr_ptr = (void**)(&p6);
        void* p6_ptr = (void*)(*p6_ptr_ptr);
        p6_convert_addr = TypeConvert(script_context, TYPE_string, p6_ptr, ve_p6->GetType());
    }
    else
        p6_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()), (void*)&p6, ve_p6->GetType());
    if (!p6_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 6\n", UnHash(func_hash));
        return false;
    }

    ve_p6->SetValueAddr(NULL, p6_convert_addr);

    TinScript::CVariableEntry* ve_p7 = fe->GetContext()->GetParameter(7);
    if (ve_p7 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p7_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T7>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p7_ptr_ptr = (void**)(&p7);
        void* p7_ptr = (void*)(*p7_ptr_ptr);
        p7_convert_addr = TypeConvert(script_context, TYPE_string, p7_ptr, ve_p7->GetType());
    }
    else
        p7_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T7>()), (void*)&p7, ve_p7->GetType());
    if (!p7_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 7\n", UnHash(func_hash));
        return false;
    }

    ve_p7->SetValueAddr(NULL, p7_convert_addr);

    // -- execute the function
    if (!ExecuteScheduledFunction(GetContext(), object_id, ns_hash, func_hash, fe->GetContext()))
    {
        TinPrint(script_context, "Error - unable to exec function %s()\n", UnHash(func_hash));
        return false;
    }

    // -- return true if we're able to convert to the return type requested
    return (ReturnExecfResult(script_context, return_value));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
inline bool8 ExecFunction(R& return_value, const char* func_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !func_name || !func_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, TinScript::Hash(func_name), p1, p2, p3, p4, p5, p6, p7));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
inline bool8 ExecFunction(R& return_value, uint32 func_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, func_hash, p1, p2, p3, p4, p5, p6, p7));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, const char* method_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, TinScript::Hash(method_name), p1, p2, p3, p4, p5, p6, p7));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, uint32 method_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, method_hash, p1, p2, p3, p4, p5, p6, p7));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
inline bool8 ObjExecNSMethod(uint32 object_id, R& return_value, uint32 ns_hash, uint32 method_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7)
{
    return (ExecFunctionImpl<R>(return_value, object_id, ns_hash, method_hash, p1, p2, p3, p4, p5, p6, p7));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
inline bool8 ObjExecMethod(uint32 object_id, R& return_value, const char* method_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, object_id, 0, TinScript::Hash(method_name), p1, p2, p3, p4, p5, p6, p7));
}



// -- Parameter count: 8
template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
inline bool8 ExecFunctionImpl(R& return_value, uint32 object_id, uint32 ns_hash, uint32 func_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    // -- get the object, if one was required
    CObjectEntry* oe = object_id > 0 ? script_context->FindObjectEntry(object_id) : NULL;
    if (!oe && object_id > 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object %d not found\n", object_id);
        return false;
    }

    TinScript::CFunctionEntry* fe = oe ? oe->GetFunctionEntry(ns_hash, func_hash)
                            : script_context->GetGlobalNamespace()->GetFuncTable()->FindItem(func_hash);
    TinScript::CVariableEntry* return_ve = fe ? fe->GetContext()->GetParameter(0) : NULL;
    if (!fe || !return_ve)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() not found\n", UnHash(func_hash));
        return false;
    }

    // -- see if we can recognize an appropriate type
    eVarType returntype = TinScript::GetRegisteredType(TinScript::GetTypeID<R>());
    if (returntype == TYPE_NULL)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - invalid return type (use an int32 if void)\n");
        return false;
    }

    TinScript::CVariableEntry* ve_p1 = fe->GetContext()->GetParameter(1);
    if (ve_p1 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p1_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p1_ptr_ptr = (void**)(&p1);
        void* p1_ptr = (void*)(*p1_ptr_ptr);
        p1_convert_addr = TypeConvert(script_context, TYPE_string, p1_ptr, ve_p1->GetType());
    }
    else
        p1_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), (void*)&p1, ve_p1->GetType());
    if (!p1_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 1\n", UnHash(func_hash));
        return false;
    }

    ve_p1->SetValueAddr(NULL, p1_convert_addr);

    TinScript::CVariableEntry* ve_p2 = fe->GetContext()->GetParameter(2);
    if (ve_p2 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p2_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p2_ptr_ptr = (void**)(&p2);
        void* p2_ptr = (void*)(*p2_ptr_ptr);
        p2_convert_addr = TypeConvert(script_context, TYPE_string, p2_ptr, ve_p2->GetType());
    }
    else
        p2_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), (void*)&p2, ve_p2->GetType());
    if (!p2_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 2\n", UnHash(func_hash));
        return false;
    }

    ve_p2->SetValueAddr(NULL, p2_convert_addr);

    TinScript::CVariableEntry* ve_p3 = fe->GetContext()->GetParameter(3);
    if (ve_p3 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p3_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p3_ptr_ptr = (void**)(&p3);
        void* p3_ptr = (void*)(*p3_ptr_ptr);
        p3_convert_addr = TypeConvert(script_context, TYPE_string, p3_ptr, ve_p3->GetType());
    }
    else
        p3_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), (void*)&p3, ve_p3->GetType());
    if (!p3_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 3\n", UnHash(func_hash));
        return false;
    }

    ve_p3->SetValueAddr(NULL, p3_convert_addr);

    TinScript::CVariableEntry* ve_p4 = fe->GetContext()->GetParameter(4);
    if (ve_p4 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p4_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p4_ptr_ptr = (void**)(&p4);
        void* p4_ptr = (void*)(*p4_ptr_ptr);
        p4_convert_addr = TypeConvert(script_context, TYPE_string, p4_ptr, ve_p4->GetType());
    }
    else
        p4_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), (void*)&p4, ve_p4->GetType());
    if (!p4_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 4\n", UnHash(func_hash));
        return false;
    }

    ve_p4->SetValueAddr(NULL, p4_convert_addr);

    TinScript::CVariableEntry* ve_p5 = fe->GetContext()->GetParameter(5);
    if (ve_p5 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p5_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p5_ptr_ptr = (void**)(&p5);
        void* p5_ptr = (void*)(*p5_ptr_ptr);
        p5_convert_addr = TypeConvert(script_context, TYPE_string, p5_ptr, ve_p5->GetType());
    }
    else
        p5_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), (void*)&p5, ve_p5->GetType());
    if (!p5_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 5\n", UnHash(func_hash));
        return false;
    }

    ve_p5->SetValueAddr(NULL, p5_convert_addr);

    TinScript::CVariableEntry* ve_p6 = fe->GetContext()->GetParameter(6);
    if (ve_p6 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p6_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p6_ptr_ptr = (void**)(&p6);
        void* p6_ptr = (void*)(*p6_ptr_ptr);
        p6_convert_addr = TypeConvert(script_context, TYPE_string, p6_ptr, ve_p6->GetType());
    }
    else
        p6_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()), (void*)&p6, ve_p6->GetType());
    if (!p6_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 6\n", UnHash(func_hash));
        return false;
    }

    ve_p6->SetValueAddr(NULL, p6_convert_addr);

    TinScript::CVariableEntry* ve_p7 = fe->GetContext()->GetParameter(7);
    if (ve_p7 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p7_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T7>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p7_ptr_ptr = (void**)(&p7);
        void* p7_ptr = (void*)(*p7_ptr_ptr);
        p7_convert_addr = TypeConvert(script_context, TYPE_string, p7_ptr, ve_p7->GetType());
    }
    else
        p7_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T7>()), (void*)&p7, ve_p7->GetType());
    if (!p7_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 7\n", UnHash(func_hash));
        return false;
    }

    ve_p7->SetValueAddr(NULL, p7_convert_addr);

    TinScript::CVariableEntry* ve_p8 = fe->GetContext()->GetParameter(8);
    if (ve_p8 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p8_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T8>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p8_ptr_ptr = (void**)(&p8);
        void* p8_ptr = (void*)(*p8_ptr_ptr);
        p8_convert_addr = TypeConvert(script_context, TYPE_string, p8_ptr, ve_p8->GetType());
    }
    else
        p8_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T8>()), (void*)&p8, ve_p8->GetType());
    if (!p8_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 8\n", UnHash(func_hash));
        return false;
    }

    ve_p8->SetValueAddr(NULL, p8_convert_addr);

    // -- execute the function
    if (!ExecuteScheduledFunction(GetContext(), object_id, ns_hash, func_hash, fe->GetContext()))
    {
        TinPrint(script_context, "Error - unable to exec function %s()\n", UnHash(func_hash));
        return false;
    }

    // -- return true if we're able to convert to the return type requested
    return (ReturnExecfResult(script_context, return_value));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
inline bool8 ExecFunction(R& return_value, const char* func_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !func_name || !func_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, TinScript::Hash(func_name), p1, p2, p3, p4, p5, p6, p7, p8));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
inline bool8 ExecFunction(R& return_value, uint32 func_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, func_hash, p1, p2, p3, p4, p5, p6, p7, p8));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, const char* method_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, TinScript::Hash(method_name), p1, p2, p3, p4, p5, p6, p7, p8));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, uint32 method_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, method_hash, p1, p2, p3, p4, p5, p6, p7, p8));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
inline bool8 ObjExecNSMethod(uint32 object_id, R& return_value, uint32 ns_hash, uint32 method_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8)
{
    return (ExecFunctionImpl<R>(return_value, object_id, ns_hash, method_hash, p1, p2, p3, p4, p5, p6, p7, p8));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
inline bool8 ObjExecMethod(uint32 object_id, R& return_value, const char* method_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, object_id, 0, TinScript::Hash(method_name), p1, p2, p3, p4, p5, p6, p7, p8));
}



// -- Parameter count: 9
template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
inline bool8 ExecFunctionImpl(R& return_value, uint32 object_id, uint32 ns_hash, uint32 func_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    // -- get the object, if one was required
    CObjectEntry* oe = object_id > 0 ? script_context->FindObjectEntry(object_id) : NULL;
    if (!oe && object_id > 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object %d not found\n", object_id);
        return false;
    }

    TinScript::CFunctionEntry* fe = oe ? oe->GetFunctionEntry(ns_hash, func_hash)
                            : script_context->GetGlobalNamespace()->GetFuncTable()->FindItem(func_hash);
    TinScript::CVariableEntry* return_ve = fe ? fe->GetContext()->GetParameter(0) : NULL;
    if (!fe || !return_ve)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() not found\n", UnHash(func_hash));
        return false;
    }

    // -- see if we can recognize an appropriate type
    eVarType returntype = TinScript::GetRegisteredType(TinScript::GetTypeID<R>());
    if (returntype == TYPE_NULL)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - invalid return type (use an int32 if void)\n");
        return false;
    }

    TinScript::CVariableEntry* ve_p1 = fe->GetContext()->GetParameter(1);
    if (ve_p1 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p1_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p1_ptr_ptr = (void**)(&p1);
        void* p1_ptr = (void*)(*p1_ptr_ptr);
        p1_convert_addr = TypeConvert(script_context, TYPE_string, p1_ptr, ve_p1->GetType());
    }
    else
        p1_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), (void*)&p1, ve_p1->GetType());
    if (!p1_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 1\n", UnHash(func_hash));
        return false;
    }

    ve_p1->SetValueAddr(NULL, p1_convert_addr);

    TinScript::CVariableEntry* ve_p2 = fe->GetContext()->GetParameter(2);
    if (ve_p2 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p2_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p2_ptr_ptr = (void**)(&p2);
        void* p2_ptr = (void*)(*p2_ptr_ptr);
        p2_convert_addr = TypeConvert(script_context, TYPE_string, p2_ptr, ve_p2->GetType());
    }
    else
        p2_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), (void*)&p2, ve_p2->GetType());
    if (!p2_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 2\n", UnHash(func_hash));
        return false;
    }

    ve_p2->SetValueAddr(NULL, p2_convert_addr);

    TinScript::CVariableEntry* ve_p3 = fe->GetContext()->GetParameter(3);
    if (ve_p3 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p3_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p3_ptr_ptr = (void**)(&p3);
        void* p3_ptr = (void*)(*p3_ptr_ptr);
        p3_convert_addr = TypeConvert(script_context, TYPE_string, p3_ptr, ve_p3->GetType());
    }
    else
        p3_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), (void*)&p3, ve_p3->GetType());
    if (!p3_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 3\n", UnHash(func_hash));
        return false;
    }

    ve_p3->SetValueAddr(NULL, p3_convert_addr);

    TinScript::CVariableEntry* ve_p4 = fe->GetContext()->GetParameter(4);
    if (ve_p4 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p4_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p4_ptr_ptr = (void**)(&p4);
        void* p4_ptr = (void*)(*p4_ptr_ptr);
        p4_convert_addr = TypeConvert(script_context, TYPE_string, p4_ptr, ve_p4->GetType());
    }
    else
        p4_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), (void*)&p4, ve_p4->GetType());
    if (!p4_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 4\n", UnHash(func_hash));
        return false;
    }

    ve_p4->SetValueAddr(NULL, p4_convert_addr);

    TinScript::CVariableEntry* ve_p5 = fe->GetContext()->GetParameter(5);
    if (ve_p5 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p5_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p5_ptr_ptr = (void**)(&p5);
        void* p5_ptr = (void*)(*p5_ptr_ptr);
        p5_convert_addr = TypeConvert(script_context, TYPE_string, p5_ptr, ve_p5->GetType());
    }
    else
        p5_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), (void*)&p5, ve_p5->GetType());
    if (!p5_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 5\n", UnHash(func_hash));
        return false;
    }

    ve_p5->SetValueAddr(NULL, p5_convert_addr);

    TinScript::CVariableEntry* ve_p6 = fe->GetContext()->GetParameter(6);
    if (ve_p6 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p6_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p6_ptr_ptr = (void**)(&p6);
        void* p6_ptr = (void*)(*p6_ptr_ptr);
        p6_convert_addr = TypeConvert(script_context, TYPE_string, p6_ptr, ve_p6->GetType());
    }
    else
        p6_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()), (void*)&p6, ve_p6->GetType());
    if (!p6_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 6\n", UnHash(func_hash));
        return false;
    }

    ve_p6->SetValueAddr(NULL, p6_convert_addr);

    TinScript::CVariableEntry* ve_p7 = fe->GetContext()->GetParameter(7);
    if (ve_p7 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p7_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T7>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p7_ptr_ptr = (void**)(&p7);
        void* p7_ptr = (void*)(*p7_ptr_ptr);
        p7_convert_addr = TypeConvert(script_context, TYPE_string, p7_ptr, ve_p7->GetType());
    }
    else
        p7_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T7>()), (void*)&p7, ve_p7->GetType());
    if (!p7_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 7\n", UnHash(func_hash));
        return false;
    }

    ve_p7->SetValueAddr(NULL, p7_convert_addr);

    TinScript::CVariableEntry* ve_p8 = fe->GetContext()->GetParameter(8);
    if (ve_p8 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p8_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T8>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p8_ptr_ptr = (void**)(&p8);
        void* p8_ptr = (void*)(*p8_ptr_ptr);
        p8_convert_addr = TypeConvert(script_context, TYPE_string, p8_ptr, ve_p8->GetType());
    }
    else
        p8_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T8>()), (void*)&p8, ve_p8->GetType());
    if (!p8_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 8\n", UnHash(func_hash));
        return false;
    }

    ve_p8->SetValueAddr(NULL, p8_convert_addr);

    TinScript::CVariableEntry* ve_p9 = fe->GetContext()->GetParameter(9);
    if (ve_p9 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p9_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T9>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p9_ptr_ptr = (void**)(&p9);
        void* p9_ptr = (void*)(*p9_ptr_ptr);
        p9_convert_addr = TypeConvert(script_context, TYPE_string, p9_ptr, ve_p9->GetType());
    }
    else
        p9_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T9>()), (void*)&p9, ve_p9->GetType());
    if (!p9_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 9\n", UnHash(func_hash));
        return false;
    }

    ve_p9->SetValueAddr(NULL, p9_convert_addr);

    // -- execute the function
    if (!ExecuteScheduledFunction(GetContext(), object_id, ns_hash, func_hash, fe->GetContext()))
    {
        TinPrint(script_context, "Error - unable to exec function %s()\n", UnHash(func_hash));
        return false;
    }

    // -- return true if we're able to convert to the return type requested
    return (ReturnExecfResult(script_context, return_value));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
inline bool8 ExecFunction(R& return_value, const char* func_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !func_name || !func_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, TinScript::Hash(func_name), p1, p2, p3, p4, p5, p6, p7, p8, p9));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
inline bool8 ExecFunction(R& return_value, uint32 func_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, func_hash, p1, p2, p3, p4, p5, p6, p7, p8, p9));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, const char* method_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, TinScript::Hash(method_name), p1, p2, p3, p4, p5, p6, p7, p8, p9));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, uint32 method_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, method_hash, p1, p2, p3, p4, p5, p6, p7, p8, p9));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
inline bool8 ObjExecNSMethod(uint32 object_id, R& return_value, uint32 ns_hash, uint32 method_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9)
{
    return (ExecFunctionImpl<R>(return_value, object_id, ns_hash, method_hash, p1, p2, p3, p4, p5, p6, p7, p8, p9));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
inline bool8 ObjExecMethod(uint32 object_id, R& return_value, const char* method_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, object_id, 0, TinScript::Hash(method_name), p1, p2, p3, p4, p5, p6, p7, p8, p9));
}



// -- Parameter count: 10
template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10>
inline bool8 ExecFunctionImpl(R& return_value, uint32 object_id, uint32 ns_hash, uint32 func_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    // -- get the object, if one was required
    CObjectEntry* oe = object_id > 0 ? script_context->FindObjectEntry(object_id) : NULL;
    if (!oe && object_id > 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object %d not found\n", object_id);
        return false;
    }

    TinScript::CFunctionEntry* fe = oe ? oe->GetFunctionEntry(ns_hash, func_hash)
                            : script_context->GetGlobalNamespace()->GetFuncTable()->FindItem(func_hash);
    TinScript::CVariableEntry* return_ve = fe ? fe->GetContext()->GetParameter(0) : NULL;
    if (!fe || !return_ve)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() not found\n", UnHash(func_hash));
        return false;
    }

    // -- see if we can recognize an appropriate type
    eVarType returntype = TinScript::GetRegisteredType(TinScript::GetTypeID<R>());
    if (returntype == TYPE_NULL)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - invalid return type (use an int32 if void)\n");
        return false;
    }

    TinScript::CVariableEntry* ve_p1 = fe->GetContext()->GetParameter(1);
    if (ve_p1 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p1_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p1_ptr_ptr = (void**)(&p1);
        void* p1_ptr = (void*)(*p1_ptr_ptr);
        p1_convert_addr = TypeConvert(script_context, TYPE_string, p1_ptr, ve_p1->GetType());
    }
    else
        p1_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), (void*)&p1, ve_p1->GetType());
    if (!p1_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 1\n", UnHash(func_hash));
        return false;
    }

    ve_p1->SetValueAddr(NULL, p1_convert_addr);

    TinScript::CVariableEntry* ve_p2 = fe->GetContext()->GetParameter(2);
    if (ve_p2 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p2_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p2_ptr_ptr = (void**)(&p2);
        void* p2_ptr = (void*)(*p2_ptr_ptr);
        p2_convert_addr = TypeConvert(script_context, TYPE_string, p2_ptr, ve_p2->GetType());
    }
    else
        p2_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), (void*)&p2, ve_p2->GetType());
    if (!p2_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 2\n", UnHash(func_hash));
        return false;
    }

    ve_p2->SetValueAddr(NULL, p2_convert_addr);

    TinScript::CVariableEntry* ve_p3 = fe->GetContext()->GetParameter(3);
    if (ve_p3 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p3_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p3_ptr_ptr = (void**)(&p3);
        void* p3_ptr = (void*)(*p3_ptr_ptr);
        p3_convert_addr = TypeConvert(script_context, TYPE_string, p3_ptr, ve_p3->GetType());
    }
    else
        p3_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), (void*)&p3, ve_p3->GetType());
    if (!p3_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 3\n", UnHash(func_hash));
        return false;
    }

    ve_p3->SetValueAddr(NULL, p3_convert_addr);

    TinScript::CVariableEntry* ve_p4 = fe->GetContext()->GetParameter(4);
    if (ve_p4 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p4_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p4_ptr_ptr = (void**)(&p4);
        void* p4_ptr = (void*)(*p4_ptr_ptr);
        p4_convert_addr = TypeConvert(script_context, TYPE_string, p4_ptr, ve_p4->GetType());
    }
    else
        p4_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), (void*)&p4, ve_p4->GetType());
    if (!p4_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 4\n", UnHash(func_hash));
        return false;
    }

    ve_p4->SetValueAddr(NULL, p4_convert_addr);

    TinScript::CVariableEntry* ve_p5 = fe->GetContext()->GetParameter(5);
    if (ve_p5 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p5_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p5_ptr_ptr = (void**)(&p5);
        void* p5_ptr = (void*)(*p5_ptr_ptr);
        p5_convert_addr = TypeConvert(script_context, TYPE_string, p5_ptr, ve_p5->GetType());
    }
    else
        p5_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), (void*)&p5, ve_p5->GetType());
    if (!p5_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 5\n", UnHash(func_hash));
        return false;
    }

    ve_p5->SetValueAddr(NULL, p5_convert_addr);

    TinScript::CVariableEntry* ve_p6 = fe->GetContext()->GetParameter(6);
    if (ve_p6 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p6_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p6_ptr_ptr = (void**)(&p6);
        void* p6_ptr = (void*)(*p6_ptr_ptr);
        p6_convert_addr = TypeConvert(script_context, TYPE_string, p6_ptr, ve_p6->GetType());
    }
    else
        p6_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()), (void*)&p6, ve_p6->GetType());
    if (!p6_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 6\n", UnHash(func_hash));
        return false;
    }

    ve_p6->SetValueAddr(NULL, p6_convert_addr);

    TinScript::CVariableEntry* ve_p7 = fe->GetContext()->GetParameter(7);
    if (ve_p7 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p7_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T7>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p7_ptr_ptr = (void**)(&p7);
        void* p7_ptr = (void*)(*p7_ptr_ptr);
        p7_convert_addr = TypeConvert(script_context, TYPE_string, p7_ptr, ve_p7->GetType());
    }
    else
        p7_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T7>()), (void*)&p7, ve_p7->GetType());
    if (!p7_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 7\n", UnHash(func_hash));
        return false;
    }

    ve_p7->SetValueAddr(NULL, p7_convert_addr);

    TinScript::CVariableEntry* ve_p8 = fe->GetContext()->GetParameter(8);
    if (ve_p8 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p8_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T8>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p8_ptr_ptr = (void**)(&p8);
        void* p8_ptr = (void*)(*p8_ptr_ptr);
        p8_convert_addr = TypeConvert(script_context, TYPE_string, p8_ptr, ve_p8->GetType());
    }
    else
        p8_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T8>()), (void*)&p8, ve_p8->GetType());
    if (!p8_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 8\n", UnHash(func_hash));
        return false;
    }

    ve_p8->SetValueAddr(NULL, p8_convert_addr);

    TinScript::CVariableEntry* ve_p9 = fe->GetContext()->GetParameter(9);
    if (ve_p9 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p9_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T9>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p9_ptr_ptr = (void**)(&p9);
        void* p9_ptr = (void*)(*p9_ptr_ptr);
        p9_convert_addr = TypeConvert(script_context, TYPE_string, p9_ptr, ve_p9->GetType());
    }
    else
        p9_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T9>()), (void*)&p9, ve_p9->GetType());
    if (!p9_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 9\n", UnHash(func_hash));
        return false;
    }

    ve_p9->SetValueAddr(NULL, p9_convert_addr);

    TinScript::CVariableEntry* ve_p10 = fe->GetContext()->GetParameter(10);
    if (ve_p10 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p10_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T10>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p10_ptr_ptr = (void**)(&p10);
        void* p10_ptr = (void*)(*p10_ptr_ptr);
        p10_convert_addr = TypeConvert(script_context, TYPE_string, p10_ptr, ve_p10->GetType());
    }
    else
        p10_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T10>()), (void*)&p10, ve_p10->GetType());
    if (!p10_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 10\n", UnHash(func_hash));
        return false;
    }

    ve_p10->SetValueAddr(NULL, p10_convert_addr);

    // -- execute the function
    if (!ExecuteScheduledFunction(GetContext(), object_id, ns_hash, func_hash, fe->GetContext()))
    {
        TinPrint(script_context, "Error - unable to exec function %s()\n", UnHash(func_hash));
        return false;
    }

    // -- return true if we're able to convert to the return type requested
    return (ReturnExecfResult(script_context, return_value));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10>
inline bool8 ExecFunction(R& return_value, const char* func_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !func_name || !func_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, TinScript::Hash(func_name), p1, p2, p3, p4, p5, p6, p7, p8, p9, p10));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10>
inline bool8 ExecFunction(R& return_value, uint32 func_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, func_hash, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, const char* method_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, TinScript::Hash(method_name), p1, p2, p3, p4, p5, p6, p7, p8, p9, p10));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, uint32 method_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, method_hash, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10>
inline bool8 ObjExecNSMethod(uint32 object_id, R& return_value, uint32 ns_hash, uint32 method_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10)
{
    return (ExecFunctionImpl<R>(return_value, object_id, ns_hash, method_hash, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10>
inline bool8 ObjExecMethod(uint32 object_id, R& return_value, const char* method_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, object_id, 0, TinScript::Hash(method_name), p1, p2, p3, p4, p5, p6, p7, p8, p9, p10));
}



// -- Parameter count: 11
template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11>
inline bool8 ExecFunctionImpl(R& return_value, uint32 object_id, uint32 ns_hash, uint32 func_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    // -- get the object, if one was required
    CObjectEntry* oe = object_id > 0 ? script_context->FindObjectEntry(object_id) : NULL;
    if (!oe && object_id > 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object %d not found\n", object_id);
        return false;
    }

    TinScript::CFunctionEntry* fe = oe ? oe->GetFunctionEntry(ns_hash, func_hash)
                            : script_context->GetGlobalNamespace()->GetFuncTable()->FindItem(func_hash);
    TinScript::CVariableEntry* return_ve = fe ? fe->GetContext()->GetParameter(0) : NULL;
    if (!fe || !return_ve)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() not found\n", UnHash(func_hash));
        return false;
    }

    // -- see if we can recognize an appropriate type
    eVarType returntype = TinScript::GetRegisteredType(TinScript::GetTypeID<R>());
    if (returntype == TYPE_NULL)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - invalid return type (use an int32 if void)\n");
        return false;
    }

    TinScript::CVariableEntry* ve_p1 = fe->GetContext()->GetParameter(1);
    if (ve_p1 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p1_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p1_ptr_ptr = (void**)(&p1);
        void* p1_ptr = (void*)(*p1_ptr_ptr);
        p1_convert_addr = TypeConvert(script_context, TYPE_string, p1_ptr, ve_p1->GetType());
    }
    else
        p1_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), (void*)&p1, ve_p1->GetType());
    if (!p1_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 1\n", UnHash(func_hash));
        return false;
    }

    ve_p1->SetValueAddr(NULL, p1_convert_addr);

    TinScript::CVariableEntry* ve_p2 = fe->GetContext()->GetParameter(2);
    if (ve_p2 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p2_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p2_ptr_ptr = (void**)(&p2);
        void* p2_ptr = (void*)(*p2_ptr_ptr);
        p2_convert_addr = TypeConvert(script_context, TYPE_string, p2_ptr, ve_p2->GetType());
    }
    else
        p2_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), (void*)&p2, ve_p2->GetType());
    if (!p2_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 2\n", UnHash(func_hash));
        return false;
    }

    ve_p2->SetValueAddr(NULL, p2_convert_addr);

    TinScript::CVariableEntry* ve_p3 = fe->GetContext()->GetParameter(3);
    if (ve_p3 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p3_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p3_ptr_ptr = (void**)(&p3);
        void* p3_ptr = (void*)(*p3_ptr_ptr);
        p3_convert_addr = TypeConvert(script_context, TYPE_string, p3_ptr, ve_p3->GetType());
    }
    else
        p3_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), (void*)&p3, ve_p3->GetType());
    if (!p3_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 3\n", UnHash(func_hash));
        return false;
    }

    ve_p3->SetValueAddr(NULL, p3_convert_addr);

    TinScript::CVariableEntry* ve_p4 = fe->GetContext()->GetParameter(4);
    if (ve_p4 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p4_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p4_ptr_ptr = (void**)(&p4);
        void* p4_ptr = (void*)(*p4_ptr_ptr);
        p4_convert_addr = TypeConvert(script_context, TYPE_string, p4_ptr, ve_p4->GetType());
    }
    else
        p4_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), (void*)&p4, ve_p4->GetType());
    if (!p4_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 4\n", UnHash(func_hash));
        return false;
    }

    ve_p4->SetValueAddr(NULL, p4_convert_addr);

    TinScript::CVariableEntry* ve_p5 = fe->GetContext()->GetParameter(5);
    if (ve_p5 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p5_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p5_ptr_ptr = (void**)(&p5);
        void* p5_ptr = (void*)(*p5_ptr_ptr);
        p5_convert_addr = TypeConvert(script_context, TYPE_string, p5_ptr, ve_p5->GetType());
    }
    else
        p5_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), (void*)&p5, ve_p5->GetType());
    if (!p5_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 5\n", UnHash(func_hash));
        return false;
    }

    ve_p5->SetValueAddr(NULL, p5_convert_addr);

    TinScript::CVariableEntry* ve_p6 = fe->GetContext()->GetParameter(6);
    if (ve_p6 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p6_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p6_ptr_ptr = (void**)(&p6);
        void* p6_ptr = (void*)(*p6_ptr_ptr);
        p6_convert_addr = TypeConvert(script_context, TYPE_string, p6_ptr, ve_p6->GetType());
    }
    else
        p6_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()), (void*)&p6, ve_p6->GetType());
    if (!p6_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 6\n", UnHash(func_hash));
        return false;
    }

    ve_p6->SetValueAddr(NULL, p6_convert_addr);

    TinScript::CVariableEntry* ve_p7 = fe->GetContext()->GetParameter(7);
    if (ve_p7 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p7_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T7>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p7_ptr_ptr = (void**)(&p7);
        void* p7_ptr = (void*)(*p7_ptr_ptr);
        p7_convert_addr = TypeConvert(script_context, TYPE_string, p7_ptr, ve_p7->GetType());
    }
    else
        p7_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T7>()), (void*)&p7, ve_p7->GetType());
    if (!p7_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 7\n", UnHash(func_hash));
        return false;
    }

    ve_p7->SetValueAddr(NULL, p7_convert_addr);

    TinScript::CVariableEntry* ve_p8 = fe->GetContext()->GetParameter(8);
    if (ve_p8 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p8_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T8>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p8_ptr_ptr = (void**)(&p8);
        void* p8_ptr = (void*)(*p8_ptr_ptr);
        p8_convert_addr = TypeConvert(script_context, TYPE_string, p8_ptr, ve_p8->GetType());
    }
    else
        p8_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T8>()), (void*)&p8, ve_p8->GetType());
    if (!p8_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 8\n", UnHash(func_hash));
        return false;
    }

    ve_p8->SetValueAddr(NULL, p8_convert_addr);

    TinScript::CVariableEntry* ve_p9 = fe->GetContext()->GetParameter(9);
    if (ve_p9 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p9_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T9>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p9_ptr_ptr = (void**)(&p9);
        void* p9_ptr = (void*)(*p9_ptr_ptr);
        p9_convert_addr = TypeConvert(script_context, TYPE_string, p9_ptr, ve_p9->GetType());
    }
    else
        p9_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T9>()), (void*)&p9, ve_p9->GetType());
    if (!p9_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 9\n", UnHash(func_hash));
        return false;
    }

    ve_p9->SetValueAddr(NULL, p9_convert_addr);

    TinScript::CVariableEntry* ve_p10 = fe->GetContext()->GetParameter(10);
    if (ve_p10 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p10_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T10>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p10_ptr_ptr = (void**)(&p10);
        void* p10_ptr = (void*)(*p10_ptr_ptr);
        p10_convert_addr = TypeConvert(script_context, TYPE_string, p10_ptr, ve_p10->GetType());
    }
    else
        p10_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T10>()), (void*)&p10, ve_p10->GetType());
    if (!p10_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 10\n", UnHash(func_hash));
        return false;
    }

    ve_p10->SetValueAddr(NULL, p10_convert_addr);

    TinScript::CVariableEntry* ve_p11 = fe->GetContext()->GetParameter(11);
    if (ve_p11 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p11_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T11>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p11_ptr_ptr = (void**)(&p11);
        void* p11_ptr = (void*)(*p11_ptr_ptr);
        p11_convert_addr = TypeConvert(script_context, TYPE_string, p11_ptr, ve_p11->GetType());
    }
    else
        p11_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T11>()), (void*)&p11, ve_p11->GetType());
    if (!p11_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 11\n", UnHash(func_hash));
        return false;
    }

    ve_p11->SetValueAddr(NULL, p11_convert_addr);

    // -- execute the function
    if (!ExecuteScheduledFunction(GetContext(), object_id, ns_hash, func_hash, fe->GetContext()))
    {
        TinPrint(script_context, "Error - unable to exec function %s()\n", UnHash(func_hash));
        return false;
    }

    // -- return true if we're able to convert to the return type requested
    return (ReturnExecfResult(script_context, return_value));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11>
inline bool8 ExecFunction(R& return_value, const char* func_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !func_name || !func_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, TinScript::Hash(func_name), p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11>
inline bool8 ExecFunction(R& return_value, uint32 func_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, func_hash, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, const char* method_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, TinScript::Hash(method_name), p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, uint32 method_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, method_hash, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11>
inline bool8 ObjExecNSMethod(uint32 object_id, R& return_value, uint32 ns_hash, uint32 method_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11)
{
    return (ExecFunctionImpl<R>(return_value, object_id, ns_hash, method_hash, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11>
inline bool8 ObjExecMethod(uint32 object_id, R& return_value, const char* method_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, object_id, 0, TinScript::Hash(method_name), p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11));
}



// -- Parameter count: 12
template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12>
inline bool8 ExecFunctionImpl(R& return_value, uint32 object_id, uint32 ns_hash, uint32 func_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11, T12 p12)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    // -- get the object, if one was required
    CObjectEntry* oe = object_id > 0 ? script_context->FindObjectEntry(object_id) : NULL;
    if (!oe && object_id > 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object %d not found\n", object_id);
        return false;
    }

    TinScript::CFunctionEntry* fe = oe ? oe->GetFunctionEntry(ns_hash, func_hash)
                            : script_context->GetGlobalNamespace()->GetFuncTable()->FindItem(func_hash);
    TinScript::CVariableEntry* return_ve = fe ? fe->GetContext()->GetParameter(0) : NULL;
    if (!fe || !return_ve)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() not found\n", UnHash(func_hash));
        return false;
    }

    // -- see if we can recognize an appropriate type
    eVarType returntype = TinScript::GetRegisteredType(TinScript::GetTypeID<R>());
    if (returntype == TYPE_NULL)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - invalid return type (use an int32 if void)\n");
        return false;
    }

    TinScript::CVariableEntry* ve_p1 = fe->GetContext()->GetParameter(1);
    if (ve_p1 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p1_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p1_ptr_ptr = (void**)(&p1);
        void* p1_ptr = (void*)(*p1_ptr_ptr);
        p1_convert_addr = TypeConvert(script_context, TYPE_string, p1_ptr, ve_p1->GetType());
    }
    else
        p1_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T1>()), (void*)&p1, ve_p1->GetType());
    if (!p1_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 1\n", UnHash(func_hash));
        return false;
    }

    ve_p1->SetValueAddr(NULL, p1_convert_addr);

    TinScript::CVariableEntry* ve_p2 = fe->GetContext()->GetParameter(2);
    if (ve_p2 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p2_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p2_ptr_ptr = (void**)(&p2);
        void* p2_ptr = (void*)(*p2_ptr_ptr);
        p2_convert_addr = TypeConvert(script_context, TYPE_string, p2_ptr, ve_p2->GetType());
    }
    else
        p2_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T2>()), (void*)&p2, ve_p2->GetType());
    if (!p2_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 2\n", UnHash(func_hash));
        return false;
    }

    ve_p2->SetValueAddr(NULL, p2_convert_addr);

    TinScript::CVariableEntry* ve_p3 = fe->GetContext()->GetParameter(3);
    if (ve_p3 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p3_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p3_ptr_ptr = (void**)(&p3);
        void* p3_ptr = (void*)(*p3_ptr_ptr);
        p3_convert_addr = TypeConvert(script_context, TYPE_string, p3_ptr, ve_p3->GetType());
    }
    else
        p3_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T3>()), (void*)&p3, ve_p3->GetType());
    if (!p3_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 3\n", UnHash(func_hash));
        return false;
    }

    ve_p3->SetValueAddr(NULL, p3_convert_addr);

    TinScript::CVariableEntry* ve_p4 = fe->GetContext()->GetParameter(4);
    if (ve_p4 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p4_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p4_ptr_ptr = (void**)(&p4);
        void* p4_ptr = (void*)(*p4_ptr_ptr);
        p4_convert_addr = TypeConvert(script_context, TYPE_string, p4_ptr, ve_p4->GetType());
    }
    else
        p4_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T4>()), (void*)&p4, ve_p4->GetType());
    if (!p4_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 4\n", UnHash(func_hash));
        return false;
    }

    ve_p4->SetValueAddr(NULL, p4_convert_addr);

    TinScript::CVariableEntry* ve_p5 = fe->GetContext()->GetParameter(5);
    if (ve_p5 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p5_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p5_ptr_ptr = (void**)(&p5);
        void* p5_ptr = (void*)(*p5_ptr_ptr);
        p5_convert_addr = TypeConvert(script_context, TYPE_string, p5_ptr, ve_p5->GetType());
    }
    else
        p5_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T5>()), (void*)&p5, ve_p5->GetType());
    if (!p5_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 5\n", UnHash(func_hash));
        return false;
    }

    ve_p5->SetValueAddr(NULL, p5_convert_addr);

    TinScript::CVariableEntry* ve_p6 = fe->GetContext()->GetParameter(6);
    if (ve_p6 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p6_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p6_ptr_ptr = (void**)(&p6);
        void* p6_ptr = (void*)(*p6_ptr_ptr);
        p6_convert_addr = TypeConvert(script_context, TYPE_string, p6_ptr, ve_p6->GetType());
    }
    else
        p6_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T6>()), (void*)&p6, ve_p6->GetType());
    if (!p6_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 6\n", UnHash(func_hash));
        return false;
    }

    ve_p6->SetValueAddr(NULL, p6_convert_addr);

    TinScript::CVariableEntry* ve_p7 = fe->GetContext()->GetParameter(7);
    if (ve_p7 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p7_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T7>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p7_ptr_ptr = (void**)(&p7);
        void* p7_ptr = (void*)(*p7_ptr_ptr);
        p7_convert_addr = TypeConvert(script_context, TYPE_string, p7_ptr, ve_p7->GetType());
    }
    else
        p7_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T7>()), (void*)&p7, ve_p7->GetType());
    if (!p7_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 7\n", UnHash(func_hash));
        return false;
    }

    ve_p7->SetValueAddr(NULL, p7_convert_addr);

    TinScript::CVariableEntry* ve_p8 = fe->GetContext()->GetParameter(8);
    if (ve_p8 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p8_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T8>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p8_ptr_ptr = (void**)(&p8);
        void* p8_ptr = (void*)(*p8_ptr_ptr);
        p8_convert_addr = TypeConvert(script_context, TYPE_string, p8_ptr, ve_p8->GetType());
    }
    else
        p8_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T8>()), (void*)&p8, ve_p8->GetType());
    if (!p8_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 8\n", UnHash(func_hash));
        return false;
    }

    ve_p8->SetValueAddr(NULL, p8_convert_addr);

    TinScript::CVariableEntry* ve_p9 = fe->GetContext()->GetParameter(9);
    if (ve_p9 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p9_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T9>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p9_ptr_ptr = (void**)(&p9);
        void* p9_ptr = (void*)(*p9_ptr_ptr);
        p9_convert_addr = TypeConvert(script_context, TYPE_string, p9_ptr, ve_p9->GetType());
    }
    else
        p9_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T9>()), (void*)&p9, ve_p9->GetType());
    if (!p9_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 9\n", UnHash(func_hash));
        return false;
    }

    ve_p9->SetValueAddr(NULL, p9_convert_addr);

    TinScript::CVariableEntry* ve_p10 = fe->GetContext()->GetParameter(10);
    if (ve_p10 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p10_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T10>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p10_ptr_ptr = (void**)(&p10);
        void* p10_ptr = (void*)(*p10_ptr_ptr);
        p10_convert_addr = TypeConvert(script_context, TYPE_string, p10_ptr, ve_p10->GetType());
    }
    else
        p10_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T10>()), (void*)&p10, ve_p10->GetType());
    if (!p10_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 10\n", UnHash(func_hash));
        return false;
    }

    ve_p10->SetValueAddr(NULL, p10_convert_addr);

    TinScript::CVariableEntry* ve_p11 = fe->GetContext()->GetParameter(11);
    if (ve_p11 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p11_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T11>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p11_ptr_ptr = (void**)(&p11);
        void* p11_ptr = (void*)(*p11_ptr_ptr);
        p11_convert_addr = TypeConvert(script_context, TYPE_string, p11_ptr, ve_p11->GetType());
    }
    else
        p11_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T11>()), (void*)&p11, ve_p11->GetType());
    if (!p11_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 11\n", UnHash(func_hash));
        return false;
    }

    ve_p11->SetValueAddr(NULL, p11_convert_addr);

    TinScript::CVariableEntry* ve_p12 = fe->GetContext()->GetParameter(12);
    if (ve_p12 == nullptr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects no more than %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    void* p12_convert_addr = NULL;
    if (TinScript::GetRegisteredType(TinScript::GetTypeID<T12>()) == TYPE_string)
    {
        // -- if the type is string, then pX is a const char*, however, templated code must compile for pX being, say, an int32
        void** p12_ptr_ptr = (void**)(&p12);
        void* p12_ptr = (void*)(*p12_ptr_ptr);
        p12_convert_addr = TypeConvert(script_context, TYPE_string, p12_ptr, ve_p12->GetType());
    }
    else
        p12_convert_addr = TypeConvert(script_context, TinScript::GetRegisteredType(TinScript::GetTypeID<T12>()), (void*)&p12, ve_p12->GetType());
    if (!p12_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 12\n", UnHash(func_hash));
        return false;
    }

    ve_p12->SetValueAddr(NULL, p12_convert_addr);

    // -- execute the function
    if (!ExecuteScheduledFunction(GetContext(), object_id, ns_hash, func_hash, fe->GetContext()))
    {
        TinPrint(script_context, "Error - unable to exec function %s()\n", UnHash(func_hash));
        return false;
    }

    // -- return true if we're able to convert to the return type requested
    return (ReturnExecfResult(script_context, return_value));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12>
inline bool8 ExecFunction(R& return_value, const char* func_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11, T12 p12)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !func_name || !func_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, TinScript::Hash(func_name), p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12>
inline bool8 ExecFunction(R& return_value, uint32 func_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11, T12 p12)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, func_hash, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, const char* method_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11, T12 p12)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, TinScript::Hash(method_name), p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, uint32 method_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11, T12 p12)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, method_hash, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12>
inline bool8 ObjExecNSMethod(uint32 object_id, R& return_value, uint32 ns_hash, uint32 method_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11, T12 p12)
{
    return (ExecFunctionImpl<R>(return_value, object_id, ns_hash, method_hash, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12>
inline bool8 ObjExecMethod(uint32 object_id, R& return_value, const char* method_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11, T12 p12)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, object_id, 0, TinScript::Hash(method_name), p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12));
}

} // TinScript

#endif // __REGISTRATIONEXECS_H
