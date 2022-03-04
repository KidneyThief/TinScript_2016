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
// TinHashtable.cpp
// ====================================================================================================================

// -- includes --------------------------------------------------------------------------------------------------------
#include "TinHashtable.h"

#include "TinHash.h"
#include "TinRegistration.h"

// -- use the DECLARE_FILE/REGISTER_FILE macros to prevent deadstripping
DECLARE_FILE(tinhashtable_cpp);

// == namespace TinScript =============================================================================================

namespace TinScript
{

// -- class CHashtable ------------------------------------------------------------------------------------------------
// 
// ====================================================================================================================
// destructor
// ====================================================================================================================
    CHashtable::CHashtable()
{
    // -- on construction, we want to create an internal hashtable instance
    // -- it's an internal variable entry
    static const char* ve_name = "<internal>";
    static uint32 ve_hash = Hash(ve_name);
    mHashtableVE = TinAlloc(ALLOC_VarEntry, CVariableEntry, TinScript::GetContext(), ve_name, ve_hash,
                            TYPE_hashtable, 1, false, 0, false, false);
}

// ====================================================================================================================
// destructor
// ====================================================================================================================
CHashtable::~CHashtable()
{
    if (mHashtableVE)
    {
        TinFree(mHashtableVE);
    }
}

// ====================================================================================================================
// Dump():  Debug function to print out the contents of a hashtable
// ====================================================================================================================
void CHashtable::Dump()
{
    // $$$TZA support CHashtable's as members of registered classes
    tVarTable* var_table = (tVarTable*)mHashtableVE->GetAddr(nullptr);
    TinScript::DumpVarTable(TinScript::GetContext(), nullptr, var_table);
}

// ====================================================================================================================
// CopyHashtableVEToVe():  copy the actual ve...  static so we can use from within the VM
// ====================================================================================================================
bool CHashtable::CopyHashtableVEToVe(const CVariableEntry* src_ve, CVariableEntry* dest_ve)
{
    // sanity check
    if (src_ve == nullptr || dest_ve == nullptr)
    {
        return false;
    }

    // -- check types as well
    if (src_ve->GetType() != TYPE_hashtable || dest_ve->GetType() != TYPE_hashtable)
    {
        return false;
    }

    // -- iterate through the ve, and duplicate all values to our internal
    tVarTable* src_vartable = (tVarTable*)src_ve->GetAddr(NULL);
    if (src_vartable == nullptr)
        return false;

    // -- clear out our internal hashtable
    // -- get the internal hashtable (the *actual* hashtable that this class wraps)
    tVarTable* dest_vartable = (tVarTable*)dest_ve->GetAddr(nullptr);
    if (dest_vartable == nullptr)
        return false;
    dest_vartable->DestroyAll();

    uint32 var_hash = 0;
    CVariableEntry* ht_var = src_vartable->First(&var_hash);
    while (ht_var != nullptr)
    {
        CVariableEntry* ht_var_copy = ht_var->Clone();
        if (ht_var_copy == nullptr)
            return false;

        if (!CopyHashtableEntry(var_hash, ht_var, dest_ve))
            return false;
        ht_var = src_vartable->Next(&var_hash);
    }

    // -- success
    return true;
}

// ====================================================================================================================
// CopyFromHashtableVE():  Copies the contents of the given hashtable ve, to the internal hashtable
// ====================================================================================================================
bool CHashtable::CopyFromHashtableVE(const CVariableEntry* ve)
{
    // -- sanity check
    if (ve == nullptr || ve->GetType() != TYPE_hashtable)
        return false;

    // -- iterate through the ve, and duplicate all values to our internal
    tVarTable* source_vartable = (tVarTable*)ve->GetAddr(NULL);
    if (source_vartable == nullptr)
        return false;

    // -- clear out our internal hashtable
    // -- get the internal hashtable (the *actual* hashtable that this class wraps)
    tVarTable* dest_hashtable = (tVarTable*)mHashtableVE->GetAddr(nullptr);
    dest_hashtable->DestroyAll();

    uint32 var_hash = 0;
    CVariableEntry* ht_var = source_vartable->First(&var_hash);
    while (ht_var != nullptr)
    {
        if (!CopyHashtableEntry(var_hash, ht_var, mHashtableVE))
            return false;
        ht_var = source_vartable->Next(&var_hash);
    }

    // -- success
    return true;
}

// ====================================================================================================================
// CopyHashtableEntry():  The source ve here is an entry in a hashtable, not a hashtable itself
// ====================================================================================================================
bool CHashtable::CopyHashtableEntry(const char* key, const CVariableEntry* src_value, CVariableEntry* dest_hashtable)
{
    // -- sanity check
    if (key == nullptr || key[0] == '\0' || src_value == nullptr || dest_hashtable == nullptr)
        return false;

    return CopyHashtableEntry(Hash(key), src_value, dest_hashtable);
}

    // ====================================================================================================================
// CopyHashtableEntry():  The source ve here is an entry in a hashtable, not a hashtable itself
// ====================================================================================================================
bool CHashtable::CopyHashtableEntry(uint32 key_hash, const CVariableEntry* source, CVariableEntry* dest_hashtable)
{
    // -- sanity check
    if (key_hash == 0 || source == nullptr || dest_hashtable == nullptr)
        return false;

    // -- see if this hashtable already has an entry for the given key
    tVarTable* dest_vartable = (tVarTable*)dest_hashtable->GetAddr(nullptr);
    if (dest_vartable == nullptr)
        return false;

    CVariableEntry* hte = dest_vartable->FindItem(key_hash);

    // -- if the entry already exists, ensure it's the same type
    if (hte && hte->GetType() != source->GetType())
    {
        TinPrint(TinScript::GetContext(), "Error - CHashtable::AddEntry(): entry %s of type %s already exists\n",
                                           UnHash(key_hash), GetRegisteredTypeName(hte->GetType()));
        return false;
    }

    // -- otherwise add the variable entry to the hash table
    // -- note:  by definition, hash table entries are dynamic
    else if (!hte)
    {
        // -- create a dup of the source
        hte = source->Clone();
        if (hte == nullptr)
        {
            TinPrint(TinScript::GetContext(), "Error - CHashtable::CopyHashtableEntry(): failed to duplicate entry: %s\n",
                UnHash(key_hash));
            return false;
        }
        dest_vartable->AddItem(*hte, key_hash);
    }
    else
    {
        // -- perform the assignment
        hte->SetValueAddr(false, source->GetValueAddr(nullptr));
    }

    // -- success
    return true;
}

// -- test passing hashtables to Cpp ----------------------------------------------------------------------------------

REGISTER_SCRIPT_CLASS_BEGIN(CHashtable, VOID)
REGISTER_SCRIPT_CLASS_END()

REGISTER_METHOD(CHashtable, Dump, Dump);

}

// == eof =============================================================================================================
