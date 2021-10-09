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

// ------------------------------------------------------------------------------------------------
// integration.h
// ------------------------------------------------------------------------------------------------


#ifndef __INTEGRATION_H
#define __INTEGRATION_H

// -- system includes -------------------------------------------------------------------------------------------------

#include <new>
#include <cstdint>

// -- The stack size is a limiting factor, if TinScript is to be used for any recursive scripting.
// -- Highly recommended to avoid this, as there are no optimizations (such as tail-end recursion)
// -- Testing with the implementation of Fibonacci(nth) found in unittest.ts, The maximum
// -- depth with the default stack is Fibonacci(12).
// -- Setting a stack size of 4Mb, Fibonacci(30) was successful, but prohibitively slow.
#pragma comment(linker, "/STACK:4000000")

#ifdef _DEBUG
    #define MEMORY_TRACKER_ENABLE 1
#else
    #define MEMORY_TRACKER_ENABLE 0
#endif

// -- define whether we're 64-bit
// note:  not overly robust, as otherwise, we force 32-bit
// -- until we actually port TinScript to other platforms,
// 32-bit windows and 64-bit windows UE4 are my only tested environments
#if INTPTR_MAX == INT64_MAX
    #define BUILD_64 1
#else
    #define BUILD_64 0
#endif

// -- some platforms (e.g. UE4) need special treatment
// -- modify the target copy of this file with the appropriate define(s)
#define PLATFORM_UE4 0

// ------------------------------------------------------------------------------------------------
// -- TYPES
// ------------------------------------------------------------------------------------------------

// NOTE:  if you change typedef int32 from an int (to say, a long), the registered types in TinTypes.h will not match
#if !PLATFORM_UE4
    typedef char                int8;
    typedef unsigned char       uint8;
    typedef short               int16;
    typedef unsigned short      uint16;
    typedef int                 int32;
    typedef unsigned int        uint32;
    typedef long long           int64;
    typedef unsigned long long  uint64;
#endif
// -- a little unusual - pre-Cx11, I wanted all types to have a width specifier
typedef bool                bool8;
typedef float               float32;

#define kBytesToWordCount(a) ((a) + 3) / 4;
#define kPointerToUInt64(a) (*(uint64*)(&a))

// -- pointer conversion macros to assist with compiler complaints
#if !BUILD_64
    #define kPointerToUInt32(a) ((uint32)(*(uint32*)(&a)))
    #define kPointerDiffUInt32(a, b) ((uint32)(((uint32)(a)) - ((uint32)(b))))
#else
    #define kPointerToUInt32(a) ((uint32)(*(uint64*)(&a)))
    #define kPointerDiffUInt32(a, b) ((uint32)(((uint64)(a)) - ((uint64)(b))))
    #define kPointer64FromUInt32(a, b) ((uint64*)((static_cast<uint64>(a) << 32) + (static_cast<uint64>(b))))
    #define kPointer64UpperUInt32(a) ((uint32)(((uint64)a) >> 32))
    #define kPointer64LowerUInt32(a) ((uint32)(((uint64)a) & 0xffffffff))
#endif

#define Unused_(var) __pragma(warning(suppress:4100)) var
#define Offsetof_(s,m) (uint32)(unsigned long long)&(((s *)0)->m)

#define Assert_(condition) assert(condition)

// ====================================================================================================================
// -- CONSTANTS
// ====================================================================================================================

const int32 kMaxArgs = 256;
const int32 kMaxArgLength = 256;

// -- change this constant, if you genregclasses.py -maxparam X, to generate higher count templated bindings
const int32 kMaxRegisteredParameterCount = 8;

const int32 kMaxVariableArraySize = 256;

const int32 kScriptContextThreadSize = 7;

const int32 kDebuggerCallstackSize = 32;
const int32 kDebuggerWatchWindowSize = 128;
const int32 kBreakpointTableSize = 17;

const int32 kGlobalFuncTableSize = 251;
const int32 kGlobalVarTableSize = 251;

const int32 kLocalFuncTableSize = 17;
const int32 kLocalVarTableSize = 17;

const int32 kExecStackSize = 4096;
const int32 kExecFuncCallDepth = 2048;
const int32 kExecFuncCallMaxLocalObjects = 32;

const int32 kStringTableSize = 1024 * 1024;
const int32 kStringTableDictionarySize = 1553;

const int32 kObjectTableSize = 10007;

const int32 kMasterMembershipTableSize = 97;
const int32 kObjectGroupTableSize = 17;
const int32 kHashTableIteratorTableSize = 7;

const int32 kMaxScratchBuffers = 32;

const int32 kThreadExecBufferSize = 32 * 1024;

// ------------------------------------------------------------------------------------------------
// -- MEMORY
// ------------------------------------------------------------------------------------------------

// -- memory allocation types - to adjust to custom memory strategies
// -- e.g.  TreeNode is temporary mem used only for compiling
// --       VarEntry, ObjEntry, FuncEntry are all fixed sizes, and would
// --       perform well if allocated from a memory pool...
#define AllocTypeTuple              \
    AllocTypeEntry(ScriptContext)   \
    AllocTypeEntry(TreeNode)        \
    AllocTypeEntry(CodeBlock)       \
    AllocTypeEntry(FuncCallStack)   \
    AllocTypeEntry(VarTable)        \
    AllocTypeEntry(FuncTable)       \
    AllocTypeEntry(FuncEntry)       \
    AllocTypeEntry(FuncContext)     \
    AllocTypeEntry(VarEntry)        \
    AllocTypeEntry(VarStorage)      \
    AllocTypeEntry(HashTable)       \
    AllocTypeEntry(ObjEntry)        \
    AllocTypeEntry(Namespace)       \
    AllocTypeEntry(SchedCmd)        \
    AllocTypeEntry(FuncCallEntry)   \
    AllocTypeEntry(CreateObj)       \
    AllocTypeEntry(StringTable)     \
    AllocTypeEntry(ObjectGroup)     \
    AllocTypeEntry(FileBuf)         \
    AllocTypeEntry(Debugger)        \

enum eAllocType
{
    #define AllocTypeEntry(a) ALLOC_##a,
    AllocTypeTuple
    #undef AllocTypeEntry
    AllocType_Count
};

// -- forward declarations --------------------------------------------------------------------------------------------

namespace TinScript
{
    class CScriptContext;
    void NotifyMemoryAlloc(eAllocType alloc_type, int32 size);
}

// -- allocation macros -----------------------------------------------------------------------------------------------

#if MEMORY_TRACKER_ENABLE

#define TinAlloc(alloctype, T, ...) \
    new (reinterpret_cast<T*>(TinScript::CMemoryTracker::Alloc(alloctype, sizeof(T)))) T(__VA_ARGS__);

#define TinFree(addr) \
    { \
        void* notify_addr = (void*)addr; \
        TinScript::CMemoryTracker::Free(notify_addr);                    \
        delete addr; \
    }

#define TinAllocArray(alloctype, T, size) \
        new (reinterpret_cast<T*>(TinScript::CMemoryTracker::Alloc(alloctype, sizeof(T) * size))) T[size];

#define TinFreeArray(addr) \
    { \
        void* notify_addr = (void*)addr; \
        TinScript::CMemoryTracker::Free(notify_addr); \
        delete[] addr; \
    }

#define TinObjectCreated(object_id, funccallstack) \
    TinScript::CMemoryTracker::NotifyObjectCreated(object_id, funccallstack);

#define TinObjectDestroyed(object_id) \
    TinScript::CMemoryTracker::NotifyObjectDestroyed(object_id);

#else

#define TinAlloc(alloctype, T, ...)                                         \
    new (reinterpret_cast<T*>(::operator new(sizeof(T)))) T(__VA_ARGS__);

#define TinFree(addr) \
    delete addr;

#define TinAllocArray(alloctype, T, size) \
    new T[size];


#define TinFreeArray(addr) \
    delete [] addr;

#define TinObjectCreated(object_id, codeblock_hash, line_number) ;
#define TinObjectDestroyed(object_id);

#endif

// ------------------------------------------------------------------------------------------------
// Misc hooks
// ------------------------------------------------------------------------------------------------
// -- Pass a function of the following prototype when creating the CScriptContext
typedef bool8 (*TinAssertHandler)(TinScript::CScriptContext* script_context, const char* condition,
                                  const char* file, int32 linenumber, const char* fmt, ...);
#define ScriptAssert_(scriptcontext, condition, file, linenumber, fmt, ...)                     \
    {                                                                                           \
        if(!(condition) && (!scriptcontext->mDebuggerConnected ||								\
							!scriptcontext->mDebuggerBreakLoopGuard)) {                         \
            if(!scriptcontext->GetAssertHandler()(scriptcontext, #condition, file, linenumber,  \
                                                  fmt, ##__VA_ARGS__)) {                        \
                __debugbreak();                                                                 \
            }                                                                                   \
        }                                                                                       \
    }

// -- Pass a function of the following prototype when creating the CScriptContext
typedef int32 (*TinPrintHandler)(const char* fmt, ...);
#define TinPrint(scriptcontext, fmt, ...)                           \
    if (scriptcontext != NULL)                                      \
    {                                                               \
        scriptcontext->GetPrintHandler()(fmt, ##__VA_ARGS__);       \
        scriptcontext->DebuggerSendPrint(fmt, ##__VA_ARGS__);       \
    }

#endif // __INTEGRATION_H

// ------------------------------------------------------------------------------------------------
// eof
// ------------------------------------------------------------------------------------------------
