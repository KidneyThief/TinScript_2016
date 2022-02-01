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
// TinStringTable.cpp
// ====================================================================================================================

// -- includes
#include "string.h"

// -- TinScript includes
#include "TinScript.h"
#include "TinRegistration.h"
#include "TinStringTable.h"

// == namespace TinScript =============================================================================================

namespace TinScript
{

// == CStringTable ====================================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CStringTable::CStringTable(CScriptContext* owner, uint32 _size)
{
    mContextOwner = owner;

    assert(_size > 0);
    mSize = _size;
    mBuffer = TinAllocArray(ALLOC_StringTable, char, mSize);
    mBufptr = mBuffer;

    mStringDictionary = TinAlloc(ALLOC_StringTable, CHashTable<tStringEntry>, kStringTableDictionarySize);

#if STRING_TABLE_USE_POOLS
    // -- initialize all the pools
    for (int32 pool = (int32)eStringPool::Size16; pool < (int32)eStringPool::Count; ++pool)
    {
        // -- get the count
        int32 pool_count = kStringPoolSizesCount[pool];
        assert(pool_count > 0);

        // -- get the size (note:  if we change the strategy to not use 16, 32, ...  then adjust this
        int32 string_size = GetPoolStringSize(pool);
        assert(string_size > 0);

        // -- get the total size of the string pool
        int32 pool_size = string_size * pool_count;

        // -- allocate the pool buffer;
        mStringPoolBuffer[pool] = TinAllocArray(ALLOC_StringTable, char, pool_size);
        assert(mStringPoolBuffer[pool] != nullptr);

        // allocate the string entry buffer for the pool
        mStringPoolFreeList[pool] = nullptr;
        mStringPoolEntryBuffer[pool] = TinAllocArray(ALLOC_StringTable, tStringEntry, pool_count);
        assert(mStringPoolEntryBuffer[pool] != nullptr);

        // -- initialize all string pool entries
        char* pool_buffer_ptr = mStringPoolBuffer[pool];
        tStringEntry* pool_entry_ptr = mStringPoolEntryBuffer[pool];
        for (int32 entry_index = 0; entry_index < pool_count; ++entry_index)
        {
            // -- construct the entry in-place, set the pool member, and hook it into the free list
            pool_entry_ptr->mPool = (eStringPool)pool;
            pool_entry_ptr->mNextFree = mStringPoolFreeList[pool];
            mStringPoolFreeList[pool] = pool_entry_ptr;

            // -- initialize the actual string buffer
            pool_entry_ptr->mString = pool_buffer_ptr;
            pool_buffer_ptr[0] = '\0';

            // -- update the ptrs
            pool_buffer_ptr += string_size;
            pool_entry_ptr++;
        }

        // -- initialize the used and high watermark counts
        mStringPoolEntryUsedCount[pool] = 0;
        mStringPoolEntryHighCount[pool] = 0;
    }
#endif
}

// ====================================================================================================================
// Destructor
// ====================================================================================================================
CStringTable::~CStringTable()
{
    DumpStringTableStats();

    // -- zero out the ref count for all strings, and then remove unreferenced strings
    // -- this will return pool entries to the pool, and allocated entries will be deleted
    tStringEntry* ste = mStringDictionary->First();
    while (ste != nullptr)
    {
        ste->mRefCount = 0;
        ste = mStringDictionary->Next();
    }
    RemoveUnreferencedStrings();

    // -- destroy the table and buffer
    TinFree(mStringDictionary);
    TinFree(mBuffer);

#if STRING_TABLE_USE_POOLS
    // -- destroy the pools
    for (int32 pool = (int32)eStringPool::Size16; pool < (int32)eStringPool::Count; ++pool)
    {
        TinFreeArray(mStringPoolBuffer[pool]);
        TinFreeArray(mStringPoolEntryBuffer[pool]);
    }
#endif
}

// ====================================================================================================================
// GetPoolStringSize():  we're using sizes of 16, 32, 64, 128 by default...  we use this function in case this changes
// ====================================================================================================================
int32 CStringTable::GetPoolStringSize(int32 pool) const
{
#if STRING_TABLE_USE_POOLS
    if (pool >= 0 && pool < (int32)eStringPool::Count)
    {
        return 16 * (int32)pow(2, pool);
    }
#endif
    return 0;
}

// ====================================================================================================================
// AddString():  Add a string to the dictionary, potentially with an initial ref count
// ====================================================================================================================
const char* CStringTable::AddString(const char* s, int length, uint32 hash, bool inc_refcount)
{
    // -- sanity check
    if (!s)
        return "";

    if (hash == 0)
        hash = Hash(s, length);

    // -- see if the string is already in the dictionary
    const char* exists = FindString(hash);
    if (!exists)
    {
        if (length < 0)
            length = (int32)strlen(s);

#if STRING_TABLE_USE_POOLS
        int32 pool = (int32)eStringPool::None;
        int32 pool_string_size = 0;
        for (; pool < (int32)eStringPool::Count; ++pool)
        {
            // --remember to leave room for the terminator
            pool_string_size = GetPoolStringSize(pool);
            if (length + 1 <= pool_string_size)
            {
                break;
            }
        }

        // -- see if we can add the string to the pool
        if (pool < (int32)eStringPool::Count)
        {
            if (mStringPoolFreeList[pool] != nullptr)
            {
                // -- pop an entry off the free list
                tStringEntry* pool_entry = mStringPoolFreeList[pool];
                mStringPoolFreeList[pool] = pool_entry->mNextFree;
                pool_entry->mNextFree = nullptr;

                // -- initialize the ref count
                if (inc_refcount)
                {
                    pool_entry->mRefCount = 1;
                    pool_entry->mMarkedForDelete = false;
                }
                else
                {
                    // -- because we're not referencing, this string may be temporary - we'll add it now
                    // to the pool delete list, so if it's still unreferenced, it can be cleaned
                    // at when the current execution stack has concluded
                    pool_entry->mRefCount = 0;
                    pool_entry->mNextFree = mPoolDeleteList;
                    mPoolDeleteList = pool_entry;
                    pool_entry->mMarkedForDelete = true;
                }

                // -- copy the string
                SafeStrcpy(const_cast<char*>(pool_entry->mString), pool_string_size, s, length + 1);

                // -- add to the dictionary
                pool_entry->mHash = hash;
                mStringDictionary->AddItem(*pool_entry, hash);

                // -- update the count - if we've depleted the pool the first time, send an assert message
                mStringPoolEntryUsedCount[pool]++;
                if (mStringPoolEntryUsedCount[pool] == kStringPoolSizesCount[pool] &&
                    mStringPoolEntryHighCount[pool] < kStringPoolSizesCount[pool])
                {
                    // -- no free entries - assert, and fall through - the string will be added to the
                    // regular string table
                    ScriptAssert_(TinScript::GetContext(), 0, "<internal>", -1,
                                  "Warning - StringTable pool of size %d is full\n", pool_string_size);
                }

                if (mStringPoolEntryUsedCount[pool] > mStringPoolEntryHighCount[pool])
                    mStringPoolEntryHighCount[pool] = mStringPoolEntryUsedCount[pool];

                return pool_entry->mString;
            }
        }
#endif

        // -- space left
        int32 remaining = int32(mSize - (kPointerToUInt32(mBufptr) -
                                            kPointerToUInt32(mBuffer)));
        if (remaining < length + 1)
        {
            ScriptAssert_(mContextOwner, 0,
                            "<internal>", -1, "Error - StringTable of size %d is full!\n",
                            mSize);
            return NULL;
        }
        const char* stringptr = mBufptr;
        SafeStrcpy(mBufptr, remaining, s, length + 1);
        mBufptr += length + 1;

        // -- create the string table entry, also add it to the tail list
        tStringEntry* new_entry = TinAlloc(ALLOC_StringTable, tStringEntry, stringptr);
        new_entry->mNextFree = mTailEntryList;
        mTailEntryList = new_entry;

        // -- add the entry to the dictionary
        new_entry->mHash = hash;
        mStringDictionary->AddItem(*new_entry, hash);

        // -- if this item is meant to persist, increment the ref count
        if (inc_refcount)
            new_entry->mRefCount++;

        return (stringptr);
    }

    // -- else check for a collision
    else
    {
        if (length < 0)
            length = (int32)strlen(s);

        if (strncmp(exists, s, length) != 0)
        {
            ScriptAssert_(mContextOwner, 0, "<internal>", -1,
                          "Error - Hash collision [0x%x]: '%s', '%s'\n", hash, exists, s);
        }

        // -- if this item is meant to persist, increment the ref count
        if (inc_refcount)
            RefCountIncrement(hash);

        return exists;
    }
}

// ====================================================================================================================
// FindString():  Finds a string entry in the dictionary - returns the actual const char*
// ====================================================================================================================
const char* CStringTable::FindString(uint32 hash)
{
    // -- sanity check
    if (hash == 0)
        return ("");

    tStringEntry* ste = mStringDictionary->FindItem(hash);
    return (ste ? ste->mString : NULL);
}

// ====================================================================================================================
// RefCountIncrement(): called when the string is assigned, and (obviously) being referenced
// ====================================================================================================================
void CStringTable::RefCountIncrement(uint32 hash)
{
    // -- sanity check
    if (hash == 0)
        return;

    tStringEntry* ste = mStringDictionary->FindItem(hash);
    if (ste)
        ste->mRefCount++;
}

// ====================================================================================================================
// RefCountDecrement():  called when a variable is no longer refering to this string
// ====================================================================================================================
void CStringTable::RefCountDecrement(uint32 hash)
{
    // -- sanity check
    if (hash == 0)
        return;

    tStringEntry* ste = mStringDictionary->FindItem(hash);
    if (!ste)
        return;

    // -- decrement the ref count
    ste->mRefCount--;

#if STRING_TABLE_USE_POOLS
    if (ste->mPool != eStringPool::None && ste->mRefCount == 0)
    {
        // -- we only want to add to the delete list once of course
        if (!ste->mMarkedForDelete)
        {
            // -- push it onto the delete list - we'll delete *after* completion of
            // the execution stack... (in case the value is still needed)
            ste->mNextFree = mPoolDeleteList;
            mPoolDeleteList = ste;
            ste->mMarkedForDelete = true;
        }
    }
#endif
}

// ====================================================================================================================
// RemoveUnreferencedStrings():  All strings at the *end* of the dictionary, to which there are no variables
// assigned are removed.
// $$$TZA Convert this to an actual string handle system, allowing all unreferenced strings to be removed
// and the buffer fragmentation to be reduced
// ====================================================================================================================
void CStringTable::RemoveUnreferencedStrings()
{
    // -- loop back from the end of the linked list, removing all entries
    // -- with a zero refcount (note:  entries from the main buffer, not pooled entries
    while (mTailEntryList != nullptr)
    {
        // -- if we hit an entry that is still being used (e.g. assigned to
        // -- a variable) we're done
        if (mTailEntryList->mRefCount > 0)
            break;

        // -- remove the entry from the dictionary
        tStringEntry* delete_me = mTailEntryList;
        mTailEntryList = mTailEntryList->mNextFree;
        mStringDictionary->RemoveItem(delete_me->mHash);

        // -- back up the buf ptr, to store the next string
        mBufptr = const_cast<char*>(delete_me->mString);

        // -- delete the entry
        TinFree(delete_me);
    }

#if STRING_TABLE_USE_POOLS
    while (mPoolDeleteList != nullptr)
    {
        // -- pop the entry off the front of the delete list
        tStringEntry* ste = mPoolDeleteList;
        mPoolDeleteList = ste->mNextFree;
        ste->mMarkedForDelete = false;

        // -- if the ref count is *still* 0, reset the entry and return to the free list
        if (ste->mRefCount == 0)
        {
            // -- pop the ste back onto its free list
            ste->mNextFree = mStringPoolFreeList[(int32)ste->mPool];
            mStringPoolFreeList[(int32)ste->mPool] = ste;
            const_cast<char*>(ste->mString)[0] = '\0';

            // -- remove it from the dictionary
            mStringDictionary->RemoveItem(ste->mHash);
            ste->mHash = 0;

            // -- decrement the used count
            mStringPoolEntryUsedCount[(int32)ste->mPool]--;
        }
    }
#endif
}

// ====================================================================================================================
// DumpStringTableStats():  print out the used and high pool entry stats
// ====================================================================================================================
void CStringTable::DumpStringTableStats()
{
    CScriptContext* script_context = TinScript::GetContext();
    TinPrint(script_context, "### StringTable Stats:\n");
    int32 remaining = int32(mSize - (kPointerToUInt32(mBufptr) - kPointerToUInt32(mBuffer)));
    int32 used = kStringTableSize - remaining;
    TinPrint(script_context, "    Main Buffer used %d / %d, %.2f%%%%\n", used,
                             kStringTableSize, (float(used) / float(kStringTableSize)) * 100.0f);

#if STRING_TABLE_USE_POOLS
    for (int32 pool = 0; pool < (int32)eStringPool::Count; ++pool)
    {
        int string_size = GetPoolStringSize(pool);
        TinPrint(script_context, "    Pool %d used: %d, high: %d, max: %d  [%.2f%%%%]\n", string_size,
                                 mStringPoolEntryUsedCount[pool], mStringPoolEntryHighCount[pool],
                                 kStringPoolSizesCount[pool],
                                 (float(mStringPoolEntryHighCount[pool]) / float(kStringPoolSizesCount[pool])) * 100.0f);
    }
#endif
}

} // TinScript

// == Script Registration =============================================================================================

// ====================================================================================================================
// StringLen():  registered version of strlen()
// ====================================================================================================================
int32 StringLen(const char* string)
{
    // -- sanity check
    if (!string)
        return (0);

    int32 len = (int32)strlen(string);
    return (len);
}

REGISTER_FUNCTION(StringLen, StringLen);

// ====================================================================================================================
// StringCat():  multi-arg implementation of strcat
// ====================================================================================================================
const char* StringCat(const char* str0, const char* str1, const char* str2, const char* str3, const char* str4,
                      const char* str5, const char* str6, const char* str7)
{
    // -- ensure we have at least two strings
    if (!str0 || !str1 || !str1[0])
        return (str0);

    // -- concatenate the arguments
    // -- stop as soon as we have an empty string
    char buf[2048];
    if (!str2 || !str2[0])
        sprintf_s(buf, "%s%s", str0, str1);
    else if (!str3 || !str3[0])
        sprintf_s(buf, "%s%s%s", str0, str1, str2);
    else if (!str4 || !str4[0])
        sprintf_s(buf, "%s%s%s%s", str0, str1, str2, str3);
    else if (!str5 || !str5[0])
        sprintf_s(buf, "%s%s%s%s%s", str0, str1, str2, str3, str4);
    else if (!str6 || !str6[0])
        sprintf_s(buf, "%s%s%s%s%s%s", str0, str1, str2, str3, str4, str5);
    else if (!str7 || !str7[0])
        sprintf_s(buf, "%s%s%s%s%s%s%s", str0, str1, str2, str3, str4, str5, str6);
    else
        sprintf_s(buf, "%s%s%s%s%s%s%s%s", str0, str1, str2, str3, str4, str5, str6, str7);

    // -- add the result to the string table
    TinScript::CScriptContext* script_context = ::TinScript::GetContext();
    const char* result = script_context->GetStringTable()->AddString(buf);

    // -- return the result
    return (result);
}

REGISTER_FUNCTION(StringCat, StringCat);

// ====================================================================================================================
// StringCmp():  Same as strcmp, case sensitive
// ====================================================================================================================
int32 StringCmp(const char* str0, const char* str1)
{
    // -- be design, a NULL string is the same as an ""
    if (!str0)
        str0 = "";
    if (!str1)
        str1 = "";

    return (strcmp(str0, str1));
}

REGISTER_FUNCTION(StringCmp, StringCmp);

// ====================================================================================================================
// IntToChar():  Converts from an int to a single character string
// ====================================================================================================================
const char* IntToChar(int32 ascii_value)
{
    // -- skip hidden characters
    if (ascii_value < 0x20 || ascii_value > 255)
        return ("");

    char* buffer = TinScript::GetContext()->GetScratchBuffer();
    buffer[0] = (char)ascii_value;
    buffer[1] = '\0';
    return (buffer);
}

// -- UE4 needs the param specific registration
// $$$TZA related to using a pointer as an arg?  (const char* is 64-bit)??
#if PLATFORM_UE4
    REGISTER_FUNCTION_P1(IntToChar, IntToChar, const char*, int32);
#else	
	REGISTER_FUNCTION(IntToChar, IntToChar);
#endif

// ====================================================================================================================
// CharToInt():  Converts from char to the ascii int value
// ====================================================================================================================
int32 CharToInt(const char* input_string)
{
    if (! input_string)
        return (0);
    else
        return ((int32)(input_string[0]));
}

REGISTER_FUNCTION(CharToInt, CharToInt);

// ====================================================================================================================
// PrintWithSeverity():  helper to print with different severities, from Print(), Warn(), Error(), and Assert()
// ====================================================================================================================
const char* PrintWithSeverity(int32 severity, const char* str0, const char* str1, const char* str2, const char* str3,
                              const char* str4, const char* str5, const char* str6, const char* str7)
{
    TinScript::CScriptContext* script_context = ::TinScript::GetContext();
    const char* str_concat = StringCat(str0, str1, str2, str3, str4, str5, str6, str7);
    if (script_context == nullptr || !str_concat)
        return ("");

    // -- automatically add a '\n'
    switch (severity)
    {
        default:
        case 0:
            TinPrint(script_context, "%s\n", str_concat);
            break;
        case 1:
            TinWarning(script_context, "%s\n", str_concat);
            break;
        case 2:
            TinError(script_context, "%s\n", str_concat);
            break;
        case 3:
            TinAssert(script_context, "%s\n", str_concat);
            break;
    }

    // -- return the concatenated string
    return str_concat;
}

// ====================================================================================================================
// Print():  script method to call the main thread print handler with severity 0
// ====================================================================================================================
const char* Print(const char* str0, const char* str1, const char* str2, const char* str3, const char* str4,
                  const char* str5, const char* str6, const char* str7)
{
    return (PrintWithSeverity(0, str0, str1, str2, str3, str4, str5, str6, str7));
}

// ====================================================================================================================
// Warn():  script method to call the main thread print handler with severity 1
// ====================================================================================================================
const char* Warn(const char* str0, const char* str1, const char* str2, const char* str3, const char* str4,
    const char* str5, const char* str6, const char* str7)
{
    return (PrintWithSeverity(1, str0, str1, str2, str3, str4, str5, str6, str7));
}

// ====================================================================================================================
// Error():  script method to call the main thread print handler with severity 2
// ====================================================================================================================
const char* Error(const char* str0, const char* str1, const char* str2, const char* str3, const char* str4,
    const char* str5, const char* str6, const char* str7)
{
    return (PrintWithSeverity(2, str0, str1, str2, str3, str4, str5, str6, str7));
}

REGISTER_FUNCTION(Print, Print);
REGISTER_FUNCTION(Warn, Warn);
REGISTER_FUNCTION(Error, Error);

// ====================================================================================================================
// StringTablePoolMem():  dump out the used and high pool usage for each pool
// ====================================================================================================================
void StringTableDumpStats()
{
    if (TinScript::GetContext() != nullptr && TinScript::GetContext()->GetStringTable() != nullptr)
    {
        TinScript::GetContext()->GetStringTable()->DumpStringTableStats();
    }
}

REGISTER_FUNCTION(StringTableDumpStats, StringTableDumpStats);

// ====================================================================================================================
// EOF
// ====================================================================================================================
