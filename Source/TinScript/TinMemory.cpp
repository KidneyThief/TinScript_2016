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
#include "TinExecute.h"
#include "TinRegistration.h"

// -- use the DECLARE_FILE/REGISTER_FILE macros to prevent deadstripping
DECLARE_FILE(tinmemory_cpp);

namespace TinScript
{

// -- thread singleton ------------------------------------------------------------------------------------------------

_declspec(thread) CMemoryTracker* g_memoryTrackerInstance = nullptr;

// -- create the string table of allocation type names
const char* g_allocationTypeNames[] =
{
    #define AllocTypeEntry(a) "ALLOC_" #a,
    AllocTypeTuple
    #undef AllocTypeEntry
};

// == class CMemoryTracker ============================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CMemoryTracker::CMemoryTracker()
{
    // -- initialize the array of allocations
    for (int32 i = 0; i < AllocType_Count; ++i)
        m_allocationTotals[i] = 0;

    // -- initialize our untracked hash tables
    for (int i = 0; i < k_trackedAllocationTableSize; ++i)
    {
        m_allocationTable[i] = nullptr;
        m_objectCreatedTable[i] = nullptr;
        m_objectCreatedFileLineTable[i] = nullptr;
    }
}

// ====================================================================================================================
// Destructor
// ====================================================================================================================
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

        tObjectCreateEntry* object_entry = m_objectCreatedTable[i];
        while (object_entry != nullptr)
        {
            tObjectCreateEntry* next = object_entry->next;
            delete object_entry;
            object_entry = next;
        }

        tObjectCreatedFileLine* object_file_line = m_objectCreatedFileLineTable[i];
        while (object_file_line != nullptr)
        {
            tObjectCreatedFileLine* next = object_file_line->next;
            delete object_file_line;
            object_file_line = next;
        }
    }
}

// ====================================================================================================================
// Initialize():  Called on the first allocation, initializes the system (creates the thread singleton)
// ====================================================================================================================
void CMemoryTracker::Initialize()
{
    // -- nothing to do if we've already initialized the tracker
    if (g_memoryTrackerInstance != nullptr)
        return;

    // -- the memory tracker itself doesn't use allocations which would
    // -- interfere with what it is tracking...
    g_memoryTrackerInstance = new CMemoryTracker();
}

// ====================================================================================================================
// Shutdown():  Shuts down the system, destroys the thread singleton
// ====================================================================================================================
void CMemoryTracker::Shutdown()
{
    // -- if we had been initialized, destroy the tracker
    if (g_memoryTrackerInstance != nullptr)
    {
        delete g_memoryTrackerInstance;
        g_memoryTrackerInstance = nullptr;
    }
}

// ====================================================================================================================
// Alloc():  If the system is enabled, this is a wrapper to allocate memory, and add track it in the allocation tables.
// ====================================================================================================================
void* CMemoryTracker::Alloc(eAllocType alloc_type, int32 size)
{
   
    if (size == 0)
    {
        return nullptr;
    }
    
    // -- ensure we've been initialized
    void* addr = ::operator new(size);

    // -- if this is not the main thread, don't track the allocation
    if (TinScript::GetContext() == nullptr || !TinScript::GetContext()->IsMainThread())
        return (addr);

    // -- if the memory tracker hasn't yet been initialized, it's because
    // -- we're performing allocations during context initialization
    if (g_memoryTrackerInstance == nullptr)
        Initialize();

    // -- add the address to the table
    tAllocEntry* alloc_entry = new tAllocEntry(alloc_type, addr, size);
    uint32 hash = kPointerToUInt32(addr);
    int32 bucket = hash % k_trackedAllocationTableSize;
    alloc_entry->next = g_memoryTrackerInstance->m_allocationTable[bucket];
    g_memoryTrackerInstance->m_allocationTable[bucket] = alloc_entry;

    // -- update the allocation total
    g_memoryTrackerInstance->m_allocationTotals[alloc_type] += size;

    return (addr);
}

// ====================================================================================================================
// Free():  If enabled, this wrapper updates the allocation tables
// ====================================================================================================================
void CMemoryTracker::Free(void* addr)
{
    // -- if we have no memory tracker, we're done
    if (g_memoryTrackerInstance == nullptr)
        return;

    // -- find and remove the allocation entry
    uint32 hash = kPointerToUInt32(addr);
    int32 bucket = hash % k_trackedAllocationTableSize;

    // -- note:  the CMemoryTracker lives in the main thread - however, any allocation from
    // -- the socket thread will not be tracked - but it could be freed from the main thread
    // -- causing alloc_entry to be null
    if (g_memoryTrackerInstance->m_allocationTable[bucket] == nullptr)
        return;

    tAllocEntry* alloc_entry = nullptr;
    if (kPointerToUInt32(g_memoryTrackerInstance->m_allocationTable[bucket]->addr) == hash)
    {
        alloc_entry = g_memoryTrackerInstance->m_allocationTable[bucket];
        g_memoryTrackerInstance->m_allocationTable[bucket] = alloc_entry->next;
    }
    else
    {
        tAllocEntry* alloc_prev = g_memoryTrackerInstance->m_allocationTable[bucket];
        while (alloc_prev->next != nullptr && kPointerToUInt32(alloc_prev->next->addr) != hash)
            alloc_prev = alloc_prev->next;

        // -- if we found our allocation, remove it from the list
        if (alloc_prev->next != nullptr)
        {
            alloc_entry = alloc_prev->next;
            alloc_prev->next = alloc_prev->next->next;
        }
    }

    if (alloc_entry == nullptr)
        return;

    // -- update the allocation total
    g_memoryTrackerInstance->m_allocationTotals[alloc_entry->type] -= alloc_entry->size;
    if (g_memoryTrackerInstance->m_allocationTotals[alloc_entry->type] < 0)
    {
        Assert_(g_memoryTrackerInstance->m_allocationTotals[alloc_entry->type] >= 0);
    }

    // -- delete the entry
    delete alloc_entry;
}

// ====================================================================================================================
// CalculateFileLineHash():  Creates a hash value, for tracking the file/line origin where an object is created.
// ====================================================================================================================
uint32 CMemoryTracker::CalculateFileLineHash(uint32 codeblock_hash, int32 line_number)
{
    char buffer[kMaxArgLength];
    sprintf_s(buffer, "%s:%d", UnHash(codeblock_hash), line_number);
    uint32 file_line_hash = Hash(buffer);
    return (file_line_hash);
}

// ====================================================================================================================
// Constructor for the helper class, implementing a non-tracked simple hash table
// ====================================================================================================================
CMemoryTracker::tObjectCreateEntry::tObjectCreateEntry(uint32 _object_id, int32 _stack_size,
                                                       uint32* _codeblock_array, int32* _line_number_array)
{
    object_id = _object_id;
    stack_size = _stack_size < 0 ? 0 : _stack_size > kDebuggerCallstackSize ? kDebuggerCallstackSize : _stack_size;
    next = nullptr;
    if (stack_size > 0 && _codeblock_array != nullptr && _line_number_array != nullptr)
    {
        memcpy(codeblock_array, _codeblock_array, sizeof(uint32) * stack_size);
        memcpy(line_number_array, _line_number_array, sizeof(int32) * stack_size);
    }

    file_line_hash = stack_size > 0 ? CMemoryTracker::CalculateFileLineHash(codeblock_array[0], line_number_array[0])
                                    : 0;
}

// ====================================================================================================================
// Constructor for the helper class, implementing a non-tracked simple hash table
// ====================================================================================================================
CMemoryTracker::tObjectCreatedFileLine::tObjectCreatedFileLine(uint32 _file_line_hash, uint32 _codeblock_hash,
                                                               int32 _line_number)
{
    file_line_hash = _file_line_hash;
    codeblock_hash = _codeblock_hash;
    line_number = _line_number;

    object_count = 0;
    next = nullptr;
}

// ====================================================================================================================
// NotifyObjectCreated():  Called from the virtual machine, when an object is created, tracking the file/line.
// ====================================================================================================================
void CMemoryTracker::NotifyObjectCreated(uint32 object_id, const CFunctionCallStack* funccallstack)
{
    if (g_memoryTrackerInstance == nullptr || funccallstack == nullptr)
    {
        return;
    }

    // -- build the callstack arrays, in preparation to send them to the debugger
    uint32 codeblock_array[kDebuggerCallstackSize];
    uint32 objid_array[kDebuggerCallstackSize];
    uint32 namespace_array[kDebuggerCallstackSize];
    uint32 func_array[kDebuggerCallstackSize];
    int32 linenumber_array[kDebuggerCallstackSize];
    int32 stack_size =
        funccallstack->DebuggerGetCallstack(codeblock_array,objid_array,
                                            namespace_array,func_array,
                                            linenumber_array, kDebuggerCallstackSize);

    // -- create the entry, and add it to the object created hash table
    // -- add the address to the table
    tObjectCreateEntry* object_entry = new tObjectCreateEntry(object_id, stack_size, codeblock_array, linenumber_array);
    int32 bucket = object_id % k_trackedAllocationTableSize;
    object_entry->next = g_memoryTrackerInstance->m_objectCreatedTable[bucket];
    g_memoryTrackerInstance->m_objectCreatedTable[bucket] = object_entry;

    // -- now add the object entry to the file_line hashtable
    int32 file_line_bucket = object_entry->file_line_hash % k_trackedAllocationTableSize;
    tObjectCreatedFileLine* file_line_entry = new tObjectCreatedFileLine(object_entry->file_line_hash,
                                                    stack_size > 0 ? codeblock_array[0] : 0,
                                                    stack_size > 0 ? linenumber_array[0] : -1);
    file_line_entry->next = g_memoryTrackerInstance->m_objectCreatedFileLineTable[file_line_bucket];
    g_memoryTrackerInstance->m_objectCreatedFileLineTable[file_line_bucket] = file_line_entry;

    // -- increment the count
    ++file_line_entry->object_count;
}

// ====================================================================================================================
// NotifyObjectDestroyed():  Called from the virtual machine, when an object is destroyed.
// ====================================================================================================================
void CMemoryTracker::NotifyObjectDestroyed(uint32 object_id)
{
    // -- if we have no memory tracker, we're done
    if (g_memoryTrackerInstance == nullptr)
        return;

    // -- find the entry in the object creation table
    tObjectCreateEntry* object_entry = nullptr;
    int32 bucket = object_id % k_trackedAllocationTableSize;
    if (g_memoryTrackerInstance->m_objectCreatedTable[bucket] != nullptr &&
        g_memoryTrackerInstance->m_objectCreatedTable[bucket]->object_id == object_id)
    {
        object_entry = g_memoryTrackerInstance->m_objectCreatedTable[bucket];
        g_memoryTrackerInstance->m_objectCreatedTable[bucket] = object_entry->next;
    }
    else
    {
        tObjectCreateEntry* object_prev_entry = g_memoryTrackerInstance->m_objectCreatedTable[bucket];
        while (object_prev_entry != nullptr && object_prev_entry->next != nullptr &&
            object_prev_entry->next->object_id != object_id)
        {
            object_prev_entry = object_prev_entry->next;
        }

        // -- if we found our prev entry, set the object entry, and remove it from the list
        if (object_prev_entry != nullptr && object_prev_entry->next != nullptr)
        {
            object_entry = object_prev_entry->next;
            object_prev_entry->next = object_entry->next;
        }
    }

    // -- we had better have found a matching entry
    Assert_(object_entry != nullptr);

    //-- find the entry in the object file/line table
    // -- in this case, we decriment the object count, and only remove if the count is zero
    tObjectCreatedFileLine* file_line_entry = nullptr;
    int32 file_line_bucket = object_entry->file_line_hash % k_trackedAllocationTableSize;
    if (g_memoryTrackerInstance->m_objectCreatedFileLineTable[file_line_bucket] != nullptr &&
        g_memoryTrackerInstance->m_objectCreatedFileLineTable[file_line_bucket]->file_line_hash == object_entry->file_line_hash)
    {
        file_line_entry = g_memoryTrackerInstance->m_objectCreatedFileLineTable[file_line_bucket];
        --file_line_entry->object_count;
        if (file_line_entry->object_count <= 0 )
            g_memoryTrackerInstance->m_objectCreatedFileLineTable[file_line_bucket] = file_line_entry->next;
    }
    else
    {
        tObjectCreatedFileLine* prev_file_line_entry = g_memoryTrackerInstance->m_objectCreatedFileLineTable[file_line_bucket];
        while (prev_file_line_entry != nullptr && prev_file_line_entry->next != nullptr &&
               prev_file_line_entry->next->file_line_hash != object_entry->file_line_hash)
        {
            prev_file_line_entry = prev_file_line_entry->next;
        }

        // -- if we found our previous entry, set the current entry, and remove from the list
        if (prev_file_line_entry != nullptr && prev_file_line_entry->next != nullptr)
        {
            file_line_entry = prev_file_line_entry->next;
            --file_line_entry->object_count;
            if (file_line_entry->object_count <= 0)
                prev_file_line_entry->next = file_line_entry->next;
        }
    }

    // -- we had better have found our entry
    Assert_(file_line_entry != nullptr);

    // -- delete the object entry
    delete object_entry;

    // -- if the count of the file_line entry is <= 0, delete it
    if (file_line_entry->object_count <= 0)
        delete file_line_entry;
}

// ====================================================================================================================
// GetCreatedCallstack():  Given a object_id, find the file/line callstack from where this object was instantiated.
// ====================================================================================================================
bool8 CMemoryTracker::GetCreatedCallstack(uint32 object_id, int32& out_stack_size, uint32* out_file_array,
                                         int32* out_lines_array)
{
    // -- nothing to do if the memory tracker isn't enabled
    if (g_memoryTrackerInstance == nullptr)
        return (false);

    int32 bucket = object_id % k_trackedAllocationTableSize;
    tObjectCreateEntry* object_entry = g_memoryTrackerInstance->m_objectCreatedTable[bucket];
    while (object_entry != nullptr && object_entry->object_id != object_id)
        object_entry = object_entry->next;

    // -- if we found our entry, populate the object's creation stack
    if (object_entry != nullptr && object_entry->stack_size > 0)
    {
        CObjectEntry* oe = TinScript::GetContext()->FindObjectEntry(object_id);
        if (oe != nullptr)
        {
            out_stack_size = object_entry->stack_size;
            memcpy(out_file_array, object_entry->codeblock_array, sizeof(uint32) * out_stack_size);
            memcpy(out_lines_array, object_entry->line_number_array, sizeof(int32) * out_stack_size);
            return (true);
        }
    }

    // -- not found
    return (false);
}

// ====================================================================================================================
// DumpTotals():  Registered method to dump the bytes allocated for each TinAlloc() allocation type.
// ====================================================================================================================
void CMemoryTracker::DumpTotals()
{
    // -- nothing to do if the memory tracker isn't enabled
    if (g_memoryTrackerInstance == nullptr)
    {
        TinPrint(TinScript::GetContext(),
                 "Not available: #define MEMORY_TRACKER_ENABLE 1, in integration.h");
        return;
    }

    // -- print the totals from the table
    for (int32 i = 0; i < AllocType_Count; ++i)
    {
        TinPrint(TinScript::GetContext(), "%s: %d\n", g_allocationTypeNames[i],
                 g_memoryTrackerInstance->m_allocationTotals[i])
    }
}

// ====================================================================================================================
// DumpObjects():  List the number of objects created from each unique file/line location.
// ====================================================================================================================
void CMemoryTracker::DumpObjects()
{
    // -- nothing to do if the memory tracker isn't enabled
    if (g_memoryTrackerInstance == nullptr)
    {
        TinPrint(TinScript::GetContext(),
                 "Not available: #define MEMORY_TRACKER_ENABLE 1, in integration.h");
        return;
    }

    // -- loop through the file/line created table, and dump the count from each location
    // -- note:  editors count from 1, TinScript counts line numbers from 0
    for (int32 i = 0; i < k_trackedAllocationTableSize; ++i)
    {
        tObjectCreatedFileLine* file_line_entry = g_memoryTrackerInstance->m_objectCreatedFileLineTable[i];
        while (file_line_entry != nullptr)
        {
            TinPrint(TinScript::GetContext(), "%3d objects from: %s @ %d\n", file_line_entry->object_count,
                     UnHash(file_line_entry->codeblock_hash), file_line_entry->line_number + 1);
            file_line_entry = file_line_entry->next;
        }
    }
}

// ====================================================================================================================
// FindObject():  Given a object_id, print the script file/line from which this object was created.
// ====================================================================================================================
void CMemoryTracker::FindObject(const char* object_name)
{
    // -- nothing to do if the memory tracker isn't enabled
    if (g_memoryTrackerInstance == nullptr)
    {
        TinPrint(TinScript::GetContext(),
            "Not available: #define MEMORY_TRACKER_ENABLE 1, in integration.h");
        return;
    }

    // -- find the object - first by name, then if the string was a number, by ID instead
    CObjectEntry* oe = TinScript::GetContext()->FindObjectByName(object_name);
    uint32 object_id = oe != nullptr ? oe->GetID() : 0;
    if (object_id == 0)
    {
        object_id = TinScript::Atoi(object_name, -1);
    }

    int32 bucket = object_id % k_trackedAllocationTableSize;
    tObjectCreateEntry* object_entry = g_memoryTrackerInstance->m_objectCreatedTable[bucket];
    while (object_entry != nullptr && object_entry->object_id != object_id)
        object_entry = object_entry->next;

    // -- if we found our entry, print out the file/line origin  (add 1 since editors don't count from 0)
    if (object_entry != nullptr)
    {
        oe = TinScript::GetContext()->FindObjectEntry(object_id);
        if (oe != nullptr)
            TinScript::GetContext()->PrintObject(oe);
        if (object_entry->stack_size > 0)
        {
            TinPrint(TinScript::GetContext(), "MemoryFindObject(): object %d creation callstack:\n",  object_id);
            for (int32 i = 0; i < object_entry->stack_size; ++i)
            {
                TinPrint(TinScript::GetContext(),"    %s: %d\n", UnHash(object_entry->codeblock_array[i]),
                         object_entry->line_number_array[i] + 1);
            }
        }
        else
        {
            TinPrint(TinScript::GetContext(),"MemoryFindObject(): object %d created from <stdin>\n",object_id);
        }
    }
    else
    {
        TinPrint(TinScript::GetContext(), "MemoryFindObject(): object '%s' not found\n", object_name);
    }
}

// == Function Registration ===========================================================================================

REGISTER_CLASS_FUNCTION(CMemoryTracker, MemoryDumpTotals, DumpTotals);
REGISTER_CLASS_FUNCTION(CMemoryTracker, MemoryDumpObjects, DumpObjects);
REGISTER_CLASS_FUNCTION(CMemoryTracker, MemoryFindObject, FindObject);

}

// --------------------------------------------------------------------------------------------------------------------
// eof
// --------------------------------------------------------------------------------------------------------------------

