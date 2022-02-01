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
// TinStringTable.h
// ====================================================================================================================

#ifndef __TINSTRINGTABLE_H
#define __TINSTRINGTABLE_H

// -- includes
#include "integration.h"
#include "string.h"

// == namespace TinScript =============================================================================================

namespace TinScript
{

// ====================================================================================================================
// class CStringTable
// Used to create a dictionary of hashed strings, refcounted to allow unused strings to be deleted
// ====================================================================================================================
class CStringTable
{
    public:

        // the string table "as is", only ever "frees" unused strings from the end of the mBuffer...
        // we can help avoid fragmentation, if we set up pools for strings of size 16, 32, 64, 128
        // to use this, enable STRING_TABLE_USE_POOLS in integration.h
        enum class eStringPool : int8
        {
            None = -1,
            Size16,
            Size32,
            Size64,
            Size128,
            Count
        };

        // -- each string table entry is a ref counted const char*, so when a string is no longer
        // -- being used, it can be deleted from the dictionary buffer
        // -- without garbage collection, we simply remove unreferenced strings from the end
        // -- of the linked list
        struct tStringEntry
        {
            tStringEntry(const char* _string = nullptr)
                : mRefCount(0)
                , mString(_string)
                , mHash(0)
                , mNextFree(nullptr)
            {
            }

            int32 mRefCount = 0;
            const char* mString = nullptr;
            uint32 mHash = 0;
            tStringEntry* mNextFree = nullptr;
            eStringPool mPool = eStringPool::None;
            bool mMarkedForDelete = false;
        };

        int32 GetPoolStringSize(int32 pool) const;

        CStringTable(CScriptContext* owner, uint32 _size);
        virtual ~CStringTable();

        CScriptContext* GetScriptContext() { return (mContextOwner); }

        const char* AddString(const char* s, int length = -1, uint32 hash = 0, bool inc_refcount = false);
        const char* FindString(uint32 hash);

        void RefCountIncrement(uint32 hash);
        void RefCountDecrement(uint32 hash);

        void RemoveUnreferencedStrings();

        const CHashTable<tStringEntry>* GetStringDictionary() { return (mStringDictionary); }

        void DumpStringTableStats();

    private:
        CScriptContext* mContextOwner;

        uint32 mSize;
        char* mBuffer;
        char* mBufptr;

        CHashTable<tStringEntry>* mStringDictionary;
        tStringEntry* mTailEntryList = nullptr;

#if STRING_TABLE_USE_POOLS
        char* mStringPoolBuffer[(int32)eStringPool::Count];
        tStringEntry* mStringPoolEntryBuffer[(int32)eStringPool::Count];
        tStringEntry* mStringPoolFreeList[(int32)eStringPool::Count];
        int32 mStringPoolEntryUsedCount[(int32)eStringPool::Count];
        int32 mStringPoolEntryHighCount[(int32)eStringPool::Count];
        tStringEntry* mPoolDeleteList = nullptr;
#endif
};

} // TinScript

#endif // __TINSTRINGTABLE_H

// ====================================================================================================================
// EOF
// ====================================================================================================================
