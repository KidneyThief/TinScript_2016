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
// TinMemory.h
// ====================================================================================================================

#ifndef __TINMEMORY_H
#define __TINMEMORY_H

// -- includes --------------------------------------------------------------------------------------------------------

#include "integration.h"
#include "TinHash.h"

// -- forward declarations --------------------------------------------------------------------------------------------

enum eAllocType;

namespace TinScript
{

class CMemoryTracker
{
    public:
        CMemoryTracker();
        ~CMemoryTracker();

        static void Initialize();
        static void Shutdown();

        static CMemoryTracker* sm_instance;

        static void* Alloc(eAllocType alloc_type, int32 size);
        static void Free(void* addr);

        static void DumpTotals();

    private:
        struct tAllocEntry
        {
            tAllocEntry(eAllocType _type, void* _addr, int32 _size)
            {
                type = _type;
                addr = _addr;
                size = _size;
                next = nullptr;
            }

            eAllocType type;
            void* addr;
            int32 size;
            tAllocEntry* next;
        };

        int32 m_allocationTotals[AllocType_Count];

        // -- an untracked (minimal) hash table
        static const int k_trackedAllocationTableSize = 1553;
        tAllocEntry* m_allocationTable[k_trackedAllocationTableSize];
};

}

#endif

// --------------------------------------------------------------------------------------------------------------------
// eof
// --------------------------------------------------------------------------------------------------------------------

