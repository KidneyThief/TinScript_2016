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
// TinMemory.cpp
// ====================================================================================================================

// -- includes --------------------------------------------------------------------------------------------------------

#include "TinMemory.h"
#include "TinStringTable.h"
#include "TinScript.h"
#include "TinInterface.h"
#include "TinRegistration.h"

// -- use the DECLARE_FILE/REGISTER_FILE macros to prevent deadstripping
DECLARE_FILE(tinmemory_cpp);

namespace TinScript
{

CMemoryTracker* CMemoryTracker::sm_instance = nullptr;

// -- create the string table of allocation type names
const char* g_allocationTypeNames[] =
{
    #define AllocTypeEntry(a) "ALLOC_" #a,
    AllocTypeTuple
    #undef AllocTypeEntry
};

CMemoryTracker::CMemoryTracker()
{
    // -- initialize the array of allocations
    for (int32 i = 0; i < AllocType_Count; ++i)
        m_allocationTotals[i] = 0;

    // -- initialize our untracked hash table
    for (int i = 0; i < k_trackedAllocationTableSize; ++i)
        m_allocationTable[i] = nullptr;
}

CMemoryTracker::~CMemoryTracker()
{
    // -- destroy the entries in the allocation table
    for (int i = 0; i < k_trackedAllocationTableSize; ++i)
    {
        tAllocEntry* alloc_entry = m_allocationTable[i];
        while (alloc_entry != nullptr)
        {
            tAllocEntry* next = alloc_entry->next;
            delete alloc_entry;
            alloc_entry = next;
        }
    }
}

void CMemoryTracker::Initialize()
{
    // -- nothing to do if we've already initialized the tracker
    if (sm_instance != nullptr)
        return;

    // -- the memory tracker itself doesn't use allocations which would
    // -- interfere with what it is tracking...
    sm_instance = new CMemoryTracker();
}

void CMemoryTracker::Shutdown()
{
    // -- if we had been initialized, destroy the tracker
    if (sm_instance != nullptr)
    {
        TinFree(sm_instance);
        sm_instance = nullptr;
    }
}

void* CMemoryTracker::Alloc(eAllocType alloc_type, int32 size)
{
    // -- ensure we've been initialized
    void* addr = ::operator new(size);

    // -- if the memory tracker hasn't yet been initialized, it's because
    // -- we're performing allocations during context initialization
    if (sm_instance == nullptr)
        Initialize();

    // -- add the address to the table
    tAllocEntry* alloc_entry = new tAllocEntry(alloc_type, addr, size);
    uint32 hash = kPointerToUInt32(addr);
    int32 bucket = hash % k_trackedAllocationTableSize;
    alloc_entry->next = sm_instance->m_allocationTable[bucket];
    sm_instance->m_allocationTable[bucket] = alloc_entry;

    // -- update the allocation total
    sm_instance->m_allocationTotals[alloc_type] += size;

    return (addr);
}

void CMemoryTracker::Free(void* addr)
{
    Assert_(sm_instance != nullptr);

    // -- find and remove the allocation entry
    uint32 hash = kPointerToUInt32(addr);
    int32 bucket = hash % k_trackedAllocationTableSize;
    tAllocEntry* alloc_entry = nullptr;
    if (kPointerToUInt32(sm_instance->m_allocationTable[bucket]->addr) == hash)
    {
        alloc_entry = sm_instance->m_allocationTable[bucket];
        sm_instance->m_allocationTable[bucket] = alloc_entry->next;
    }
    else
    {
        tAllocEntry* alloc_prev = sm_instance->m_allocationTable[bucket];
        while (alloc_prev != nullptr)
        {
            if (alloc_prev->next != nullptr && kPointerToUInt32(alloc_prev->next->addr))
            {
                alloc_entry = alloc_prev->next;
                alloc_prev->next = alloc_entry->next;
                break;
            }

            alloc_prev = alloc_prev->next;
        }
    }

    // -- we had better have found a matching entry
    Assert_(alloc_entry != nullptr);

    // -- update the allocation total
    sm_instance->m_allocationTotals[alloc_entry->type] -= alloc_entry->size;
    Assert_(sm_instance->m_allocationTotals[alloc_entry->type] >= 0);

    // -- delete the entry
    delete alloc_entry;
}

void CMemoryTracker::DumpTotals()
{
    // -- nothing to do if the memory tracker isn't enabled
    if (CMemoryTracker::sm_instance == nullptr)
    {
        TinPrint(TinScript::GetContext(),
                 "Not available: #define MEMORY_TRACKER_ENABLE 1, in integration.h");
        return;
    }

    // -- print the totals from the table
    for (int32 i = 0; i < AllocType_Count; ++i)
    {
        TinPrint(TinScript::GetContext(), "%s: %d\n", g_allocationTypeNames[i],
                 CMemoryTracker::sm_instance->m_allocationTotals[i])
    }
}

REGISTER_FUNCTION_P0(MemoryDump, CMemoryTracker::DumpTotals, void);

}

// --------------------------------------------------------------------------------------------------------------------
// eof
// --------------------------------------------------------------------------------------------------------------------

