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
        // -- each string table entry is a ref counted const char*, so when a string is no longer
        // -- being used, it can be deleted from the dictionary buffer
        // -- without garbage collection, we simply remove unreferenced strings from the end
        // -- of the linked list
        struct tStringEntry
        {
            tStringEntry(const char* _string)
            {
                mRefCount = 0;
                mString = _string;
            }

            int32 mRefCount;
            const char* mString;
        };

        CStringTable(CScriptContext* owner, uint32 _size);
        virtual ~CStringTable();

        CScriptContext* GetScriptContext() { return (mContextOwner); }

        const char* AddString(const char* s, int length = -1, uint32 hash = 0, bool inc_refcount = false);
        const char* FindString(uint32 hash);

        void RefCountIncrement(uint32 hash);
        void RefCountDecrement(uint32 hash);

        void RemoveUnreferencedStrings();

        const CHashTable<tStringEntry>* GetStringDictionary() { return (mStringDictionary); }

    private:
        CScriptContext* mContextOwner;

        uint32 mSize;
        char* mBuffer;
        char* mBufptr;

        CHashTable<tStringEntry>* mStringDictionary;
};

} // TinScript

#endif // __TINSTRINGTABLE_H

// ====================================================================================================================
// EOF
// ====================================================================================================================
