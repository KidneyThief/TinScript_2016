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
// TinNamespace.cpp
// ====================================================================================================================

// -- lib includes
#include "stdio.h"

// -- includes
#include "TinNamespace.h"
#include "TinParse.h"
#include "TinScheduler.h"
#include "TinObjectGroup.h"
#include "TinStringTable.h"
#include "TinRegistration.h"

#include "registrationexecs.h"

// == namespace TinScript =============================================================================================

namespace TinScript
{

// --------------------------------------------------------------------------------------------------------------------
// -- statics
CNamespaceReg* CNamespaceReg::head = NULL;

// == class CObjectEntry ==============================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CObjectEntry::CObjectEntry(CScriptContext* script_context, uint32 _objid, uint32 _namehash,
                           CNamespace* _objnamespace, void* _objaddr, bool8 register_manual)
{
    mContextOwner = script_context;
    mObjectID = _objid;
    mNameHash = _namehash;
    mObjectNamespace = _objnamespace;
    mObjectAddr = _objaddr;
    mDynamicVariables = NULL;
    mGroupOwner = NULL;
    mManualRegister = register_manual;
    mIsDestroyed = false;
}

// ====================================================================================================================
// Destructor
// ====================================================================================================================
CObjectEntry::~CObjectEntry()
{
    if (mDynamicVariables)
   { 
        mDynamicVariables->DestroyAll();
        TinFree(mDynamicVariables);
    }
}

// ====================================================================================================================
// GetGroupID():  Returns the ID of the group owning this object
// ====================================================================================================================
uint32 CObjectEntry::GetGroupID() const
{
    CObjectEntry* group_oe = mContextOwner->FindObjectByAddress((void*)mGroupOwner);
    return (group_oe ? group_oe->GetID() : 0);
}

// ====================================================================================================================
// GetVariableEntry():  Search the linked list of namespaces, and the dynamically added list for a registered variable.
// ====================================================================================================================
CVariableEntry* CObjectEntry::GetVariableEntry(uint32 varhash)
{
    CVariableEntry* ve = NULL;
    CNamespace* objns = GetNamespace();
    while (objns && !ve) {
        ve = objns->GetVarTable()->FindItem(varhash);
        objns = objns->GetNext();
    }

    // -- if we weren't able to find the variable in the legitimate namespace hierarchy,
    // -- check the dynamic variables
    if (!ve && mDynamicVariables)
        ve = mDynamicVariables->FindItem(varhash);

    return (ve);
}

// ====================================================================================================================
// GetFunctionEntry():  Search the linked list of namespaces looking for a registered method.
// ====================================================================================================================
CFunctionEntry* CObjectEntry::GetFunctionEntry(uint32 nshash, uint32 funchash)
{
    CFunctionEntry* fe = NULL;
    CNamespace* objns = GetNamespace();
    while (!fe && objns)
    {
        if (nshash == 0 || objns->GetHash() == nshash)
            fe = objns->GetFuncTable()->FindItem(funchash);
        objns = objns->GetNext();
    }

    return fe;
}

// ====================================================================================================================
// CallHierarchicalFunction():  Execute the given method for the entire derived hierarchy of the given object.
// ====================================================================================================================
void CObjectEntry::CallFunctionHierarchy(uint32 function_hash, bool8 ascending)
{
    // -- if the call is ascending, we have to create the namespace stack, so we can drill down
    // -- and call from parent to child
    if (ascending)
    {
        int32 depth = 0;
        CNamespace* namespace_stack[256];
        CNamespace* obj_ns = GetNamespace();
        while (obj_ns)
        {
            namespace_stack[depth++] = obj_ns;
            obj_ns = obj_ns->GetNext();
        }

        // -- now that we have the stack, work backwards and execute the function (if it exists) at each level
        while (depth > 0)
        {
            obj_ns = namespace_stack[--depth];
            CFunctionEntry* fe = obj_ns->GetFuncTable()->FindItem(function_hash);
            if (fe)
            {
                int32 dummy = 0;
                if (!ObjExecNSMethod(GetID(), dummy, obj_ns->GetHash(), function_hash))
                {
                    ScriptAssert_(GetScriptContext(), 0, "<internal>", -1,
                                  "Error - [%d] Object method %s::%s() failed\n",
                                  GetID(), UnHash(obj_ns->GetHash()), UnHash(function_hash));
                }
            }
        }
    }

    // -- not ascending - simply call through the hierarchy directly
    else
    {
        CNamespace* obj_ns = GetNamespace();
        while (obj_ns)
        {
            CFunctionEntry* fe = obj_ns->GetFuncTable()->FindItem(function_hash);
            if (fe)
            {
                int32 dummy = 0;
                if (!ObjExecNSMethod(GetID(), dummy, obj_ns->GetHash(), function_hash))
                {
                    ScriptAssert_(GetScriptContext(), 0, "<internal>", -1,
                                    "Error - [%d] Object method %s::%s() failed\n",
                                    GetID(), UnHash(obj_ns->GetHash()), UnHash(function_hash));
                }
            }

            // -- next namespace
            obj_ns = obj_ns->GetNext();
        }
    }
}

// ====================================================================================================================
// HasNamespace():  Search the linked list to see if this object is "derived" from a given namespace.
// ====================================================================================================================
CNamespace* CObjectEntry::HasNamespace(uint32 nshash)
{
    if (nshash == 0)
        return (NULL);
    CNamespace* objns = GetNamespace();
    while (objns && objns->GetHash() != nshash)
        objns = objns->GetNext();
    return objns;
}

// ====================================================================================================================
// AddDynamicVariable():  Add a dynamic variable to this object.
// ====================================================================================================================
bool8 CObjectEntry::AddDynamicVariable(uint32 varhash, eVarType vartype, int32 array_size)
{
    // -- sanity check
    if (varhash == 0 || vartype < FIRST_VALID_TYPE)
        return (false);

    // -- see if the variable already exists
    CVariableEntry* ve = GetVariableEntry(varhash);

    // -- if we do, it had better be the same type
    if (ve)
    {
        if (ve->GetType() != vartype)
        {
            ScriptAssert_(GetScriptContext(), 0, "<internal>", -1,
                          "Error - Variable already exists: %s, type: %s\n",
                          UnHash(varhash), GetRegisteredTypeName(ve->GetType()));
            return (false);
        }
        return (true);
    }

    // -- ensure we have a dictionary to hold the dynamic tags
    if (!mDynamicVariables)
    {
        mDynamicVariables = TinAlloc(ALLOC_HashTable, CHashTable<CVariableEntry>,
                                    kLocalVarTableSize);
    }

	ve = TinAlloc(ALLOC_VarEntry, CVariableEntry, GetScriptContext(), UnHash(varhash), varhash,
                  vartype, array_size, false, 0, true);
	mDynamicVariables->AddItem(*ve, varhash);

    return (ve != NULL);
}

// ====================================================================================================================
// SetMemberVar():  Set the value of an object member.
// ====================================================================================================================
bool8 CObjectEntry::SetMemberVar(uint32 varhash, void* value)
{
    if (!value)
        return (false);

    // -- find the variable
    CVariableEntry* ve = GetVariableEntry(varhash);
    if (!ve)
    {
        ScriptAssert_(GetScriptContext(), 0, "<internal>", -1,
                      "Error - Unable to find variable %s for object %d\n",
                      UnHash(varhash), GetID());
        return (false);
    }

    // -- set the value
    ve->SetValue(GetAddr(), value);
    return (true);
}

// ====================================================================================================================
// FindOrCreateNamespace():  Find a namespace by name, create if required.
// ====================================================================================================================
CNamespace* CScriptContext::FindOrCreateNamespace(const char* _nsname, bool8 create)
{
    // -- ensure the name lives in the string table
    const char* nsname = _nsname && _nsname[0] ? GetStringTable()->AddString(_nsname)
                                               : GetStringTable()->AddString(kGlobalNamespace);
    uint32 nshash = Hash(nsname);
    CNamespace* namespaceentry = mNamespaceDictionary->FindItem(nshash);
    if (!namespaceentry && create) {
        namespaceentry = TinAlloc(ALLOC_Namespace, CNamespace, this, nsname, 0, nullptr);

        // -- add the namespace to the dictionary
        mNamespaceDictionary->AddItem(*namespaceentry, nshash);
    }

    return namespaceentry;
}

// ====================================================================================================================
// FindNamespace():  Find a namespace by hash.
// ====================================================================================================================
CNamespace* CScriptContext::FindNamespace(uint32 nshash)
{
    if (nshash == 0)
        nshash = Hash(kGlobalNamespace);
    CNamespace* namespaceentry = mNamespaceDictionary->FindItem(nshash);
    return (namespaceentry);
}

// ====================================================================================================================
// LinkNamespaces():  Link a child namespace to a parent, creating a hierarchy.
// ====================================================================================================================
bool8 CScriptContext::LinkNamespaces(const char* childnsname, const char* parentnsname)
{
    // sanity check
    if (!childnsname || !childnsname[0] || !parentnsname || !parentnsname[0])
        return (false);

    // -- ensure the child exists and the parent exists
    TinScript::CNamespace* childns = FindOrCreateNamespace(childnsname, true);
    TinScript::CNamespace* parentns = FindOrCreateNamespace(parentnsname, true);
    return (LinkNamespaces(childns, parentns));
}

// ====================================================================================================================
// LinkNamespaces():  Link a child namespace to a parent, creating a hierarchy.
// ====================================================================================================================
bool8 CScriptContext::LinkNamespaces(CNamespace* childns, CNamespace* parentns)
{
    // -- sanity check
    if (!childns || !parentns || parentns == childns)
        return (false);

    if (childns->GetNext() == NULL)
    {
        // -- verify the parent is not already in the hierarchy, or we'll have a circular list
        CNamespace* tempns = parentns;
        while (tempns)
        {
            if (tempns == childns)
            {
                ScriptAssert_(this, 0, "<internal>", -1,
                             "Error - attempting to link namespace %s to %s, which is already its child\n",
                             UnHash(childns->GetHash()), UnHash(parentns->GetHash()));
                return (false);
            }
            tempns = tempns->GetNext();
        }

        // -- nothing found in the hierarchy - go ahead and link
        childns->SetNext(parentns);
        return (true);
    }

    // -- child is already linked - see if the new parent is already in the hierarchy
    else
    {
        CNamespace* tempns = childns->GetNext();
        while (tempns)
        {
            if (tempns == parentns)
                return (true);
            else
                tempns = tempns->GetNext();
        }

        // -- not found in the hierarchy already - we need see then if the current parent
        // -- is in the hierarchy of the new parent
        bool8 found = false;
        CNamespace* oldparent = childns->GetNext();
        tempns = parentns;
        while (tempns)
        {
            if (tempns == oldparent)
            {
                found = true;
                break;
            }
            tempns = tempns->GetNext();
        }

        // -- if it was found, link the namespaces and exit
        if (found)
        {
            childns->SetNext(parentns);
            return (true);
        }

        // -- not found in the hierarchy - we can insert the new parent into the hierarchy
        // -- *only* if the order is deterministic - which means one must be a registered
        // -- class, and one must be a script class
        if (parentns->GetNext() == NULL)
        {
            // -- if the parent is *not* a registered class, then it goes at the end of the
            // -- child's hierarchy, as long as the child's hierarchy is still purely script
            bool found_registered_class = false;
            if (parentns->IsRegisteredClass() && !childns->IsRegisteredClass())
            {
                tempns = childns;
                while (tempns->GetNext())
                {
                    if (tempns->GetNext()->IsRegisteredClass())
                    {
                        found_registered_class = true;
                        break;
                    }

                    // -- get the next
                    tempns = tempns->GetNext();
                }

                // -- see if we can append to the hierarchy
                if (!found_registered_class)
                {
                    tempns->SetNext(parentns);
                    return (true);
                }
            }

            // -- else if the parent is not a registered class, then it gets inserted at the front of the
            // -- current child hierarchy
            else if (!parentns->IsRegisteredClass())
            {
                parentns->SetNext(childns->GetNext());
                childns->SetNext(parentns);
            }
            return (true);
        }
    }

    // -- not found in the hierarchy - assert
    ScriptAssert_(this, 0, "<internal>", -1,
                  "Error - attempting to link namespace %s to %s, already linked to %s\n",
                  UnHash(childns->GetHash()), UnHash(parentns->GetHash()),
                  UnHash(childns->GetNext()->GetHash()));

    // -- failed
    return (false);
}

// ====================================================================================================================
// FunctionExists():  Returns true if the function entry exists, in the given namespace if it exists.
// ====================================================================================================================
bool8 CScriptContext::FunctionExists(uint32 func_hash, uint32 ns_hash)
{
    // -- if a namespace was given, find it
    CNamespace* ns = ns_hash != 0 ? mNamespaceDictionary->FindItem(ns_hash) : GetGlobalNamespace();
    if (ns == nullptr)
        return (false);

    // -- see if the namespace has a function entry for the hash
    if (func_hash == 0 || !ns->GetFuncTable() || ns->GetFuncTable()->FindItem(func_hash) == nullptr)
        return (false);

    // -- success
    return (true);
}

// ====================================================================================================================
// FunctionExists():  Returns true if the function entry exists, in the given namespace if it exists.
// ====================================================================================================================
bool8 CScriptContext::FunctionExists(const char* function_name, const char* ns_name)
{
    return (FunctionExists(Hash(function_name), Hash(ns_name)));
}

// ====================================================================================================================
// GetNextObjectID():  Generate the object ID for the next object registered.
// ====================================================================================================================
uint32 CScriptContext::GetNextObjectID()
{
    // -- every object created gets a unique ID, so we can find it in the object dictionary
    // -- providing a way to register code-instantiated objects
    return ++mObjectIDGenerator;
}

// ====================================================================================================================
// CreateObject():  Create an object instance, given the hash of the class to use, and the hash of the object's name.
// ====================================================================================================================
uint32 CScriptContext::CreateObject(uint32 classhash, uint32 objnamehash, const CFunctionCallStack* funccallstack)
{
    uint32 objectid = GetNextObjectID();

    // -- find the creation function
    CNamespace* namespaceentry = GetNamespaceDictionary()->FindItem(classhash);
    if (namespaceentry != NULL)
    {
        // -- loop down the hierarchy, until you find the actual registered class namespace
        CNamespace* class_namespace = namespaceentry;
        while (class_namespace && !class_namespace->IsRegisteredClass())
            class_namespace = class_namespace->GetNext();

        // -- if we get all the way down and don't find a registered class, assume the CScriptObject base class,
        // -- and the hierarchy of linked namespaces will uncover any errors
        if (!class_namespace)
        {
            class_namespace = GetNamespaceDictionary()->FindItem(Hash("CScriptObject"));
            if (class_namespace)
            {
                LinkNamespaces(namespaceentry, class_namespace);
                TinPrint(this, "Warning - CreateObject():  Unable to find registered class %s.\n"
                               "Linking to default base class CScriptObject\n", UnHash(classhash));
            }
        }

        CNamespace::CreateInstance funcptr = class_namespace ? class_namespace->GetCreateInstance() : NULL;
        if (funcptr == NULL)
        {
            ScriptAssert_(this, 0, "<internal>", -1,
                          "Error - Class is not registered: %s\n", UnHash(classhash));
            return 0;
        }

        // -- create the object
        void* newobj = (*funcptr)();

        // -- see if we can hook this object up to the namespace for it's object name
        CNamespace* objnamens = namespaceentry;
        if (objnamehash != 0)
        {
            objnamens = GetNamespaceDictionary()->FindItem(objnamehash);
            if (!objnamens)
                objnamens = namespaceentry;
            else
            {
                // -- link the namespaces
                LinkNamespaces(objnamens, namespaceentry);
            }
        }

        // -- need to verify that if we're using an objnamens (scripted), that the namespaceentry
        // -- is the highest level registered class
        if (objnamens != namespaceentry)
        {
            CNamespace* tempns = objnamens;
            while (tempns && tempns->GetCreateInstance() == NULL)
                tempns = tempns->GetNext();

            // -- if we run out of namespaces... how'd we create this object?
            if (!tempns)
            {
                ScriptAssert_(this, 0, "<internal>", -1,
                              "Error - Unable to verify hierarchy for namespace: %s\n",
                              UnHash(objnamens->GetHash()));
                // $$$TZA find a way to delete the newly created, but non-registered object
                //delete newobj;
                return 0;
            }
            else if (tempns != class_namespace)
            {
                ScriptAssert_(this, 0, "<internal>", -1,
                              "Error - Unable to create an instance of base class: %s, using object namespace: %s.\n"
                              "Use derived class: %s\n",
                              UnHash(class_namespace->GetHash()), UnHash(objnamehash), UnHash(tempns->GetHash()));
                // $$$TZA find a way to delete the newly created, but non-registered object
                //delete newobj;
                return 0;
            }
        }

        // -- add this object to the dictionary of all objects created from script
        CObjectEntry* newobjectentry = TinAlloc(ALLOC_ObjEntry, CObjectEntry, this,
                                                objectid, objnamehash, objnamens, newobj, false);
        GetObjectDictionary()->AddItem(*newobjectentry, objectid);

        // -- add the object to the dictionary by address
        GetAddressDictionary()->AddItem(*newobjectentry, kPointerToUInt32(newobj));

        // -- if the item is named, add it to the name dictionary
        // $$$TZA Note:  names are not guaranteed unique...  warn?
        if (objnamehash != Hash(""))
            GetNameDictionary()->AddItem(*newobjectentry, objnamehash);

#if MEMORY_TRACKER_ENABLE
        // -- used by the memory tracker (if enabled)
        TinObjectCreated(objectid, funccallstack);
#endif

        // -- notify the debugger of the new object (before we call OnCreate(), as that may add the object to a set)
        DebuggerNotifyCreateObject(newobjectentry);

        // -- "OnCreate" is the equivalent of a constructor - we want to call every OnCreate
        // -- from the bottom of the hierarchy to the highest derivation for which it is defined
        // -- NOTE:  it is not required to be defined for any level
        newobjectentry->CallFunctionHierarchy(Hash("OnCreate"), true);

        return (objectid);
    }
    else
    {
        ScriptAssert_(this, 0, "<internal>", -1,
                      "Error - Class is not registered: %s\n", UnHash(classhash));
        return 0;
    }
}

// ====================================================================================================================
// RegisterObject():  Given the address of an object instantiated outside TinScript, register it independently.
// ====================================================================================================================
uint32 CScriptContext::RegisterObject(void* objaddr, const char* classname, const char* objectname)
{
    // -- sanity check
    if (!objaddr || !classname || !classname[0])
        return 0;

    // -- ensure the classname is for an existing registered class
    uint32 nshash = Hash(classname);
    CNamespace* namespaceentry = FindNamespace(nshash);
    if (!namespaceentry) {
        ScriptAssert_(this, 0, "<internal>", -1,
                       "Error - Class is not registered: %s\n", classname);
        return 0;
    }

    uint32 objectid = GetNextObjectID();

        // -- see if we can hook this object up to the namespace for it's object name
    uint32 objnamehash = objectname ? Hash(objectname) : 0;
    CNamespace* objnamens = namespaceentry;
    if (objnamehash != 0) {
        objnamens = GetNamespaceDictionary()->FindItem(objnamehash);
        if (!objnamens)
            objnamens = namespaceentry;
        else {
            // -- link the namespaces
            LinkNamespaces(objnamens, namespaceentry);
        }
    }

    // -- add this object to the dictionary of all objects created from script
    CObjectEntry* newobjectentry = TinAlloc(ALLOC_ObjEntry, CObjectEntry, this,
                                            objectid, objnamehash, objnamens, objaddr, true);
    GetObjectDictionary()->AddItem(*newobjectentry, objectid);

    // -- add the object to the dictionary by address
    GetAddressDictionary()->AddItem(*newobjectentry, kPointerToUInt32(objaddr));

    // -- if the item is named, add it to the name dictionary
    if (objnamehash != Hash("")) {
        GetNameDictionary()->AddItem(*newobjectentry, objnamehash);
    }

    // -- notify the debugger of the new object (before we call OnCreate(), as that may add the object to a set)
    DebuggerNotifyCreateObject(newobjectentry);

    // -- "OnCreate" is the equivalent of a constructor - we want to call every OnCreate
    // -- from the bottom of the hierarchy to the highest derivation for which it is defined
    // -- NOTE:  it is not required to be defined for any level
    newobjectentry->CallFunctionHierarchy(Hash("OnCreate"), true);

    return objectid;
}

// ====================================================================================================================
// UnregisterObject():  Used to remove all TinScript references to an object instantiated outside the system.
// ====================================================================================================================
void CScriptContext::UnregisterObject(void* objaddr)
{
    // -- find the ID of the registered object
    uint32 objectid = FindIDByAddress(objaddr);
    if (objectid == 0)
        return;

    // -- destroy the object (will only remove from dictionaries and call ::OnDestroy())
    // -- registered objects are new'd from code, and must be delete'd as well
    DestroyObject(objectid);
}

// ====================================================================================================================
// DestroyObject():  Destroy an object, calls the OnDestroy() method, and deletes it.
// ====================================================================================================================
void CScriptContext::DestroyObject(uint32 objectid)
{
    // -- find this object in the dictionary of all objects created from script
    CObjectEntry* oe = GetObjectDictionary()->FindItem(objectid);
    if (!oe)
    {
        ScriptAssert_(this, 0, "<internal>", -1,
                      "Error - Unable to find object: %d\n", objectid);
        return;
    }

    // -- guard against re-entrant destruction.
    // -- Happens if ::Unregister() is called from code, causing "OnDestroy()" to be called,
    // -- which may contain a call back to code which leads to the re-entrant ::Unregister()
    if (oe->IsDestroyed())
        return;
    oe->SetDestroyed();

    // -- notify the master membership list to remove it from all groups
    GetMasterMembershipList()->OnDelete(oe);

    // -- get the namespace entry for the object
    CNamespace* namespaceentry = oe->GetNamespace();
    if (!namespaceentry)
    {
        ScriptAssert_(this, 0, "<internal>", -1,
                      "Error - Unable to find the namespace for object: %d\n", objectid);
        return;
    }

    // -- get the Destroy function
    CNamespace::DestroyInstance destroyptr = namespaceentry->GetDestroyInstance();
    if (destroyptr == NULL)
    {
        ScriptAssert_(this, 0, "<internal>", -1,
                      "Error - no Destroy() function registered for class: %s\n",
                      UnHash(namespaceentry->GetHash()));
        return;
    }

    // -- "OnDestroy" is the equivalent of a destructor - we want to call every OnDestroy
    // -- from the top of the hierarchy through to the root base implementation
    // -- NOTE:  it is not required to be defined for any level
    oe->CallFunctionHierarchy(Hash("OnDestroy"), false);

    // -- get the address of the object
    void* objaddr = oe->GetAddr();
    if (!objaddr)
    {
        ScriptAssert_(this, 0, "<internal>", -1,
                      "Error - no address for object: %d\n", objectid);
        return;
    }

    // -- cancel all pending schedules related to this object
    GetScheduler()->CancelObject(objectid);

    // -- if the object was not registered externally, delete the actual object
    if (!oe->IsManuallyRegistered())
        (*destroyptr)(objaddr);

    // -- notify the debugger, after the destructor has had a chance to send "RemoveFromSet" notifications
    DebuggerNotifyDestroyObject(objectid);

    // -- remove the object from the dictionary, and delete the entry
    GetObjectDictionary()->RemoveItem(objectid);
    GetAddressDictionary()->RemoveItem(kPointerToUInt32(objaddr));
    GetNameDictionary()->RemoveItem(oe, oe->GetNameHash());

    // -- delete the object entry *after* the object
    TinFree(oe);
}

// ====================================================================================================================
// FindObjectEntry():  Finds an object, given the ID.
// ====================================================================================================================
CObjectEntry* CScriptContext::FindObjectEntry(uint32 objectid)
{
    // -- note:  on shutdown, there may not be an object dictionary anymore
    CObjectEntry* oe = objectid != 0 && GetObjectDictionary() ? GetObjectDictionary()->FindItem(objectid) : nullptr;
    return oe;
}

// ====================================================================================================================
// FindObjectByAddress():  Find an entry, given the address of an object registered but instantiated outside TinScript.
// ====================================================================================================================
CObjectEntry* CScriptContext::FindObjectByAddress(void* addr)
{
    CObjectEntry* oe = GetAddressDictionary()->FindItem(kPointerToUInt32(addr));
    return oe;
}

// ====================================================================================================================
// FindObjectByName():  Find an object entry, given it's name.
// ====================================================================================================================
CObjectEntry* CScriptContext::FindObjectByName(const char* objname)
{
    if (!objname || !objname[0])
        return (NULL);
    CObjectEntry* oe = GetNameDictionary()->FindItem(Hash(objname));
    return oe;
}

// ====================================================================================================================
// FindIDByAddress():  Find an object entry, give the address, and return the ID.
// ====================================================================================================================
uint32 CScriptContext::FindIDByAddress(void* addr)
{
    CObjectEntry* oe = GetAddressDictionary()->FindItem(kPointerToUInt32(addr));
    return oe ? oe->GetID() : 0;
}

// ====================================================================================================================
// FindObject():  Find an object by ID, but return NULL if a required namespace is not part of its hierarchy.
// ====================================================================================================================
void* CScriptContext::FindObject(uint32 objectid, const char* required_namespace) {
    CObjectEntry* oe = GetObjectDictionary()->FindItem(objectid);
    if (oe && (!required_namespace || !required_namespace[0] ||
              oe->HasNamespace(Hash(required_namespace))))
    {
        return (oe->GetAddr());
    }
    return (NULL);
}

// ====================================================================================================================
// HasMethod():  Return 'true' if there's a registered or scripted method in the hierarchy of an object.
// ====================================================================================================================
bool8 CScriptContext::HasMethod(void* addr, const char* method_name)
{
    if (!addr || !method_name)
        return (false);

    CObjectEntry* oe = GetAddressDictionary()->FindItem(kPointerToUInt32(addr));
    if (!oe)
        return (false);

    uint32 function_hash = Hash(method_name);
    CFunctionEntry* fe = oe->GetFunctionEntry(0, function_hash);
    return (fe != NULL);
}

// ====================================================================================================================
// HasMethod():  Return 'true' if there's a registered or scripted method in the hierarchy of an object.
// ====================================================================================================================
bool8 CScriptContext::HasMethod(uint32 objectid, const char* method_name)
{
    if (!method_name)
        return (false);

    CObjectEntry* oe = GetObjectDictionary()->FindItem(objectid);
    if (!oe)
        return (false);

    uint32 function_hash = Hash(method_name);
    CFunctionEntry* fe = oe->GetFunctionEntry(0, function_hash);
    return (fe != NULL);
}

// ====================================================================================================================
// HasMember():  Return 'true' if there's a registered or scripted member in the hierarchy of an object.
// ====================================================================================================================
bool8 CScriptContext::HasMember(void* addr, const char* member_name)
{
    if (!addr || !member_name)
        return (false);

    CObjectEntry* oe = GetAddressDictionary()->FindItem(kPointerToUInt32(addr));
    if (!oe)
        return (false);

    uint32 member_hash = Hash(member_name);
    CVariableEntry* ve = oe->GetVariableEntry(member_hash);
    return (ve != NULL);
}

// ====================================================================================================================
// HasMethod():  Return 'true' if there's a registered or scripted member in the hierarchy of an object.
// ====================================================================================================================
bool8 CScriptContext::HasMember(uint32 objectid, const char* member_name)
{
    if (!member_name)
        return (false);

    CObjectEntry* oe = GetObjectDictionary()->FindItem(objectid);
    if (!oe)
        return (false);

    uint32 member_hash = Hash(member_name);
    CVariableEntry* ve = oe->GetVariableEntry(member_hash);
    return (ve != NULL);
}

// ====================================================================================================================
// AddDynamicVariable():  Given an object id, add a variable (or array) of a given type.
// ====================================================================================================================
bool8 CScriptContext::AddDynamicVariable(uint32 objectid, uint32 varhash, eVarType vartype, int32 array_size)
{
    CObjectEntry* oe = GetObjectDictionary()->FindItem(objectid);
    if (!oe) {
        ScriptAssert_(this, 0, "<internal>", -1,
                      "Error - Unable to find object: %d\n", objectid);
        return (false);
    }
    return (oe->AddDynamicVariable(varhash, vartype, array_size));
}

// ====================================================================================================================
// SetMemberVar():  Given an object id, and the name of a member, set the value.
// ====================================================================================================================
bool8 CScriptContext::SetMemberVar(uint32 objectid, const char* varname, void* value)
{
    if (!varname || !value)
    {
        ScriptAssert_(this, 0, "<internal>", -1,
                      "Error - invalid call to SetMemberVar\n");
        return (false);
    }

    CObjectEntry* oe = GetObjectDictionary()->FindItem(objectid);
    if (!oe)
    {
        ScriptAssert_(this, 0, "<internal>", -1,
                      "Error - Unable to find object: %d\n", objectid);
        return (false);
    }

    uint32 varhash = Hash(varname);
    return oe->SetMemberVar(varhash, value);
}

// ====================================================================================================================
// PrintObject():  Debug method to go through the hierarchy of namespaces, and dump the members/values.
// ====================================================================================================================
void CScriptContext::PrintObject(CObjectEntry* oe, int32 indent)
{
    if (!oe)
        return;

    // -- find the actual class
    CNamespace* classns = oe->GetNamespace();
    while (classns && !classns->IsRegisteredClass())
        classns = classns->GetNext();
    if (!classns)
    {
        ScriptAssert_(this, 0, "<internal>", -1,
                      "Error - Registered object with no class: [%d] %s\n",
                      oe->GetID(), oe->GetName());
        return;
    }

    // -- print the indent
    const char* indentbuf = "    ";
    for (int32 i = 0; i < indent; ++i)
        TinPrint(this, indentbuf);

    // -- print the object id and name
    TinPrint(this, "[%d] %s:", oe->GetID(), oe->GetName());
    bool8 first = true;
    CNamespace* ns = oe->GetNamespace();
    while (ns)
    {
        // -- if this is registered class, highlight it
        if (ns->IsRegisteredClass())
        {
            TinPrint(this, "%s[%s]", !first ? "-->" : " ", UnHash(ns->GetHash()));
        }
        else
        {
            TinPrint(this, "%s%s", !first ? "-->" : " ", UnHash(ns->GetHash()));
        }
        first = false;
        ns = ns->GetNext();
    }

    TinPrint(this, "\n");
}

// ====================================================================================================================
// ListObjects():  Debug method to list all registered objects.
// ====================================================================================================================
void CScriptContext::ListObjects(const char* partial)
{
    // -- sanity check
    if (partial == nullptr)
        partial = "";
    bool use_partial = partial[0] != '\0';

    TinPrint(this, "\n");
    CObjectEntry* oe = GetObjectDictionary()->First();
    while (oe)
    {
        // -- if the object has a parent group, don't bother printing it - it'll have already
        // -- been printed by its parent group
        // note:  if we're searching by partial name, we don't print hierarchies at all 
        if (use_partial || oe->GetObjectGroup() == NULL)
        {
            if (!use_partial || SafeStrStr(oe->GetName(), partial) != 0)
                PrintObject(oe);
        }

        // -- if the object itself is a group, list it's children
        if (!use_partial && HasMethod(oe->GetID(), "ListObjects"))
        {
            int32 dummy = 0;
            ObjExecF(oe->GetID(), dummy, "ListObjects(1);");
        }

        // -- next object
        oe = GetObjectDictionary()->Next();
    }
}

// ====================================================================================================================
// ExportFormattedValue():  Converts a value into an exported format that can be restored, based on type.
// ====================================================================================================================
const char* CScriptContext::ExportFormattedValue(eVarType type, void* addr)
{
    char* convertbuf = GetScratchBuffer();
    char* formatbuf = GetScratchBuffer();
	gRegisteredTypeToString[type](this, addr, convertbuf, kMaxTokenLength);

    // -- no change for int, float, or bool
    if (type == TYPE_int || type == TYPE_float || type== TYPE_bool)
        return (convertbuf);

    // -- object IDs, for export, are actually object variables, who's name contains the ID at the time of export
    else if (type == TYPE_object)
    {
        sprintf_s(formatbuf, kMaxTokenLength, "obj_%s", convertbuf);
        return (formatbuf);
    }

    // -- everything else is enquoted
    else
    {
        sprintf_s(formatbuf, kMaxTokenLength, "`%s`", convertbuf);
        return (formatbuf);
    }
}

// ====================================================================================================================
// ExportObjectMember():  Write out the variable entry of an object.
// ====================================================================================================================
bool8 CScriptContext::ExportObjectMember(CObjectEntry* oe, CVariableEntry* ve, FILE* filehandle)
{
    // -- write the variable
    if (ve->IsArray())
    {
        if (!FileWritef(filehandle, "    %s[%d] obj_%d.%s;", GetRegisteredTypeName(ve->GetType()),
                                    ve->GetArraySize(), oe->GetID(), UnHash(ve->GetHash())))
        {
            return (false);
        }

        // -- write the array members
        for (int i = 0; i < ve->GetArraySize(); ++i)
        {
            // -- format the string for an array entry
            void* addr = ve->GetArrayVarAddr(oe->GetAddr(), i);
            if (!FileWritef(filehandle, "    %s obj_%d.%s[%d] = %s;",
                                        GetRegisteredTypeName(ve->GetType()), oe->GetID(),
                                        UnHash(ve->GetHash()), i, ExportFormattedValue(ve->GetType(), addr)))
            {
                return (false);
            }
        }
    }
    else
    {
        void* addr = ve->GetValueAddr(oe->GetAddr());
        if (!FileWritef(filehandle, "    %s obj_%d.%s = %s;", GetRegisteredTypeName(ve->GetType()),
                                    oe->GetID(), UnHash(ve->GetHash()), ExportFormattedValue(ve->GetType(), addr)))
        {
            return (false);
        }
    }

    // -- success
    return (true);
}

// ====================================================================================================================
// ExportObjectVarTable():  Write out a var table in the hierarchy of namespaces for the given object.
// ====================================================================================================================
bool8 CScriptContext::ExportObjectVarTable(CObjectEntry* oe, tVarTable* var_table, FILE* filehandle)
{
    CVariableEntry* ve = var_table->First();
    while (ve)
    {
        if (!ExportObjectMember(oe, ve, filehandle))
            return (false);
        ve = var_table->Next();
    }

    // -- success
    return (true);
}

// ====================================================================================================================
// ExportObjectCreate():  Writes out the create statement for a given object
// ====================================================================================================================
bool8 CScriptContext::ExportObjectCreate(CObjectEntry* oe, FILE* filehandle, char* indent_buf)
{
    // -- first write this object create statement
    if (!FileWritef(filehandle, "%sobject obj_%d = create %s('%s');", indent_buf, oe->GetID(),
                                UnHash(oe->GetNamespace()->GetHash()),
                                oe->GetNameHash() != 0 ? UnHash(oe->GetNameHash()) : ""))
    {
        return (false);
    }

    // -- success
    return (true);
}


// ====================================================================================================================
// ExportObjectMembers():  Writes out the member initialization statements for the given object
// ====================================================================================================================
bool8 CScriptContext::ExportObjectMembers(CObjectEntry* oe, FILE* filehandle)
{
    // -- write the lead commend
    if (!FileWritef(filehandle, "    // -- object obj_%d member initialization", oe->GetID()))
    {
        return (false);
    }

    // -- write the hierarchy of members
    // -- send the dynamic var table
    if (oe->GetDynamicVarTable())
    {
        if (!ExportObjectVarTable(oe, oe->GetDynamicVarTable(), filehandle))
            return (false);
    }

    // -- loop through the hierarchy of namespaces
    CNamespace* ns = oe->GetNamespace();
    while (ns)
    {
        if (!ExportObjectVarTable(oe, ns->GetVarTable(), filehandle))
            return (false);

        // -- get the next namespace
        ns = ns->GetNext();
    }

    // -- blank line after every object's member pass
    if (!FileWritef(filehandle, ""))
        return (false);

    // -- success
    return (true);
}

// ====================================================================================================================
// ExportObjectTreeCreate():  Writes out the create statements for a hierarchy of objects.
// ====================================================================================================================
bool8 CScriptContext::ExportObjectTreeCreate(CObjectEntry* oe, FILE* filehandle, char* indent_buf)
{
    if (!ExportObjectCreate(oe, filehandle, indent_buf))
    {
        ScriptAssert_(this, 0, "<internal>", -1, "Error - ExportObjectTree(%d) failed\n", oe->GetID());
        return (false);
    }

    // -- if this object is a group (specifically, not a set)
    static uint32 object_group_hash = Hash("CObjectGroup");
    if (oe->HasNamespace(object_group_hash))
    {
        // -- bump the index
        int32 indent = (int32)strlen(indent_buf);
        indent_buf[indent++] = ' ';
        indent_buf[indent++] = ' ';
        indent_buf[indent++] = ' ';
        indent_buf[indent++] = ' ';
        indent_buf[indent] = '\0';

        CObjectGroup* group = static_cast<CObjectGroup*>(FindObject(oe->GetID()));
        uint32 child_id = group->First();
        CObjectEntry* child_oe = FindObjectEntry(child_id);
        while (child_oe)
        {
            // -- dump out this child's tree
            ExportObjectTreeCreate(child_oe, filehandle, indent_buf);

            // -- next child
            child_id = group->Next();
            child_oe = FindObjectEntry(child_id);
        }

        // -- retract the indent
        indent -= 4;
        indent_buf[indent] = '\0';
    }

    // -- success
    return (true);
}

// ====================================================================================================================
// ExportObjectTreeMembers():  Writes out the member statements for a hierarchy of objects.
// ====================================================================================================================
bool8 CScriptContext::ExportObjectTreeMembers(CObjectEntry* oe, FILE* filehandle)
{
    if (!ExportObjectMembers(oe, filehandle))
    {
        ScriptAssert_(this, 0, "<internal>", -1, "Error - ExportObjectMembers(%d) failed\n", oe->GetID());
        return (false);
    }

    // -- if this object is a group (specifically, not a set)
    static uint32 object_group_hash = Hash("CObjectGroup");
    if (oe->HasNamespace(object_group_hash))
    {
        CObjectGroup* group = static_cast<CObjectGroup*>(FindObject(oe->GetID()));
        uint32 child_id = group->First();
        CObjectEntry* child_oe = FindObjectEntry(child_id);
        while (child_oe)
        {
            // -- dump out this child's tree
            if (!ExportObjectTreeMembers(child_oe, filehandle))
                return (false);

            // -- next child
            child_id = group->Next();
            child_oe = FindObjectEntry(child_id);
        }
    }

    // -- success
    return (true);
}

// ====================================================================================================================
// ExportObjectTreeHierarchy():  Writes out the AddObject() statements to restore the hierarchy.
// ====================================================================================================================
bool8 CScriptContext::ExportObjectTreeHierarchy(CObjectEntry* oe, FILE* filehandle, char* indent_buf)
{
    // -- if this object is a set (re-creating the hierarchy includes sets)
    static uint32 object_set_hash = Hash("CObjectSet");
    if (oe->HasNamespace(object_set_hash))
    {
        CObjectSet* set = static_cast<CObjectSet*>(FindObject(oe->GetID()));
        uint32 child_id = set->First();
        CObjectEntry* child_oe = FindObjectEntry(child_id);
        while (child_oe)
        {
            // -- export the AddObject() statement
            if (!FileWritef(filehandle, "%sobj_%d.AddObject(obj_%d);", indent_buf, oe->GetID(), child_id))
                return (false);

            // -- if this child is a set, (not a group), we add it's children
            if (child_oe->HasNamespace(object_set_hash))
            {
                // -- bump the index
                int32 indent = (int32)strlen(indent_buf);
                indent_buf[indent++] = ' ';
                indent_buf[indent++] = ' ';
                indent_buf[indent++] = ' ';
                indent_buf[indent++] = ' ';
                indent_buf[indent] = '\0';

                // -- dump out this child's AddObject() statements
                if (!ExportObjectTreeHierarchy(child_oe, filehandle, indent_buf))
                    return (false);

                // -- retract the indent
                indent -= 4;
                indent_buf[indent] = '\0';
            }

            // -- next child
            child_id = set->Next();
            child_oe = FindObjectEntry(child_id);
        }
    }

    // -- success
    return (true);
}

// ====================================================================================================================
// SaveObjectTree():  Writes out the entire hierarchy object (or all objects) to the given filename
// ====================================================================================================================
bool8 CScriptContext::SaveObjectTree(uint32 object_id, const char* savefilename)
{
    // -- ensure we have a filename
    if (!savefilename || !savefilename[0])
    {
        ScriptAssert_(this, 0, "<internal>", -1, "Error - SaveObjectTree() with no filename\n");
        return (false);
    }

    // -- ensure we have a valid object id (if one was given)
    CObjectEntry* oe = FindObjectEntry(object_id);
    if (object_id > 0 && !oe)
    {
        ScriptAssert_(this, 0, "<internal>", -1, "Error - SaveObjectTree() invalid object ID %d\n", object_id);
        return (false);
    }

    // -- auto-append the ".ts" to the filename, if needed
    char file_name_buffer[kMaxNameLength];
    int32 length = (int32)strlen(savefilename);
    if (length < 3 || strcmp(&savefilename[length - 3], ".ts") != 0)
    {
        sprintf_s(file_name_buffer, kMaxNameLength, "%s.ts", savefilename);
        savefilename = file_name_buffer;
    }

    // -- open the file handle
    FILE* filehandle;
    int32 result = fopen_s(&filehandle, savefilename, "w");
	if (result != 0)
    {
        ScriptAssert_(this, 0, "<internal>", -1, "Error - unable to write file %s\n", savefilename);
		return (false);
    }

    // -- start the function
    if (!FileWritef(filehandle, "void LoadObjectTree()\n{"))
    {
        fclose(filehandle);
        ScriptAssert_(this, 0, "<internal>", -1, "Error - SaveObjectTree(%d) failed, file: %s\n", object_id,
                        savefilename);
        return (false);
    }

    // -- we're going to pass a buffer of spaces around, to create the appearance of the hierarchy through indenting
    char indent_buf[kMaxTokenLength];
    strcpy_s(indent_buf, "    ");

    // -- write out the tree beginning with our oe
    if (oe)
    {
        // -- create the objects
        FileWritef(filehandle, "\n    // -- Create the objects --");
        if (!ExportObjectTreeCreate(oe, filehandle, indent_buf))
        {
            fclose(filehandle);
            ScriptAssert_(this, 0, "<internal>", -1, "Error - SaveObjectTree(%d) failed, file: %s\n", object_id,
                          savefilename);
            return (false);
        }

        // -- initialize object members
        FileWritef(filehandle, "\n    // -- Initialize object members --");
        if (!ExportObjectTreeMembers(oe, filehandle))
        {
            fclose(filehandle);
            ScriptAssert_(this, 0, "<internal>", -1, "Error - SaveObjectTree(%d) failed, file: %s\n", object_id,
                          savefilename);
            return (false);
        }

        // -- restore the hierarchy
        FileWritef(filehandle, "\n    // -- Restore object hierarchy --");
        if (!ExportObjectTreeHierarchy(oe, filehandle, indent_buf))
        {
            fclose(filehandle);
            ScriptAssert_(this, 0, "<internal>", -1, "Error - SaveObjectTree(%d) failed, file: %s\n", object_id,
                          savefilename);
            return (false);
        }
    }

    // -- if we didn't have a starting object, write out the hierarchy of all root objects
    else
    {
        // -- create the objects
        FileWritef(filehandle, "\n    // -- Create the objects --");
        oe = GetObjectDictionary()->First();
        while (oe)
        {
            if (!oe->GetObjectGroup())
            {
                // -- we do three passes, first create the objects
                if (!ExportObjectTreeCreate(oe, filehandle, indent_buf))
                {
                    fclose(filehandle);
                    ScriptAssert_(this, 0, "<internal>", -1, "Error - SaveObjectTree(%d) failed, file: %s\n", object_id,
                                  savefilename);
                    return (false);
                }
            }

            // -- get the next object
            oe = GetObjectDictionary()->Next();
        }

        // -- initialize object members
        FileWritef(filehandle, "\n    // -- Initialize object members --");
        oe = GetObjectDictionary()->First();
        while (oe)
        {
            if (!oe->GetObjectGroup())
            {
                // -- then a pass to restore the members
                if (!ExportObjectTreeMembers(oe, filehandle))
                {
                    fclose(filehandle);
                    ScriptAssert_(this, 0, "<internal>", -1, "Error - SaveObjectTree(%d) failed, file: %s\n", object_id,
                                  savefilename);
                    return (false);
                }
            }

            // -- get the next object
            oe = GetObjectDictionary()->Next();
        }

        // -- restore the hierarchy
        FileWritef(filehandle, "\n    // -- Restore object hierarchy --");
        oe = GetObjectDictionary()->First();
        while (oe)
        {
            if (!oe->GetObjectGroup())
            {
                // -- finally a pass to restore the hierarchy
                if (!ExportObjectTreeHierarchy(oe, filehandle, indent_buf))
                {
                    fclose(filehandle);
                    ScriptAssert_(this, 0, "<internal>", -1, "Error - SaveObjectTree(%d) failed, file: %s\n", object_id,
                                    savefilename);
                    return (false);
                }
            }

            // -- get the next object
            oe = GetObjectDictionary()->Next();
        }
    }

    // -- close the function, and call it
    if (!FileWritef(filehandle, "}\n\nschedule(0, 1, hash('LoadObjectTree'));"))
    {
        fclose(filehandle);
        ScriptAssert_(this, 0, "<internal>", -1, "Error - SaveObjectTree(%d) failed, file: %s\n", object_id,
                        savefilename);
        return (false);
    }

    // -- close the file, return success
    TinPrint(this, "Object Tree for %d saved to file: %s\n", object_id, savefilename);
    fclose(filehandle);
    return (true);
}

// ====================================================================================================================
// FileWritef():  A convenience function to make it simpler to write to a file using a formatted string.
// ====================================================================================================================
bool8 CScriptContext::FileWritef(FILE* filehandle, const char* fmt, ...)
{
    // -- compose the message
    va_list args;
    va_start(args, fmt);
    char msg_buf[kMaxTokenLength];
    vsprintf_s(msg_buf, kMaxTokenLength - 2, fmt, args);
    va_end(args);

    // -- length
    int32 msg_length = (int32)strlen(msg_buf);

    // -- auto-appened the \n
    msg_buf[msg_length++] = '\n';
    msg_buf[msg_length] = '\0';

    int32 bytes_written = (int32)fwrite((void*)msg_buf, sizeof(char), msg_length, filehandle);
    if (bytes_written != msg_length)
        return (false);

    // -- success
    return (true);
}

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CNamespace::CNamespace(CScriptContext* script_context, const char* _name, uint32 _typeID,
                       CreateInstance _createinstance, DestroyInstance _destroyinstance)
{
    mContextOwner = script_context;
    // -- ensure the name lives in the string table
    mName = _name && _name[0] ? script_context->GetStringTable()->AddString(_name)
                             : script_context->GetStringTable()->AddString(CScriptContext::kGlobalNamespace);
    mHash = Hash(mName);
    mTypeID = _typeID;
    mNext = NULL;
    mCreateFuncptr = _createinstance;
    mDestroyFuncptr = _destroyinstance;
    mMemberTable = TinAlloc(ALLOC_VarTable, tVarTable, kLocalVarTableSize);
    mMethodTable = TinAlloc(ALLOC_FuncTable, tFuncTable, kLocalFuncTableSize);
}

// ====================================================================================================================
// Destructor
// ====================================================================================================================
CNamespace::~CNamespace()
{
    mMemberTable->DestroyAll();
    TinFree(mMemberTable);
    mMethodTable->DestroyAll();
    TinFree(mMethodTable);
}

// ====================================================================================================================
// GetVarEntry():  Find a variable registered in a given namespace
// ====================================================================================================================
CVariableEntry* CNamespace::GetVarEntry(uint32 varhash)
{
    CNamespace* curnamespace = this;
    while (curnamespace)
    {
        CVariableEntry* ve = GetVarTable()->FindItem(varhash);
        if (ve)
            return ve;
        else
            curnamespace = curnamespace->GetNext();
    }

    // -- not found
    return (NULL);
}

};

// ====================================================================================================================
// EOF
// ====================================================================================================================
