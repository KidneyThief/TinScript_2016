// ------------------------------------------------------------------------------------------------
//  The MIT License
//  
//  Copyright (c) 2022 Tim Andersen
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
// TinHashtable.h
// ====================================================================================================================

#pragma once

// -- includes --------------------------------------------------------------------------------------------------------

#include "TinHash.h"
#include "TinVariableEntry.h"

// == namespace TinScript =============================================================================================

namespace TinScript
{

// --------------------------------------------------------------------------------------------------------------------
class CHashtable
{
    public:
        CHashtable();
        virtual ~CHashtable();

        void Wrap(CVariableEntry* ve);
        void CreateInternalHashtable();

        bool CopyFromHashtableVE(const CVariableEntry* ve);
        void Dump();

        bool HasKey(const char* key)
        {
            // see if we can get a value for a given key
            if (key == nullptr || key[0] == '\0')
                return {};

            // -- get the internal hashtable (the *actual* hashtable that this class wraps)
            tVarTable* hashtable = (tVarTable*)mHashtableVE->GetAddr(nullptr);

            // -- see if this hashtable already has an entry for the given key
            uint32 key_hash = Hash(key);
            return (hashtable->FindItem(key_hash) != nullptr);
        }

        template <typename T>
        bool GetValue(const char* key, T& out_value)
        {
            // see if we can get a value for a given key
            if (key == nullptr || key[0] == '\0')
                return false;

            // -- get the internal hashtable (the *actual* hashtable that this class wraps)
            tVarTable* hashtable = (tVarTable*)mHashtableVE->GetAddr(nullptr);

            // -- see if this hashtable already has an entry for the given key
            uint32 key_hash = Hash(key);
            CVariableEntry* hte = hashtable->FindItem(key_hash);
            if (hte == nullptr)
                return false;

            // -- see if the type we're asking for is legit
            eVarType out_type = GetRegisteredType(GetTypeID<T>());
            if (out_type == TYPE_NULL)
                return false;

            // -- get the value, and see if we convert it to the appropriate type
            void* source_addr = hte->GetAddr(nullptr);
            void* converted_val = TypeConvert(TinScript::GetContext(), hte->GetType(), source_addr, out_type);
            if (converted_val == nullptr)
                return false;

            // -- if this is an object, then converted_val will contain a uint32* containing the object id
            if (out_type == TYPE_object)
            {
                CObjectEntry* oe = TinScript::GetContext()->FindObjectEntry(*(uint32*)converted_val);
                if (oe != nullptr)
                {
                    void* object_add = oe->GetAddr();
                    T actual_object = convert_from_void_ptr<T>::Convert(object_add);
                    out_value = actual_object;
                    return true;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                // -- theoretically, we've got our type!
                out_value = *(T*)converted_val;
                return true;
            }
        }

        template <>
        bool GetValue(const char* key, const char*& out_string)
        {
            // see if we can get a value for a given key
            if (key == nullptr || key[0] == '\0')
                return false;

            // -- get the internal hashtable (the *actual* hashtable that this class wraps)
            tVarTable* hashtable = (tVarTable*)mHashtableVE->GetAddr(nullptr);

            // -- see if this hashtable already has an entry for the given key
            uint32 key_hash = Hash(key);
            CVariableEntry* hte = hashtable->FindItem(key_hash);
            if (hte == nullptr)
                return false;

            // -- get the value, and see if we convert it to the appropriate type
            void* source_addr = hte->GetAddr(nullptr);
            void* converted_val = TypeConvert(TinScript::GetContext(), hte->GetType(), source_addr, TYPE_string);
            if (converted_val == nullptr)
                return false;

            // -- theoretically, we've got our type!
            out_string = (const char*)UnHash(*(uint32*)converted_val);
            return true;
        }

        static bool CopyHashtableVEToVe(const CVariableEntry* src_ve, CVariableEntry* dest_ve);

        // -- add an entry to a CHashtable from C++
        template<typename T>
        bool AddEntry(const char* key, T value)
        {
            // -- sanity check
            if (key == nullptr || key[0] == '\0')
                return false;

            // -- get the type of value we're attempting to add
            eVarType type = GetRegisteredType(GetTypeID<T>());
            if (type == TYPE_NULL)
            {
                TinPrint(TinScript::GetContext(), "Error - CHashtable::AddEntry(): invalid type\n");
                return false;
            }

            // -- get the internal hashtable (the *actual* hashtable that this class wraps)
            tVarTable* hashtable = (tVarTable*)mHashtableVE->GetAddr(nullptr);

            // -- see if this hashtable already has an entry for the given key
            uint32 key_hash = Hash(key);
            CVariableEntry* hte = hashtable->FindItem(key_hash);

            // -- if the entry already exists, ensure it's the same type
            if (hte && hte->GetType() != type)
            {
                TinPrint(TinScript::GetContext(), "Error - CHashtable::AddEntry(): entry %s of type %s already exists\n",
                                                  key, GetRegisteredTypeName(hte->GetType()));
                return false;
            }

            // -- otherwise add the variable entry to the hash table
            // -- note:  by definition, hash table entries are dynamic
            else if (!hte)
            {
                hte = TinAlloc(ALLOC_VarEntry, CVariableEntry, TinScript::GetContext(), key, key_hash, type, 1, false, 0, true);
                hashtable->AddItem(*hte, key_hash);
            }

            // -- get the address to copy from - note, strings are special, since they're already a pointer
            void* value_addr = &value;
            if (type == TYPE_string)
            {
                const char** value_str = (const char**)&value;
                value_addr = (void*)*value_str;
            }

            // -- perform the assignment
            hte->SetValueAddr(false, value_addr);

            // -- success
            return true;
        }

        static bool CopyHashtableEntry(uint32 key_hash, const CVariableEntry* src_value, CVariableEntry* dest_hashtable);
        static bool CopyHashtableEntry(const char* key, const CVariableEntry* src_value, CVariableEntry* dest_hashtable);

        static void NotifyHashtableWrapped(CVariableEntry* ve, CHashtable* wrapper);
        static void NotifyHashtableUnwrapped(CVariableEntry* ve, CHashtable* wrapper);
        static void NotifyHashtableDestroyed(CVariableEntry* ve);
        static void Shutdown();

    private:
        CVariableEntry* mHashtableVE = nullptr;
        bool mHashtableIsInternal = true;
};

}

// ====================================================================================================================
// eof
// ====================================================================================================================
