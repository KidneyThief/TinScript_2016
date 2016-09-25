// ------------------------------------------------------------------------------------------------
//  The MIT License
//  
//  Copyright (c) 2016 Tim Andersen
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
#include "TinExecute.h"

namespace TinScript
{


// -- Parameter count: 0
template<typename R>
inline bool8 ExecFunction(R& return_value, const char* func_name)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !func_name || !func_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, Hash(func_name)));
}

template<typename R>
inline bool8 ExecFunction(R& return_value, uint32 func_hash)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, func_hash));
}

template<typename R>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, const char* method_name)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, Hash(method_name)));
}

template<typename R>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, uint32 method_hash)
{
    CScriptContext* script_context = GetContext();
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
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, object_id, 0, Hash(method_name)));
}

template<typename R>
inline bool8 ExecFunctionImpl(R& return_value, uint32 object_id, uint32 ns_hash, uint32 func_hash)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    // -- get the object, if one was required
    CObjectEntry* oe = object_id > 0 ? script_context->FindObjectEntry(object_id) : NULL;
    if (!oe && object_id > 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object %d not found\n", object_id);
        return false;
    }

    CFunctionEntry* fe = oe ? oe->GetFunctionEntry(ns_hash, func_hash)
                            : script_context->GetGlobalNamespace()->GetFuncTable()->FindItem(func_hash);
    CVariableEntry* return_ve = fe ? fe->GetContext()->GetParameter(0) : NULL;
    if (!fe || !return_ve)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() not found\n", UnHash(func_hash));
        return false;
    }

    // -- see if we can recognize an appropriate type
    eVarType returntype = GetRegisteredType(GetTypeID<R>());
    if (returntype == TYPE_NULL)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - invalid return type (use an int32 if void)\n");
        return false;
    }

    // -- fill in the parameters
    if (fe->GetContext()->GetParameterCount() < 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    // -- execute the function
    if (!ExecuteScheduledFunction(GetContext(), object_id, ns_hash, func_hash, fe->GetContext()))
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - unable to exec function %s()\n", UnHash(func_hash));
        return false;
    }

    // -- return true if we're able to convert to the return type requested
    return (ReturnExecfResult(script_context, return_value));
}


// -- Parameter count: 1
template<typename R, typename T1>
inline bool8 ExecFunction(R& return_value, const char* func_name, T1 p1)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !func_name || !func_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, Hash(func_name), p1));
}

template<typename R, typename T1>
inline bool8 ExecFunction(R& return_value, uint32 func_hash, T1 p1)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, func_hash, p1));
}

template<typename R, typename T1>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, const char* method_name, T1 p1)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, Hash(method_name), p1));
}

template<typename R, typename T1>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, uint32 method_hash, T1 p1)
{
    CScriptContext* script_context = GetContext();
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
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, object_id, 0, Hash(method_name), p1));
}

template<typename R, typename T1>
inline bool8 ExecFunctionImpl(R& return_value, uint32 object_id, uint32 ns_hash, uint32 func_hash, T1 p1)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    // -- get the object, if one was required
    CObjectEntry* oe = object_id > 0 ? script_context->FindObjectEntry(object_id) : NULL;
    if (!oe && object_id > 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object %d not found\n", object_id);
        return false;
    }

    CFunctionEntry* fe = oe ? oe->GetFunctionEntry(ns_hash, func_hash)
                            : script_context->GetGlobalNamespace()->GetFuncTable()->FindItem(func_hash);
    CVariableEntry* return_ve = fe ? fe->GetContext()->GetParameter(0) : NULL;
    if (!fe || !return_ve)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() not found\n", UnHash(func_hash));
        return false;
    }

    // -- see if we can recognize an appropriate type
    eVarType returntype = GetRegisteredType(GetTypeID<R>());
    if (returntype == TYPE_NULL)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - invalid return type (use an int32 if void)\n");
        return false;
    }

    // -- fill in the parameters
    if (fe->GetContext()->GetParameterCount() < 1)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    CVariableEntry* ve_p1 = fe->GetContext()->GetParameter(1);
    void* p1_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T1>()) == TYPE_string)
        p1_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p1, ve_p1->GetType());
    else
        p1_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T1>()), (void*)&p1, ve_p1->GetType());
    if (!p1_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 1\n", UnHash(func_hash));
        return false;
    }

    ve_p1->SetValueAddr(oe ? oe->GetAddr() : NULL, p1_convert_addr);

    // -- execute the function
    if (!ExecuteScheduledFunction(GetContext(), object_id, ns_hash, func_hash, fe->GetContext()))
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - unable to exec function %s()\n", UnHash(func_hash));
        return false;
    }

    // -- return true if we're able to convert to the return type requested
    return (ReturnExecfResult(script_context, return_value));
}


// -- Parameter count: 2
template<typename R, typename T1, typename T2>
inline bool8 ExecFunction(R& return_value, const char* func_name, T1 p1, T2 p2)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !func_name || !func_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, Hash(func_name), p1, p2));
}

template<typename R, typename T1, typename T2>
inline bool8 ExecFunction(R& return_value, uint32 func_hash, T1 p1, T2 p2)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, func_hash, p1, p2));
}

template<typename R, typename T1, typename T2>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, const char* method_name, T1 p1, T2 p2)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, Hash(method_name), p1, p2));
}

template<typename R, typename T1, typename T2>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, uint32 method_hash, T1 p1, T2 p2)
{
    CScriptContext* script_context = GetContext();
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
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, object_id, 0, Hash(method_name), p1, p2));
}

template<typename R, typename T1, typename T2>
inline bool8 ExecFunctionImpl(R& return_value, uint32 object_id, uint32 ns_hash, uint32 func_hash, T1 p1, T2 p2)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    // -- get the object, if one was required
    CObjectEntry* oe = object_id > 0 ? script_context->FindObjectEntry(object_id) : NULL;
    if (!oe && object_id > 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object %d not found\n", object_id);
        return false;
    }

    CFunctionEntry* fe = oe ? oe->GetFunctionEntry(ns_hash, func_hash)
                            : script_context->GetGlobalNamespace()->GetFuncTable()->FindItem(func_hash);
    CVariableEntry* return_ve = fe ? fe->GetContext()->GetParameter(0) : NULL;
    if (!fe || !return_ve)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() not found\n", UnHash(func_hash));
        return false;
    }

    // -- see if we can recognize an appropriate type
    eVarType returntype = GetRegisteredType(GetTypeID<R>());
    if (returntype == TYPE_NULL)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - invalid return type (use an int32 if void)\n");
        return false;
    }

    // -- fill in the parameters
    if (fe->GetContext()->GetParameterCount() < 2)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    CVariableEntry* ve_p1 = fe->GetContext()->GetParameter(1);
    void* p1_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T1>()) == TYPE_string)
        p1_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p1, ve_p1->GetType());
    else
        p1_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T1>()), (void*)&p1, ve_p1->GetType());
    if (!p1_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 1\n", UnHash(func_hash));
        return false;
    }

    ve_p1->SetValueAddr(oe ? oe->GetAddr() : NULL, p1_convert_addr);

    CVariableEntry* ve_p2 = fe->GetContext()->GetParameter(2);
    void* p2_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T2>()) == TYPE_string)
        p2_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p2, ve_p2->GetType());
    else
        p2_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T2>()), (void*)&p2, ve_p2->GetType());
    if (!p2_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 2\n", UnHash(func_hash));
        return false;
    }

    ve_p2->SetValueAddr(oe ? oe->GetAddr() : NULL, p2_convert_addr);

    // -- execute the function
    if (!ExecuteScheduledFunction(GetContext(), object_id, ns_hash, func_hash, fe->GetContext()))
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - unable to exec function %s()\n", UnHash(func_hash));
        return false;
    }

    // -- return true if we're able to convert to the return type requested
    return (ReturnExecfResult(script_context, return_value));
}


// -- Parameter count: 3
template<typename R, typename T1, typename T2, typename T3>
inline bool8 ExecFunction(R& return_value, const char* func_name, T1 p1, T2 p2, T3 p3)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !func_name || !func_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, Hash(func_name), p1, p2, p3));
}

template<typename R, typename T1, typename T2, typename T3>
inline bool8 ExecFunction(R& return_value, uint32 func_hash, T1 p1, T2 p2, T3 p3)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, func_hash, p1, p2, p3));
}

template<typename R, typename T1, typename T2, typename T3>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, const char* method_name, T1 p1, T2 p2, T3 p3)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, Hash(method_name), p1, p2, p3));
}

template<typename R, typename T1, typename T2, typename T3>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, uint32 method_hash, T1 p1, T2 p2, T3 p3)
{
    CScriptContext* script_context = GetContext();
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
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, object_id, 0, Hash(method_name), p1, p2, p3));
}

template<typename R, typename T1, typename T2, typename T3>
inline bool8 ExecFunctionImpl(R& return_value, uint32 object_id, uint32 ns_hash, uint32 func_hash, T1 p1, T2 p2, T3 p3)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    // -- get the object, if one was required
    CObjectEntry* oe = object_id > 0 ? script_context->FindObjectEntry(object_id) : NULL;
    if (!oe && object_id > 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object %d not found\n", object_id);
        return false;
    }

    CFunctionEntry* fe = oe ? oe->GetFunctionEntry(ns_hash, func_hash)
                            : script_context->GetGlobalNamespace()->GetFuncTable()->FindItem(func_hash);
    CVariableEntry* return_ve = fe ? fe->GetContext()->GetParameter(0) : NULL;
    if (!fe || !return_ve)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() not found\n", UnHash(func_hash));
        return false;
    }

    // -- see if we can recognize an appropriate type
    eVarType returntype = GetRegisteredType(GetTypeID<R>());
    if (returntype == TYPE_NULL)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - invalid return type (use an int32 if void)\n");
        return false;
    }

    // -- fill in the parameters
    if (fe->GetContext()->GetParameterCount() < 3)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    CVariableEntry* ve_p1 = fe->GetContext()->GetParameter(1);
    void* p1_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T1>()) == TYPE_string)
        p1_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p1, ve_p1->GetType());
    else
        p1_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T1>()), (void*)&p1, ve_p1->GetType());
    if (!p1_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 1\n", UnHash(func_hash));
        return false;
    }

    ve_p1->SetValueAddr(oe ? oe->GetAddr() : NULL, p1_convert_addr);

    CVariableEntry* ve_p2 = fe->GetContext()->GetParameter(2);
    void* p2_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T2>()) == TYPE_string)
        p2_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p2, ve_p2->GetType());
    else
        p2_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T2>()), (void*)&p2, ve_p2->GetType());
    if (!p2_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 2\n", UnHash(func_hash));
        return false;
    }

    ve_p2->SetValueAddr(oe ? oe->GetAddr() : NULL, p2_convert_addr);

    CVariableEntry* ve_p3 = fe->GetContext()->GetParameter(3);
    void* p3_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T3>()) == TYPE_string)
        p3_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p3, ve_p3->GetType());
    else
        p3_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T3>()), (void*)&p3, ve_p3->GetType());
    if (!p3_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 3\n", UnHash(func_hash));
        return false;
    }

    ve_p3->SetValueAddr(oe ? oe->GetAddr() : NULL, p3_convert_addr);

    // -- execute the function
    if (!ExecuteScheduledFunction(GetContext(), object_id, ns_hash, func_hash, fe->GetContext()))
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - unable to exec function %s()\n", UnHash(func_hash));
        return false;
    }

    // -- return true if we're able to convert to the return type requested
    return (ReturnExecfResult(script_context, return_value));
}


// -- Parameter count: 4
template<typename R, typename T1, typename T2, typename T3, typename T4>
inline bool8 ExecFunction(R& return_value, const char* func_name, T1 p1, T2 p2, T3 p3, T4 p4)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !func_name || !func_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, Hash(func_name), p1, p2, p3, p4));
}

template<typename R, typename T1, typename T2, typename T3, typename T4>
inline bool8 ExecFunction(R& return_value, uint32 func_hash, T1 p1, T2 p2, T3 p3, T4 p4)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, func_hash, p1, p2, p3, p4));
}

template<typename R, typename T1, typename T2, typename T3, typename T4>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, const char* method_name, T1 p1, T2 p2, T3 p3, T4 p4)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, Hash(method_name), p1, p2, p3, p4));
}

template<typename R, typename T1, typename T2, typename T3, typename T4>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, uint32 method_hash, T1 p1, T2 p2, T3 p3, T4 p4)
{
    CScriptContext* script_context = GetContext();
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
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, object_id, 0, Hash(method_name), p1, p2, p3, p4));
}

template<typename R, typename T1, typename T2, typename T3, typename T4>
inline bool8 ExecFunctionImpl(R& return_value, uint32 object_id, uint32 ns_hash, uint32 func_hash, T1 p1, T2 p2, T3 p3, T4 p4)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    // -- get the object, if one was required
    CObjectEntry* oe = object_id > 0 ? script_context->FindObjectEntry(object_id) : NULL;
    if (!oe && object_id > 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object %d not found\n", object_id);
        return false;
    }

    CFunctionEntry* fe = oe ? oe->GetFunctionEntry(ns_hash, func_hash)
                            : script_context->GetGlobalNamespace()->GetFuncTable()->FindItem(func_hash);
    CVariableEntry* return_ve = fe ? fe->GetContext()->GetParameter(0) : NULL;
    if (!fe || !return_ve)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() not found\n", UnHash(func_hash));
        return false;
    }

    // -- see if we can recognize an appropriate type
    eVarType returntype = GetRegisteredType(GetTypeID<R>());
    if (returntype == TYPE_NULL)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - invalid return type (use an int32 if void)\n");
        return false;
    }

    // -- fill in the parameters
    if (fe->GetContext()->GetParameterCount() < 4)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    CVariableEntry* ve_p1 = fe->GetContext()->GetParameter(1);
    void* p1_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T1>()) == TYPE_string)
        p1_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p1, ve_p1->GetType());
    else
        p1_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T1>()), (void*)&p1, ve_p1->GetType());
    if (!p1_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 1\n", UnHash(func_hash));
        return false;
    }

    ve_p1->SetValueAddr(oe ? oe->GetAddr() : NULL, p1_convert_addr);

    CVariableEntry* ve_p2 = fe->GetContext()->GetParameter(2);
    void* p2_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T2>()) == TYPE_string)
        p2_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p2, ve_p2->GetType());
    else
        p2_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T2>()), (void*)&p2, ve_p2->GetType());
    if (!p2_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 2\n", UnHash(func_hash));
        return false;
    }

    ve_p2->SetValueAddr(oe ? oe->GetAddr() : NULL, p2_convert_addr);

    CVariableEntry* ve_p3 = fe->GetContext()->GetParameter(3);
    void* p3_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T3>()) == TYPE_string)
        p3_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p3, ve_p3->GetType());
    else
        p3_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T3>()), (void*)&p3, ve_p3->GetType());
    if (!p3_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 3\n", UnHash(func_hash));
        return false;
    }

    ve_p3->SetValueAddr(oe ? oe->GetAddr() : NULL, p3_convert_addr);

    CVariableEntry* ve_p4 = fe->GetContext()->GetParameter(4);
    void* p4_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T4>()) == TYPE_string)
        p4_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p4, ve_p4->GetType());
    else
        p4_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T4>()), (void*)&p4, ve_p4->GetType());
    if (!p4_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 4\n", UnHash(func_hash));
        return false;
    }

    ve_p4->SetValueAddr(oe ? oe->GetAddr() : NULL, p4_convert_addr);

    // -- execute the function
    if (!ExecuteScheduledFunction(GetContext(), object_id, ns_hash, func_hash, fe->GetContext()))
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - unable to exec function %s()\n", UnHash(func_hash));
        return false;
    }

    // -- return true if we're able to convert to the return type requested
    return (ReturnExecfResult(script_context, return_value));
}


// -- Parameter count: 5
template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5>
inline bool8 ExecFunction(R& return_value, const char* func_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !func_name || !func_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, Hash(func_name), p1, p2, p3, p4, p5));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5>
inline bool8 ExecFunction(R& return_value, uint32 func_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, func_hash, p1, p2, p3, p4, p5));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, const char* method_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, Hash(method_name), p1, p2, p3, p4, p5));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, uint32 method_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5)
{
    CScriptContext* script_context = GetContext();
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
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, object_id, 0, Hash(method_name), p1, p2, p3, p4, p5));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5>
inline bool8 ExecFunctionImpl(R& return_value, uint32 object_id, uint32 ns_hash, uint32 func_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    // -- get the object, if one was required
    CObjectEntry* oe = object_id > 0 ? script_context->FindObjectEntry(object_id) : NULL;
    if (!oe && object_id > 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object %d not found\n", object_id);
        return false;
    }

    CFunctionEntry* fe = oe ? oe->GetFunctionEntry(ns_hash, func_hash)
                            : script_context->GetGlobalNamespace()->GetFuncTable()->FindItem(func_hash);
    CVariableEntry* return_ve = fe ? fe->GetContext()->GetParameter(0) : NULL;
    if (!fe || !return_ve)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() not found\n", UnHash(func_hash));
        return false;
    }

    // -- see if we can recognize an appropriate type
    eVarType returntype = GetRegisteredType(GetTypeID<R>());
    if (returntype == TYPE_NULL)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - invalid return type (use an int32 if void)\n");
        return false;
    }

    // -- fill in the parameters
    if (fe->GetContext()->GetParameterCount() < 5)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    CVariableEntry* ve_p1 = fe->GetContext()->GetParameter(1);
    void* p1_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T1>()) == TYPE_string)
        p1_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p1, ve_p1->GetType());
    else
        p1_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T1>()), (void*)&p1, ve_p1->GetType());
    if (!p1_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 1\n", UnHash(func_hash));
        return false;
    }

    ve_p1->SetValueAddr(oe ? oe->GetAddr() : NULL, p1_convert_addr);

    CVariableEntry* ve_p2 = fe->GetContext()->GetParameter(2);
    void* p2_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T2>()) == TYPE_string)
        p2_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p2, ve_p2->GetType());
    else
        p2_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T2>()), (void*)&p2, ve_p2->GetType());
    if (!p2_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 2\n", UnHash(func_hash));
        return false;
    }

    ve_p2->SetValueAddr(oe ? oe->GetAddr() : NULL, p2_convert_addr);

    CVariableEntry* ve_p3 = fe->GetContext()->GetParameter(3);
    void* p3_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T3>()) == TYPE_string)
        p3_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p3, ve_p3->GetType());
    else
        p3_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T3>()), (void*)&p3, ve_p3->GetType());
    if (!p3_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 3\n", UnHash(func_hash));
        return false;
    }

    ve_p3->SetValueAddr(oe ? oe->GetAddr() : NULL, p3_convert_addr);

    CVariableEntry* ve_p4 = fe->GetContext()->GetParameter(4);
    void* p4_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T4>()) == TYPE_string)
        p4_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p4, ve_p4->GetType());
    else
        p4_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T4>()), (void*)&p4, ve_p4->GetType());
    if (!p4_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 4\n", UnHash(func_hash));
        return false;
    }

    ve_p4->SetValueAddr(oe ? oe->GetAddr() : NULL, p4_convert_addr);

    CVariableEntry* ve_p5 = fe->GetContext()->GetParameter(5);
    void* p5_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T5>()) == TYPE_string)
        p5_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p5, ve_p5->GetType());
    else
        p5_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T5>()), (void*)&p5, ve_p5->GetType());
    if (!p5_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 5\n", UnHash(func_hash));
        return false;
    }

    ve_p5->SetValueAddr(oe ? oe->GetAddr() : NULL, p5_convert_addr);

    // -- execute the function
    if (!ExecuteScheduledFunction(GetContext(), object_id, ns_hash, func_hash, fe->GetContext()))
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - unable to exec function %s()\n", UnHash(func_hash));
        return false;
    }

    // -- return true if we're able to convert to the return type requested
    return (ReturnExecfResult(script_context, return_value));
}


// -- Parameter count: 6
template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
inline bool8 ExecFunction(R& return_value, const char* func_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !func_name || !func_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, Hash(func_name), p1, p2, p3, p4, p5, p6));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
inline bool8 ExecFunction(R& return_value, uint32 func_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, func_hash, p1, p2, p3, p4, p5, p6));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, const char* method_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, Hash(method_name), p1, p2, p3, p4, p5, p6));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, uint32 method_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6)
{
    CScriptContext* script_context = GetContext();
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
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, object_id, 0, Hash(method_name), p1, p2, p3, p4, p5, p6));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
inline bool8 ExecFunctionImpl(R& return_value, uint32 object_id, uint32 ns_hash, uint32 func_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    // -- get the object, if one was required
    CObjectEntry* oe = object_id > 0 ? script_context->FindObjectEntry(object_id) : NULL;
    if (!oe && object_id > 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object %d not found\n", object_id);
        return false;
    }

    CFunctionEntry* fe = oe ? oe->GetFunctionEntry(ns_hash, func_hash)
                            : script_context->GetGlobalNamespace()->GetFuncTable()->FindItem(func_hash);
    CVariableEntry* return_ve = fe ? fe->GetContext()->GetParameter(0) : NULL;
    if (!fe || !return_ve)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() not found\n", UnHash(func_hash));
        return false;
    }

    // -- see if we can recognize an appropriate type
    eVarType returntype = GetRegisteredType(GetTypeID<R>());
    if (returntype == TYPE_NULL)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - invalid return type (use an int32 if void)\n");
        return false;
    }

    // -- fill in the parameters
    if (fe->GetContext()->GetParameterCount() < 6)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    CVariableEntry* ve_p1 = fe->GetContext()->GetParameter(1);
    void* p1_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T1>()) == TYPE_string)
        p1_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p1, ve_p1->GetType());
    else
        p1_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T1>()), (void*)&p1, ve_p1->GetType());
    if (!p1_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 1\n", UnHash(func_hash));
        return false;
    }

    ve_p1->SetValueAddr(oe ? oe->GetAddr() : NULL, p1_convert_addr);

    CVariableEntry* ve_p2 = fe->GetContext()->GetParameter(2);
    void* p2_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T2>()) == TYPE_string)
        p2_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p2, ve_p2->GetType());
    else
        p2_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T2>()), (void*)&p2, ve_p2->GetType());
    if (!p2_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 2\n", UnHash(func_hash));
        return false;
    }

    ve_p2->SetValueAddr(oe ? oe->GetAddr() : NULL, p2_convert_addr);

    CVariableEntry* ve_p3 = fe->GetContext()->GetParameter(3);
    void* p3_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T3>()) == TYPE_string)
        p3_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p3, ve_p3->GetType());
    else
        p3_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T3>()), (void*)&p3, ve_p3->GetType());
    if (!p3_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 3\n", UnHash(func_hash));
        return false;
    }

    ve_p3->SetValueAddr(oe ? oe->GetAddr() : NULL, p3_convert_addr);

    CVariableEntry* ve_p4 = fe->GetContext()->GetParameter(4);
    void* p4_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T4>()) == TYPE_string)
        p4_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p4, ve_p4->GetType());
    else
        p4_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T4>()), (void*)&p4, ve_p4->GetType());
    if (!p4_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 4\n", UnHash(func_hash));
        return false;
    }

    ve_p4->SetValueAddr(oe ? oe->GetAddr() : NULL, p4_convert_addr);

    CVariableEntry* ve_p5 = fe->GetContext()->GetParameter(5);
    void* p5_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T5>()) == TYPE_string)
        p5_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p5, ve_p5->GetType());
    else
        p5_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T5>()), (void*)&p5, ve_p5->GetType());
    if (!p5_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 5\n", UnHash(func_hash));
        return false;
    }

    ve_p5->SetValueAddr(oe ? oe->GetAddr() : NULL, p5_convert_addr);

    CVariableEntry* ve_p6 = fe->GetContext()->GetParameter(6);
    void* p6_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T6>()) == TYPE_string)
        p6_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p6, ve_p6->GetType());
    else
        p6_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T6>()), (void*)&p6, ve_p6->GetType());
    if (!p6_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 6\n", UnHash(func_hash));
        return false;
    }

    ve_p6->SetValueAddr(oe ? oe->GetAddr() : NULL, p6_convert_addr);

    // -- execute the function
    if (!ExecuteScheduledFunction(GetContext(), object_id, ns_hash, func_hash, fe->GetContext()))
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - unable to exec function %s()\n", UnHash(func_hash));
        return false;
    }

    // -- return true if we're able to convert to the return type requested
    return (ReturnExecfResult(script_context, return_value));
}


// -- Parameter count: 7
template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
inline bool8 ExecFunction(R& return_value, const char* func_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !func_name || !func_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, Hash(func_name), p1, p2, p3, p4, p5, p6, p7));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
inline bool8 ExecFunction(R& return_value, uint32 func_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, func_hash, p1, p2, p3, p4, p5, p6, p7));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, const char* method_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, Hash(method_name), p1, p2, p3, p4, p5, p6, p7));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, uint32 method_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7)
{
    CScriptContext* script_context = GetContext();
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
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, object_id, 0, Hash(method_name), p1, p2, p3, p4, p5, p6, p7));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
inline bool8 ExecFunctionImpl(R& return_value, uint32 object_id, uint32 ns_hash, uint32 func_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    // -- get the object, if one was required
    CObjectEntry* oe = object_id > 0 ? script_context->FindObjectEntry(object_id) : NULL;
    if (!oe && object_id > 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object %d not found\n", object_id);
        return false;
    }

    CFunctionEntry* fe = oe ? oe->GetFunctionEntry(ns_hash, func_hash)
                            : script_context->GetGlobalNamespace()->GetFuncTable()->FindItem(func_hash);
    CVariableEntry* return_ve = fe ? fe->GetContext()->GetParameter(0) : NULL;
    if (!fe || !return_ve)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() not found\n", UnHash(func_hash));
        return false;
    }

    // -- see if we can recognize an appropriate type
    eVarType returntype = GetRegisteredType(GetTypeID<R>());
    if (returntype == TYPE_NULL)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - invalid return type (use an int32 if void)\n");
        return false;
    }

    // -- fill in the parameters
    if (fe->GetContext()->GetParameterCount() < 7)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    CVariableEntry* ve_p1 = fe->GetContext()->GetParameter(1);
    void* p1_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T1>()) == TYPE_string)
        p1_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p1, ve_p1->GetType());
    else
        p1_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T1>()), (void*)&p1, ve_p1->GetType());
    if (!p1_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 1\n", UnHash(func_hash));
        return false;
    }

    ve_p1->SetValueAddr(oe ? oe->GetAddr() : NULL, p1_convert_addr);

    CVariableEntry* ve_p2 = fe->GetContext()->GetParameter(2);
    void* p2_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T2>()) == TYPE_string)
        p2_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p2, ve_p2->GetType());
    else
        p2_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T2>()), (void*)&p2, ve_p2->GetType());
    if (!p2_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 2\n", UnHash(func_hash));
        return false;
    }

    ve_p2->SetValueAddr(oe ? oe->GetAddr() : NULL, p2_convert_addr);

    CVariableEntry* ve_p3 = fe->GetContext()->GetParameter(3);
    void* p3_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T3>()) == TYPE_string)
        p3_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p3, ve_p3->GetType());
    else
        p3_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T3>()), (void*)&p3, ve_p3->GetType());
    if (!p3_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 3\n", UnHash(func_hash));
        return false;
    }

    ve_p3->SetValueAddr(oe ? oe->GetAddr() : NULL, p3_convert_addr);

    CVariableEntry* ve_p4 = fe->GetContext()->GetParameter(4);
    void* p4_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T4>()) == TYPE_string)
        p4_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p4, ve_p4->GetType());
    else
        p4_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T4>()), (void*)&p4, ve_p4->GetType());
    if (!p4_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 4\n", UnHash(func_hash));
        return false;
    }

    ve_p4->SetValueAddr(oe ? oe->GetAddr() : NULL, p4_convert_addr);

    CVariableEntry* ve_p5 = fe->GetContext()->GetParameter(5);
    void* p5_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T5>()) == TYPE_string)
        p5_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p5, ve_p5->GetType());
    else
        p5_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T5>()), (void*)&p5, ve_p5->GetType());
    if (!p5_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 5\n", UnHash(func_hash));
        return false;
    }

    ve_p5->SetValueAddr(oe ? oe->GetAddr() : NULL, p5_convert_addr);

    CVariableEntry* ve_p6 = fe->GetContext()->GetParameter(6);
    void* p6_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T6>()) == TYPE_string)
        p6_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p6, ve_p6->GetType());
    else
        p6_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T6>()), (void*)&p6, ve_p6->GetType());
    if (!p6_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 6\n", UnHash(func_hash));
        return false;
    }

    ve_p6->SetValueAddr(oe ? oe->GetAddr() : NULL, p6_convert_addr);

    CVariableEntry* ve_p7 = fe->GetContext()->GetParameter(7);
    void* p7_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T7>()) == TYPE_string)
        p7_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p7, ve_p7->GetType());
    else
        p7_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T7>()), (void*)&p7, ve_p7->GetType());
    if (!p7_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 7\n", UnHash(func_hash));
        return false;
    }

    ve_p7->SetValueAddr(oe ? oe->GetAddr() : NULL, p7_convert_addr);

    // -- execute the function
    if (!ExecuteScheduledFunction(GetContext(), object_id, ns_hash, func_hash, fe->GetContext()))
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - unable to exec function %s()\n", UnHash(func_hash));
        return false;
    }

    // -- return true if we're able to convert to the return type requested
    return (ReturnExecfResult(script_context, return_value));
}


// -- Parameter count: 8
template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
inline bool8 ExecFunction(R& return_value, const char* func_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !func_name || !func_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, Hash(func_name), p1, p2, p3, p4, p5, p6, p7, p8));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
inline bool8 ExecFunction(R& return_value, uint32 func_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, func_hash, p1, p2, p3, p4, p5, p6, p7, p8));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, const char* method_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, Hash(method_name), p1, p2, p3, p4, p5, p6, p7, p8));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, uint32 method_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8)
{
    CScriptContext* script_context = GetContext();
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
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, object_id, 0, Hash(method_name), p1, p2, p3, p4, p5, p6, p7, p8));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
inline bool8 ExecFunctionImpl(R& return_value, uint32 object_id, uint32 ns_hash, uint32 func_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    // -- get the object, if one was required
    CObjectEntry* oe = object_id > 0 ? script_context->FindObjectEntry(object_id) : NULL;
    if (!oe && object_id > 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object %d not found\n", object_id);
        return false;
    }

    CFunctionEntry* fe = oe ? oe->GetFunctionEntry(ns_hash, func_hash)
                            : script_context->GetGlobalNamespace()->GetFuncTable()->FindItem(func_hash);
    CVariableEntry* return_ve = fe ? fe->GetContext()->GetParameter(0) : NULL;
    if (!fe || !return_ve)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() not found\n", UnHash(func_hash));
        return false;
    }

    // -- see if we can recognize an appropriate type
    eVarType returntype = GetRegisteredType(GetTypeID<R>());
    if (returntype == TYPE_NULL)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - invalid return type (use an int32 if void)\n");
        return false;
    }

    // -- fill in the parameters
    if (fe->GetContext()->GetParameterCount() < 8)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    CVariableEntry* ve_p1 = fe->GetContext()->GetParameter(1);
    void* p1_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T1>()) == TYPE_string)
        p1_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p1, ve_p1->GetType());
    else
        p1_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T1>()), (void*)&p1, ve_p1->GetType());
    if (!p1_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 1\n", UnHash(func_hash));
        return false;
    }

    ve_p1->SetValueAddr(oe ? oe->GetAddr() : NULL, p1_convert_addr);

    CVariableEntry* ve_p2 = fe->GetContext()->GetParameter(2);
    void* p2_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T2>()) == TYPE_string)
        p2_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p2, ve_p2->GetType());
    else
        p2_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T2>()), (void*)&p2, ve_p2->GetType());
    if (!p2_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 2\n", UnHash(func_hash));
        return false;
    }

    ve_p2->SetValueAddr(oe ? oe->GetAddr() : NULL, p2_convert_addr);

    CVariableEntry* ve_p3 = fe->GetContext()->GetParameter(3);
    void* p3_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T3>()) == TYPE_string)
        p3_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p3, ve_p3->GetType());
    else
        p3_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T3>()), (void*)&p3, ve_p3->GetType());
    if (!p3_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 3\n", UnHash(func_hash));
        return false;
    }

    ve_p3->SetValueAddr(oe ? oe->GetAddr() : NULL, p3_convert_addr);

    CVariableEntry* ve_p4 = fe->GetContext()->GetParameter(4);
    void* p4_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T4>()) == TYPE_string)
        p4_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p4, ve_p4->GetType());
    else
        p4_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T4>()), (void*)&p4, ve_p4->GetType());
    if (!p4_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 4\n", UnHash(func_hash));
        return false;
    }

    ve_p4->SetValueAddr(oe ? oe->GetAddr() : NULL, p4_convert_addr);

    CVariableEntry* ve_p5 = fe->GetContext()->GetParameter(5);
    void* p5_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T5>()) == TYPE_string)
        p5_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p5, ve_p5->GetType());
    else
        p5_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T5>()), (void*)&p5, ve_p5->GetType());
    if (!p5_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 5\n", UnHash(func_hash));
        return false;
    }

    ve_p5->SetValueAddr(oe ? oe->GetAddr() : NULL, p5_convert_addr);

    CVariableEntry* ve_p6 = fe->GetContext()->GetParameter(6);
    void* p6_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T6>()) == TYPE_string)
        p6_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p6, ve_p6->GetType());
    else
        p6_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T6>()), (void*)&p6, ve_p6->GetType());
    if (!p6_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 6\n", UnHash(func_hash));
        return false;
    }

    ve_p6->SetValueAddr(oe ? oe->GetAddr() : NULL, p6_convert_addr);

    CVariableEntry* ve_p7 = fe->GetContext()->GetParameter(7);
    void* p7_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T7>()) == TYPE_string)
        p7_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p7, ve_p7->GetType());
    else
        p7_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T7>()), (void*)&p7, ve_p7->GetType());
    if (!p7_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 7\n", UnHash(func_hash));
        return false;
    }

    ve_p7->SetValueAddr(oe ? oe->GetAddr() : NULL, p7_convert_addr);

    CVariableEntry* ve_p8 = fe->GetContext()->GetParameter(8);
    void* p8_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T8>()) == TYPE_string)
        p8_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p8, ve_p8->GetType());
    else
        p8_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T8>()), (void*)&p8, ve_p8->GetType());
    if (!p8_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 8\n", UnHash(func_hash));
        return false;
    }

    ve_p8->SetValueAddr(oe ? oe->GetAddr() : NULL, p8_convert_addr);

    // -- execute the function
    if (!ExecuteScheduledFunction(GetContext(), object_id, ns_hash, func_hash, fe->GetContext()))
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - unable to exec function %s()\n", UnHash(func_hash));
        return false;
    }

    // -- return true if we're able to convert to the return type requested
    return (ReturnExecfResult(script_context, return_value));
}


// -- Parameter count: 9
template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
inline bool8 ExecFunction(R& return_value, const char* func_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !func_name || !func_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, Hash(func_name), p1, p2, p3, p4, p5, p6, p7, p8, p9));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
inline bool8 ExecFunction(R& return_value, uint32 func_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, func_hash, p1, p2, p3, p4, p5, p6, p7, p8, p9));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, const char* method_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, Hash(method_name), p1, p2, p3, p4, p5, p6, p7, p8, p9));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, uint32 method_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9)
{
    CScriptContext* script_context = GetContext();
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
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, object_id, 0, Hash(method_name), p1, p2, p3, p4, p5, p6, p7, p8, p9));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
inline bool8 ExecFunctionImpl(R& return_value, uint32 object_id, uint32 ns_hash, uint32 func_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    // -- get the object, if one was required
    CObjectEntry* oe = object_id > 0 ? script_context->FindObjectEntry(object_id) : NULL;
    if (!oe && object_id > 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object %d not found\n", object_id);
        return false;
    }

    CFunctionEntry* fe = oe ? oe->GetFunctionEntry(ns_hash, func_hash)
                            : script_context->GetGlobalNamespace()->GetFuncTable()->FindItem(func_hash);
    CVariableEntry* return_ve = fe ? fe->GetContext()->GetParameter(0) : NULL;
    if (!fe || !return_ve)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() not found\n", UnHash(func_hash));
        return false;
    }

    // -- see if we can recognize an appropriate type
    eVarType returntype = GetRegisteredType(GetTypeID<R>());
    if (returntype == TYPE_NULL)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - invalid return type (use an int32 if void)\n");
        return false;
    }

    // -- fill in the parameters
    if (fe->GetContext()->GetParameterCount() < 9)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    CVariableEntry* ve_p1 = fe->GetContext()->GetParameter(1);
    void* p1_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T1>()) == TYPE_string)
        p1_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p1, ve_p1->GetType());
    else
        p1_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T1>()), (void*)&p1, ve_p1->GetType());
    if (!p1_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 1\n", UnHash(func_hash));
        return false;
    }

    ve_p1->SetValueAddr(oe ? oe->GetAddr() : NULL, p1_convert_addr);

    CVariableEntry* ve_p2 = fe->GetContext()->GetParameter(2);
    void* p2_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T2>()) == TYPE_string)
        p2_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p2, ve_p2->GetType());
    else
        p2_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T2>()), (void*)&p2, ve_p2->GetType());
    if (!p2_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 2\n", UnHash(func_hash));
        return false;
    }

    ve_p2->SetValueAddr(oe ? oe->GetAddr() : NULL, p2_convert_addr);

    CVariableEntry* ve_p3 = fe->GetContext()->GetParameter(3);
    void* p3_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T3>()) == TYPE_string)
        p3_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p3, ve_p3->GetType());
    else
        p3_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T3>()), (void*)&p3, ve_p3->GetType());
    if (!p3_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 3\n", UnHash(func_hash));
        return false;
    }

    ve_p3->SetValueAddr(oe ? oe->GetAddr() : NULL, p3_convert_addr);

    CVariableEntry* ve_p4 = fe->GetContext()->GetParameter(4);
    void* p4_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T4>()) == TYPE_string)
        p4_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p4, ve_p4->GetType());
    else
        p4_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T4>()), (void*)&p4, ve_p4->GetType());
    if (!p4_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 4\n", UnHash(func_hash));
        return false;
    }

    ve_p4->SetValueAddr(oe ? oe->GetAddr() : NULL, p4_convert_addr);

    CVariableEntry* ve_p5 = fe->GetContext()->GetParameter(5);
    void* p5_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T5>()) == TYPE_string)
        p5_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p5, ve_p5->GetType());
    else
        p5_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T5>()), (void*)&p5, ve_p5->GetType());
    if (!p5_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 5\n", UnHash(func_hash));
        return false;
    }

    ve_p5->SetValueAddr(oe ? oe->GetAddr() : NULL, p5_convert_addr);

    CVariableEntry* ve_p6 = fe->GetContext()->GetParameter(6);
    void* p6_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T6>()) == TYPE_string)
        p6_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p6, ve_p6->GetType());
    else
        p6_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T6>()), (void*)&p6, ve_p6->GetType());
    if (!p6_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 6\n", UnHash(func_hash));
        return false;
    }

    ve_p6->SetValueAddr(oe ? oe->GetAddr() : NULL, p6_convert_addr);

    CVariableEntry* ve_p7 = fe->GetContext()->GetParameter(7);
    void* p7_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T7>()) == TYPE_string)
        p7_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p7, ve_p7->GetType());
    else
        p7_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T7>()), (void*)&p7, ve_p7->GetType());
    if (!p7_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 7\n", UnHash(func_hash));
        return false;
    }

    ve_p7->SetValueAddr(oe ? oe->GetAddr() : NULL, p7_convert_addr);

    CVariableEntry* ve_p8 = fe->GetContext()->GetParameter(8);
    void* p8_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T8>()) == TYPE_string)
        p8_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p8, ve_p8->GetType());
    else
        p8_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T8>()), (void*)&p8, ve_p8->GetType());
    if (!p8_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 8\n", UnHash(func_hash));
        return false;
    }

    ve_p8->SetValueAddr(oe ? oe->GetAddr() : NULL, p8_convert_addr);

    CVariableEntry* ve_p9 = fe->GetContext()->GetParameter(9);
    void* p9_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T9>()) == TYPE_string)
        p9_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p9, ve_p9->GetType());
    else
        p9_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T9>()), (void*)&p9, ve_p9->GetType());
    if (!p9_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 9\n", UnHash(func_hash));
        return false;
    }

    ve_p9->SetValueAddr(oe ? oe->GetAddr() : NULL, p9_convert_addr);

    // -- execute the function
    if (!ExecuteScheduledFunction(GetContext(), object_id, ns_hash, func_hash, fe->GetContext()))
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - unable to exec function %s()\n", UnHash(func_hash));
        return false;
    }

    // -- return true if we're able to convert to the return type requested
    return (ReturnExecfResult(script_context, return_value));
}


// -- Parameter count: 10
template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10>
inline bool8 ExecFunction(R& return_value, const char* func_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !func_name || !func_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, Hash(func_name), p1, p2, p3, p4, p5, p6, p7, p8, p9, p10));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10>
inline bool8 ExecFunction(R& return_value, uint32 func_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, func_hash, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, const char* method_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, Hash(method_name), p1, p2, p3, p4, p5, p6, p7, p8, p9, p10));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, uint32 method_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10)
{
    CScriptContext* script_context = GetContext();
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
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, object_id, 0, Hash(method_name), p1, p2, p3, p4, p5, p6, p7, p8, p9, p10));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10>
inline bool8 ExecFunctionImpl(R& return_value, uint32 object_id, uint32 ns_hash, uint32 func_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    // -- get the object, if one was required
    CObjectEntry* oe = object_id > 0 ? script_context->FindObjectEntry(object_id) : NULL;
    if (!oe && object_id > 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object %d not found\n", object_id);
        return false;
    }

    CFunctionEntry* fe = oe ? oe->GetFunctionEntry(ns_hash, func_hash)
                            : script_context->GetGlobalNamespace()->GetFuncTable()->FindItem(func_hash);
    CVariableEntry* return_ve = fe ? fe->GetContext()->GetParameter(0) : NULL;
    if (!fe || !return_ve)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() not found\n", UnHash(func_hash));
        return false;
    }

    // -- see if we can recognize an appropriate type
    eVarType returntype = GetRegisteredType(GetTypeID<R>());
    if (returntype == TYPE_NULL)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - invalid return type (use an int32 if void)\n");
        return false;
    }

    // -- fill in the parameters
    if (fe->GetContext()->GetParameterCount() < 10)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    CVariableEntry* ve_p1 = fe->GetContext()->GetParameter(1);
    void* p1_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T1>()) == TYPE_string)
        p1_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p1, ve_p1->GetType());
    else
        p1_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T1>()), (void*)&p1, ve_p1->GetType());
    if (!p1_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 1\n", UnHash(func_hash));
        return false;
    }

    ve_p1->SetValueAddr(oe ? oe->GetAddr() : NULL, p1_convert_addr);

    CVariableEntry* ve_p2 = fe->GetContext()->GetParameter(2);
    void* p2_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T2>()) == TYPE_string)
        p2_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p2, ve_p2->GetType());
    else
        p2_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T2>()), (void*)&p2, ve_p2->GetType());
    if (!p2_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 2\n", UnHash(func_hash));
        return false;
    }

    ve_p2->SetValueAddr(oe ? oe->GetAddr() : NULL, p2_convert_addr);

    CVariableEntry* ve_p3 = fe->GetContext()->GetParameter(3);
    void* p3_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T3>()) == TYPE_string)
        p3_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p3, ve_p3->GetType());
    else
        p3_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T3>()), (void*)&p3, ve_p3->GetType());
    if (!p3_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 3\n", UnHash(func_hash));
        return false;
    }

    ve_p3->SetValueAddr(oe ? oe->GetAddr() : NULL, p3_convert_addr);

    CVariableEntry* ve_p4 = fe->GetContext()->GetParameter(4);
    void* p4_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T4>()) == TYPE_string)
        p4_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p4, ve_p4->GetType());
    else
        p4_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T4>()), (void*)&p4, ve_p4->GetType());
    if (!p4_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 4\n", UnHash(func_hash));
        return false;
    }

    ve_p4->SetValueAddr(oe ? oe->GetAddr() : NULL, p4_convert_addr);

    CVariableEntry* ve_p5 = fe->GetContext()->GetParameter(5);
    void* p5_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T5>()) == TYPE_string)
        p5_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p5, ve_p5->GetType());
    else
        p5_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T5>()), (void*)&p5, ve_p5->GetType());
    if (!p5_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 5\n", UnHash(func_hash));
        return false;
    }

    ve_p5->SetValueAddr(oe ? oe->GetAddr() : NULL, p5_convert_addr);

    CVariableEntry* ve_p6 = fe->GetContext()->GetParameter(6);
    void* p6_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T6>()) == TYPE_string)
        p6_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p6, ve_p6->GetType());
    else
        p6_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T6>()), (void*)&p6, ve_p6->GetType());
    if (!p6_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 6\n", UnHash(func_hash));
        return false;
    }

    ve_p6->SetValueAddr(oe ? oe->GetAddr() : NULL, p6_convert_addr);

    CVariableEntry* ve_p7 = fe->GetContext()->GetParameter(7);
    void* p7_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T7>()) == TYPE_string)
        p7_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p7, ve_p7->GetType());
    else
        p7_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T7>()), (void*)&p7, ve_p7->GetType());
    if (!p7_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 7\n", UnHash(func_hash));
        return false;
    }

    ve_p7->SetValueAddr(oe ? oe->GetAddr() : NULL, p7_convert_addr);

    CVariableEntry* ve_p8 = fe->GetContext()->GetParameter(8);
    void* p8_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T8>()) == TYPE_string)
        p8_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p8, ve_p8->GetType());
    else
        p8_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T8>()), (void*)&p8, ve_p8->GetType());
    if (!p8_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 8\n", UnHash(func_hash));
        return false;
    }

    ve_p8->SetValueAddr(oe ? oe->GetAddr() : NULL, p8_convert_addr);

    CVariableEntry* ve_p9 = fe->GetContext()->GetParameter(9);
    void* p9_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T9>()) == TYPE_string)
        p9_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p9, ve_p9->GetType());
    else
        p9_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T9>()), (void*)&p9, ve_p9->GetType());
    if (!p9_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 9\n", UnHash(func_hash));
        return false;
    }

    ve_p9->SetValueAddr(oe ? oe->GetAddr() : NULL, p9_convert_addr);

    CVariableEntry* ve_p10 = fe->GetContext()->GetParameter(10);
    void* p10_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T10>()) == TYPE_string)
        p10_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p10, ve_p10->GetType());
    else
        p10_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T10>()), (void*)&p10, ve_p10->GetType());
    if (!p10_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 10\n", UnHash(func_hash));
        return false;
    }

    ve_p10->SetValueAddr(oe ? oe->GetAddr() : NULL, p10_convert_addr);

    // -- execute the function
    if (!ExecuteScheduledFunction(GetContext(), object_id, ns_hash, func_hash, fe->GetContext()))
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - unable to exec function %s()\n", UnHash(func_hash));
        return false;
    }

    // -- return true if we're able to convert to the return type requested
    return (ReturnExecfResult(script_context, return_value));
}


// -- Parameter count: 11
template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11>
inline bool8 ExecFunction(R& return_value, const char* func_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !func_name || !func_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, Hash(func_name), p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11>
inline bool8 ExecFunction(R& return_value, uint32 func_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, func_hash, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, const char* method_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, Hash(method_name), p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, uint32 method_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11)
{
    CScriptContext* script_context = GetContext();
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
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, object_id, 0, Hash(method_name), p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11>
inline bool8 ExecFunctionImpl(R& return_value, uint32 object_id, uint32 ns_hash, uint32 func_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    // -- get the object, if one was required
    CObjectEntry* oe = object_id > 0 ? script_context->FindObjectEntry(object_id) : NULL;
    if (!oe && object_id > 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object %d not found\n", object_id);
        return false;
    }

    CFunctionEntry* fe = oe ? oe->GetFunctionEntry(ns_hash, func_hash)
                            : script_context->GetGlobalNamespace()->GetFuncTable()->FindItem(func_hash);
    CVariableEntry* return_ve = fe ? fe->GetContext()->GetParameter(0) : NULL;
    if (!fe || !return_ve)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() not found\n", UnHash(func_hash));
        return false;
    }

    // -- see if we can recognize an appropriate type
    eVarType returntype = GetRegisteredType(GetTypeID<R>());
    if (returntype == TYPE_NULL)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - invalid return type (use an int32 if void)\n");
        return false;
    }

    // -- fill in the parameters
    if (fe->GetContext()->GetParameterCount() < 11)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    CVariableEntry* ve_p1 = fe->GetContext()->GetParameter(1);
    void* p1_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T1>()) == TYPE_string)
        p1_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p1, ve_p1->GetType());
    else
        p1_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T1>()), (void*)&p1, ve_p1->GetType());
    if (!p1_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 1\n", UnHash(func_hash));
        return false;
    }

    ve_p1->SetValueAddr(oe ? oe->GetAddr() : NULL, p1_convert_addr);

    CVariableEntry* ve_p2 = fe->GetContext()->GetParameter(2);
    void* p2_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T2>()) == TYPE_string)
        p2_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p2, ve_p2->GetType());
    else
        p2_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T2>()), (void*)&p2, ve_p2->GetType());
    if (!p2_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 2\n", UnHash(func_hash));
        return false;
    }

    ve_p2->SetValueAddr(oe ? oe->GetAddr() : NULL, p2_convert_addr);

    CVariableEntry* ve_p3 = fe->GetContext()->GetParameter(3);
    void* p3_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T3>()) == TYPE_string)
        p3_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p3, ve_p3->GetType());
    else
        p3_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T3>()), (void*)&p3, ve_p3->GetType());
    if (!p3_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 3\n", UnHash(func_hash));
        return false;
    }

    ve_p3->SetValueAddr(oe ? oe->GetAddr() : NULL, p3_convert_addr);

    CVariableEntry* ve_p4 = fe->GetContext()->GetParameter(4);
    void* p4_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T4>()) == TYPE_string)
        p4_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p4, ve_p4->GetType());
    else
        p4_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T4>()), (void*)&p4, ve_p4->GetType());
    if (!p4_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 4\n", UnHash(func_hash));
        return false;
    }

    ve_p4->SetValueAddr(oe ? oe->GetAddr() : NULL, p4_convert_addr);

    CVariableEntry* ve_p5 = fe->GetContext()->GetParameter(5);
    void* p5_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T5>()) == TYPE_string)
        p5_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p5, ve_p5->GetType());
    else
        p5_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T5>()), (void*)&p5, ve_p5->GetType());
    if (!p5_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 5\n", UnHash(func_hash));
        return false;
    }

    ve_p5->SetValueAddr(oe ? oe->GetAddr() : NULL, p5_convert_addr);

    CVariableEntry* ve_p6 = fe->GetContext()->GetParameter(6);
    void* p6_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T6>()) == TYPE_string)
        p6_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p6, ve_p6->GetType());
    else
        p6_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T6>()), (void*)&p6, ve_p6->GetType());
    if (!p6_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 6\n", UnHash(func_hash));
        return false;
    }

    ve_p6->SetValueAddr(oe ? oe->GetAddr() : NULL, p6_convert_addr);

    CVariableEntry* ve_p7 = fe->GetContext()->GetParameter(7);
    void* p7_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T7>()) == TYPE_string)
        p7_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p7, ve_p7->GetType());
    else
        p7_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T7>()), (void*)&p7, ve_p7->GetType());
    if (!p7_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 7\n", UnHash(func_hash));
        return false;
    }

    ve_p7->SetValueAddr(oe ? oe->GetAddr() : NULL, p7_convert_addr);

    CVariableEntry* ve_p8 = fe->GetContext()->GetParameter(8);
    void* p8_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T8>()) == TYPE_string)
        p8_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p8, ve_p8->GetType());
    else
        p8_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T8>()), (void*)&p8, ve_p8->GetType());
    if (!p8_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 8\n", UnHash(func_hash));
        return false;
    }

    ve_p8->SetValueAddr(oe ? oe->GetAddr() : NULL, p8_convert_addr);

    CVariableEntry* ve_p9 = fe->GetContext()->GetParameter(9);
    void* p9_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T9>()) == TYPE_string)
        p9_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p9, ve_p9->GetType());
    else
        p9_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T9>()), (void*)&p9, ve_p9->GetType());
    if (!p9_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 9\n", UnHash(func_hash));
        return false;
    }

    ve_p9->SetValueAddr(oe ? oe->GetAddr() : NULL, p9_convert_addr);

    CVariableEntry* ve_p10 = fe->GetContext()->GetParameter(10);
    void* p10_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T10>()) == TYPE_string)
        p10_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p10, ve_p10->GetType());
    else
        p10_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T10>()), (void*)&p10, ve_p10->GetType());
    if (!p10_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 10\n", UnHash(func_hash));
        return false;
    }

    ve_p10->SetValueAddr(oe ? oe->GetAddr() : NULL, p10_convert_addr);

    CVariableEntry* ve_p11 = fe->GetContext()->GetParameter(11);
    void* p11_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T11>()) == TYPE_string)
        p11_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p11, ve_p11->GetType());
    else
        p11_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T11>()), (void*)&p11, ve_p11->GetType());
    if (!p11_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 11\n", UnHash(func_hash));
        return false;
    }

    ve_p11->SetValueAddr(oe ? oe->GetAddr() : NULL, p11_convert_addr);

    // -- execute the function
    if (!ExecuteScheduledFunction(GetContext(), object_id, ns_hash, func_hash, fe->GetContext()))
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - unable to exec function %s()\n", UnHash(func_hash));
        return false;
    }

    // -- return true if we're able to convert to the return type requested
    return (ReturnExecfResult(script_context, return_value));
}


// -- Parameter count: 12
template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12>
inline bool8 ExecFunction(R& return_value, const char* func_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11, T12 p12)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !func_name || !func_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, Hash(func_name), p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12>
inline bool8 ExecFunction(R& return_value, uint32 func_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11, T12 p12)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    return (ExecFunctionImpl<R>(return_value, 0, 0, func_hash, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, const char* method_name, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11, T12 p12)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    uint32 object_id = script_context->FindIDByAddress(obj_addr);
    if (object_id == 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object not registered: 0x%x\n", kPointerToUInt32(obj_addr));
        return false;
    }

    return (ExecFunctionImpl<R>(return_value, object_id, 0, Hash(method_name), p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12>
inline bool8 ObjExecMethod(void* obj_addr, R& return_value, uint32 method_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11, T12 p12)
{
    CScriptContext* script_context = GetContext();
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
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace() || !method_name || !method_name[0])
        return false;

    return (ExecFunctionImpl<R>(return_value, object_id, 0, Hash(method_name), p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12));
}

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11, typename T12>
inline bool8 ExecFunctionImpl(R& return_value, uint32 object_id, uint32 ns_hash, uint32 func_hash, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7, T8 p8, T9 p9, T10 p10, T11 p11, T12 p12)
{
    CScriptContext* script_context = GetContext();
    if (!script_context->GetGlobalNamespace())
        return false;

    // -- get the object, if one was required
    CObjectEntry* oe = object_id > 0 ? script_context->FindObjectEntry(object_id) : NULL;
    if (!oe && object_id > 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - object %d not found\n", object_id);
        return false;
    }

    CFunctionEntry* fe = oe ? oe->GetFunctionEntry(ns_hash, func_hash)
                            : script_context->GetGlobalNamespace()->GetFuncTable()->FindItem(func_hash);
    CVariableEntry* return_ve = fe ? fe->GetContext()->GetParameter(0) : NULL;
    if (!fe || !return_ve)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() not found\n", UnHash(func_hash));
        return false;
    }

    // -- see if we can recognize an appropriate type
    eVarType returntype = GetRegisteredType(GetTypeID<R>());
    if (returntype == TYPE_NULL)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - invalid return type (use an int32 if void)\n");
        return false;
    }

    // -- fill in the parameters
    if (fe->GetContext()->GetParameterCount() < 12)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() expects %d parameters\n", UnHash(func_hash), fe->GetContext()->GetParameterCount());
        return (false);
    }

    CVariableEntry* ve_p1 = fe->GetContext()->GetParameter(1);
    void* p1_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T1>()) == TYPE_string)
        p1_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p1, ve_p1->GetType());
    else
        p1_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T1>()), (void*)&p1, ve_p1->GetType());
    if (!p1_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 1\n", UnHash(func_hash));
        return false;
    }

    ve_p1->SetValueAddr(oe ? oe->GetAddr() : NULL, p1_convert_addr);

    CVariableEntry* ve_p2 = fe->GetContext()->GetParameter(2);
    void* p2_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T2>()) == TYPE_string)
        p2_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p2, ve_p2->GetType());
    else
        p2_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T2>()), (void*)&p2, ve_p2->GetType());
    if (!p2_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 2\n", UnHash(func_hash));
        return false;
    }

    ve_p2->SetValueAddr(oe ? oe->GetAddr() : NULL, p2_convert_addr);

    CVariableEntry* ve_p3 = fe->GetContext()->GetParameter(3);
    void* p3_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T3>()) == TYPE_string)
        p3_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p3, ve_p3->GetType());
    else
        p3_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T3>()), (void*)&p3, ve_p3->GetType());
    if (!p3_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 3\n", UnHash(func_hash));
        return false;
    }

    ve_p3->SetValueAddr(oe ? oe->GetAddr() : NULL, p3_convert_addr);

    CVariableEntry* ve_p4 = fe->GetContext()->GetParameter(4);
    void* p4_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T4>()) == TYPE_string)
        p4_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p4, ve_p4->GetType());
    else
        p4_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T4>()), (void*)&p4, ve_p4->GetType());
    if (!p4_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 4\n", UnHash(func_hash));
        return false;
    }

    ve_p4->SetValueAddr(oe ? oe->GetAddr() : NULL, p4_convert_addr);

    CVariableEntry* ve_p5 = fe->GetContext()->GetParameter(5);
    void* p5_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T5>()) == TYPE_string)
        p5_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p5, ve_p5->GetType());
    else
        p5_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T5>()), (void*)&p5, ve_p5->GetType());
    if (!p5_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 5\n", UnHash(func_hash));
        return false;
    }

    ve_p5->SetValueAddr(oe ? oe->GetAddr() : NULL, p5_convert_addr);

    CVariableEntry* ve_p6 = fe->GetContext()->GetParameter(6);
    void* p6_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T6>()) == TYPE_string)
        p6_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p6, ve_p6->GetType());
    else
        p6_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T6>()), (void*)&p6, ve_p6->GetType());
    if (!p6_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 6\n", UnHash(func_hash));
        return false;
    }

    ve_p6->SetValueAddr(oe ? oe->GetAddr() : NULL, p6_convert_addr);

    CVariableEntry* ve_p7 = fe->GetContext()->GetParameter(7);
    void* p7_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T7>()) == TYPE_string)
        p7_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p7, ve_p7->GetType());
    else
        p7_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T7>()), (void*)&p7, ve_p7->GetType());
    if (!p7_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 7\n", UnHash(func_hash));
        return false;
    }

    ve_p7->SetValueAddr(oe ? oe->GetAddr() : NULL, p7_convert_addr);

    CVariableEntry* ve_p8 = fe->GetContext()->GetParameter(8);
    void* p8_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T8>()) == TYPE_string)
        p8_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p8, ve_p8->GetType());
    else
        p8_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T8>()), (void*)&p8, ve_p8->GetType());
    if (!p8_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 8\n", UnHash(func_hash));
        return false;
    }

    ve_p8->SetValueAddr(oe ? oe->GetAddr() : NULL, p8_convert_addr);

    CVariableEntry* ve_p9 = fe->GetContext()->GetParameter(9);
    void* p9_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T9>()) == TYPE_string)
        p9_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p9, ve_p9->GetType());
    else
        p9_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T9>()), (void*)&p9, ve_p9->GetType());
    if (!p9_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 9\n", UnHash(func_hash));
        return false;
    }

    ve_p9->SetValueAddr(oe ? oe->GetAddr() : NULL, p9_convert_addr);

    CVariableEntry* ve_p10 = fe->GetContext()->GetParameter(10);
    void* p10_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T10>()) == TYPE_string)
        p10_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p10, ve_p10->GetType());
    else
        p10_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T10>()), (void*)&p10, ve_p10->GetType());
    if (!p10_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 10\n", UnHash(func_hash));
        return false;
    }

    ve_p10->SetValueAddr(oe ? oe->GetAddr() : NULL, p10_convert_addr);

    CVariableEntry* ve_p11 = fe->GetContext()->GetParameter(11);
    void* p11_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T11>()) == TYPE_string)
        p11_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p11, ve_p11->GetType());
    else
        p11_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T11>()), (void*)&p11, ve_p11->GetType());
    if (!p11_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 11\n", UnHash(func_hash));
        return false;
    }

    ve_p11->SetValueAddr(oe ? oe->GetAddr() : NULL, p11_convert_addr);

    CVariableEntry* ve_p12 = fe->GetContext()->GetParameter(12);
    void* p12_convert_addr = NULL;
    if (GetRegisteredType(GetTypeID<T12>()) == TYPE_string)
        p12_convert_addr = TypeConvert(script_context, TYPE_string, (void*)p12, ve_p12->GetType());
    else
        p12_convert_addr = TypeConvert(script_context, GetRegisteredType(GetTypeID<T12>()), (void*)&p12, ve_p12->GetType());
    if (!p12_convert_addr)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - function %s() unable to convert parameter 12\n", UnHash(func_hash));
        return false;
    }

    ve_p12->SetValueAddr(oe ? oe->GetAddr() : NULL, p12_convert_addr);

    // -- execute the function
    if (!ExecuteScheduledFunction(GetContext(), object_id, ns_hash, func_hash, fe->GetContext()))
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - unable to exec function %s()\n", UnHash(func_hash));
        return false;
    }

    // -- return true if we're able to convert to the return type requested
    return (ReturnExecfResult(script_context, return_value));
}
} // TinScript

#endif // __REGISTRATIONEXECS_H
