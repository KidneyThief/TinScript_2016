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
// TinScriptContextReg.cpp
// ====================================================================================================================

// -- system includes
#include <string.h>

// -- includes
#include "TinRegistration.h"
#include "TinScheduler.h"
#include "TinObjectGroup.h"
#include "TinScript.h"


// == namespace TinScript =============================================================================================

namespace TinScript
{

// --------------------------------------------------------------------------------------------------------------------
// -- statics
CRegisterGlobal* CRegisterGlobal::head = NULL;

// == class CRegisterGlobal ===========================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CRegisterGlobal::CRegisterGlobal(const char* _name, TinScript::eVarType _type, void* _addr, int32 _array_size)
{
    name = _name;
    type = _type;
    addr = _addr;
    array_size = _array_size;
    next = NULL;

    // -- if we've got and address, hook it into the linked list for registration
    if (name && name[0] && addr)
    {
        next = head;
        head = this;
    }
}

// ====================================================================================================================
// RegisterGlobals():  On startup, iterate through the registration objects and actually perform the registration.
// ====================================================================================================================
void CRegisterGlobal::RegisterGlobals(CScriptContext* script_context)
{
    CRegisterGlobal* global = CRegisterGlobal::head;
    while(global)
    {
        // -- ensure our global type is valid
        if (global->type < FIRST_VALID_TYPE)
        {
            ScriptAssert_(script_context, false, "<internal>", -1,
                          "Error - registered global %s has an invalid type\n", global->name);
        }
        else
        {
            // -- create the var entry, add it to the global namespace
            CVariableEntry* ve = TinAlloc(ALLOC_VarEntry, CVariableEntry, script_context, global->name, global->type,
                                          global->array_size, global->addr);
	        uint32 hash = ve->GetHash();
	        script_context->GetGlobalNamespace()->GetVarTable()->AddItem(*ve, hash);
        }

        // -- next registration object
        global = global->next;
    }
}

// ====================================================================================================================
// -- Set of functions that (mostly) access an internal method of a ScriptContext
// -- They retrieve the correct CScriptContext instance from the thread currently executing

// ====================================================================================================================
// CalcHash():  Wrapper for Hash, but allows multiple string arguments
// ====================================================================================================================
int32 CalcHash(const char* str0, const char* str1, const char* str2, const char* str3)
{
    uint32 hashval = Hash(str0);
    hashval = HashAppend(hashval, str1);
    hashval = HashAppend(hashval, str2);
    hashval = HashAppend(hashval, str3);

    return static_cast<int32>(hashval);
}

// ====================================================================================================================
// CalcUnhash():  Reverse lookup, returns the string that hashes to the given value.
// ====================================================================================================================
const char* CalcUnhash(int32 hashval)
{
    return UnHash(static_cast<int32>(hashval));
}

// ====================================================================================================================
// ContextPrintObject():  Find the object in the CScriptContext belonging to this thread, and debug print it's members.
// ====================================================================================================================
void ContextPrintObject(uint32 objectid)
{
    CScriptContext* script_context = TinScript::GetContext();
    CObjectEntry* oe = script_context->FindObjectEntry(objectid);
    script_context->PrintObject(oe);
}

// ====================================================================================================================
// ContextDebugBreak():  Cause the current thread's CScriptContext, and trigger an assert.  Used for debugging.
// ====================================================================================================================
void ContextDebugBreak(const char* msg)
{
    // -- force an assert on the executing thread
    CScriptContext* script_context = TinScript::GetContext();
    ScriptAssert_(script_context, false, "<internal>", -1, "Scripted DebugBreak()\n");
}

// ====================================================================================================================
// ContextListKeywords():  List the keywords defined in TinScript
// ====================================================================================================================
void ContextListKeywords(const char* partial_name)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (script_context == nullptr)
        return;

    TinPrint(script_context,"TinScript Keywords:\n");
    int32 keyword_count = 0;
    const char** keyword_list = GetReservedKeywords(keyword_count);
    for (int32 i = 0; i < keyword_count; ++i)
    {
        if(!partial_name || !partial_name[0] || SafeStrStr(keyword_list[i], partial_name) != 0)
        {
            TinPrint(script_context,"    %s\n", keyword_list[i]);
        }
    }
}

// ====================================================================================================================
// ContextListObjects():  List the objects registered to the current thread's CScriptContext.
// ====================================================================================================================
void ContextListObjects(const char* partial)
{
    TinScript::GetContext()->ListObjects(partial);
}

// ====================================================================================================================
// ContextIsObject():  Returns true, if the object is registered to the current thread's CScriptContext.
// ====================================================================================================================
bool8 ContextIsObject(uint32 objectid)
{
    CScriptContext* script_context = TinScript::GetContext();
    bool8 found = script_context->FindObject(objectid) != NULL;
    return found;
}

// ====================================================================================================================
// ContextFindObject(): Returns the object ID, for an object of the given name is found within
// the current thread's CScriptContext.
// ====================================================================================================================
uint32 ContextFindObject(const char* obj_name_or_id)
{
    CScriptContext* script_context = TinScript::GetContext();
    TinScript::CObjectEntry* oe = script_context->FindObjectByName(obj_name_or_id);
    if (oe == nullptr)
    {
        uint32 obj_id = TinScript::Atoi(obj_name_or_id);
        if (obj_id > 0)
            oe = script_context->FindObjectEntry(obj_id);
    }
    return oe ? oe->GetID() : 0;
}

// ====================================================================================================================
// ContextObjectIsDerivedFrom():  Returns true if the object has the given namespace in it's hierarchy.
// ====================================================================================================================
bool8 ContextObjectIsDerivedFrom(uint32 objectid, const char* requred_namespace)
{
    CScriptContext* script_context = TinScript::GetContext();
    void* objaddr = script_context->FindObject(objectid, requred_namespace);
    return (objaddr != NULL);
}

// ====================================================================================================================
// ContextObjectHasMethod():  Returns true if the object has an implementation for the given method.
// ====================================================================================================================
bool8 ContextObjectHasMethod(uint32 objectid, const char* method_name)
{
    CScriptContext* script_context = TinScript::GetContext();
    bool8 has_method = script_context->HasMethod(objectid, method_name);
    return (has_method);
}

// ====================================================================================================================
// ContextLinkNamespaces():  Link the child namespace to the parent, in the current thread's CScriptContext.
// ====================================================================================================================
void ContextLinkNamespaces(const char* childns, const char* parentns)
{
    CScriptContext* script_context = TinScript::GetContext();
    script_context->LinkNamespaces(childns, parentns);
}

// ====================================================================================================================
// ContextListVariables():  If an object id is given, dump it's hierarchy of members.
// Otherwise, dump the global variables in this thread's CScriptContext.
// ====================================================================================================================
void ContextListVariables(const char* partial)
{
    CScriptContext* script_context = TinScript::GetContext();
    TinScript::DumpVarTable(script_context, NULL, script_context->GetGlobalNamespace()->GetVarTable(), partial);
}

// ====================================================================================================================
// ContextIsVariable():  returns true if the global Variable has been defined
// ====================================================================================================================
bool8 ContextIsVariable(const char* name)
{
    if (name == nullptr || name[0] == '\0')
        return false;
    CScriptContext* script_context = TinScript::GetContext();
    return (script_context->GetGlobalNamespace()->GetVarTable()->FindItem(Hash(name)) != nullptr);
}

// ====================================================================================================================
// ContextListFunctions():  If a partial function name is provided, list all functions containing the partial.
// ====================================================================================================================
void ContextListFunctions(const char* partial)
{
    CScriptContext* script_context = TinScript::GetContext();
    TinScript::DumpFuncTable(script_context, script_context->GetGlobalNamespace()->GetFuncTable(), partial);
}

// ====================================================================================================================
// ContextIsFunction():  return true if the global function is defined
// ====================================================================================================================
bool8 ContextIsFunction(const char* name)
{
    return (TinScript::GetContext()->FunctionExists(name, ""));
}

// ====================================================================================================================
// ContextListNamespaces():  If an partial namespace is given, list all that contain it, otherwise list all
// ====================================================================================================================
void ContextListNamespaces(const char* partial_name)
{
    CScriptContext* script_context = TinScript::GetContext();
    CHashTable<CNamespace>* namespaces = script_context->GetNamespaceDictionary();
    if (namespaces != nullptr)
    {
        // -- we're going to sort them alphabetically...  no allocations, so use a big limit
        int namespace_count = 0;
        CNamespace* namespace_list[1024];
        CNamespace* current_namespace = namespaces->First();
        while (current_namespace != nullptr)
        {
            const char* current_name = current_namespace->GetName();
            if (current_name && current_name[0])
            {
                if  (!partial_name || !partial_name[0] || SafeStrStr(current_name, partial_name) != 0)
                {
                    namespace_list[namespace_count++] = current_namespace;
                    if (namespace_count >= 1024)
                    {
                        break;
                    }
                }       
            }
            current_namespace = namespaces->Next();
        }

        // -- sort the namespaces alphabetically
        if (namespace_count > 0)
        {
            auto namespace_sort = [](const void* a, const void* b) -> int
            {
                CNamespace* ns_a = *(CNamespace**)a;
                CNamespace* ns_b = *(CNamespace**)b;

                const char* name_a = ns_a != nullptr ? TinScript::UnHash(ns_a->GetHash()) : "";
                const char* name_b = ns_b != nullptr ? TinScript::UnHash(ns_b->GetHash()) : "";
                int result = _stricmp(name_a, name_b);
                return (result);
             };

            qsort(namespace_list, namespace_count, sizeof(CNamespace*), namespace_sort);
        }

        // -- print out the function names
        for (int i = 0; i < namespace_count; ++i)
        {
            TinPrint(script_context, "    %s\n", UnHash(namespace_list[i]->GetHash()));
        }
    }
}

// ====================================================================================================================
// ContextIsNamespaces():  see if the given name is a valid (instantiable) namespace
// ====================================================================================================================
bool8 ContextIsNamespace(const char* name)
{
    if (!name || !name[0])
        return false;
    CNamespace* ns = TinScript::GetContext()->FindNamespace(Hash(name));
    return (ns != nullptr);
}

// ====================================================================================================================
// ContextGetObjectNamespace():  Return the last (leaf) namespace in the hierarchy for the given object.
// ====================================================================================================================
const char* ContextGetObjectNamespace(uint32 objectid)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (objectid > 0)
    {
        TinScript::CObjectEntry* oe = script_context->FindObjectEntry(objectid);
        if (!oe) {
            return "";
        }
        else {
            return TinScript::UnHash(oe->GetNamespace()->GetHash());
        }
    }
    else {
        return "";
    }
}

// ====================================================================================================================
// ContextSaveObjects():  Save the entire tree hierarchy for an object, to the given filename.
// ====================================================================================================================
void ContextSaveObjects(uint32 objectid, const char* filename)
{
    CScriptContext* script_context = TinScript::GetContext();
    if (script_context)
        script_context->SaveObjectTree(objectid, filename);
}

// ====================================================================================================================
// ListSchedules():  Dump the pending scheduled requests for the current thread's CScriptContext.
// ====================================================================================================================
void ContextListSchedules()
{
    CScriptContext* script_context = TinScript::GetContext();
    script_context->GetScheduler()->Dump();
}

// ====================================================================================================================
// ContextScheduleCancel():  Cancel a pending scheduled request in the current thread's CScriptContext.
// ====================================================================================================================
void ContextScheduleCancel(int32 reqid)
{
    CScriptContext* script_context = TinScript::GetContext();
    script_context->GetScheduler()->CancelRequest(reqid);
}

// ====================================================================================================================
// ContextScheduleCancelObject():  Cancel all pending scheduled requests for an object, in the current CScriptContext.
// ====================================================================================================================
void ContextScheduleCancelObject(uint32 objectid)
{
    CScriptContext* script_context = TinScript::GetContext();
    script_context->GetScheduler()->CancelObject(objectid);
}

// ====================================================================================================================
// ContextSetAssertConnectTime():  On Assert, if the IDE is not connected, how long do we wait for a connection.
// ====================================================================================================================
void ContextSetAssertConnectTime(float in_time_sec)
{
    CScriptContext* script_context = TinScript::GetContext();
    script_context->SetAssertConnectTime(in_time_sec);
}

// ====================================================================================================================
// ContextSetAssertConnectTime():  On Assert, how deep to we append the callstack to the assert message
// ====================================================================================================================
void ContextSetAssertStackDepth(int32 depth)
{
    CScriptContext* script_context = TinScript::GetContext();
    script_context->SetAssertStackDepth(depth);
}

// == Registration ====================================================================================================

// -- these methods all wrap some call to a member of ScriptContext
// -- we want to execute the context created by the thread, in which this function is being called
REGISTER_FUNCTION(PrintObject, ContextPrintObject);
REGISTER_FUNCTION(DebugBreak, ContextDebugBreak);
REGISTER_FUNCTION(ListKeywords, ContextListKeywords);
REGISTER_FUNCTION(ListObjects, ContextListObjects);
REGISTER_FUNCTION(IsObject, ContextIsObject);
REGISTER_FUNCTION(FindObject, ContextFindObject);
REGISTER_FUNCTION(ObjectHasNamespace, ContextObjectIsDerivedFrom);
REGISTER_FUNCTION(ObjectHasMethod, ContextObjectHasMethod);
REGISTER_FUNCTION(LinkNamespaces, ContextLinkNamespaces);
REGISTER_FUNCTION(ListVariables, ContextListVariables);
REGISTER_FUNCTION(IsVariable, ContextIsVariable);
REGISTER_FUNCTION(ListGlobals, ContextListVariables);  // duplicate of ListVariables
REGISTER_FUNCTION(IsGlobal, ContextIsVariable);     // duplicate of IsVariable
REGISTER_FUNCTION(ListFunctions, ContextListFunctions);
REGISTER_FUNCTION(IsFunction, ContextIsFunction);
REGISTER_FUNCTION(ListNamespaces, ContextListNamespaces);
REGISTER_FUNCTION(IsNamespace, ContextIsNamespace);
REGISTER_FUNCTION(GetObjectNamespace, ContextGetObjectNamespace);
REGISTER_FUNCTION(SaveObjects, ContextSaveObjects);

REGISTER_FUNCTION(ListSchedules, ContextListSchedules);
REGISTER_FUNCTION(ScheduleCancel, ContextScheduleCancel);
REGISTER_FUNCTION(ScheduleCancelObject, ContextScheduleCancelObject);

REGISTER_FUNCTION(SetAssertConnectTime, ContextSetAssertConnectTime);
REGISTER_FUNCTION(SetAssertStackDepth, ContextSetAssertStackDepth);

// -- while technically not a context specific function, we need access to these functions anyways
REGISTER_FUNCTION(Hash, CalcHash);
REGISTER_FUNCTION(Unhash, CalcUnhash);

} // TinScript

// ====================================================================================================================
// EOF
// ====================================================================================================================
