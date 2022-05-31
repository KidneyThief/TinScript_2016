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
// TinNamespace.h
// ====================================================================================================================

#ifndef __TINNAMESPACE_H
#define __TINNAMESPACE_H

#include "TinHash.h"
#include "TinScript.h"

// == namespace TinScript =============================================================================================

namespace TinScript {

class CScriptContext;
class CVariableEntry;
class CFunctionEntry;
class CNamespace;
class CNamespaceReg;
class CObjectGroup;

typedef CHashTable<CVariableEntry> tVarTable;
typedef CHashTable<CFunctionEntry> tFuncTable;

// ====================================================================================================================
// class CObjectEntry:  Refers to an instance of an object, retrievable by name or hash, etc...
// ====================================================================================================================
class CObjectEntry
{
    public:
        CObjectEntry(CScriptContext* script_context, uint32 _objid, uint32 _namehash,
                     CNamespace* _objnamespace, void* _objaddr, bool8 register_manual);
        virtual ~CObjectEntry();

        CScriptContext* GetScriptContext() { return (mContextOwner); }

        uint32 GetID() const { return mObjectID; }
        const char* GetName() const { return UnHash(mNameHash); }
        uint32 GetNameHash() const { return mNameHash; }
        uint32 GetGroupID() const;

        void SetDestroyed() { mIsDestroyed = true; }
        bool8 IsDestroyed() const { return (mIsDestroyed); }

        CNamespace* GetNamespace() const { return mObjectNamespace; }

        void* GetAddr() const { return mObjectAddr; }

        CObjectGroup* GetObjectGroup() const { return (mGroupOwner); }
        void SetObjectGroup(CObjectGroup* group_owner) { mGroupOwner = group_owner; }

        bool8 IsManuallyRegistered() { return (mManualRegister); }

        CVariableEntry* GetVariableEntry(uint32 varhash);
        CFunctionEntry* GetFunctionEntry(uint32 nshash, uint32 funchash);
        CFunctionEntry* GetSuperFunctionEntry(uint32 nshash, uint32 funchash);
        void CallFunctionHierarchy(uint32 function_hash, bool8 ascending);
        CNamespace* HasNamespace(uint32 nshash);

        bool8 AddDynamicVariable(uint32 varhash, eVarType vartype, int32 array_size = 1);
        bool8 SetMemberVar(uint32 varhash, void* value);
        tVarTable* GetDynamicVarTable() { return (mDynamicVariables); }

    private:
        CScriptContext* mContextOwner;

        uint32 mObjectID;
        uint32 mNameHash;
        CNamespace* mObjectNamespace;
        void* mObjectAddr;
        CObjectGroup* mGroupOwner;
        bool8 mManualRegister;
        bool8 mIsDestroyed;
        CHashTable<CVariableEntry>* mDynamicVariables;
};

// ====================================================================================================================
// class CNamespace:  A class used to store hashtables for registered members and methods, forming a linked list.
// ====================================================================================================================
class CNamespace
{
    public:
        typedef void* (*CreateInstance)();
        typedef void (*DestroyInstance)(void* addr);
        typedef void (*Register)(CScriptContext* script_context, CNamespace* reg);

        CNamespace(CScriptContext* script_context, const char* name, uint32 _typeID = 0,
                   CreateInstance _createinstance = NULL, DestroyInstance _destroyinstance = NULL);
        virtual ~CNamespace();

        CScriptContext* GetScriptContext() { return (mContextOwner); }

        const char* GetName() const { return (mName); }
        uint32 GetHash() const { return (mHash); }
        uint32 GetTypeID() const { return (mTypeID); }

        // -- returns false only if there's a function entry with a non-interface definition
        // e.g. a function definition with a return type other than TYPE__resolve
        bool IsInterface() const;

        // -- when we ensure a "Create-able" non-interface namespace has an interface implemented,
        // we also need to ensure that the interface itself is consistent in its hierarchy
        bool IsInterfaceVerfied() const { return mIsInterfaceVerified; }
        void SetInterfaceVerified() { mIsInterfaceVerified = true; }

        bool8 IsRegisteredClass() const
        {
            // -- only classes can be instantiated (obviously)
            return (mCreateFuncptr != NULL);
        }

        CNamespace* GetNext() const { return (mNext); }
        void SetNext(CNamespace* _next) { mNext = _next; }

        CreateInstance GetCreateInstance() const { return (mCreateFuncptr); }

        // -- it's possible that this is a script-derived namespace...
        // -- find the highest level child with a proper destructor
        DestroyInstance GetDestroyInstance() const
        {
            const CNamespace* ns = this;
            while(ns && ns->mDestroyFuncptr == NULL)
                ns = ns->mNext;
            if(ns)
                return (ns->mDestroyFuncptr);
            else
                return (NULL);
        }

        CVariableEntry* GetVarEntry(uint32 varhash);
        tVarTable* GetVarTable() const { return (mMemberTable); }
        tFuncTable* GetFuncTable() const { return (mMethodTable); }

    private:
        CNamespace() { }

        CScriptContext* mContextOwner;

        const char* mName;
        uint32 mHash;
        uint32 mTypeID;
        CNamespace* mNext;
        bool mIsInterfaceVerified = false;

        CreateInstance mCreateFuncptr;
        DestroyInstance mDestroyFuncptr;

        tVarTable* mMemberTable;
        tFuncTable* mMethodTable;
};

// ====================================================================================================================
// class CNamespaceReg:  A class instantiated in the global namespace used to store registration details.
// ====================================================================================================================
class CNamespaceReg
{
    public:
        CNamespaceReg(const char* _name, const char* _parentname, uint32 _typeID,
                      void* _createfuncptr, void* _destroyfuncptr,
                      void* _regfuncptr)
        {
            mName = _name;
            mHash = Hash(mName);
            mTypeID = _typeID;
            mParentName = _parentname;
            mParentHash = Hash(mParentName);
            mCreateFuncptr = (CNamespace::CreateInstance)_createfuncptr;
            mDestroyFuncptr = (CNamespace::DestroyInstance)_destroyfuncptr;
            mRegFuncptr = (CNamespace::Register)_regfuncptr;

            next = head;
            head = this;
        }

        // -- head of the linked list so we can register (build hashtables) each namespace
        static CNamespaceReg* head;
        CNamespaceReg* next;

        const char* GetName() const { return mName; }
        uint32 GetTypeID() const { return mTypeID; }

        const char* GetParentName() const { return mParentName; }

        uint32 GetHash() const { return mHash; }
        uint32 GetParentHash() const { return mParentHash; }

        void SetRegistered(bool8 torf) { mRegistered = torf; }
        bool8 GetRegistered() const { return (mRegistered); }

        CNamespaceReg* GetNext() const { return next;}

        CNamespace::CreateInstance GetCreateFunction() const { return (mCreateFuncptr); }
        CNamespace::DestroyInstance GetDestroyFunction() const { return (mDestroyFuncptr); }

        void RegisterNamespace(CScriptContext* script_context, CNamespace* _namespace)
        {
            mRegFuncptr(script_context, _namespace);
        }

    private:
        const char* mName;
        uint32 mHash;
        uint32 mTypeID;
        const char* mParentName;
        uint32 mParentHash;
        bool8 mRegistered;

        CNamespace::CreateInstance mCreateFuncptr;
        CNamespace::DestroyInstance mDestroyFuncptr;
        CNamespace::Register mRegFuncptr;
        CNamespace** mClassNamespace;

        CNamespaceReg() { }
};

}; // namespace TinScript

#endif // __TINNAMESPACE_H

// ====================================================================================================================
// EOF
// ====================================================================================================================
