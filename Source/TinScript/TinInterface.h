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
// TinInterface.h
// ====================================================================================================================

#ifndef __TININTERFACE_H
#define __TININTERFACE_H

// -- includes
#include "TinVariableEntry.h"

// == namespace TinScript =============================================================================================

namespace TinScript
{

// ====================================================================================================================
// CreateContext():  Creates a singleton context, max of one for each thread
// ====================================================================================================================
CScriptContext* CreateContext(TinPrintHandler printhandler, TinAssertHandler asserthandler, bool is_main_thread);

// ====================================================================================================================
// UpdateContext():  Updates the singleton context in the calling thread
// ====================================================================================================================
void UpdateContext(uint32 current_time_msec);

// ====================================================================================================================
// DestroyContext():  Destroys the context created from the calling thread
// ====================================================================================================================
void DestroyContext();

// ====================================================================================================================
// GetContext():  Uses a thread local global var to return the specific context created from this thread
// ====================================================================================================================
CScriptContext* GetContext();

// ====================================================================================================================
// GetMainThreadContext():  returns the script context from the MainThread, needed if we receive remote commands
// ====================================================================================================================
CScriptContext* GetMainThreadContext();

// ====================================================================================================================
// ExecCommand():  Executes a text block of valid script code
// ====================================================================================================================
bool8 ExecCommand(const char* statement);

// ====================================================================================================================
// CompileScript():  Compile (without executing) a text file containing script code
// ====================================================================================================================
bool8 CompileScript(const char* filename);

// ====================================================================================================================

// ====================================================================================================================
// CompileToC():  Compile a script to a 'C' source header file 
// ====================================================================================================================
bool8 CompileToC(const char* filename);

// ====================================================================================================================
// SetDirectory():  sets the current working directory, so all scripts executed will have their path prepended
// ====================================================================================================================
bool8 SetDirectory(const char* path);

// ====================================================================================================================
// ExecScript():  Executes a text file containing script code
// ====================================================================================================================
bool8 ExecScript(const char* filename, bool allow_no_exist = false);

// ====================================================================================================================
// SetTimeScale():  Allows for accurate communication with the debugger, if the application adjusts timescale
// ====================================================================================================================
void SetTimeScale(float time_scale);

// ====================================================================================================================
// ExecResult():  Pass the value returned by a script function back to code
// ====================================================================================================================
template <typename T>
bool8 ReturnExecfResult(CScriptContext* script_context, T& code_return_value)
{
    // -- see if we can recognize an appropriate type
    eVarType code_return_type = GetRegisteredType(GetTypeID<T>());
    if (code_return_type == TYPE_NULL)
        return (false);

    void* script_return_value = NULL;
    eVarType script_return_type = TYPE_NULL;
    if (script_context->GetFunctionReturnValue(script_return_value, script_return_type))
    {
        // -- ensure we were able to convert to the correct type required by code
        void* convertedAddr = NULL;

        // -- as always, strings are "special"...  the return value stored for strings, since
        // -- it's coming from script, is an STE
        if (script_return_type == TYPE_string && code_return_type == TYPE_string)
        {
            convertedAddr = (void*)script_context->GetStringTable()->FindString(*(uint32*)script_return_value);
            code_return_value = convert_from_void_ptr<T>::Convert(convertedAddr);
            return (true);
        }
        else
            convertedAddr = TypeConvert(script_context, script_return_type, script_return_value, code_return_type);
        if (!convertedAddr)
            return (false);

        // -- doublely safe - ensure our type doesn't somehow exceed our max var type
        size_t type_size = sizeof(T);
        if (type_size > kMaxTypeSize)
        {
            ScriptAssert_(script_context, 0, "<internal>", -1,
                            "Error - return type size exceeds the max size of any registered type.\n");
            return (false);
        }

        // -- success
        memcpy(&code_return_value, convertedAddr, type_size);
        return (true);
    }

    // -- unable to get the return value - fill in with a NULL, so we don't crash - still requires
    // -- caller to see if we return (false)
    else
    {
        int32 no_return = 0;
        void* converted_addr = code_return_type == TYPE_string
                               ? (void*)""
                               : TypeConvert(script_context, TYPE_int, &no_return, code_return_type);
        code_return_value = convert_from_void_ptr<T>::Convert(converted_addr);
        return (false);
    }
}

// ====================================================================================================================
// !!! NOTE !!! The following methods have a simpler implementation, but for performance, consider using the
// -- templated methods in registeredexecs.h, especially those that take an object_id and a function_hash
// -- over raw strings.
// ====================================================================================================================

// ====================================================================================================================
// ObjExecF():  From code, Executed a method, either registered or scripted for an object
// Used when the actual object address is provided
// ====================================================================================================================
template <typename T>
bool8 ObjExecF(void* objaddr, T& returnval, const char* methodformat, ...)
{
    CScriptContext* script_context = TinScript::GetContext();

    // -- sanity check
    if (!script_context || !objaddr)
        return (false);

    uint32 objectid = script_context->FindIDByAddress(objaddr);
    if (objectid == 0)
    {
        ScriptAssert_(script_context, 0,
                      "<internal>", -1, "Error - object not registered: 0x%x\n",
                      kPointerToUInt32(objaddr));
        return (false);
    }

    // -- expand the formatted buffer
    va_list args;
    va_start(args, methodformat);
    char methodbuf[kMaxTokenLength];
    vsprintf_s(methodbuf, kMaxTokenLength, methodformat, args);
    va_end(args);

    char execbuf[kMaxTokenLength];
    snprintf(execbuf, kMaxTokenLength - strlen(methodbuf), "%d.%s", objectid,
              methodbuf);

        // -- execute the command
    bool result = script_context->ExecCommand(execbuf);

    // -- if successful, return the result
    if (result)
        return (ReturnExecfResult(script_context, returnval));
    else
        return (false);
}

// ====================================================================================================================
// ObjExecF():  From code, Executed a method, either registered or scripted for an object
// Used when the object ID (not the address) is provided
// ====================================================================================================================
template <typename T>
bool8 ObjExecF(uint32 objectid, T& returnval, const char* methodformat, ...)
{
    CScriptContext* script_context = TinScript::GetContext();

    // -- sanity check
    if (!script_context || objectid == 0 || !methodformat || !methodformat[0])
        return (false);

    CObjectEntry* oe = script_context->FindObjectEntry(objectid);
    if (!oe) {
        ScriptAssert_(script_context, 0,
                      "<internal>", -1, "Error - unable to find object: %d\n", objectid);
        return (false);
    }

    // -- expand the formated buffer
    va_list args;
    va_start(args, methodformat);
    char methodbuf[kMaxTokenLength];
    vsprintf_s(methodbuf, kMaxTokenLength, methodformat, args);
    va_end(args);

	// -- the registered variable to hold the return result is a thread local const char*, registered as "__return"
    char execbuf[kMaxTokenLength];
    snprintf(execbuf, kMaxTokenLength - strlen(methodbuf), "%d.%s", objectid,
              methodbuf);

    // -- execute the command
    bool result = script_context->ExecCommand(execbuf);

    // -- if successful, return the result
    if (result)
        return (ReturnExecfResult(script_context, returnval));
    else
        return (false);
}

// ====================================================================================================================
// ExecF():  From code, Executed a global function either registered or scripted
// ====================================================================================================================
template <typename T>
bool ExecF(T& returnval, const char* stmtformat, ...)
{
    CScriptContext* script_context = TinScript::GetContext();

    // -- sanity check
    if (!script_context || !stmtformat || !stmtformat[0])
        return (false);

    va_list args;
    va_start(args, stmtformat);
    char stmtbuf[kMaxTokenLength];
    vsprintf_s(stmtbuf, kMaxTokenLength, stmtformat, args);
    va_end(args);

    // -- execute the command
    bool result = script_context->ExecCommand(stmtbuf);

    // -- if successful, return the result
    if (result)
        return (ReturnExecfResult(script_context, returnval));
    else
        return (false);
}

} // TinScript

#endif // __TININTERFACE

// ====================================================================================================================
// EOF
// ====================================================================================================================
