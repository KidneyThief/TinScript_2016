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
// TinScript.cpp
// ====================================================================================================================

#include "integration.h"

// -- includes
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <iostream>
#include <iomanip>

#include "socket.h"
#include "TinHash.h"
#include "TinHashtable.h"
#include "TinParse.h"
#include "TinCompile.h"
#include "TinExecute.h"
#include "TinNamespace.h"
#include "TinScheduler.h"
#include "TinObjectGroup.h"
#include "TinStringTable.h"
#include "TinRegistration.h"
#include "TinOpExecFunctions.h"
#include "TinScript.h"

// == namespace TinScript =============================================================================================

namespace TinScript
{

// --------------------------------------------------------------------------------------------------------------------
// -- statics
static const char* gStringTableFileName = "stringtable.txt";

bool8 CScriptContext::gDebugParseTree = false;
bool8 CScriptContext::gDebugCodeBlock = false;
bool8 CScriptContext::gDebugTrace = false;

bool8 CScriptContext::gDebugForceCompile = false;
std::time_t CScriptContext::gDebugForceCompileTime;

const char* CScriptContext::kGlobalNamespace = "_global";
uint32 CScriptContext::kGlobalNamespaceHash = Hash(CScriptContext::kGlobalNamespace);

// -- this is a *thread* variable, each thread can reference a separate context
_declspec(thread) CScriptContext* gThreadContext = NULL;
CScriptContext* gMainThreadContext = NULL;

// == Interface implementation ========================================================================================

// ====================================================================================================================
// CreateContext():  Creates a singleton context, max of one for each thread
// ====================================================================================================================
CScriptContext* CreateContext(TinPrintHandler printhandler, TinAssertHandler asserthandler, bool is_main_thread)
{
    CScriptContext* script_context = CScriptContext::Create(printhandler, asserthandler, is_main_thread);
    return (script_context);
}

// ====================================================================================================================
// UpdateContext():  Updates the singleton context in the calling thread
// ====================================================================================================================
void UpdateContext(uint32 current_time_msec)
{
    // -- during shutdown, the context may become null
    CScriptContext* script_context = GetContext();
    if (script_context != nullptr)
        script_context->Update(current_time_msec);
}

// ====================================================================================================================
// DestroyContext():  Destroys the context created from the calling thread
// ====================================================================================================================
void DestroyContext()
{
    CScriptContext::Destroy();
}

// ====================================================================================================================
// GetContext():  Uses a thread local global var to return the specific context created from this thread
// ====================================================================================================================
CScriptContext* GetContext()
{
    return (gThreadContext);
}

// ====================================================================================================================
// GetMainThreadContext():  returns the script context for the MainThread... needed if we receive a
// a remote command via the socket...
// ====================================================================================================================
CScriptContext* GetMainThreadContext()
{
    return (gMainThreadContext);
}

// ====================================================================================================================
// ExecCommand():  Executes a text block of valid script code
// ====================================================================================================================
bool8 ExecCommand(const char* statement)
{
    CScriptContext* script_context = GetContext();
    assert(script_context != NULL);
    return (script_context->ExecCommand(statement));
}

// ====================================================================================================================
// CompileScript():  Compiles (without executing) a text file containing script code
// ====================================================================================================================
bool8 CompileScript(const char* filename)
{
    CScriptContext* script_context = GetContext();
    assert(script_context != NULL);
    CCodeBlock* codeblock = script_context->CompileScript(filename);
    return (codeblock != NULL);
}

// ====================================================================================================================
// CompileToC():  Compile a script to a 'C source header 
// ====================================================================================================================
bool8 CompileToC(const char* filename)
{
    CScriptContext* script_context = GetContext();
    assert(script_context != NULL);
    return script_context->CompileToC(filename);
}

// ====================================================================================================================
// SetDirectory():  sets the current working directory, so all scripts executed will have their path prepended
// ====================================================================================================================
bool8 SetDirectory(const char* path)
{
    CScriptContext* script_context = GetContext();
    assert(script_context != NULL);
    return (script_context->SetDirectory(path));
}

// ====================================================================================================================
// ExecScript():  Executes a text file containing script code
// ====================================================================================================================
bool8 ExecScript(const char* filename, bool allow_no_exist)
{
    CScriptContext* script_context = GetContext();
    assert(script_context != NULL);
    return (script_context->ExecScript(filename, !allow_no_exist, true));
}

// ====================================================================================================================
// IncludeScript():  Same as ExecScript(), the file must exist, but need not be executed twice.
// ====================================================================================================================
bool8 IncludeScript(const char* filename)
{
    CScriptContext* script_context = GetContext();
    assert(script_context != NULL);
    return (script_context->ExecScript(filename, true, false));
}

// ====================================================================================================================
// SetTimeScale():  Allows for accurate communication with the debugger, if the application adjusts timescale
// ====================================================================================================================
void SetTimeScale(float time_scale)
{
    CScriptContext* script_context = GetContext();
    assert(script_context != NULL);
    script_context->GetScheduler()->SetSimTimeScale(time_scale);
}

// -- Registration ----------------------------------------------------------------------------------------------------

REGISTER_FUNCTION(Compile, CompileScript);
REGISTER_FUNCTION(SetDirectory, SetDirectory);
REGISTER_FUNCTION(Exec, ExecScript);
REGISTER_FUNCTION(Include, IncludeScript);
REGISTER_FUNCTION(CompileToC, CompileToC);

// ====================================================================================================================
// NullAssertHandler():  Default assert handler called, if one isn't provided
// ====================================================================================================================
bool8 NullAssertHandler(CScriptContext*, const char*, const char*, int32, const char*, ...)
{
    return false;
}

// ====================================================================================================================
// NullAssertHandler():  Default assert handler called, if one isn't provided
// ====================================================================================================================
int NullPrintHandler(int32 severity, const char*, ...)
{
	Unused_(severity);
    return (0);
}

// == CScriptContext ==================================================================================================

// ====================================================================================================================
// ResetAssertStack():  Allows the next assert to trace it's own (error) path
// ====================================================================================================================
void CScriptContext::ResetAssertStack()
{
    mAssertStackSkipped = false;
}

// ====================================================================================================================
// Create():  Static interface - only one context per thread
// ====================================================================================================================
CScriptContext* CScriptContext::Create(TinPrintHandler printhandler, TinAssertHandler asserthandler,
                                       bool is_main_thread)
{
    // -- only one script context per thread
    if (gThreadContext != NULL)
    {
        assert(gThreadContext == NULL);
        return (gThreadContext);
    }

    // -- set the thread context
    TinAlloc(ALLOC_ScriptContext, CScriptContext, printhandler, asserthandler, is_main_thread);
    return (gThreadContext);
}

// ====================================================================================================================
// Destroy():  Destroys the context singleton specific to the calling thread
// ====================================================================================================================
void CScriptContext::Destroy()
{
    assert(gThreadContext != nullptr);

    // -- shutdown the memory tracker
    CMemoryTracker::Shutdown();

    // -- cleanup the namespace context
    // -- note:  the global namespace is owned by the namespace dictionary
    // -- within the context - it'll be automatically cleaned up
    gThreadContext->ShutdownDictionaries();

    // -- cleanup all related codeblocks
    // -- by deleting the namespace dictionaries, all codeblocks should now be unused
    gThreadContext->mDeferredBreakpointsList.DestroyAll();
    CCodeBlock::DestroyUnusedCodeBlocks(gThreadContext->mCodeBlockList);
    assert(gThreadContext->mCodeBlockList->IsEmpty());
    TinFree(gThreadContext->mCodeBlockList);

    gThreadContext->mDefiningFunctionsList->RemoveAll();
    TinFree(gThreadContext->mDefiningFunctionsList);

    // -- clean up the scheduler
    TinFree(gThreadContext->mScheduler);

    // -- cleanup the membership list
    TinFree(gThreadContext->mMasterMembershipList);

    // -- clean up the string table
    TinFree(gThreadContext->mStringTable);

    // -- clean up the CHashtable class
    CHashtable::Shutdown();

    // -- if this is the MainThread context, shutdown types
    if (gThreadContext->mIsMainThread)
    {
        ShutdownTypes();
    }

    if (gThreadContext == gMainThreadContext)
        gMainThreadContext = nullptr;

	// -- we clear the thread context first, so destructors can tell we're
	// -- shutting down
	CScriptContext* currentContext = gThreadContext;
	gThreadContext = NULL;
	TinFree(currentContext);
}

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CScriptContext::CScriptContext(TinPrintHandler printfunction, TinAssertHandler asserthandler, bool is_main_thread)
{
    // -- set the flag
    mIsMainThread = is_main_thread;

    // -- initialize the ID generator
    mObjectIDGenerator = 0;

    // -- initialize the print msg ID generator
    mDebuggerPrintMsgId = 0;

    // -- set the thread local singleton
    gThreadContext = this;
    if (is_main_thread)
    {
        assert(gMainThreadContext == nullptr);
        gMainThreadContext = this;
    }

    // -- initialize and populate the string table
    mStringTable = TinAlloc(ALLOC_StringTable, CStringTable, this, kStringTableSize);
    LoadStringTable();

    // -- ensure our types have all been initialized - only from the main thread
    // -- this will set up global tables of type info... convert functions, op overrides, etc...
    if (is_main_thread)
    {
        InitializeTypes();
    }

    // -- set the handlers
    mTinPrintHandler = printfunction ? printfunction : NullPrintHandler;
    mTinAssertHandler = asserthandler ? asserthandler : NullAssertHandler;
    mAssertStackSkipped = false;

    // -- initialize the current working directory
    InitializeDirectory(true);

    // -- initialize the namespaces dictionary, and all object dictionaries
    InitializeDictionaries();

    // -- create the global namespace for this context
    mGlobalNamespace = FindOrCreateNamespace(NULL);

    // -- register functions, each to their namespace
    CRegFunctionBase* regfunc = CRegFunctionBase::gRegistrationList;
    while (regfunc != NULL)
    {
        if (!regfunc->Register())
        {
            if (regfunc->GetClassNameHash() != 0)
            {
                TinPrint(this, "Failed to register method %s::%s()",
                              TinScript::UnHash(regfunc->GetClassNameHash()), TinScript::UnHash(regfunc->GetFunctionNameHash()));
                assert(0);
            }
            else
            {
                TinPrint(this, "Failed to register function %s()",
                              TinScript::UnHash(regfunc->GetFunctionNameHash()));
                assert(0);
            }
        }
        regfunc = regfunc->GetNext();
    }

    // -- for all registered functions/methods, register the defined default values
    CRegDefaultArgValues::RegisterDefaultValues();

    // -- register globals
    CRegisterGlobal::RegisterGlobals(this);

    // -- after registration, we want to save the string table to ensure all registered
    // functions and global name hashes are available
    if (mIsMainThread)
        SaveStringTable();

    // -- initialize the scheduler
    mScheduler = TinAlloc(ALLOC_SchedCmd, CScheduler, this);

    // -- initialize the master object list
    mMasterMembershipList = TinAlloc(ALLOC_ObjectGroup, CMasterMembershipList, this, kMasterMembershipTableSize);

    // -- initialize the code block hash table
    mCodeBlockList = TinAlloc(ALLOC_HashTable, CHashTable<CCodeBlock>, kGlobalFuncTableSize);
    mDefiningFunctionsList = TinAlloc(ALLOC_HashTable, CHashTable<CFunctionEntry>, kGlobalFuncTableSize);

    // -- initialize the scratch buffer index
    mScratchBufferIndex = 0;

    // -- debugger members
    m_DebuggerAssertConnectTime = kExecAssertConnectWaitTime;
    m_AssertMsgStackDepth = kExecAssertStackDepth;
	mDebuggerSessionNumber = 0;
    mDebuggerConnected = false;
    mDebuggerActionForceBreak = false;
    mDebuggerActionStep = false;
    mDebuggerActionStepOver = false;
    mDebuggerActionStepOut = false;
    mDebuggerActionRun = true;

	mDebuggerBreakLoopGuard = false;
	mDebuggerBreakFuncCallStack = nullptr;
	mDebuggerBreakExecStack = nullptr;
	mDebuggerVarWatchRequestID = 0;
    mDebuggerWatchStackOffset = 0;
    mDebuggerForceExecLineNumber = -1;

    // -- initialize the thread command
    mThreadBufPtr = NULL;

    m_socketCommandList = nullptr;
    m_socketCurrentCommand = nullptr;
}

// ====================================================================================================================
// InitializeDictionaries():  Create the dictionaries, namespace, object, etc... and perform the startup registration.
// ====================================================================================================================
void CScriptContext::InitializeDictionaries()
{
    // -- allocate the dictionary to store creation functions
    mNamespaceDictionary = TinAlloc(ALLOC_HashTable, CHashTable<CNamespace>, kGlobalFuncTableSize);

    // -- allocate the dictionary to store the address of all objects created from script.
    mObjectDictionary = TinAlloc(ALLOC_HashTable, CHashTable<CObjectEntry>, kObjectTableSize);
    mAddressDictionary = TinAlloc(ALLOC_HashTable, CHashTable<CObjectEntry>, kObjectTableSize);
    mNameDictionary = TinAlloc(ALLOC_HashTable, CHashTable<CObjectEntry>, kObjectTableSize);

    // $$$TZA still working on how we're going to handle different threads
    // -- for now, every thread populates its dictionaries from the same list of registered objects
    CNamespaceReg* tempptr = CNamespaceReg::head;
    while (tempptr)
    {
        tempptr->SetRegistered(false);
        tempptr = tempptr->next;
    }

    // -- register the namespace - these are the namespaces
    // -- registered from code, so we need to populate the NamespaceDictionary,
    // -- and register the members/methods
    // -- note, because we register class derived from parent, we need to
    // -- iterate and ensure parents are always registered before children
    while (true)
    {
        CNamespaceReg* found_unregistered = NULL;
        bool8 abletoregister = false;
        CNamespaceReg* regptr = CNamespaceReg::head;
        while (regptr)
        {
            // -- see if this namespace is already registered
            if (regptr->GetRegistered())
            {
                regptr = regptr->GetNext();
                continue;
            }

            // -- there's at least one namespace awaiting registration
            found_unregistered = regptr;

            // -- see if this namespace still requires its parent to be registered
            static const uint32 nullparenthash = Hash("VOID");
            CNamespace* parentnamespace = NULL;
            if (regptr->GetParentHash() != nullparenthash)
            {
                parentnamespace = mNamespaceDictionary->FindItem(regptr->GetParentHash());
                if (!parentnamespace)
                {
                    // -- skip this one, and wait until the parent is registered
                    regptr = regptr->GetNext();
                    continue;
                }
            }

            // -- set the bool8 to track that we're actually making progress
            abletoregister = true;

            // -- ensure the namespace doesn't already exist
            CNamespace* namespaceentry = mNamespaceDictionary->FindItem(regptr->GetHash());
            if (namespaceentry == NULL)
            {
                // -- create the namespace
                CNamespace* newnamespace = TinAlloc(ALLOC_Namespace, CNamespace,
                                                    this, regptr->GetName(),
                                                    regptr->GetTypeID(),
                                                    regptr->GetCreateFunction(),
                                                    regptr->GetDestroyFunction());

                // -- add the creation method to the hash dictionary
                mNamespaceDictionary->AddItem(*newnamespace, regptr->GetHash());

                // -- link this namespace to its parent
                if (parentnamespace)
                {
                    if (!LinkNamespaces(newnamespace, parentnamespace))
                    {
                        ScriptAssert_(this, 0, "<internal>", -1,
                                      "Error - Failed to link namespace ::%s to parent namespace ::%s\n",
                                      UnHash(regptr->GetHash()), UnHash(regptr->GetParentHash()));
                        return;
                    }
                }

                // -- call the class registration method, to register members/methods
                regptr->RegisterNamespace(this, newnamespace);
                regptr->SetRegistered(true);
            }
            else
            {
                ScriptAssert_(this, 0, "<internal>", -1,
                              "Error - Namespace already created: %s\n",
                              UnHash(regptr->GetHash()));
                return;
            }

            regptr = regptr->GetNext();
        }

        // -- we'd better have registered at least one namespace, otherwise we're stuck
        if (found_unregistered && !abletoregister)
        {
            ScriptAssert_(this, 0, "<internal>", -1,
                          "Error - Unable to register Namespace: %s\n",
                          UnHash(found_unregistered->GetHash()));
            return;
        }

        // -- else see if we're done
        else if (!found_unregistered)
        {
            break;
        }
    }
}

// ====================================================================================================================
// Destructor
// ====================================================================================================================
CScriptContext::~CScriptContext()
{
    // -- ensure TinScript::Destroy() is called for a clean shutdown
    assert(gThreadContext == nullptr);
}

void CScriptContext::ShutdownDictionaries()
{
    // -- delete the Namespace dictionary
    if (mNamespaceDictionary)
    {
        mNamespaceDictionary->DestroyAll();
        TinFree(mNamespaceDictionary);
        mNamespaceDictionary = nullptr;
        mGlobalNamespace = nullptr;
    }

    // -- delete the Object dictionaries
    if (mObjectDictionary)
    {
        mObjectDictionary->DestroyAll();
        TinFree(mObjectDictionary);
        mObjectDictionary = nullptr;
    }

    // -- objects will have been destroyed above, so simply clear this hash table
    if (mAddressDictionary)
    {
        mAddressDictionary->RemoveAll();
        TinFree(mAddressDictionary);
        mAddressDictionary = nullptr;
    }

    if (mNameDictionary)
    {
        mNameDictionary->RemoveAll();
        TinFree(mNameDictionary);
        mNameDictionary = nullptr;
    }
}

void CScriptContext::Update(uint32 curtime)
{
    mScheduler->Update(curtime);

    // -- execute any commands queued from a different thread
    ProcessThreadCommands();

    // $$$TZA This doesn't need to happen every frame...
    CCodeBlock::DestroyUnusedCodeBlocks(mCodeBlockList);
}

// ====================================================================================================================
// Hash():  A core function for converting strings, used primarily for hash table keys.
// ====================================================================================================================
uint32 Hash(const char *string, int32 length, bool add_to_table)
{
	if (!string || !string[0])
		return 0;

    const char* s = string;
	int32 remaining = length;

	uint32 h = 5381;
	for (uint8 c = *s; c != '\0' && remaining != 0; c = *++s)
    {
		--remaining;

#if !CASE_SENSITIVE
        // -- if we're using this language as case insensitive, ensure the character is lower case
        if (c >= 'A' && c <= 'Z')
            c = 'z' + (c - 'A');
#endif

		h = ((h << 5) + h) + c;
	}

    // $$$TZA this should only happen in a DEBUG build
    // $$$TZA This is also not thread safe - only the main thread should be allowed to populate the
    // -- the string dictionary
    if (TinScript::GetContext() && TinScript::GetContext()->GetStringTable())
    {
        TinScript::GetContext()->GetStringTable()->AddString(string, length, h, add_to_table);
    }

	return h;
}

// ====================================================================================================================
// HashAppend():  Uses the same algorithm as Hash(), but allows "concatenation" of the string through multiple calls.
// ====================================================================================================================
uint32 HashAppend(uint32 h, const char *string, int32 length)
{
	if (!string || !string[0])
		return h;

    const char* s = string;
	int32 remaining = length;

	for (uint8 c = *s; c != '\0' && remaining != 0; c = *++s)
    {
		--remaining;
		h = ((h << 5) + h) + c;
	}
	return h;
}

// ====================================================================================================================
// UnHash():  Looks up the hash value in the string table, or returns a string version of the value.
// ====================================================================================================================
const char* UnHash(uint32 hash)
{
    const char* string = TinScript::GetContext()->GetStringTable()->FindString(hash);
    if (!string || !string[0])
    {
        static char buffers[8][20];
        static int32 bufindex = -1;
        bufindex = (bufindex + 1) % 8;
        sprintf_s(buffers[bufindex], 20, "<hash:0x%08x>", hash);
        return buffers[bufindex];
    }
    else
        return string;
}

// ====================================================================================================================
// GetStringTableName():  Returns the file name used to save/load the string table.
// ====================================================================================================================
const char* GetStringTableName()
{
    return (gStringTableFileName);
}

// ====================================================================================================================
// SaveStringTable():  Write the string table to a file (always writes to the .exe directory)
// ====================================================================================================================
void SaveStringTable()
{
    // -- ensure we have a valid filename
    const char* string_table_fn = GetStringTableName();

    // -- get the context for this thread
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context)
        return;

    CStringTable* string_table_src = script_context->GetStringTable();
    if (string_table_src == nullptr)
        return;

    const CHashTable<CStringTable::tStringEntry>* string_table = string_table_src->GetStringDictionary();
    if (!string_table)
        return;

    // -- before we write, lets remove all unreferenced strings
    string_table_src->RemoveUnreferencedStrings();

  	// -- open the file
	FILE* filehandle = NULL;
	int32 result = fopen_s(&filehandle, string_table_fn, "wb");
	if (result != 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - unable to write file %s\n", string_table_fn);
		return;
    }

	if (!filehandle)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - unable to write file %s\n", string_table_fn);
		return;
    }

    uint32 ste_hash = 0;
	CStringTable::tStringEntry* ste = string_table->First(&ste_hash);
	while (ste)
    {
        // -- only write out ref-counted strings (the remaining haven't been cleaned up)
        if (ste->mRefCount <= 0)
        {
            // -- next entry
       	    ste = string_table->Next(&ste_hash);
            continue;
        }

        const char* string = ste->mString;
        int32 length = (int32)strlen(string);
        char tempbuf[kMaxTokenLength];

        // -- write the hash
        sprintf_s(tempbuf, kMaxTokenLength, "0x%08x: ", ste_hash);
        int32 count = (int32)fwrite(tempbuf, sizeof(char), 12, filehandle);
        if (count != 12)
        {
            fclose(filehandle);
            ScriptAssert_(script_context, 0, "<internal>", -1, "Error - unable to write file %s\n", string_table_fn);
            return;
        }

        // -- write the string length
        sprintf_s(tempbuf, kMaxTokenLength, "%04d: ", length);
        count = (int32)fwrite(tempbuf, sizeof(char), 6, filehandle);
        if (count != 6)
        {
            fclose(filehandle);
            ScriptAssert_(script_context, 0, "<internal>", -1, "Error - unable to write file %s\n", string_table_fn);
            return;
        }

        // -- write the string
        count = (int32)fwrite(string, sizeof(char), length, filehandle);
        if (count != length)
        {
            fclose(filehandle);
            ScriptAssert_(script_context, 0, "<internal>", -1, "Error - unable to write file %s\n", string_table_fn);
            return;
        }

        // -- write the eol
        count = (int32)fwrite("\r\n", sizeof(char), 2, filehandle);
        if (count != 2)
        {
            fclose(filehandle);
            ScriptAssert_(script_context, 0, "<internal>", -1, "Error - unable to write file %s\n", string_table_fn);
            return;
        }

        // -- next entry
       	ste = string_table->Next(&ste_hash);
	}

    // -- close the file before we leave
	fclose(filehandle);
}

// ====================================================================================================================
// LoadStringTable():  Load the string table from a file.
// ====================================================================================================================
void LoadStringTable(const char* from_dir)
{
    // -- ensure we have a valid filename
    const char* filename = GetStringTableName();

    // -- build the full path
    if (from_dir == nullptr)
        from_dir = "";
    char full_path[kMaxNameLength * 2];
    size_t dir_len = strlen(from_dir);
    if (dir_len > 0)
    {
        SafeStrcpy(full_path, sizeof(full_path), from_dir);
        if (full_path[dir_len - 1] != '/')
        {
            full_path[dir_len++] = '/';
            full_path[dir_len] = '\0';
        }
    }
    SafeStrcpy(&full_path[dir_len], sizeof(full_path) - dir_len, filename);


    // -- get the context for this thread
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context)
        return;

  	// -- open the file
	FILE* filehandle = NULL;
	int32 result = fopen_s(&filehandle, full_path, "rb");
	if (result != 0)
		return;

	if (!filehandle)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - unable to read file %s\n", full_path);
		return;
    }

    CStringTable* string_table = script_context->GetStringTable();
    if (!string_table)
        return;

    while (!feof(filehandle))
    {
        // -- read the hash
        uint32 hash = 0;
        int32 length = 0;
        char string[kMaxTokenLength];
        char tempbuf[16];

        // -- read the hash
        int32 count = (int32)fread(tempbuf, sizeof(char), 12, filehandle);
        if (ferror(filehandle) || count != 12)
        {
            // -- we're done
            break;
        }
        tempbuf[12] = '\0';
        sscanf_s(tempbuf, "0x%08x: ", &hash);

        // -- read the string length
        count = (int32)fread(tempbuf, sizeof(char), 6, filehandle);
        if (ferror(filehandle) || count != 6)
        {
            fclose(filehandle);
            ScriptAssert_(script_context, 0, "<internal>", -1, "Error - unable to read file: %s\n", full_path);
            return;
        }
        tempbuf[count] = '\0';
        sscanf_s(tempbuf, "%04d: ", &length);

        // -- read the string
        count = (int32)fread(string, sizeof(char), length, filehandle);
        if (ferror(filehandle) || count != length)
        {
            fclose(filehandle);
            ScriptAssert_(script_context, 0, "<internal>", -1, "Error - unable to read file: %s\n", full_path);
            return;
        }
        string[length] = '\0';

        // -- read the eol
        count = (int32)fread(tempbuf, sizeof(char), 2, filehandle);
        if (ferror(filehandle) || count != 2)
        {
            fclose(filehandle);
            ScriptAssert_(script_context, 0, "<internal>", -1, "Error - unable to read file: %s\n", full_path);
            return;
        }

        // -- add the string to the table (including bumping the hash)
        string_table->AddString(string, length, hash, true);
    }

    // -- close the file before we leave
	fclose(filehandle);
}

// ====================================================================================================================
// GetLastWriteTime():  Given a filename, get the last time the file was written.
// ====================================================================================================================
bool8 GetLastWriteTime(const char* file_path, std::filesystem::file_time_type& writetime)
{
    if (!file_path || !file_path[0])
        return (false);

    // -- ensure the file exists
    if (!std::filesystem::exists(file_path))
        return false;

    //const auto file_handle = std::ofstream(file_path);
    writetime = std::filesystem::last_write_time(file_path);
    return (true);
}

// ====================================================================================================================
// GetFullPath():  given a source filename, pre-pend the file name with the current working directory
// ====================================================================================================================
bool8 CScriptContext::GetFullPath(const char* in_file_name, char* out_full_path, int32 in_max_length)
{
    // -- sanity check
    if (in_file_name == nullptr || out_full_path == nullptr || in_max_length <= 0)
        return (false);

    // -- if there's no current working directory, or our in_file_name is *already* prepended, simply copy
    if (mCurrentWorkingDirectory[0] == '\0' ||
        _strnicmp(in_file_name, mCurrentWorkingDirectory, strlen(mCurrentWorkingDirectory)) == 0)
    {
        SafeStrcpy(out_full_path, in_max_length, in_file_name);
        return (true);
    }

    // -- get the full path name, by pre-pending the current working directory (if required)
    int32 fn_length = (int32)strlen(in_file_name);
    int32 dir_length = (int32)strlen(mCurrentWorkingDirectory);
    if (fn_length + dir_length > in_max_length)
    {
        TinPrint(this, "Error GetFullPath() - full path length exceeds %d: %s%s",
                       in_max_length, mCurrentWorkingDirectory, in_file_name);
        return false;
    }

    SafeStrcpy(out_full_path, in_max_length, mCurrentWorkingDirectory);
    SafeStrcpy(&out_full_path[dir_length], in_max_length - dir_length, in_file_name);

    // -- the current working directory is *always* using '/', so ensure the final path is correct,
    // in case someone passes a filename with a '\\'
    char* fix_dir_char = out_full_path;
    while(*fix_dir_char != '\0')
    {
        if(*fix_dir_char == '\\')
            *fix_dir_char = '/';
        ++fix_dir_char;
    }

    return true;
}

// ====================================================================================================================
// GetBinaryFileName():  Given a source filename, return the file to write the compiled byte code to.
// ====================================================================================================================
bool8 GetBinaryFileName(const char* filename, char* binfilename, int32 maxnamelength)
{
    if (!filename)
        return false;

    // -- a script file should end in ".ts"
    const char* extptr = strrchr(filename, '.');
    if (!extptr || Strncmp_(extptr, ".ts", 4) != 0)
        return false;

    // -- copy the root name (make sure we've got room)
    uint32 length = kPointerDiffUInt32(extptr, filename);
    if (length + 5 > (uint32)maxnamelength)
        return false;

    SafeStrcpy(binfilename, maxnamelength, filename, maxnamelength);
    SafeStrcpy(&binfilename[length], maxnamelength, ".tso", maxnamelength - length);

    return true;
}

// ====================================================================================================================
// NeedToCompile():  Returns 'true' if the source file needs to be compiled.
// ====================================================================================================================
bool8 NeedToCompile(const char* full_path_name, const char* binfilename, bool check_only)
{
    // -- sanity check
    if (full_path_name == nullptr || full_path_name[0] == '\0' ||
        binfilename == nullptr || binfilename[0] == '\0')
    {
        return (false);
    }

    // -- get the filetime for the original script
    // -- if fail, then we have nothing to compile
    // note:  if we're checking for a file that's been modified externally, it may take a
    // frame or two before Windows will allow us access to the last write time
    std::filesystem::file_time_type scriptft;
    if (!GetLastWriteTime(full_path_name, scriptft))
    {
        if (!check_only)
        {
            TinPrint(TinScript::GetContext(), "Error - Compile() - file not found: %s\n", full_path_name);
        }
        return (false);
    }

    // -- get the filetime for the binary file
    // -- if fail, we need to compile
    std::filesystem::file_time_type binft;
    if (!GetLastWriteTime(binfilename, binft))
    {
        return (!check_only ? true : false);
    }

    // -- if the scriptft is more recent than the binft, then we need to compile
    if (binft < scriptft)
    {
        return (true);
    }
    else
    {
        // convert the binft to a time_t, for comparison with the debug force compile time
        std::time_t force_compile_time;
        if (GetDebugForceCompile(force_compile_time))
        {
            // -- convert the const char* to a wchar_t*
            std::string binfilename_str(binfilename);
            std::wstring binfilename_wstr = std::wstring(binfilename_str.begin(), binfilename_str.end());
            struct _stat64 fileInfo;
            if (_wstati64(binfilename_wstr.c_str(), &fileInfo) != 0)
            {
                return true;
            }
            std::time_t file_time = fileInfo.st_mtime;
            return (file_time < force_compile_time);
        }

        // -- we're not forcing compiles
        return false;
    }
}

// ====================================================================================================================
// CheckSourceNeedToCompile():  Given just the source name, see if it needs to be (re)compiled
// ====================================================================================================================
bool CheckSourceNeedToCompile(const char* full_path)
{
    // -- sanity check
    CScriptContext* script_context = TinScript::GetContext();
    if (script_context == nullptr || full_path == nullptr || full_path[0] == '\0')
    {
        return (false);
    }

    char binfilename[kMaxNameLength * 2];
    if (!GetBinaryFileName(full_path, binfilename, kMaxNameLength * 2))
    {
        return false;
    }

    // -- check used only to compare file modification timestamps, if a source
    // file has been changed externally... should not generate errors
    return NeedToCompile(full_path, binfilename, true);
}

// ====================================================================================================================
// GetSourceCFileName():  Given a source filename, return the file to write the source 'C'.
// ====================================================================================================================
bool8 GetSourceCFileName(const char* filename, char* source_C_name, int32 maxnamelength)
{
    if (!filename)
        return false;

    // -- a script file should end in ".ts"
    const char* extptr = strrchr(filename, '.');
    if (!extptr || Strncmp_(extptr, ".ts", 4) != 0)
        return false;

    // -- copy the root name
    uint32 length = kPointerDiffUInt32(extptr, filename);
    SafeStrcpy(source_C_name, maxnamelength, filename, maxnamelength);
    SafeStrcpy(&source_C_name[length], maxnamelength - length, ".h", maxnamelength - length);

    return true;
}

// ====================================================================================================================
// NotifySourceStatus():  Print a message and notify the debugger, that a file needs to be recompiled
// ====================================================================================================================
void CScriptContext::NotifySourceStatus(const char* full_path, bool is_modified, bool has_error)
{
    // sanity check
    if (full_path == nullptr || full_path[0] == '\0')
        return;

    TinPrint(this, "Source %s: %s\n", has_error ? "error" : "modified", full_path);

    int32 session = 0;
    if (IsDebuggerConnected(session))
    {
        // -- we only send the message, if the file has been modified (e.g. not yet compiled)
        // or if a compile failed
        if (is_modified || has_error)
        {
            SocketManager::SendCommandf("DebuggerNotifySourceStatus(`%s`, %s);",
                                        full_path, has_error ? "true" : "false");
        }
    }

    // -- is modified only checks the file timestamp...
    // -- to track whether an error should be sent to the debugger on connect
    // should only be the result of an actual compile
    else if (!is_modified)
    {
#if NOTIFY_SCRIPTS_MODIFIED
        // -- if not found, add this file to the list
        uint32 file_hash = Hash(full_path);

        if (has_error)
        {
            if (mCompileErrorFileCount < kDebuggerCallstackSize)
            {
                for (int i = 0; i < mCompileErrorFileCount; ++i)
                {
                    if (mCompileErrorFileList[i] == file_hash)
                        return;
                }
                mCompileErrorFileList[mCompileErrorFileCount++] = file_hash;
            }
        }

        // -- else either there's no error, or it's been modified (and *might* not have an error)
        else
        {
            for (int i = 0; i < mCompileErrorFileCount; ++i)
            {
                if (mCompileErrorFileList[i] == file_hash)
                {
                    if (i < mCompileErrorFileCount - 1)
                    {
                        mCompileErrorFileList[i] = mCompileErrorFileList[mCompileErrorFileCount - 1];
                    }
                    --mCompileErrorFileCount;
                    return;
                }
            }
        }
#endif
    }
}

// ====================================================================================================================
// CompileScript():  Compile a source script.
// ====================================================================================================================
CCodeBlock* CScriptContext::CompileScript(const char* filename)
{
    // -- get the full path name, by pre-pending the current working directory (if required)
    char full_path[kMaxNameLength * 2];
    if (!GetFullPath(filename, full_path, kMaxNameLength * 2))
    {
        ScriptAssert_(this,0,"<internal>",-1,"Error - invalid script filename: %s\n",filename ? filename : "");
        return NULL;
    }

    // -- get the name of the output binary file
    char binfilename[kMaxNameLength * 2];
    if (!GetBinaryFileName(full_path, binfilename, kMaxNameLength))
    {
        ScriptAssert_(this, 0, "<internal>", -1, "Error - invalid script filename: %s\n", filename ? filename : "");
        return NULL;
    }

    // -- compile the source
    CCodeBlock* codeblock = ParseFile(this, full_path);
    if (codeblock == NULL)
    {
        // track or notify the debugger that this file (now) contains an error
        NotifySourceStatus(full_path, false, true);

        // -- assert, and return null
        ScriptAssert_(this, 0, "<internal>", -1, "Error - unable to parse file: %s\n", full_path);
        return nullptr;
    }
    else
    {
        // clear the notification for any error this file might have had
        NotifySourceStatus(full_path, false, false);
    }

    // -- write the binary
    if (!SaveBinary(codeblock, binfilename))
        return NULL;

    // -- save the string table - *if* we're the main thread
    if (mIsMainThread)
        SaveStringTable();

    // -- reset the assert stack
    ResetAssertStack();

    return codeblock;
}

// ====================================================================================================================
// InitializeDirectory():  initialize the current working directory
// ====================================================================================================================
void CScriptContext::InitializeDirectory(bool init_exe)
{
    if (init_exe)
    {
        // -- get the executable directory
        std::filesystem::path cur_path = std::filesystem::current_path();
        if (strlen(cur_path.string().c_str()) == 0)
        {
            // -- stub the executable directory
            mExecutableDirectory[0] = '.';
            mExecutableDirectory[1] = '/';
            mExecutableDirectory[2] = '\0';
        }
        else
        {
            SafeStrcpy(mExecutableDirectory, sizeof(mExecutableDirectory), cur_path.string().c_str());

            // -- ensure the last character in the directory is a '/'
            size_t dir_len = strlen(mExecutableDirectory);
            if (mExecutableDirectory[dir_len - 1] != '/')
            {
                mExecutableDirectory[dir_len++] = '/';
                mExecutableDirectory[dir_len] = '\0';
            }

            // -- ensure the path uses '/' characters
            // -- the current working directory is *always* using '/', so ensure the final path is correct,
            // in case someone passes a filename with a '\\'
            char* fix_dir_char = mExecutableDirectory;
            while(*fix_dir_char != '\0')
            {
                if(*fix_dir_char == '\\')
                    *fix_dir_char = '/';
                ++fix_dir_char;
            }
        }
    }

    // -- copy the current working directory, and delete the buffer
    SafeStrcpy(mCurrentWorkingDirectory, sizeof(mCurrentWorkingDirectory), mExecutableDirectory);

    // -- ensure the path uses '/' characters
    // -- the current working directory is *always* using '/', so ensure the final path is correct,
    // in case someone passes a filename with a '\\'
    char* fix_dir_char = mCurrentWorkingDirectory;
    while(*fix_dir_char != '\0')
    {
        if(*fix_dir_char == '\\')
            *fix_dir_char = '/';
        ++fix_dir_char;
    }

    // -- ensure the last character is a '/'
    size_t length = strlen(mCurrentWorkingDirectory);
    assert(length < sizeof(mCurrentWorkingDirectory) - 2);
    if (mCurrentWorkingDirectory[length - 1] != '/')
   {
        mCurrentWorkingDirectory[length] = '/';
        mCurrentWorkingDirectory[length + 1] = '\0';
    }
}

// ====================================================================================================================
// SetDirectory():  sets the current working directory for executing scripts
// ====================================================================================================================
bool8 CScriptContext::SetDirectory(const char* path)
{
    // -- if the path doesn't exist, we're done (and all scripts will execute from whatever the .exe directory is)
    if (path == nullptr || path[0] == '\0')
    {
        // -- on an empty path, we'll reset to the current
        InitializeDirectory(false);
        return (true);
    }

    // -- for now, the directory can only be verified on windows...
    if (! std::filesystem::exists(path) || !std::filesystem::is_directory(path))
    {
        TinPrint(this, "Error - SetDirectory():  not a valid directory %s\n", path);
        TinPrint(this, "cwd: %s\n", mCurrentWorkingDirectory);
        return (false);
    }

    // -- copy the new cwd
    SafeStrcpy(mCurrentWorkingDirectory, sizeof(mCurrentWorkingDirectory), path);

    // -- scrub the directory to ensure all '\\' characters are actually '/'
    char* fix_dir_char = mCurrentWorkingDirectory;
    while(*fix_dir_char != '\0')
    {
        if(*fix_dir_char == '\\')
            *fix_dir_char = '/';
        ++fix_dir_char;
    }

    // -- ensure the last character is a '/'
    size_t length = strlen(mCurrentWorkingDirectory);
    assert(length < sizeof(mCurrentWorkingDirectory) - 2);
    if (mCurrentWorkingDirectory[length - 1] != '/')
    {
        mCurrentWorkingDirectory[length] = '/';
        mCurrentWorkingDirectory[length + 1] = '\0';
    }

    // -- success
    TinPrint(this, "SetDirectory():  cwd: %s\n", mCurrentWorkingDirectory);
    return (true);
}

// ====================================================================================================================
// ExecScript():  Execute a script, compiles if necessary.
// ====================================================================================================================
bool8 CScriptContext::ExecScript(const char* filename, bool8 must_exist, bool8 re_exec)
{
    // -- sanity check
    if (filename == nullptr)
    {
        return (false);
    }

    // -- get the full path name, by pre-pending the current working directory (if required)
    char full_path[kMaxNameLength * 2];
    if (!GetFullPath(filename, full_path, kMaxNameLength * 2))
    {
        ScriptAssert_(this,0,"<internal>",-1,"Error - invalid script filename: %s\n",
                      full_path ? filename : "");
        return false;
    }

    char binfilename[kMaxNameLength * 2];
    if (!GetBinaryFileName(full_path, binfilename, kMaxNameLength * 2))
    {
        if (must_exist)
        {
            ScriptAssert_(this, 0, "<internal>", -1, "Error - invalid script filename: %s\n",
                          full_path ? filename : "");
            ResetAssertStack();
        }
        return false;
    }

    CCodeBlock* codeblock = NULL;

    // -- note:  Compile() also prepends the CWD, so we use filename to call CompileScript()
    bool8 needtocompile = NeedToCompile(full_path, binfilename, false);
    if (needtocompile)
    {
        codeblock = CompileScript(filename);
        if (!codeblock)
        {
            ResetAssertStack();
            return false;
        }
    }
    else
    {
        // -- if we don't need to compile the script, and we don't need to execute it more than once,
        // -- if we already have this codeblock loaded, we're done
        if (!re_exec)
        {
            uint32 filename_hash = Hash(full_path, -1, false);
            CCodeBlock* already_executed = GetCodeBlockList()->FindItem(filename_hash);
            if (already_executed)
            {
                return (true);
            }
        }

        bool8 old_version = false;
        codeblock = LoadBinary(this, full_path, binfilename, must_exist, old_version);

        // -- if we have an old version, recompile
        if (!codeblock && old_version)
        {
            // -- note:  Compile() also prepends the CWD, so we use filename to call CompileScript()
            codeblock = CompileScript(filename);
        }
    }

    // -- at this point, our codeblock is loaded - apply any breakpoints, as needed
    if (codeblock)
    {
        AddDeferredBreakpoints(*codeblock);
    }

    // -- notify the debugger, if one is connected
    if (codeblock && mDebuggerConnected)
    {
        DebuggerCodeblockLoaded(codeblock->GetFilenameHash());
    }

    // -- execute the codeblock
    bool8 result = true;
    if (codeblock)
    {
	    result = ExecuteCodeBlock(*codeblock);
        codeblock->SetFinishedParsing();

        if (!result)
        {
            ScriptAssert_(this, 0, "<internal>", -1,
                          "Error - unable to execute file: %s\n", filename);
            result = false;
        }
        else if (!codeblock->IsInUse())
        {
            CCodeBlock::DestroyCodeBlock(codeblock);
        }
    }

    ResetAssertStack();
    return result;
}

// ====================================================================================================================
// CompileCommand():  Compile a text block into byte code.
// ====================================================================================================================
CCodeBlock* CScriptContext::CompileCommand(const char* statement)
{
    CCodeBlock* commandblock = ParseText(this, "<stdin>", statement);
    return commandblock;
}

// ====================================================================================================================
// ExecCommand():  Compile and execute a text block.
// ====================================================================================================================
bool8 CScriptContext::ExecCommand(const char* statement)
{
    CCodeBlock* stmtblock = CompileCommand(statement);
    if (stmtblock)
    {
        bool8 result = ExecuteCodeBlock(*stmtblock);
        stmtblock->SetFinishedParsing();

        ResetAssertStack();

        // -- if the codeblock didn't define any functions, we're finished with it
        if (!stmtblock->IsInUse())
            CCodeBlock::DestroyCodeBlock(stmtblock);
        return result;
    }
    else
    {
        ScriptAssert_(this, 0, "<internal>", -1, "Error - Unable to compile: %s\n", statement);
    }

    ResetAssertStack();

    // -- failed
    return false;
}

// ====================================================================================================================
// CompileToC():  Compile a source script to a valid 'C' source file.
// ====================================================================================================================
bool8 CScriptContext::CompileToC(const char* filename)
{
    // -- get the name of the output binary file
    char source_C_name[kMaxNameLength];
    if (!GetSourceCFileName(filename, source_C_name, kMaxNameLength))
    {
        ScriptAssert_(this, 0, "<internal>", -1, "Error - invalid script filename: %s\n", filename ? filename : "");
        return (false);
    }

    // -- compile the source
    int32 source_length = 0;
    const char* source_C = ParseFile_CompileToC(this, filename, source_length);
    if (source_C == nullptr)
    {
        ScriptAssert_(this, 0, "<internal>", -1, "Error - unable to parse file: %s\n", filename);
        return NULL;
    }

    // -- convert the codeblock to a valid source 'C' file 
    if (!SaveToSourceC(filename, source_C_name, source_C, source_length))
        return (false);

    // -- save the string table - *if* we're the main thread
    if (mIsMainThread)
        SaveStringTable();

    // -- reset the assert stack
    ResetAssertStack();

    // $$$TZA ensure the codeblock is deleted after the conversion
    // -- also, we need to ensure the codeblock doesn't add function entries
    // -- to the namespace hashtables (or that they're now registered 'C' functions)
    return (true);
}

// ====================================================================================================================
// SetFunctionReturnValue():  Each time a function returns, the return value is stored for external access.
// ====================================================================================================================
void CScriptContext::SetFunctionReturnValue(void* value, eVarType valueType)
{
    // -- if the current value is a string, we do need to update the refcount
    if (mFunctionReturnValType == TYPE_string)
    {
        uint32 string_hash = *(uint32*)mFunctionReturnValue;
        GetStringTable()->RefCountDecrement(string_hash);
    }

    // -- sanity check
    if (!value || valueType < FIRST_VALID_TYPE)
    {
        mFunctionReturnValType = TYPE_NULL;
    }
    else
    {
        mFunctionReturnValType = valueType;
        memcpy(mFunctionReturnValue, value, kMaxTypeSize);

        // -- update the string table for strings
        if (mFunctionReturnValType == TYPE_string)
        {
            uint32 string_hash = *(uint32*)mFunctionReturnValue;
            GetStringTable()->RefCountIncrement(string_hash);
        }
    }
}

// ====================================================================================================================
// GetFunctionReturnValue():  Get the value returned by the last function executed.
// ====================================================================================================================
bool8 CScriptContext::GetFunctionReturnValue(void*& value, eVarType& valueType)
{
    if (mFunctionReturnValType >= FIRST_VALID_TYPE)
    {
        value = mFunctionReturnValue;
        valueType = mFunctionReturnValType;
        return (true);
    }
    else
    {
        value = NULL;
        valueType = TYPE_NULL;
        return (false);
    }
}

// ====================================================================================================================
// GetScratchBuffer():  Convenience function to store intermediate values without the bother of memory management.
// ====================================================================================================================
char* CScriptContext::GetScratchBuffer()
{
    char* scratch_buffer = mScratchBuffers[(++mScratchBufferIndex) % kMaxScratchBuffers];
    return (scratch_buffer);
}

// ====================================================================================================================
// SetDebuggerConnected():  Enables debug information to be sent through the socket to a connected debugger
// ====================================================================================================================
void CScriptContext::SetDebuggerConnected(bool connected)
{
    // -- set the bool
    mDebuggerConnected = connected;
	if (connected)
		++mDebuggerSessionNumber;

    // -- any change in debugger connectivity resets the debugger break members
    mDebuggerActionForceBreak = false;
    mDebuggerActionStep = false;
    mDebuggerActionStepOver = false;
    mDebuggerActionStepOut = false;
    mDebuggerActionRun = true;

	mDebuggerBreakLoopGuard = false;
	mDebuggerBreakFuncCallStack = NULL;
	mDebuggerBreakExecStack = NULL;
	mDebuggerVarWatchRequestID = 0;

    // -- if we're now connected, send back the current working directory
    if (connected)
    {
		// -- send the command
		DebuggerNotifyDirectories(mCurrentWorkingDirectory, mExecutableDirectory);

        // -- now notify the debugger of all the codeblocks loaded
        CCodeBlock* code_block = GetCodeBlockList()->First();
        while (code_block)
        {
            if (code_block->GetFilenameHash() != Hash("<stdin>"))
                DebuggerCodeblockLoaded(code_block->GetFilenameHash());
            code_block = GetCodeBlockList()->Next();
        }

        // -- we also send a list of all the files that were attempted to be executed, and contained
        // compile errors
        for (int i = 0; i < mCompileErrorFileCount; ++i)
        {
            const char* full_path = UnHash(mCompileErrorFileList[i]);
            SocketManager::SendCommandf("DebuggerNotifySourceStatus(`%s`, true);", full_path);
        }
        mCompileErrorFileCount = 0;
    }

    // -- if we're not connected, we need to delete all breakpoints - they'll be re-added upon reconnection
    else
    {
        CCodeBlock* code_block = GetCodeBlockList()->First();
        while (code_block)
        {
            code_block->RemoveAllBreakpoints();
            code_block = GetCodeBlockList()->Next();
        }
    }
}

// ====================================================================================================================
// IsDebuggerConnected():  Returns if we have a connected debugger.
// ====================================================================================================================
bool CScriptContext::IsDebuggerConnected(int32& cur_debugger_session)
{
	cur_debugger_session = mDebuggerSessionNumber;
    return (mDebuggerConnected || mDebuggerActionForceBreak);
}

// ====================================================================================================================
// AddBreakpoint():  Method to find a codeblock, and set a line to notify the debugger, if executed
// ====================================================================================================================
bool CScriptContext::AddBreakpoint(const char* filename, int32 line_number, bool8 break_enabled,
                                   const char* conditional, const char* trace, bool8 trace_on_condition)
{
    // -- sanity check
    if (!filename || !filename[0])
        return false;

    // -- we use the full path for code block hashes
    char full_path[kMaxNameLength * 2];
    if (!GetFullPath(filename, full_path, kMaxNameLength * 2))
    {
        TinPrint(this, "Error AddBreakpoint(): %s @ %d", filename, line_number);
        return false;
    }

    // -- find the code block within the thread
    uint32 filename_hash = Hash(full_path);
    CCodeBlock* code_block = GetCodeBlockList()->FindItem(filename_hash);
    if (!code_block)
    {
        // -- ensure the breakpoint isn't already in the list
        bool already_exists = false;
        CDebuggerWatchExpression* breakpoint = mDeferredBreakpointsList.FindItem(filename_hash);
        while (breakpoint != nullptr)
        {
            if (breakpoint->mLineNumber == line_number)
            {
                already_exists = true;
                break;
            }
            breakpoint = mDeferredBreakpointsList.FindNextItem(breakpoint, filename_hash);
        }

        // -- if we didn't already have this breakpoint, add it to the deferred list
        if (!already_exists)
        {
            // -- we're going to store this breakpoint and apply it if/when the codeblock is actually loaded
            CDebuggerWatchExpression* new_break = TinAlloc(ALLOC_Debugger, CDebuggerWatchExpression, line_number,
                                                           true, break_enabled, conditional, trace, trace_on_condition);
            mDeferredBreakpointsList.AddItem(*new_break, filename_hash);
        }

        return false;
    }

    // -- add the breakpoint
    int32 actual_line = code_block->AddBreakpoint(line_number, break_enabled, conditional, trace, trace_on_condition);

    // -- if the actual breakable line doesn't match the request, notify the debugger
    if (actual_line != line_number)
    {
        DebuggerBreakpointConfirm(filename_hash, line_number, actual_line);
    }

    // -- success
    return true;
}

// ====================================================================================================================
// AddDeferredBreakpoints():  For breakpoints added before the file was actually executed - we add them in on load
// ====================================================================================================================
void CScriptContext::AddDeferredBreakpoints(CCodeBlock& code_block)
{
    CDebuggerWatchExpression* new_breakpoint = mDeferredBreakpointsList.FindItem(code_block.GetFilenameHash());
    while (new_breakpoint)
    {
        mDeferredBreakpointsList.RemoveItem(code_block.GetFilenameHash());

        // -- we're going to add the breakpoint through the API, as the line number might be adjusted
        code_block.AddBreakpoint(new_breakpoint->mLineNumber,
                                 new_breakpoint->mIsEnabled, new_breakpoint->mConditional,
                                 new_breakpoint->mTrace, new_breakpoint->mTraceOnCondition);

        // -- free, and get the next one
        TinFree(new_breakpoint);
        new_breakpoint = mDeferredBreakpointsList.FindItem(code_block.GetFilenameHash());
    }
}

// ====================================================================================================================
// RemoveBreakpoint():  The given file/line will no longer notify the debugger if executed
// ====================================================================================================================
void CScriptContext::RemoveBreakpoint(const char* filename, int32 line_number)
{
    // -- sanity check
    if (!filename || !filename[0])
        return;

    // -- we use the full path for code block hashes
    char full_path[kMaxNameLength * 2];
    if (!GetFullPath(filename, full_path, kMaxNameLength * 2))
    {
        TinPrint(this, "Error RemoveBreakpoint(): %s @ %d", filename, line_number);
        return;
    }

    // -- find the code block within the thread
    uint32 filename_hash = Hash(full_path);
    CCodeBlock* code_block = GetCodeBlockList()->FindItem(filename_hash);
    if (code_block == nullptr)
    {
        CDebuggerWatchExpression* breakpoint = mDeferredBreakpointsList.FindItem(filename_hash);
        while (breakpoint != nullptr)
        {
            if (breakpoint->mLineNumber == line_number)
            {
                mDeferredBreakpointsList.RemoveItem(breakpoint, filename_hash);
                TinFree(breakpoint);
                break;
            }
            breakpoint = mDeferredBreakpointsList.FindNextItem(breakpoint, filename_hash);
        }
        return;
    }

    // -- remove the breakpoint
    int32 actual_line = code_block->RemoveBreakpoint(line_number);

    // -- if the actual breakable line doesn't match the request, notify the debugger
    if (actual_line != line_number)
    {
        DebuggerBreakpointConfirm(filename_hash, line_number, actual_line);
    }
}

// ====================================================================================================================
// RemoveAllBreakpoints():  No breakpoints will be set for the given file
// ====================================================================================================================
void CScriptContext::RemoveAllBreakpoints(const char* filename)
{
    // -- sanity check
    if (!filename || !filename[0])
        return;

    // -- this method must be thread safe
    mThreadLock.Lock();

    // -- find the code block within the thread
    uint32 filename_hash = Hash(filename);
    CCodeBlock* code_block = GetCodeBlockList()->FindItem(filename_hash);
    if (! code_block)
        return;

    code_block->RemoveAllBreakpoints();

    // -- unlock
    mThreadLock.Unlock();
}

// ====================================================================================================================
// SetForceBreak():  Sets the bool, forcing the VM to halt on the next statement.
// ====================================================================================================================
void CScriptContext::SetForceBreak(int32 watch_var_request_id)
{
    // -- this is usually set to when requested by the debugger, and auto set back to false when the break is handled.
    mDebuggerActionForceBreak = true;

	// -- it's also how variable watches trigger - set the request ID
	mDebuggerVarWatchRequestID = watch_var_request_id;
}

// ====================================================================================================================
// SetBreakStep():  Sets the bool, coordinating breakpoint execution with a remote debugger
// ====================================================================================================================
void CScriptContext::SetBreakActionStep(bool8 torf, bool8 step_over, bool8 step_out)
{
    // -- if we're already at a breakpoint, then this is a normal "step" action
    if (mDebuggerBreakLoopGuard)
    {
        // -- this is usually set to false when a breakpoint is hit, and then remotely set to true by the debugger
        mDebuggerActionForceBreak = false;
        mDebuggerActionStep = torf;
        mDebuggerActionStepOver = torf ? step_over : false;
        mDebuggerActionStepOut = torf ? step_out : false;

        // -- clear the var watch request ID - it'll be set on the next write if necessary
        mDebuggerVarWatchRequestID = 0;
    }

    // -- otherwise, we're forcing a break
    else
    {
        mDebuggerActionForceBreak = true;
    }
}

// ====================================================================================================================
// SetBreakRun():  Sets the bool, coordinating breakpoint execution with a remote debugger
// ====================================================================================================================
void CScriptContext::SetBreakActionRun(bool torf)
{
    // -- this is usually set to false when a breakpoint is hit, and then remotely set to true by the debugger
    mDebuggerActionRun = torf;
}

// ====================================================================================================================
// InitWatchEntryFromVarEntry():  Helper method to fill in the members of a watch entry.
// ====================================================================================================================
void CScriptContext::InitWatchEntryFromVarEntry(CVariableEntry& ve, CObjectEntry* parent_oe,
												CDebuggerWatchVarEntry& watch_entry, CObjectEntry*& oe)
{
	// -- initialize the return result
	oe = NULL;

	// -- we'll initialize the request ID later, if applicable
	watch_entry.mWatchRequestID = 0;

    // -- set the stack level
    watch_entry.mStackOffsetFromBottom = -1;

    CFunctionEntry* fe = ve.GetFunctionEntry();

	// -- fill in the watch entry
	watch_entry.mFuncNamespaceHash = fe ? fe->GetNamespaceHash() : 0;
	watch_entry.mFunctionHash = fe ? fe->GetHash() : 0;
	watch_entry.mFunctionObjectID = 0;
	watch_entry.mObjectID = parent_oe ? parent_oe->GetID() : 0;
	watch_entry.mNamespaceHash = 0;

    // -- get the variable address - if it's a stack variable, pull it off the stack
    void* value_addr = ve.GetValueAddr(parent_oe ? parent_oe->GetAddr() : NULL);
    if (mDebuggerBreakFuncCallStack && ve.IsStackVariable(*mDebuggerBreakFuncCallStack))
    {
        value_addr = GetStackVarAddr(this, *mDebuggerBreakExecStack,
                                     *mDebuggerBreakFuncCallStack, ve.GetStackOffset());
    }

	// -- type, name, and value string
	watch_entry.mType = ve.GetType();
	SafeStrcpy(watch_entry.mVarName, sizeof(watch_entry.mVarName), UnHash(ve.GetHash()), kMaxNameLength);

    // -- fill in the array size
    watch_entry.mArraySize = ve.GetArraySize();

	watch_entry.mVarHash = ve.GetHash();
	watch_entry.mVarObjectID = 0;

	if (ve.GetType() == TYPE_object)
	{
		uint32 objectID = *(uint32*)value_addr;
		oe = FindObjectEntry(objectID);
		if (oe)
			watch_entry.mVarObjectID = objectID;
	}

    // -- after the member_entry has all fields filled in, see if we can format
    // the mValue to be more readable
    DebuggerWatchFormatValue(&watch_entry, value_addr);
}

// ====================================================================================================================
// AddVariableWatchExpression():  If we're not modifying or breaking on a watch, then we evaluate the expression
// at our current stack depth
// ====================================================================================================================
void CScriptContext::AddVariableWatchExpression(int32 request_id, const char* variable_watch)
{
    // -- sanity check
    if (variable_watch == nullptr || variable_watch[0] == '\0' || request_id <= 0)
        return;

    // -- we require a callstack to evaluate a variable watch as a complete expression
    if (!mDebuggerBreakFuncCallStack || !mDebuggerBreakExecStack)
        return;
    
    // -- watch expressions can handle a the complete syntax, but are unable to set a break on a variable entry
    // note:  they're evaluated as trace expressions, but we want a result out of them, so init/execute
    // as a conditional which will wrap them with:  return (expr);
    CDebuggerWatchExpression watch_expression(-1, false, false, variable_watch, NULL, false);
    bool result = InitWatchExpression(watch_expression, false, *mDebuggerBreakFuncCallStack, mDebuggerWatchStackOffset);

    // -- if we were successful initializing the expression (e.g. a codeblock and function were created
    bool valid_response = false;
    if (result)
    {
        // -- if evaluating the watch was successful
        result = EvalWatchExpression(watch_expression, false, *mDebuggerBreakFuncCallStack, *mDebuggerBreakExecStack,
                                     mDebuggerWatchStackOffset);
        if (result)
        {
            // -- if we're able to retrieve the return value
            eVarType returnType = TYPE_void;
            void* returnValue = NULL;
            if (GetFunctionReturnValue(returnValue, returnType))
            {
    	        CDebuggerWatchVarEntry watch_result;

		        watch_result.mWatchRequestID = request_id;
                watch_result.mStackOffsetFromBottom = -1;

		        // -- fill in the watch entry
		        watch_result.mFuncNamespaceHash = 0;
		        watch_result.mFunctionHash = 0;
		        watch_result.mFunctionObjectID = 0;
		        watch_result.mObjectID = 0;
		        watch_result.mNamespaceHash = 0;

		        // -- type, name, and value string, and array size
		        watch_result.mType = returnType;
		        SafeStrcpy(watch_result.mVarName, sizeof(watch_result.mVarName), variable_watch, kMaxNameLength);
                watch_result.mArraySize = 1;

		        watch_result.mVarHash = Hash(variable_watch);
		        watch_result.mVarObjectID = 0;

                // -- if the type is an object, see if it actually exists
                if (returnType == TYPE_object)
                {
                    // -- ensure the object actually exists
                    CObjectEntry* oe_0 = FindObjectEntry(*(uint32*)returnValue);
                    if (oe_0)
                        watch_result.mVarObjectID = oe_0->GetID();
                }

                // -- after the member_entry has all fields filled in, see if we can format
                // the mValue to be more readable
                DebuggerWatchFormatValue(&watch_result, returnValue);

			    // -- send the response
                valid_response = true;
			    DebuggerSendWatchVariable(&watch_result);

			    // -- if the result is an object, then send the complete object
			    if (watch_result.mType == TYPE_object && watch_result.mVarObjectID > 0)
			    {
				    DebuggerSendObjectMembers(&watch_result, watch_result.mVarObjectID);
			    }
            }
        }
    }

    // -- if we couldn't initialize the watch expression (e.g. it's not a parse-able block of code)
    // or the pre-existing watch function isn't valid at this execution stack offset, 
    // and since it wasn't a found variable, send a "--" value result
    if (!valid_response && request_id > 0)
    {
        CDebuggerWatchVarEntry null_response;
        null_response.mWatchRequestID = request_id;
        SafeStrcpy(null_response.mVarName, sizeof(null_response.mVarName), variable_watch);
        SafeStrcpy(null_response.mValue, sizeof(null_response.mValue), "--");
        DebuggerSendWatchVariable(&null_response);
    }
}

// ====================================================================================================================
// AddVariableWatch():  Method to find a a variable entry, return or update it's value, and mark it as a data break.
// ====================================================================================================================
void CScriptContext::AddVariableWatch(int32 request_id, const char* variable_watch, bool breakOnWrite,
                                      const char* new_value)
{
    // -- sanity check
    bool update_value = (new_value && new_value[0]);
    if ((request_id < 0 && !update_value) || !variable_watch || !variable_watch[0])
        return;

    // -- if we're updating, we're not breaking
    if (update_value)
        breakOnWrite = false;

    // -- see if we can manually parse the expression, to find the specific function/member/variable
	// -- the first pattern will be an object.member (.member.member...)
	// -- looping through, as long as at each step, we find a valid object
	CDebuggerWatchVarEntry found_variable;
	found_variable.mType = TYPE_void;
    found_variable.mArraySize = 0;
	CObjectEntry* parent_oe = NULL;
	CObjectEntry* oe = NULL;
	CVariableEntry* ve = NULL;

	// -- first we look for an identifier, or 'self' and find a matching variable entry
	tReadToken token(variable_watch, 0);
	bool8 found_token = GetToken(token);
	if (found_token && (token.type == TOKEN_IDENTIFIER ||
		(token.type == TOKEN_KEYWORD && GetReservedKeywordType(token.tokenptr, token.length) == KEYWORD_self)))
	{
		// -- see if this token is a stack var
		uint32 var_hash = Hash(token.tokenptr, token.length);

		// -- if this isn't a stack variable, see if it's a global variable
		if (DebuggerFindStackVar(this, var_hash, found_variable, ve))
		{
			// -- if this refers to an object, find the object entry
			if (found_variable.mType == TYPE_object)
			{
				oe = FindObjectEntry(found_variable.mVarObjectID);
			}
		}
		else
		{
			ve = GetGlobalNamespace()->GetVarTable()->FindItem(var_hash);
			if (ve)
			{
				// -- use the helper function to fill in the results, including the oe pointer
				InitWatchEntryFromVarEntry(*ve, NULL, found_variable, oe);
			}
		}
	}

	// -- else see if we have an integer - we're going to assume any integer is meant to be an object ID
	else if (found_token && token.type == TOKEN_INTEGER)
	{
		// -- see if there's a valid oe for this
		uint32 object_id = Atoi(token.tokenptr, token.length);
		oe = FindObjectEntry(object_id);
		parent_oe = oe;

		// -- if we found one, fill in the watch entry
		if (oe)
		{
			// -- we'll initialize the request ID later, if applicable
			found_variable.mWatchRequestID = 0;
            found_variable.mStackOffsetFromBottom = -1;

			// -- fill in the watch entry
			found_variable.mFuncNamespaceHash = 0;
			found_variable.mFunctionHash = 0;
			found_variable.mFunctionObjectID = 0;
			found_variable.mObjectID = 0;
			found_variable.mNamespaceHash = 0;

			// -- type, name, and value string, and array size
			found_variable.mType = TYPE_object;
			SafeStrcpy(found_variable.mVarName, sizeof(found_variable.mVarName), UnHash(oe->GetNameHash()), kMaxNameLength);
			sprintf_s(found_variable.mValue, "%d", object_id);
            found_variable.mArraySize = 1;

			found_variable.mVarHash = oe->GetNameHash();
			found_variable.mVarObjectID = object_id;
		}
	}

	// -- at this point, if we found a variable (type will be valid)
	// -- either return what we've found, or if the current variable is an object, and the next
	// -- token is a period followed by a member, keep digging
	if (found_variable.mType != TYPE_void)
	{
		bool8 success = true;
		while (true)
		{
			// -- if we have a period, and a valid object
			tReadToken next_token(token);
			bool8 found_token_0 = GetToken(next_token);
			if (found_token_0 && next_token.type == TOKEN_PERIOD && oe != NULL)
			{
				// -- if our period is followed by a valid member
				tReadToken member_token(next_token);
				if (GetToken(member_token) && member_token.type == TOKEN_IDENTIFIER)
				{
					// -- at this point, we're dereferencing a the member of an object, so update the parent oe
					parent_oe = oe;

					// -- update the token pointer
					token = member_token;

					// -- if the member token is for a valid member, update the token pointer, and continue the pattern
					uint32 var_hash = Hash(token.tokenptr, token.length);
					ve = oe->GetVariableEntry(var_hash);
					if (ve)
					{
						// -- use the helper function to fill in the results, including the oe pointer
						InitWatchEntryFromVarEntry(*ve, parent_oe, found_variable, oe);
					}

					// -- else we have an invalid member
					else
					{
						success = false;
						break;
					}
				}

				// -- else, whatever we found doesn't fit this pattern
				else
				{
					success = false;
					break;
				}
			}

			// -- else if we found a token, but it's not a period, or we don't have an object, we fail this pattern
			else if (found_token_0)
			{
				success = false;
				break;
			}

			// -- else there are no more tokens - we're at the end of a successful chain
			else
			{
				break;
			}
		}

		// -- once we've finished the pattern search, see if we still have a valid result to send
		if (success)
		{
            if (!update_value)
            {
			    // -- set the request ID back in the result
			    found_variable.mWatchRequestID = request_id;
                found_variable.mStackOffsetFromBottom = -1;

			    // -- send the response
			    DebuggerSendWatchVariable(&found_variable);

			    // -- if the result is an object, then send the complete object
			    if (found_variable.mType == TYPE_object)
			    {
				    DebuggerSendObjectMembers(&found_variable, found_variable.mVarObjectID);
			    }

			    // -- if we've been requested to break on write, set the flag on the variable entry
			    if (ve && breakOnWrite)
			    {
				    ve->SetBreakOnWrite(request_id, mDebuggerSessionNumber, true, NULL, NULL, false);

				    // -- confirm the variable watch
				    DebuggerVarWatchConfirm(request_id, parent_oe ? parent_oe->GetID() : 0, ve->GetHash());
			    }

				// -- and we're done
			    return;
            }

            // -- if we are updating the value, set it now, without any confirmation or other access
            else if (ve)
            {
                // -- if the variable is owned by a function, we can only change it
                // -- if the function is currently executing, and we're at a break point
                uint32 hash_val = Hash(new_value);
                void* value_addr = TypeConvert(this, TYPE_string, (void*)&hash_val, ve->GetType());
                void* stack_addr = NULL;
                if (value_addr)
                {
                    if (ve->GetFunctionEntry())
                    {
                        if (mDebuggerBreakFuncCallStack && ve->IsStackVariable(*mDebuggerBreakFuncCallStack))
                        {
                            stack_addr = GetStackVarAddr(this, *mDebuggerBreakExecStack,
                                                            *mDebuggerBreakFuncCallStack, ve->GetStackOffset());
                            if (stack_addr)
                            {
                                memcpy(stack_addr, value_addr, gRegisteredTypeSize[ve->GetType()]);
                            }
                        }
                    }
                    else
                    {
                        ve->SetValueAddr(parent_oe ? parent_oe->GetAddr() : NULL, value_addr);
                    }

                    // -- send the variable update
                    InitWatchEntryFromVarEntry(*ve, parent_oe, found_variable, oe);

			        // -- send the response
                    found_variable.mWatchRequestID = request_id;
                    found_variable.mStackOffsetFromBottom = -1;

			        DebuggerSendWatchVariable(&found_variable);
			        if (found_variable.mType == TYPE_object)
			        {
				        DebuggerSendObjectMembers(&found_variable, found_variable.mVarObjectID);
			        }
                }
            }
		}

        // -- if we were successful, or only wanting to update the value, we're done
        if (success || update_value)
        {
			return;
        }
	}

    // -- if we were unable to evaluate the watch expression from above
    // -- evaluate it as a watch expression.
    // -- this will correctly evaluate any valid syntax, bot doesn't give us a variable on which to break
    // -- for that, we'd probably have to flag certain operations in the VM to find out which variable it
    // -- found, etc...
    if (!update_value && !breakOnWrite)
    {
        AddVariableWatchExpression(request_id, variable_watch);
    }
}

// ====================================================================================================================
// HasWatchExpression():  Given a watch structure, return true if we actually have a conditional to evaluate.
// ====================================================================================================================
bool8 CScriptContext::HasWatchExpression(CDebuggerWatchExpression& debugger_watch)
{
    return (debugger_watch.mConditional[0] != '\0');
}

// ====================================================================================================================
// HasTraceExpression():  Given a watch structure, return true if we actually have a trace expression to evaluate.
// ====================================================================================================================
bool8 CScriptContext::HasTraceExpression(CDebuggerWatchExpression& debugger_watch)
{
    return (debugger_watch.mTrace[0] != '\0');
}

// ====================================================================================================================
// InitWatchExpression():  Given a watch structure, create and compile a codeblock that can be stored and evaluated.
// ====================================================================================================================
bool8 CScriptContext::InitWatchExpression(CDebuggerWatchExpression& debugger_watch, bool use_trace,
                                          CFunctionCallStack& cur_call_stack, int32 execution_offset)
{
    // -- depending on whether we're initializing the trace expression or the conditional, set the local vars
    const char* expression = use_trace ? debugger_watch.mTrace : debugger_watch.mConditional;
    CFunctionEntry*& watch_function = use_trace ? debugger_watch.mTraceFunctionEntry
                                                : debugger_watch.mWatchFunctionEntry;

    // -- if we have no expression, or we've already initialized, then we're done
    if (!expression[0] || watch_function != NULL)
        return (true);

    // -- every time a watch is initialized, we bump the ID to ensure a 100% unique name
    int32 watch_id = CDebuggerWatchExpression::gWatchExpressionID++;

    int32 stack_offset = -1;
    int32 stack_offset_from_bottom = -1;
    const CFunctionCallStack* debug_callstack =
        cur_call_stack.GetBreakExecutionFunctionCallEntry(execution_offset, stack_offset, stack_offset_from_bottom);
    CExecStack* debug_execstack = debug_callstack != nullptr ? debug_callstack->GetVariableExecStack() : nullptr;
    if (debug_callstack == nullptr || debug_execstack == nullptr)
        return false;

    // -- this isn't safe - do not cache/use this address beyond the above function call...!!
    const CFunctionCallStack::tFunctionCallEntry* func_call_entry =
        debug_callstack->GetExecutingCallByIndex(stack_offset);
    if (func_call_entry == nullptr)
        return false;

    // -- get the object and function entry (function has to exist)
    CObjectEntry* cur_object = func_call_entry->objentry;
    CFunctionEntry* cur_function = func_call_entry->funcentry;
    if (cur_function == nullptr)
        return false;

    // -- create the name to uniquely identify both the codeblock and the associated function
    char watch_name[kMaxNameLength];
    sprintf_s(watch_name, "_%s_expr_%d_", use_trace ? "trace" : debugger_watch.mIsConditional ? "cond" : "watch", watch_id);
    uint32 watch_name_hash = Hash(watch_name);

	// create the code block and the starting root node
    CCodeBlock* codeblock = TinAlloc(ALLOC_CodeBlock, CCodeBlock, this, watch_name);
	CCompileTreeNode* root = CCompileTreeNode::CreateTreeRoot(codeblock);

    // -- create the watch function
	CFunctionEntry* fe = FuncDeclaration(this, GetGlobalNamespace(), watch_name, watch_name_hash, eFuncTypeScript);

    // -- create a set of local variables in the context to match those of the current function
    CFunctionContext* cur_func_context = cur_function->GetContext();
    CFunctionContext* temp_context = fe->GetContext();

    bool returnAdded = false;
    tVarTable* cur_var_table = cur_func_context->GetLocalVarTable();
    CVariableEntry* cur_ve = cur_var_table->First();
    while (cur_ve)
    {
        // $$$TZA TYPE__array
        // -- first variable is always the "__return" parameter
        if (!returnAdded)
        {
            returnAdded = true;
            temp_context->AddParameter("__return", Hash("__return"), TYPE__resolve, 1, 0);
        }
        else
        {
            // -- create a cloned local variable
            temp_context->AddLocalVar(cur_ve->GetName(), cur_ve->GetHash(), cur_ve->GetType(), 1, false);
        }

        // -- get the next local var
        cur_ve = cur_var_table->Next();
    }

    // -- initialize the stack offsets
    temp_context->InitStackVarOffsets(fe);

    // -- push the temporary function entry onto the temp code block, so we can compile our watch function
    codeblock->smFuncDefinitionStack->Push(fe, cur_object, 0, true);

    // -- add a funcdecl node, and set its left child to be the statement block
    // -- for fun, use the watch_id as the line number - to find it while debugging
    CFuncDeclNode* funcdeclnode = TinAlloc(ALLOC_TreeNode, CFuncDeclNode, codeblock, root->next,
                                           watch_id, watch_name, (int32)strlen(watch_name), "", 0, 0);

    // -- the body of our watch function, is to simply return the given expression
    // -- parsing and returning the expression will also identify the type for us
    // -- note:  trace expressions are evaluated verbatim
    char expr_result[kMaxTokenLength];
    if (use_trace)
        SafeStrcpy(expr_result, sizeof(expr_result), expression, kMaxTokenLength);
    else
        sprintf_s(expr_result, "return (%s);", expression);

    // -- now we've got a temporary function with exactly the same set of local variables
    // -- see if we can parse the expression
	tReadToken parsetoken(expr_result, 0);
    bool success = ParseStatementBlock(codeblock, funcdeclnode->leftchild, parsetoken, false);

    // -- if we successfully created the tree, calculate the size needed by running through the tree
    int32 size = 0;
    if (success)
    {
        size = codeblock->CalcInstrCount(*root);
        success = (size > 0);
    }

    // -- allocate space for the instructions and compile the tree
    if (success)
    {

        codeblock->AllocateInstructionBlock(size, codeblock->GetLineNumberCount());
        success = codeblock->CompileTree(*root);
    }

    // -- if we're drawing parse trees, dump this tree before we clean up
    if (gDebugParseTree)
    {
	    DumpTree(root, 0, false, false);
    }

    // -- success or fail, we need to perform some cleanup
    ResetAssertStack();
    codeblock->SetFinishedParsing();
    DestroyTree(root);

    // -- if we were unsuccessful, destroy the codeblock and return failure
    if (!success)
    {
        CCodeBlock::DestroyCodeBlock(codeblock);
        return (false);
    }

    // -- we were successful - set the function entry, and return success
    watch_function = fe;
    return (true);
}

// ====================================================================================================================
// EvaluateWatchExpression():  Used by the debugger for watches and breakpoints conditionals.
// ====================================================================================================================
bool8 CScriptContext::EvalWatchExpression(CDebuggerWatchExpression& debugger_watch, bool use_trace,
                                          CFunctionCallStack& cur_call_stack, CExecStack& cur_exec_stack,
                                          int32 execution_offset)
{
    // -- depending on whether we're initializing the trace expression or the conditional, set the local vars
    const char* expression = use_trace ? debugger_watch.mTrace : debugger_watch.mConditional;
    CFunctionEntry*& watch_function = use_trace ? debugger_watch.mTraceFunctionEntry
                                                : debugger_watch.mWatchFunctionEntry;

    // -- if we have no expression, we've successfully evaluated
    if (!expression[0])
        return (true);

    // -- if we have no function entry, we're done
    if (!watch_function)
        return (false);

    int32 stack_offset = -1;
    int32 stack_offset_from_bottom = -1;
    const CFunctionCallStack* debug_callstack =
        cur_call_stack.GetBreakExecutionFunctionCallEntry(execution_offset, stack_offset, stack_offset_from_bottom);
    CExecStack* debug_execstack = debug_callstack != nullptr ? debug_callstack->GetVariableExecStack() : nullptr;
    if (debug_callstack == nullptr || debug_execstack == nullptr)
        return false;

    // -- this isn't safe - do not cache/use this address beyond the above function call...!!
    const CFunctionCallStack::tFunctionCallEntry* func_call_entry =
        debug_callstack->GetExecutingCallByIndex(stack_offset);
    if (func_call_entry == nullptr)
        return false;

    // -- get the object and function entry (function has to exist)
    CObjectEntry* cur_object = func_call_entry->objentry;
    CFunctionEntry* cur_function = func_call_entry->funcentry;
    if (cur_function == nullptr)
        return false;
    int32 debug_stacktop = func_call_entry->stackvaroffset;

    // -- create the stack used to execute the function
	CExecStack execstack;
    CFunctionCallStack funccallstack(&execstack);

    // -- push the function entry onto the call stack
    funccallstack.Push(watch_function, cur_object, 0, true);

    // -- create space on the execstack for the local variables
    int32 localvarcount = watch_function->GetContext()->CalculateLocalVarStackSize();
    execstack.Reserve(localvarcount * MAX_TYPE_SIZE);

    // -- copy the local values from the currently executing function, to stack
    CVariableEntry* cur_ve = cur_function->GetLocalVarTable()->First();
    while (cur_ve)
    {
		// -- this doesn't involve the return parameter
		// $$$TZA support hashtable and array parameters!
		if (cur_ve != cur_function->GetContext()->GetParameter(0))
		{
			void* dest_stack_addr = execstack.GetStackVarAddr(0, cur_ve->GetStackOffset());
			void* cur_stack_addr = debug_execstack->GetStackVarAddr(debug_stacktop, cur_ve->GetStackOffset());
            if (dest_stack_addr != nullptr && cur_stack_addr != nullptr)
            {
                memcpy(dest_stack_addr, cur_stack_addr, kMaxTypeSize);
            }
		}
		cur_ve = cur_function->GetLocalVarTable()->Next();
    }

    // -- call the function
    funccallstack.BeginExecution();
    bool8 result = CodeBlockCallFunction(watch_function, NULL, execstack, funccallstack, false);

    // -- if we executed successfully...
    if (result)
    {
        // -- if we can retrieve the return value
        eVarType returnType;
        void* returnValue = execstack.Pop(returnType);
        if (returnValue)
        {
            // -- set the return value in the context, so it is retrievable by whoever needs it
            SetFunctionReturnValue(returnValue, returnType);
        }
        else
        {
            result = false;
        }

        // -- we want to copy back the watch expression var parameters to the original function...
        // this way, we can use watch expressions to modify local variables instead of just
        // (e.g.) printing them
        cur_ve = cur_function->GetLocalVarTable()->First();
        while (cur_ve)
        {
			// -- this doesn't involve the return parameter
			if (cur_ve != cur_function->GetContext()->GetParameter(0))
			{
				void* dest_stack_addr = execstack.GetStackVarAddr(0, cur_ve->GetStackOffset());
				void* cur_stack_addr = debug_execstack->GetStackVarAddr(debug_stacktop, cur_ve->GetStackOffset());
                if (dest_stack_addr != nullptr && cur_stack_addr != nullptr)
                {
                    memcpy(cur_stack_addr, dest_stack_addr, kMaxTypeSize);
                }
			}
            cur_ve = cur_function->GetLocalVarTable()->Next();
        }
    }

    // -- return the result
    return (result);
}

// ====================================================================================================================
// EvaluateWatchExpression():  Used by the debugger for variable watches.
// ====================================================================================================================
bool8 CScriptContext::EvaluateWatchExpression(const char* expression)
{
    // -- ensure we have an expression
    if (!expression || !expression[0])
        return (false);

    // -- we can't create or evaluate until we've actually broken (so we have a call stack and function)
    if (!mDebuggerBreakFuncCallStack)
        return (false);

    // -- find the function we're currently executing
    int32 stacktop = 0;
    CFunctionEntry* cur_function = NULL;
    CObjectEntry* cur_object = NULL;
    uint32 cur_oe_id = 0;
    cur_function = mDebuggerBreakFuncCallStack->GetExecuting(cur_oe_id, cur_object, stacktop);

    // -- make sure we've got a valid function
    if (!cur_function)
        return (false);

	// create the temporary code block and the starting root node
    CCodeBlock* codeblock = TinAlloc(ALLOC_CodeBlock, CCodeBlock, this, "<internal>");
	CCompileTreeNode* root = CCompileTreeNode::CreateTreeRoot(codeblock);

	// -- create the function entry, and add it to the global table
    const char* temp_func_name = "_eval_watch_expr_";
	uint32 temp_func_hash = Hash(temp_func_name);
	CFunctionEntry* fe = FuncDeclaration(this, GetGlobalNamespace(), temp_func_name, temp_func_hash,
                                         eFuncTypeScript);

    // -- copy the function context from our currently executing function, to the temporary,
    // -- so we have access to the same variables, and their current values - but won't change
    // -- the *real* variables
    CFunctionContext* cur_func_context = cur_function->GetContext();
    CFunctionContext* temp_context = fe->GetContext();

    // -- first parameter is always "__return"
    bool returnAdded = false;
    tVarTable* cur_var_table = cur_func_context->GetLocalVarTable();
    CVariableEntry* cur_ve = cur_var_table->First();
    while (cur_ve)
    {
        if (!returnAdded)
        {
            // $$$TZA TYPE__array
            returnAdded = true;
            temp_context->AddParameter("__return", Hash("__return"), TYPE__resolve, 1, 0);
        }
        else
        {
            // -- create a copy, and set the same value - value lives on the stack
            CVariableEntry* temp_ve = temp_context->AddLocalVar(cur_ve->GetName(), cur_ve->GetHash(),
                                                                cur_ve->GetType(), 1, true);
            void* varaddr = mDebuggerBreakExecStack->GetStackVarAddr(stacktop, cur_ve->GetStackOffset());
            temp_ve->SetValue(NULL, varaddr);
        }

        // -- get the next local var
        cur_ve = cur_var_table->Next();
    }

	// -- push the temporary function entry onto the temp code block
    codeblock->smFuncDefinitionStack->Push(fe, NULL, 0);

        // -- add a funcdecl node, and set its left child to be the statement block
    CFuncDeclNode* funcdeclnode = TinAlloc(ALLOC_TreeNode, CFuncDeclNode, codeblock, root->next,
                                           -1, temp_func_name, (int32)strlen(temp_func_name), "", 0 , 0);

    // -- if this is a conditional, then we want to see if the value of it is true/false
    char expr_result[kMaxTokenLength];
    sprintf_s(expr_result, "return (%s);", expression);

    // -- now we've got a temporary function with exactly the same set of local variables
    // -- see if we can parse the expression
	tReadToken parsetoken(expr_result, 0);
    if (ParseStatementBlock(codeblock, funcdeclnode->leftchild, parsetoken, false))
    {
        // -- if we made it this far, execute the function
        DumpTree(root, 0, false, false);

        // we successfully created the tree, now calculate the size needed by running through the tree
        int32 size = codeblock->CalcInstrCount(*root);
        if (size > 0)
        {
            // -- allocate space for the instructions
            codeblock->AllocateInstructionBlock(size, codeblock->GetLineNumberCount());

            // -- compile the tree
            if (codeblock->CompileTree(*root))
            {
                // -- execute the code block - if we're successful, we'll have a value to return
                //bool8 result = ExecuteCodeBlock(*codeblock);
	            // -- create the stack to use for the execution
	            CExecStack execstack;
                CFunctionCallStack funccallstack(&execstack);

                // -- push the function entry onto the call stack
                funccallstack.Push(fe, NULL, 0, true);

                // -- create space on the execstack, if this is a script function
                int32 localvarcount = fe->GetContext()->CalculateLocalVarStackSize();
                execstack.Reserve(localvarcount * MAX_TYPE_SIZE);

                // -- copy the local values onto the stack
                CVariableEntry* temp_ve = temp_context->GetLocalVarTable()->First();
                while (temp_ve)
                {
                    void* stack_addr = execstack.GetStackVarAddr(0, temp_ve->GetStackOffset());
                    void* var_addr = temp_ve->GetAddr(NULL);
                    if (stack_addr != nullptr && var_addr != nullptr)
                    {
                        memcpy(stack_addr, var_addr, kMaxTypeSize);
                    }
                    temp_ve = temp_context->GetLocalVarTable()->Next();
                }

                // -- call the function
                funccallstack.BeginExecution();
                bool8 result = CodeBlockCallFunction(fe, NULL, execstack, funccallstack, false);

                // -- if we executed successfully...
                if (result)
                {
                    eVarType returnType;
                    void* returnValue = execstack.Pop(returnType);
                    if (returnValue)
                    {
                        char resultString[kMaxNameLength];
                        gRegisteredTypeToString[returnType](this, returnValue, resultString, kMaxNameLength);
                        TinPrint(this, "*** EvaluateWatchExpression(): [%s] %s\n", GetRegisteredTypeName(returnType), resultString);
                    }
                }
            }
        }
    }

    // -- on our way out, cleanup
    ResetAssertStack();
    codeblock->SetFinishedParsing();
    DestroyTree(root);
    CCodeBlock::DestroyCodeBlock(codeblock);

    // -- fail
    return (false);
}

// ====================================================================================================================
// ToggleVarWatch():  Find the given variable and toggle whether we break on write.
// ====================================================================================================================
void CScriptContext::ToggleVarWatch(int32 watch_request_id, uint32 object_id, uint32 var_name_hash, bool breakOnWrite,
                                    const char* condition, const char* trace, bool8 trace_on_cond)
{
	CVariableEntry* ve = NULL;
	if (object_id > 0)
	{
		CObjectEntry* oe = FindObjectEntry(object_id);
		if (!oe)
			return;

		ve = oe->GetVariableEntry(var_name_hash);
	}
	else
	{
		// -- first see if the variable is a local variable on the stack
		CDebuggerWatchVarEntry found_variable;
		if (!DebuggerFindStackVar(this, var_name_hash, found_variable, ve))
		{
			// -- not a stack variable - the only remaining option is a global
			ve = GetGlobalNamespace()->GetVarTable()->FindItem(var_name_hash);
		}
	}

	// -- if we found our variable, toggle the break
	if (ve)
		ve->SetBreakOnWrite(watch_request_id, mDebuggerSessionNumber, breakOnWrite, condition, trace, trace_on_cond);
}

// ====================================================================================================================
// GetExecutionCallStack():  Walk the function call execution stack, and return populated lists for the
// current script execution callstack
// ====================================================================================================================
int32 CScriptContext::GetExecutionCallStack(tIdentifierString* _obj_identifier_list, tIdentifierString* _funcname_list,
											tIdentifierString* _ns_list, tIdentifierString* _filename_list,
											int32* _linenumber_list, int32 max_count)
{
	// sanity check
	if (_obj_identifier_list == nullptr || _funcname_list == nullptr || _ns_list == nullptr ||
		_filename_list == nullptr || _linenumber_list == nullptr || max_count <= 0)
	{
		return 0;
	}

	CObjectEntry* oeList[kDebuggerCallstackSize];
	CFunctionEntry* feList[kDebuggerCallstackSize];
	uint32 nsHashList[kDebuggerCallstackSize];
	uint32 cbHashList[kDebuggerCallstackSize];
	int32 lineNumberList[kDebuggerCallstackSize];
	int32 stack_depth = CFunctionCallStack::GetCompleteExecutionStack(oeList, feList, nsHashList, cbHashList,
																	  lineNumberList, kDebuggerCallstackSize);

	// -- return the callstack as readable strings
	int32 stack_index = 0;
	while (stack_index < stack_depth && stack_index < max_count)
	{
		// -- function name
		SafeStrcpy(_funcname_list[stack_index].Text, tIdentifierString::Length,
				   TinScript::UnHash(feList[stack_index]->GetHash()));

		// -- object name/ID
		if (oeList[stack_index] != nullptr)
		{
			sprintf_s(_obj_identifier_list[stack_index].Text, tIdentifierString::Length, "[%d] %s", oeList[stack_index]->GetID(),
					  (oeList[stack_index]->GetNameHash() != 0 ? TinScript::UnHash(oeList[stack_index]->GetNameHash()) : ""));
		}
		else
		{
			_obj_identifier_list[stack_index].Text[0] = '\0';
		}

		// -- namespace
		SafeStrcpy(_ns_list[stack_index].Text, tIdentifierString::Length,
				   (nsHashList[stack_index] != 0 ? TinScript::UnHash(nsHashList[stack_index]) : ""));

		// -- filename
		SafeStrcpy(_filename_list[stack_index].Text, tIdentifierString::Length,
				   (cbHashList[stack_index] != 0 ? TinScript::UnHash(cbHashList[stack_index]) : ""));

		// -- line number
		_linenumber_list[stack_index] = lineNumberList[stack_index];

		++stack_index;
	}

	return stack_index;
}

// ====================================================================================================================
// DumpExecutionCallStack():  Print the entire script callstack
// ====================================================================================================================
void CScriptContext::DumpExecutionCallStack(int32 depth)
{
    CObjectEntry* oeList[kDebuggerCallstackSize];
    CFunctionEntry* feList[kDebuggerCallstackSize];
    uint32 nsHashList[kDebuggerCallstackSize];
    uint32 cbHashList[kDebuggerCallstackSize];
    int32 lineNumberList[kDebuggerCallstackSize];

    int dump_depth = depth > 0 && depth < kDebuggerCallstackSize ? depth : kDebuggerCallstackSize;
    int32 actual_depth = CFunctionCallStack::GetExecutionStackDepth();
    int32 stack_depth = CFunctionCallStack::GetCompleteExecutionStack(oeList, feList, nsHashList, cbHashList,
                                                                      lineNumberList, dump_depth);
    // -- if there's no execution stack available (because a console command was being executed probably), return
    if (stack_depth <= 0)
    {
        return;
    }

    if (actual_depth > stack_depth)
    {
        TinPrint(this, "### Script Callstack [%d / %d]:\n", stack_depth, actual_depth);
    }
    else
    {
        TinPrint(this, "### Script Callstack:\n");
    }
    char* bufferptr = TinScript::GetContext()->GetScratchBuffer();
    for (int32 i = 0; i < stack_depth; ++i)
    {
        CFunctionCallStack::FormatFunctionCallString(bufferptr, kMaxTokenLength, oeList[i], feList[i], nsHashList[i],
                                                     cbHashList[i], lineNumberList[i]);
        TinPrint(this, "    %s\n", bufferptr);
    }
}

// ====================================================================================================================
// DebuggerCurrentWorkingDir():  Use the packet type DATA, and notify the debugger of our current working directory
// ====================================================================================================================
void CScriptContext::DebuggerNotifyDirectories(const char* cwd, const char* exe_dir)
{
    if (!cwd)
        cwd = "./";
    if (!exe_dir)
        exe_dir = "./";

    // -- calculate the size of the data
    int32 total_size = 0;

    // -- first int32 will be identifying this data packet
    total_size += sizeof(int32);

    // -- we're sending the cwd as a string, since the debugger may not be able to unhash it yet
    // first the length, then the actual string, rounded up to 4-byte aligned
    int cwd_length = (int32)strlen(cwd) + 1;
    cwd_length += 4 - (cwd_length % 4);
    total_size += 4;
    total_size += cwd_length;

    // -- we're also sending the .exe dir
    int exe_length = (int32)strlen(exe_dir) + 1;
    exe_length += 4 - (exe_length % 4);
    total_size += 4;
    total_size += exe_length;

    // -- declare a header
    // -- note, if we ever implement a request/acknowledge approach, we can use the mID field
    SocketManager::tPacketHeader header(k_PacketVersion, SocketManager::tPacketHeader::DATA, total_size);

    // -- create the packet (null data, as we'll fill in the data directly into the packet)
    SocketManager::tDataPacket* newPacket = SocketManager::CreateDataPacket(&header, NULL);
    if (!newPacket)
    {
        TinPrint(this, "Error - DebuggerNotifyDirectories():  unable to send\n");
        return;
    }

    // -- initialize the ptr to the data buffer
    int32* dataPtr = (int32*)newPacket->mData;

    // -- write the identifier - defined in the debugger constants near the top of TinScript.h
    *dataPtr++ = k_DebuggerScriptAndExeDirsPacketID;

    // -- write the cwd string (first the length)
    *dataPtr++ = cwd_length;
    SafeStrcpy((char*)dataPtr, cwd_length, cwd, cwd_length);
    dataPtr += (cwd_length / 4);

    // -- write the exe string
    *dataPtr++ = exe_length;
    SafeStrcpy((char*)dataPtr, exe_length, exe_dir, exe_length);
    dataPtr += (exe_length / 4);

    // -- send the packet
    SocketManager::SendDataPacket(newPacket);
}

// ====================================================================================================================
// DebuggerCodeblockLoaded():  Use the packet type DATA, and notify the debugger of a codeblock we just loaded
// ====================================================================================================================
void CScriptContext::DebuggerCodeblockLoaded(uint32 codeblock_hash)
{
    // -- calculate the size of the data
    int32 total_size = 0;

    // -- first int32 will be identifying this data packet
    total_size += sizeof(int32);

    // -- we're sending the file name as a atring, since the debugger may not be able to unhash it yet
    const char* filename = UnHash(codeblock_hash);
    int32 strLength = (int32)strlen(filename) + 1;
    total_size += strLength;

    // -- declare a header
    // -- note, if we ever implement a request/acknowledge approach, we can use the mID field
    SocketManager::tPacketHeader header(k_PacketVersion, SocketManager::tPacketHeader::DATA, total_size);

    // -- create the packet (null data, as we'll fill in the data directly into the packet)
    SocketManager::tDataPacket* newPacket = SocketManager::CreateDataPacket(&header, NULL);
    if (!newPacket)
    {
        TinPrint(this, "Error - DebuggerCodeblockLoaded():  unable to send\n");
        return;
    }

    // -- initialize the ptr to the data buffer
    int32* dataPtr = (int32*)newPacket->mData;

    // -- write the identifier - defined in the debugger constants near the top of TinScript.h
    *dataPtr++ = k_DebuggerCodeblockLoadedPacketID;

    // -- write the string
    SafeStrcpy((char*)dataPtr, strLength, filename, strLength);

    // -- send the packet
    SocketManager::SendDataPacket(newPacket);
}

// ====================================================================================================================
// DebuggerBreakpointHit():  Use the packet type DATA, and send details of the current breakpoint we just hit
// ====================================================================================================================
void CScriptContext::DebuggerBreakpointHit(int32 watch_var_request_id, uint32 codeblock_hash, int32 line_number)
{
    // -- calculate the size of the data
    int32 total_size = 0;

    // -- first int32 will be identifying this data packet
    total_size += sizeof(int32);

    // -- next int32 will be the watch_var_request_id (> 0, if that's what caused the break)
    total_size += sizeof(int32);

    // -- next int32 will be the codeblock_hash
    total_size += sizeof(int32);

    // -- final int32 will be the line_number
    total_size += sizeof(int32);

    // -- declare a header
    // -- note, if we ever implement a request/acknowledge approach, we can use the mID field
    SocketManager::tPacketHeader header(k_PacketVersion, SocketManager::tPacketHeader::DATA, total_size);

    // -- create the packet (null data, as we'll fill in the data directly into the packet)
    SocketManager::tDataPacket* newPacket = SocketManager::CreateDataPacket(&header, NULL);
    if (!newPacket)
    {
        TinPrint(this, "Error - DebuggerBreakpointHit():  unable to send\n");
        return;
    }

    // -- initialize the ptr to the data buffer
    int32* dataPtr = (int32*)newPacket->mData;

    // -- write the identifier - defined in the debugger constants near the top of TinScript.h
    *dataPtr++ = k_DebuggerBreakpointHitPacketID;

	// -- write the watch_var_request_id
    *dataPtr++ = watch_var_request_id;

	// -- write the codeblock hash
    *dataPtr++ = codeblock_hash;

    // -- write the line number
    *dataPtr++ = line_number;

    // -- send the packet
    SocketManager::SendDataPacket(newPacket);
}

// ====================================================================================================================
// DebuggerBreakpointConfirm():  Use the packet type DATA, and correct the actual line number for a given breakpoint
// ====================================================================================================================
void CScriptContext::DebuggerBreakpointConfirm(uint32 codeblock_hash, int32 line_number, int32 actual_line)
{
    // -- calculate the size of the data
    int32 total_size = 0;

    // -- first int32 will be identifying this data packet
    total_size += sizeof(int32);

    // -- second int32 will be the codeblock_hash
    total_size += sizeof(int32);

    // -- next int32 will be the line_number
    total_size += sizeof(int32);

    // -- last int32 will be the actual line
    total_size += sizeof(int32);

    // -- declare a header
    // -- note, if we ever implement a request/acknowledge approach, we can use the mID field
    SocketManager::tPacketHeader header(k_PacketVersion, SocketManager::tPacketHeader::DATA, total_size);

    // -- create the packet (null data, as we'll fill in the data directly into the packet)
    SocketManager::tDataPacket* newPacket = SocketManager::CreateDataPacket(&header, NULL);
    if (!newPacket)
    {
        TinPrint(this, "Error - DebuggerBreakpointConfirm():  unable to send\n");
        return;
    }

    // -- initialize the ptr to the data buffer
    int32* dataPtr = (int32*)newPacket->mData;

    // -- write the identifier - defined in the debugger constants near the top of TinScript.h
    *dataPtr++ = k_DebuggerBreakpointConfirmPacketID;

    // -- write the codeblock hash
    *dataPtr++ = codeblock_hash;

    // -- write the line number
    *dataPtr++ = line_number;

    // -- write the line number
    *dataPtr++ = actual_line;

    // -- send the packet
    SocketManager::SendDataPacket(newPacket);
}

// ====================================================================================================================
// DebuggerVarWatchRemove():  removes a var watch (data breakpoint) by request id... only called for local vars
// ====================================================================================================================
void CScriptContext::DebuggerVarWatchRemove(int32 request_id)
{
    // -- sanity check
    if (request_id <= 0)
        return;

    // -- notify the debugger we've completed sending the list of objects
    char id_buf[8];
    sprintf_s(id_buf, sizeof(id_buf), "%d", request_id);
    SocketManager::SendExec(Hash("DebuggerVarWatchRemove"), id_buf);
}

// ====================================================================================================================
// DebuggerVarWatchConfirm():  Use the packet type DATA to confirm a variable watch
// ====================================================================================================================
void CScriptContext::DebuggerVarWatchConfirm(int32 request_id, uint32 watch_object_id, uint32 var_name_hash)
{
    // -- calculate the size of the data
    int32 total_size = 0;

    // -- first int32 will be identifying this data packet
    total_size += sizeof(int32);

    // -- next int32 will be the request_id
    total_size += sizeof(int32);

    // -- next int32 will be the watch_object_id
    total_size += sizeof(int32);

    // -- next int32 will be the var_name_hash
    total_size += sizeof(int32);

    // -- declare a header
    // -- note, if we ever implement a request/acknowledge approach, we can use the mID field
    SocketManager::tPacketHeader header(k_PacketVersion, SocketManager::tPacketHeader::DATA, total_size);

    // -- create the packet (null data, as we'll fill in the data directly into the packet)
    SocketManager::tDataPacket* newPacket = SocketManager::CreateDataPacket(&header, NULL);
    if (!newPacket)
    {
        TinPrint(this, "Error - DebuggerVarWatchConfirm():  unable to send\n");
        return;
    }

    // -- initialize the ptr to the data buffer
    int32* dataPtr = (int32*)newPacket->mData;

    // -- write the identifier - defined in the debugger constants near the top of TinScript.h
    *dataPtr++ = k_DebuggerVarWatchConfirmPacketID;

    // -- write the request_id
    *dataPtr++ = request_id;

    // -- write the object id
    *dataPtr++ = watch_object_id;

    // -- write the var name has
    *dataPtr++ = var_name_hash;

    // -- send the packet
    SocketManager::SendDataPacket(newPacket);
}

// ====================================================================================================================
// DebuggerSendCallstack():  Use the packet type DATA, and send the callstack data packet directly to the debugger.
// ====================================================================================================================
void CScriptContext::DebuggerSendCallstack(uint32* codeblock_array, uint32* objid_array,
                                           uint32* namespace_array, uint32* func_array,
                                           int32* linenumber_array, int array_size, uint32 print_msg_id)
{
    // -- calculate the size of the data
    int32 total_size = 0;

    // -- first int32 will be identifying this data packet
    total_size += sizeof(int32);

    // -- send the id of the print msg this callstack is associated with (errors for now...)
    total_size += sizeof(uint32);

    // -- next int32 will be the array size
    total_size += sizeof(int32);

    // -- finally, we have a uint32 for each of codeblock, objid, namespace, function, and line number
    // -- note:  the debugger supports a callstack of up to 32, so our max packet size is about 640 bytes
    // -- which is less than the max packet size (1024) specified in socket.h
    total_size += 5 * (sizeof(uint32)) * array_size;

    // -- declare a header
    // -- note, if we ever implement a request/acknowledge approach, we can use the mID field
    SocketManager::tPacketHeader header(k_PacketVersion, SocketManager::tPacketHeader::DATA, total_size);

    // -- create the packet (null data, as we'll fill in the data directly into the packet)
    SocketManager::tDataPacket* newPacket = SocketManager::CreateDataPacket(&header, NULL);
    if (!newPacket)
    {
        TinPrint(this, "Error - DebuggerSendCallstack():  unable to send\n");
        return;
    }

    // -- initialize the ptr to the data buffer
    int32* dataPtr = (int32*)newPacket->mData;

    // -- write the identifier - defined in the debugger constants near the top of TinScript.h
    *dataPtr++ = k_DebuggerCallstackPacketID;

    // -- now the print msg id, if this callstack is associated with an error message (as opposed to a breakpoint)
    *dataPtr++ = print_msg_id;

    // -- write the array size
    *dataPtr++ = array_size;

    // -- write the codeblocks
    memcpy(dataPtr, codeblock_array, sizeof(uint32) * array_size);
    dataPtr += array_size;

    // -- write the objid's
    memcpy(dataPtr, objid_array, sizeof(uint32) * array_size);
    dataPtr += array_size;

    // -- write the namespaces
    memcpy(dataPtr, namespace_array, sizeof(uint32) * array_size);
    dataPtr += array_size;

    // -- write the functions
    memcpy(dataPtr, func_array, sizeof(uint32) * array_size);
    dataPtr += array_size;

    // -- write the line numbers
    memcpy(dataPtr, linenumber_array, sizeof(uint32) * array_size);
    dataPtr += array_size;

    // -- now send the packet
    SocketManager::SendDataPacket(newPacket);
}

// ====================================================================================================================
// DebuggerSendCallstack():  Convert the raw execution stack arrays to the format used by the debugger.
// ====================================================================================================================
void CScriptContext::DebuggerSendCallstack(CObjectEntry** oeList, CFunctionEntry** feList, uint32* nsHashList,
                                           uint32* cbHashList, int32* lineNumberList, int array_size,
                                           uint32 print_msg_id)
{
    // -- sanity check
    if (array_size <= 0 || oeList == nullptr || feList == nullptr || nsHashList == nullptr || cbHashList == nullptr ||
        lineNumberList == nullptr)
    {
        return;
    }
    // -- limit the amount of data we're sending (this method is used for assert messages, etc... not the actual
    // callstack view in the debugger
    if (array_size > kDebuggerCallstackSize)
        array_size = kDebuggerCallstackSize;

    uint32 objid_array[kDebuggerCallstackSize];
    uint32 func_hash_array[kDebuggerCallstackSize];
    for (int32 i = 0; i < array_size; ++i)
    {
        objid_array[i] = oeList[i] != nullptr ? oeList[i]->GetID() : 0;
        func_hash_array[i] = feList[i] != nullptr ? feList[i]->GetHash() : 0;
    }

    DebuggerSendCallstack(cbHashList, objid_array, nsHashList, func_hash_array, lineNumberList, array_size, print_msg_id);
}

// ====================================================================================================================
// DebuggerWatchFormatValue(): format the mValue to be debugger-friendly (e.g.  object ID with object name, ...)
// ====================================================================================================================
void CScriptContext::DebuggerWatchFormatValue(CDebuggerWatchVarEntry* watch_var_entry, void* val_addr)
{
    // -- sanity check
    if (watch_var_entry == nullptr || val_addr == nullptr)
        return;

    switch (watch_var_entry->mType)
    {
        case TYPE_object:
        {
            CObjectEntry* oe = FindObjectEntry(watch_var_entry->mVarObjectID);
            if (oe != nullptr)
            {
                uint32 bytes_written = 0;
                if (oe->GetNameHash() != 0 && oe->GetNamespace() != nullptr)
                {
                    bytes_written = sprintf_s(watch_var_entry->mValue, sizeof(watch_var_entry->mValue), "%d: %s [%s]",
                                              oe->GetID(), UnHash(oe->GetNameHash()),
                                              UnHash(oe->GetNamespace()->GetHash()));
                }
                else if (oe->GetNamespace() != nullptr)
                {
                    bytes_written = sprintf_s(watch_var_entry->mValue, sizeof(watch_var_entry->mValue), "%d: [%s]",
                                              oe->GetID(), UnHash(oe->GetNamespace()->GetHash()));
                }
                else
                {
                    bytes_written = sprintf_s(watch_var_entry->mValue, sizeof(watch_var_entry->mValue), "%d", oe->GetID());
                }

                // -- make sure we terminate
                if (bytes_written >= kMaxNameLength)
                    bytes_written = kMaxNameLength - 1;
                watch_var_entry->mValue[bytes_written] = '\0';
            }
        }
        break;

        case TYPE_int:
        {
            int32 string_hash = *(uint32*)val_addr;
            const char* hashed_string = GetStringTable()->FindString(string_hash);
            if (hashed_string != nullptr && hashed_string[0] != '\0')
            {
                uint32 bytes_written = sprintf_s(watch_var_entry->mValue, sizeof(watch_var_entry->mValue),
                                                 "%d  [0x%x `%s`]", string_hash, string_hash, hashed_string);
                // -- make sure we terminate
                if (bytes_written >= kMaxNameLength)
                    bytes_written = kMaxNameLength - 1;
                watch_var_entry->mValue[bytes_written] = '\0';
            }
            else
            {
                sprintf_s(watch_var_entry->mValue, sizeof(watch_var_entry->mValue), "%d", string_hash);
            }
        }
        break;

        default:
        {
            // -- copy the value, as a string (to a max length)
            gRegisteredTypeToString[watch_var_entry->mType](this, val_addr, watch_var_entry->mValue, kMaxNameLength);
            return;
        }
    }
}

// ====================================================================================================================
// DebuggerSendWatchVariable():  Send a variable entry to the debugger
// ====================================================================================================================
void CScriptContext::DebuggerSendWatchVariable(CDebuggerWatchVarEntry* watch_var_entry)
{
    // -- calculate the size of the data
    int32 total_size = 0;

    // -- first int32 will be identifying this data packet
    total_size += sizeof(int32);

	// -- request ID (only used for dynamic watch requests)
	total_size += sizeof(int32);

	// -- stack level
	total_size += sizeof(int32);

    // -- function namespace hash
    total_size += sizeof(int32);

    // -- function hash
    total_size += sizeof(int32);

    // -- function object ID
    total_size += sizeof(int32);

    // -- object ID
    total_size += sizeof(int32);

    // -- namespace Hash
    total_size += sizeof(int32);

    // -- var type
    total_size += sizeof(int32);

    // -- array size
    total_size += sizeof(int32);

    // -- the next will be mName - we'll round up to a 4-byte aligned length (including the EOL)
    int32 nameLength = (int32)strlen(watch_var_entry->mVarName) + 1;
    nameLength += 4 - (nameLength % 4);

    // -- we send first the string length, then the string
    total_size += sizeof(int32);
    total_size += nameLength;

    // -- finally we add the value - we'll round up to a 4-byte aligned length (including the EOL)
    int32 valueLength = (int32)strlen(watch_var_entry->mValue) + 1;
    valueLength += 4 - (valueLength % 4);

    // -- we send first the string length, then the string
    total_size += sizeof(int32);
    total_size += valueLength;

    // -- cached var name hash
    total_size += sizeof(int32);

    // -- cached var object ID
    total_size += sizeof(int32);

    // -- declare a header
    // -- note, if we ever implement a request/acknowledge approach, we can use the mID field
    SocketManager::tPacketHeader header(k_PacketVersion, SocketManager::tPacketHeader::DATA, total_size);

    // -- create the packet (null data, as we'll fill in the data directly into the packet)
    SocketManager::tDataPacket* newPacket = SocketManager::CreateDataPacket(&header, NULL);
    if (!newPacket)
    {
        TinPrint(this, "Error - DebuggerSendWatchVariable():  unable to send\n");
        return;
    }

    // -- initialize the ptr to the data buffer
    int32* dataPtr = (int32*)newPacket->mData;

    // -- write the identifier - defined in the debugger constants near the top of TinScript.h
    *dataPtr++ = k_DebuggerWatchVarEntryPacketID;

	// -- write the request ID
	*dataPtr++ = watch_var_entry->mWatchRequestID;

	// -- write the stack level
	*dataPtr++ = watch_var_entry->mStackOffsetFromBottom;

    // -- write the function namespace
    *dataPtr++ = watch_var_entry->mFuncNamespaceHash;

    // -- write the function hash
    *dataPtr++ = watch_var_entry->mFunctionHash;

    // -- write the function object iD
    *dataPtr++ = watch_var_entry->mFunctionObjectID;

    // -- write the "objectID" (required, if this is a member)
    *dataPtr++ = watch_var_entry->mObjectID;

    // -- write the "namespace hash" (required, if this is a member)
    *dataPtr++ = watch_var_entry->mNamespaceHash;

    // -- write the type
    *dataPtr++ = watch_var_entry->mType;

    // -- write the array size
    *dataPtr++ = watch_var_entry->mArraySize;

    // -- write the name string length
    *dataPtr++ = nameLength;

    // -- write the var name string
    SafeStrcpy((char*)dataPtr, nameLength, watch_var_entry->mVarName, nameLength);
    dataPtr += (nameLength / 4);

    // -- write the value string length
    *dataPtr++ = valueLength;

    // -- write the value string
    SafeStrcpy((char*)dataPtr, valueLength, watch_var_entry->mValue, valueLength);
    dataPtr += (valueLength / 4);

    // -- write the cached var name hash
    *dataPtr++ = watch_var_entry->mVarHash;

    // -- write the cached var object ID
    *dataPtr++ = watch_var_entry->mVarObjectID;

    // -- send the packet
    SocketManager::SendDataPacket(newPacket);
}

// ====================================================================================================================
// void DebuggerSendObjectMembers():  Given an object ID, send the entire hierarchy of members to the debugger
// ====================================================================================================================
void CScriptContext::DebuggerSendObjectMembers(CDebuggerWatchVarEntry* callingFunction, uint32 object_id)
{
    CObjectEntry* oe = FindObjectEntry(object_id);
    if (!oe)
        return;

    // -- send the dynamic var table
    if (oe->GetDynamicVarTable())
    {
        // -- send the header to the debugger
        CDebuggerWatchVarEntry watch_entry;

		// -- Inherit the calling function request ID
		watch_entry.mWatchRequestID = callingFunction ? callingFunction->mWatchRequestID : 0;
        watch_entry.mStackOffsetFromBottom = callingFunction ? callingFunction->mStackOffsetFromBottom : -1;

        watch_entry.mFuncNamespaceHash = callingFunction ? callingFunction->mFuncNamespaceHash : 0;
        watch_entry.mFunctionHash = callingFunction ? callingFunction->mFunctionHash : 0;
        watch_entry.mFunctionObjectID = callingFunction ? callingFunction->mFunctionObjectID : 0;

        watch_entry.mObjectID = object_id;
        watch_entry.mNamespaceHash = Hash("self");

        // -- TYPE_void marks this as a namespace label, and set the object's name as the value
        watch_entry.mType = TYPE_void;
        SafeStrcpy(watch_entry.mVarName, sizeof(watch_entry.mVarName), "self", kMaxNameLength);
        SafeStrcpy(watch_entry.mValue, sizeof(watch_entry.mValue), oe->GetName(), kMaxNameLength);

        // -- zero out the size
        watch_entry.mArraySize = 0;

        // -- fill in the cached members
        watch_entry.mVarHash = watch_entry.mNamespaceHash;
        watch_entry.mVarObjectID = 0;

        // -- send to the Debugger
        DebuggerSendWatchVariable(&watch_entry);

        // -- now send var table members
        DebuggerSendObjectVarTable(callingFunction, oe, watch_entry.mNamespaceHash, oe->GetDynamicVarTable());
    }

    // -- loop through the hierarchy of namespaces
    CNamespace* ns = oe->GetNamespace();
    while (ns)
    {
        CDebuggerWatchVarEntry ns_entry;

		// -- Inherit the calling function request ID
		ns_entry.mWatchRequestID = callingFunction ? callingFunction->mWatchRequestID : 0;;
        ns_entry.mStackOffsetFromBottom = callingFunction ? callingFunction->mStackOffsetFromBottom : -1;

        ns_entry.mFuncNamespaceHash = callingFunction ? callingFunction->mFuncNamespaceHash : 0;
        ns_entry.mFunctionHash = callingFunction ? callingFunction->mFunctionHash : 0;
        ns_entry.mFunctionObjectID = callingFunction ? callingFunction->mFunctionObjectID : 0;

        ns_entry.mObjectID = object_id;
        ns_entry.mNamespaceHash = ns->GetHash();

        // -- TYPE_void marks this as a namespace label
        ns_entry.mType = TYPE_void;
        SafeStrcpy(ns_entry.mVarName, sizeof(ns_entry.mVarName), UnHash(ns->GetHash()), kMaxNameLength);
        ns_entry.mValue[0] = '\0';

        // -- zero out the size
        ns_entry.mArraySize = 0;

        // -- fill in the cached members
        ns_entry.mVarHash = ns_entry.mNamespaceHash;
        ns_entry.mVarObjectID = 0;

        // -- send to the Debugger
        DebuggerSendWatchVariable(&ns_entry);

        // -- dump the vtable
        DebuggerSendObjectVarTable(callingFunction, oe, ns_entry.mNamespaceHash, ns->GetVarTable());

        // -- get the next namespace
        ns = ns->GetNext();
    }
}

// ====================================================================================================================
// DebuggerSendObjectVarTable():  Send a tVarTable to the debugger.
// ====================================================================================================================
void CScriptContext::DebuggerSendObjectVarTable(CDebuggerWatchVarEntry* callingFunction, CObjectEntry* oe,
                                                uint32 ns_hash, tVarTable* var_table)
{
    if (!var_table)
        return;

    CVariableEntry* member = var_table->First();
    while (member)
    {
        // - -- declare the new entry
        CDebuggerWatchVarEntry member_entry;

		// -- Inherit the calling function request ID
		member_entry.mWatchRequestID = callingFunction ? callingFunction->mWatchRequestID : 0;
		member_entry.mStackOffsetFromBottom = callingFunction ? callingFunction->mStackOffsetFromBottom : -1;

        member_entry.mFuncNamespaceHash = callingFunction ? callingFunction->mFuncNamespaceHash : 0;
        member_entry.mFunctionHash = callingFunction ? callingFunction->mFunctionHash : 0;
        member_entry.mFunctionObjectID = callingFunction ? callingFunction->mFunctionObjectID : 0;

        // -- fill in the objectID, namespace hash
        member_entry.mObjectID = oe->GetID();
        member_entry.mNamespaceHash = ns_hash;

        // -- set the type and size
        member_entry.mType = member->GetType();
        member_entry.mArraySize = member->GetArraySize();

        // -- member name
        SafeStrcpy(member_entry.mVarName, sizeof(member_entry.mVarName), UnHash(member->GetHash()), kMaxNameLength);

        // -- fill in the cached members
        member_entry.mVarHash = member->GetHash();
        member_entry.mVarObjectID = 0;
        if (member->GetType() == TYPE_object)
        {
             member_entry.mVarObjectID = *(uint32*)(member->GetAddr(oe->GetAddr()));
        }

        // -- after the member_entry has all fields filled in, see if we can format
        // the mValue to be more readable
        DebuggerWatchFormatValue(&member_entry, member->GetAddr(oe->GetAddr()));

        // -- send to the debugger
        DebuggerSendWatchVariable(&member_entry);

        // -- get the next member
        member = var_table->Next();
    }
}

// ====================================================================================================================
// DebuggerSendAssert():  Use the packet type DATA, and notify the debugger of an assert
// ====================================================================================================================
void CScriptContext::DebuggerSendAssert(const char* assert_msg, uint32 codeblock_hash, int32 line_number)
{
    // -- no null strings
    if (!assert_msg)
        assert_msg = "";

    // -- calculate the size of the data
    int32 total_size = 0;

    // -- first int32 will be identifying this data packet
    total_size += sizeof(int32);

    // -- next is the message id
    total_size += sizeof(uint32);

    // -- send the length of the assert message, including EOL, and 4-byte aligned
    int32 msgLength = (int32)strlen(assert_msg) + 1;
    msgLength += 4 - (msgLength % 4);

    // -- we'll be sending the length of the message, followed by the actual message string
    total_size += sizeof(int32);
    total_size += msgLength;

    // -- send the codeblock hash
    total_size += sizeof(int32);

    // -- send the line number
    total_size += sizeof(int32);

    // -- declare a header
    // -- note, if we ever implement a request/acknowledge approach, we can use the mID field
    SocketManager::tPacketHeader header(k_PacketVersion, SocketManager::tPacketHeader::DATA, total_size);

    // -- create the packet (null data, as we'll fill in the data directly into the packet)
    SocketManager::tDataPacket* newPacket = SocketManager::CreateDataPacket(&header, NULL);
    if (!newPacket)
    {
        TinPrint(this, "Error - DebuggerSendAssert():  unable to send\n");
        return;
    }

    // -- initialize the ptr to the data buffer
    int32* dataPtr = (int32*)newPacket->mData;

    // -- write the identifier - defined in the debugger constants near the top of TinScript.h
    *dataPtr++ = k_DebuggerAssertMsgPacketID;

    // -- write the print msg id - so we can associated a callstack with this message (if applicable)
    uint32 print_msg_id = ++mDebuggerPrintMsgId;
    *dataPtr++ = print_msg_id;

    // -- send the length of the assert message, including EOL, and 4-byte aligned
    *dataPtr++ = msgLength;

    // -- write the message string
    SafeStrcpy((char*)dataPtr, msgLength, assert_msg, msgLength);
    dataPtr += (msgLength / 4);

    // -- send the codeblock hash
    *dataPtr++ = codeblock_hash;

    // -- send the line number
    *dataPtr++ = line_number;

    // -- send the packet
    SocketManager::SendDataPacket(newPacket);

    // -- attach a callstack to the assert msg
    // -- use the integration tunable, since this limits how much data we're sending to the debugger
    CObjectEntry* oeList[kExecAssertStackDepth];
    CFunctionEntry* feList[kExecAssertStackDepth];
    uint32 nsHashList[kExecAssertStackDepth];
    uint32 cbHashList[kExecAssertStackDepth];
    int32 lineNumberList[kExecAssertStackDepth];

    int32 depth = GetAssertStackDepth();
    int dump_depth = depth > 0 && depth < kExecAssertStackDepth ? depth : kExecAssertStackDepth;
    int32 stack_depth = CFunctionCallStack::GetCompleteExecutionStack(oeList, feList, nsHashList, cbHashList,
                                                                      lineNumberList, dump_depth);

    // -- if there's no execution stack available (because a console command was being executed probably), return
    if (stack_depth > 0)
    {
        DebuggerSendCallstack(oeList, feList, nsHashList, cbHashList, lineNumberList, stack_depth, print_msg_id);
    }
}

// ====================================================================================================================
// DebuggerSendPrint():  Send a print message to the debugger (usually to echo the local output)
// ====================================================================================================================
void CScriptContext::DebuggerSendPrint(int32 severity, const char* fmt, ...)
{
    // -- ensure we have a valid string, and a connected debugger
    if (!fmt || !SocketManager::IsConnected())
        return;

    // -- compose the message
    va_list args;
    va_start(args, fmt);
    char msg_buf[512];
    vsprintf_s(msg_buf, 512, fmt, args);
    va_end(args);

    // -- calculate the size of the data
    int32 total_size = 0;

    // -- first int32 will be identifying this data packet
    total_size += sizeof(int32);

    // -- next int32 will be the print msg id (so later we can associate a callstack with the message)
    total_size += sizeof(int32);

	// -- next int32 will be the severity
	total_size += sizeof(int32);

    // -- send the length of the assert message, including EOL, and 4-byte aligned
    int32 msgLength = (int32)strlen(msg_buf) + 1;
    msgLength += 4 - (msgLength % 4);

    // -- we'll be sending the length of the message, followed by the actual message string
    total_size += sizeof(int32);
    total_size += msgLength;

    // -- declare a header
    // -- note, if we ever implement a request/acknowledge approach, we can use the mID field
    SocketManager::tPacketHeader header(k_PacketVersion, SocketManager::tPacketHeader::DATA, total_size);

    // -- create the packet (null data, as we'll fill in the data directly into the packet)
    SocketManager::tDataPacket* newPacket = SocketManager::CreateDataPacket(&header, NULL);
    if (!newPacket)
    {
        TinPrint(this, "Error - DebuggerSendPrint():  unable to send\n");
        return;
    }

    // -- initialize the ptr to the data buffer
    int32* dataPtr = (int32*)newPacket->mData;

    // -- write the identifier - defined in the debugger constants near the top of TinScript.h
    *dataPtr++ = k_DebuggerPrintMsgPacketID;

    // -- write the print msg id - so we can associated a callstack with this message (if applicable)
    uint32 print_msg_id = ++mDebuggerPrintMsgId;
    *dataPtr++ = print_msg_id;

	// -- send the length of the assert message, including EOL, and 4-byte aligned
	*dataPtr++ = severity;

    // -- send the length of the assert message, including EOL, and 4-byte aligned
    *dataPtr++ = msgLength;

    // -- write the message string
    SafeStrcpy((char*)dataPtr, msgLength, msg_buf, msgLength);
    dataPtr += (msgLength / 4);

    // -- if the severity is 0, send as a print data packet -
    // in the case of extreme spam it might be sent out of order and/or ignored
    if (severity == 0)
    {

        // -- send the packet
        SocketManager::SendPrintDataPacket(newPacket);
    }

    // -- for now, only attach a callstack to warnings/errors etc...
    else
    {
        // -- send the packet
        SocketManager::SendDataPacket(newPacket);

        // -- use the integration tunable, since this limits how much data we're sending to the debugger
        CObjectEntry* oeList[kExecAssertStackDepth];
        CFunctionEntry* feList[kExecAssertStackDepth];
        uint32 nsHashList[kExecAssertStackDepth];
        uint32 cbHashList[kExecAssertStackDepth];
        int32 lineNumberList[kExecAssertStackDepth];

        int32 depth = GetAssertStackDepth();
        int dump_depth = depth > 0 && depth < kExecAssertStackDepth ? depth : kExecAssertStackDepth;
        int32 stack_depth = CFunctionCallStack::GetCompleteExecutionStack(oeList, feList, nsHashList, cbHashList,
                                                                          lineNumberList, dump_depth);

        // -- if there's no execution stack available (because a console command was being executed probably), return
        if (stack_depth > 0)
        {
            DebuggerSendCallstack(oeList, feList, nsHashList, cbHashList, lineNumberList, stack_depth, print_msg_id);
        }
    }
}

// ====================================================================================================================
// DebuggerRequestFunctionAssist():  Sends the debugger a list of function assist entries for each method available.
// ====================================================================================================================
void CScriptContext::DebuggerRequestFunctionAssist(uint32 object_id)
{
    // -- if we're given an id, ensure we can find the object
    CObjectEntry* oe = object_id > 0 ? FindObjectEntry(object_id) : NULL;
    if (object_id > 0 && !oe)
        return;

    // -- send the function entries through the entire hierarchy (or just the global if requested)
    tFuncTable* function_table = NULL;
    CNamespace* current_namespace = NULL;

    // -- if we're sending global functions, we need the global namespace only
    if (object_id == 0)
    {
        // -- we're sending the list of namespaces
        CHashTable<CNamespace>* namespaces = GetNamespaceDictionary();
        if (namespaces != nullptr)
        {
            current_namespace = namespaces->First();
            while (current_namespace != nullptr)
            {
                const char* current_name = current_namespace->GetName();
                if (current_name && current_name[0])
                {
                    CDebuggerFunctionAssistEntry entry;
                    entry.mEntryType = eFunctionEntryType::Namespace;
                    entry.mObjectID = 0;
                    entry.mNamespaceHash = current_namespace->GetHash();
                    entry.mFunctionHash = 0;
                    entry.mCodeBlockHash = 0;
                    entry.mLineNumber = 0;
                    entry.mParameterCount = 0;
                    SafeStrcpy(entry.mSearchName, sizeof(entry.mSearchName), current_name);

                    entry.mHasDefaultValues = false;
                    entry.mHelpString[0] = '\0';

                    // -- send the entry
                    DebuggerSendFunctionAssistEntry(entry);
                }
                current_namespace = namespaces->Next();
            }
        }

        // -- we also send the global functions
        function_table = GetGlobalNamespace()->GetFuncTable();
    }
    else
    {
        current_namespace = oe->GetNamespace();
        function_table = current_namespace->GetFuncTable();
    }

    // -- send the hierarchy
    while (function_table)
    {
        // -- send the function table for the given namespace
        DebuggerSendFunctionTable(object_id, current_namespace ? current_namespace->GetHash() : 0, function_table);

        // -- get the next function table
        if (object_id != 0)
        {
            current_namespace = current_namespace->GetNext();
            if (current_namespace)
                function_table = current_namespace->GetFuncTable();
            else
                function_table = NULL;
        }
        else
            function_table = NULL;
    }
}

// ====================================================================================================================
// DebuggerRequestNamespaceAssist():  Sends the debugger a list of functions registered for a given namespace.
// ====================================================================================================================
void CScriptContext::DebuggerRequestNamespaceAssist(uint32 ns_hash)
{
    CNamespace* current_namespace = ns_hash == 0 || (GetGlobalNamespace() != nullptr &&
                                                     ns_hash == GetGlobalNamespace()->GetHash())
                                    ? GetGlobalNamespace()
                                    : GetNamespaceDictionary()->FindItem(ns_hash);

    // -- send the entire hierarchy
    while (current_namespace != nullptr)
    {
        tFuncTable* function_table = current_namespace->GetFuncTable();
        if (function_table != nullptr)
        {
            DebuggerSendFunctionTable(0, current_namespace->GetHash(), function_table);
        }

        current_namespace = current_namespace->GetNext();
    }
}

// ====================================================================================================================
// DebuggerSendFunctionTable():  sends the debugger the list of functions for the function table
// ====================================================================================================================
void CScriptContext::DebuggerSendFunctionTable(int32 object_id, uint32 ns_hash, tFuncTable* function_table)
{
    // -- populate and send a function assist entry
    CFunctionEntry* function_entry = function_table->First();
    while (function_entry)
    {
        CDebuggerFunctionAssistEntry entry;
        entry.mEntryType = eFunctionEntryType::Function;
        entry.mObjectID = object_id;
        entry.mNamespaceHash = ns_hash;
        entry.mFunctionHash = function_entry->GetHash();
        SafeStrcpy(entry.mSearchName, sizeof(entry.mSearchName), function_entry->GetName(), kMaxNameLength);

		// -- get the codeblock, and fill in the line number
		entry.mCodeBlockHash = function_entry->GetCodeBlock()
								? function_entry->GetCodeBlock()->GetFilenameHash()
								: 0;

		// -- calculate the line number
		entry.mLineNumber = 0;
		if (entry.mCodeBlockHash != 0)
		{
			CCodeBlock* codeblock = function_entry->GetCodeBlock();
			uint32 offset = function_entry->GetCodeBlockOffset(codeblock);
			const uint32* instrptr = codeblock->GetInstructionPtr();
			instrptr += offset;
			entry.mLineNumber = codeblock->CalcLineNumber(instrptr);
		}

        // -- fill in the parameters
        CFunctionContext* function_context = function_entry->GetContext();
        entry.mParameterCount = function_context->GetParameterCount();
        if (entry.mParameterCount > kMaxRegisteredParameterCount + 1)
            entry.mParameterCount = kMaxRegisteredParameterCount + 1;
        for (int i = 0; i < entry.mParameterCount; ++i)
        {
            CVariableEntry* parameter = function_context->GetParameter(i);
            entry.mType[i] = parameter->GetType();
            entry.mIsArray[i] = parameter->IsArray();
            entry.mNameHash[i] = parameter->GetHash();
        }

        // -- fill in the help string and default values
        CRegDefaultArgValues* default_args = function_entry->GetRegObject()
                                             ? function_entry->GetRegObject()->GetDefaultArgValues()
                                             : nullptr;
        if (default_args != nullptr)
        {
            // -- copy the help string
            SafeStrcpy(entry.mHelpString, sizeof(entry.mHelpString), default_args->GetHelpString());

            // -- default arg types are guaranteed to match parameter types
            CRegDefaultArgValues::tDefaultValue* storage = nullptr;
            int32 default_count = default_args->GetDefaultArgStorage(storage);
            entry.mHasDefaultValues = (default_count > 0);
            for (int i = 0; i < default_count; ++i)
            {
                // -- replace the name hash, with the default args param name
                if (storage[i].mName[0] != '\0')
                    entry.mNameHash[i] = Hash(storage[i].mName);

                // -- copy the block of data representing the default value
                // -- if this is a string, we want to send the string hash instead
                // note:  parameter 0 is the return value, the default value is meaningless
                if (storage[i].mType == TYPE_string)
                {
                    uint32 hash = Hash(*(char**)storage[i].mValue);
                    memcpy(entry.mDefaultValue[i], &hash, sizeof(uint32));
                }
                else if (storage[i].mType < TYPE_COUNT && gRegisteredTypeSize[storage[i].mType] > 0)
                {
                    memcpy(entry.mDefaultValue[i], storage[i].mValue, gRegisteredTypeSize[storage[i].mType]);
                }
                else
                {
                    memset(entry.mDefaultValue[i], 0, sizeof(uint32) * MAX_TYPE_SIZE);
                }
            }
        }

        // -- send the entry
        DebuggerSendFunctionAssistEntry(entry);

        // -- get the next
        function_entry = function_table->Next();
    }
}

// ====================================================================================================================
// DebuggerSendFunctionAssistEntry():  Send a a function and it's parameter list to the debugger
// ====================================================================================================================
void CScriptContext::DebuggerSendFunctionAssistEntry(const CDebuggerFunctionAssistEntry& function_assist_entry)
{
    // -- calculate the size of the data
    int32 total_size = 0;

    // -- first int32 will be identifying this data packet
    total_size += sizeof(int32);

    // -- entry type
    total_size += sizeof(int32);

	// -- object ID
	total_size += sizeof(int32);

	// -- namespace hash
	total_size += sizeof(int32);

	// -- function hash
	total_size += sizeof(int32);

    // -- calculate the length of the function name, including EOL, and 4-byte aligned
    int32 nameLength = (int32)strlen(function_assist_entry.mSearchName) + 1;
    nameLength += 4 - (nameLength % 4);

    // -- add the function name length, followed by the actual string
    total_size += sizeof(int32);
    total_size += nameLength;

	// -- add the codeblock filename hash, and the linenumber for the instruction offset
	total_size += sizeof(uint32);
	total_size += sizeof(int32);

    // -- parameter count
    total_size += sizeof(int32);

    // -- types (x parameter count)
    total_size += (sizeof(int32) * function_assist_entry.mParameterCount);

    // -- is array (x parameter count)
    total_size += (sizeof(int32) * function_assist_entry.mParameterCount);

    // -- parameter name hash (x parameter count)
    total_size += (sizeof(int32) * function_assist_entry.mParameterCount);

    // -- help string
    int32 helpLength = (int32)strlen(function_assist_entry.mHelpString) + 1;
    helpLength += 4 - (helpLength % 4);

    // -- add the help string length, followed by the actual string
    total_size += sizeof(int32);
    total_size += helpLength;

    // -- add the bool for sending default arg values
    total_size += sizeof(int32);

    // -- if we send default values, add the storage size
    // note:  we don't send a default value for 0, since it's the return parameter
    if (function_assist_entry.mHasDefaultValues)
    {
        total_size += (sizeof(uint32) * MAX_TYPE_SIZE) * (function_assist_entry.mParameterCount - 1);
    }

    // -- declare a header
    // -- note, if we ever implement a request/acknowledge approach, we can use the mID field
    SocketManager::tPacketHeader header(k_PacketVersion, SocketManager::tPacketHeader::DATA, total_size);

    // -- create the packet (null data, as we'll fill in the data directly into the packet)
    SocketManager::tDataPacket* newPacket = SocketManager::CreateDataPacket(&header, NULL);
    if (!newPacket)
    {
        TinPrint(this, "Error - DebuggerSendFunctionAssistEntry():  unable to send\n");
        return;
    }

    // -- initialize the ptr to the data buffer
    int32* dataPtr = (int32*)newPacket->mData;

    // -- write the identifier - defined in the debugger constants near the top of TinScript.h
    *dataPtr++ = k_DebuggerFunctionAssistPacketID;
    
    // -- entry type
    *dataPtr++ = (int32)function_assist_entry.mEntryType;

	// -- object ID
	*dataPtr++ = function_assist_entry.mObjectID;

	// -- namespace hash
	*dataPtr++ = function_assist_entry.mNamespaceHash;

	// -- function hash
	*dataPtr++ = function_assist_entry.mFunctionHash;

    // -- send the function name length
    *dataPtr++ = nameLength;

    // -- write the message string
    SafeStrcpy((char*)dataPtr, nameLength, function_assist_entry.mSearchName, nameLength);
    dataPtr += (nameLength / 4);

	// -- codeblock filename hash and linenumber
	*dataPtr++ = function_assist_entry.mCodeBlockHash;
	*dataPtr++ = function_assist_entry.mLineNumber;

	// -- parameter count
    *dataPtr++ = function_assist_entry.mParameterCount;

    // -- loop through, and send each parameter
    for (int i = 0; i < function_assist_entry.mParameterCount; ++i)
    {
        // -- type
        *dataPtr++ = (int32)(function_assist_entry.mType[i]);

        // -- is array
        *dataPtr++ = function_assist_entry.mIsArray[i] ? 1 : 0;

        // -- name hash
        *dataPtr++ = function_assist_entry.mNameHash[i];
    }

    // -- send the help string length
    *dataPtr++ = helpLength;

    // -- write the help string
    SafeStrcpy((char*)dataPtr, helpLength, function_assist_entry.mHelpString, helpLength);
    dataPtr += (helpLength / 4);

    // -- send the bool for sending default arg values
    *dataPtr++ = function_assist_entry.mHasDefaultValues ? true : false;

    // -- if we send default values, add the storage size
    if (function_assist_entry.mHasDefaultValues)
    {
        for (int i = 1; i < function_assist_entry.mParameterCount; ++i)
        {
            memcpy((void*)dataPtr, function_assist_entry.mDefaultValue[i], sizeof(uint32) * MAX_TYPE_SIZE);
            dataPtr += MAX_TYPE_SIZE;
        }
    }

    // -- send the packet
    SocketManager::SendDataPacket(newPacket);
}

// ====================================================================================================================
// DebuggerNotifyCreateObject():  Send an object entry to the debugger, with the object's name and derivation.
// ====================================================================================================================
void CScriptContext::DebuggerNotifyCreateObject(CObjectEntry* oe)
{
    // -- sanity check
    int32 debugger_session = 0;
    if (!oe || !IsDebuggerConnected(debugger_session))
        return;

    // -- create the derivation string
    char derivation_buf[kMaxNameLength * 2];
    char* derivation_ptr = derivation_buf;
    *derivation_ptr = '\0';
    int32 remaining = kMaxNameLength;
    bool8 first = true;
    CNamespace* ns = oe->GetNamespace();
    while (ns && remaining > 0)
    {
        // -- if this is registered class, highlight it
        if (ns->IsRegisteredClass())
            sprintf_s(derivation_ptr, remaining, "%s[%s]", !first ? "-->" : " ", UnHash(ns->GetHash()));
        else
            sprintf_s(derivation_ptr, remaining, "%s%s", !first ? "-->" : " ", UnHash(ns->GetHash()));

        // -- update the pointer
        int32 length = (int32)strlen(derivation_ptr);
        remaining -= length;
        derivation_ptr += length;

        // -- next namespace in the hierarchy
        first = false;
        ns = ns->GetNext();
    }

    // -- ensure the buffer is null terminated
    derivation_buf[kMaxNameLength - 1] = '\0';

    // -- get the file/line from where this object was created
    int32 stack_size = 0;
    uint32 created_file_array[kDebuggerCallstackSize];
    int32 created_lines_array[kDebuggerCallstackSize];
    if (!CMemoryTracker::GetCreatedCallstack(oe->GetID(), stack_size, created_file_array, created_lines_array) || stack_size <= 0)
    {
        // -- memory tracking is disabled - ensure we have a zero stack size
        stack_size = 0;
    }

    // -- because we're sending the entire origin callstack, we need to use a data packet instead of a remote SendExec()
    // -- calculate the size of the data
    int32 total_size = 0;

    // -- first int32 will be identifying this data packet
    total_size += sizeof(int32);

    // -- object ID
    total_size += sizeof(int32);

    // -- get the object name and length (plus EOL)
    const char* obj_name = oe->GetNameHash() != 0 ? oe->GetName() : "<unnamed>";
    int32 obj_name_length = (int32)strlen(obj_name) + 1;
    obj_name_length += 4 - (obj_name_length % 4);

    // -- object name length
    total_size += sizeof(int32);
    total_size += obj_name_length;

    // -- derivation string length
    // -- the next will be mName - we'll round up to a 4-byte aligned length (including the EOL)
    int32 derivation_length = (int32)strlen(derivation_buf) + 1;
    derivation_length += 4 - (derivation_length % 4);

    // -- we send first the string length, then the string
    total_size += sizeof(int32);
    total_size += derivation_length;

    // -- stack size
    total_size += sizeof(int32);

    // -- filename_array
    total_size += sizeof(int32) * stack_size;

    // -- linenumber_array
    total_size += sizeof(int32) * stack_size;

    // -- now create the packet
    // -- declare a header
    // -- note, if we ever implement a request/acknowledge approach, we can use the mID field
    SocketManager::tPacketHeader header(k_PacketVersion, SocketManager::tPacketHeader::DATA, total_size);

    // -- create the packet (null data, as we'll fill in the data directly into the packet)
    SocketManager::tDataPacket* newPacket = SocketManager::CreateDataPacket(&header,NULL);
    if (!newPacket)
    {
        TinPrint(this,"Error - DebuggerNotifyCreateObject():  unable to send - not connected\n");
        return;
    }

    // -- initialize the ptr to the data buffer
    int32* dataPtr = (int32*)newPacket->mData;

    // -- write the identifier - defined in the debugger constants near the top of TinScript.h
    *dataPtr++ = k_DebuggerObjectCreatedID;

    // -- object ID
    *dataPtr++ = oe->GetID();

    // -- name length
    *dataPtr++ = obj_name_length;

    // -- name string
    SafeStrcpy((char*)dataPtr, obj_name_length, obj_name, obj_name_length);
    dataPtr += (obj_name_length / 4);

    // -- derivation string length
    *dataPtr++ = derivation_length;

    // -- write the derivation string
    SafeStrcpy((char*)dataPtr, derivation_length, derivation_buf, derivation_length);
    dataPtr += (derivation_length / 4);

    // -- origin stack size
    *dataPtr++ = stack_size;

    // -- if we don't have memory tracking enabled, we'll have a zero stack_size
    if (stack_size > 0)
    {
        // -- file array
        memcpy(dataPtr, created_file_array, sizeof(uint32) * stack_size);
        dataPtr += stack_size;

        // -- line number array
        memcpy(dataPtr, created_lines_array, sizeof(int32) * stack_size);
        dataPtr += stack_size;
    }

    // -- send the packet
    SocketManager::SendDataPacket(newPacket);
}

// ====================================================================================================================
// DebuggerNotifyDestroyObject():  Send notification to the debugger of an object's destruction.
// ====================================================================================================================
void CScriptContext::DebuggerNotifyDestroyObject(uint32 object_id)
{
    // -- sanity check
    int32 debugger_session = 0;
    if (object_id == 0 || !IsDebuggerConnected(debugger_session))
        return;

    // -- send the entry
    SocketManager::SendCommandf("DebuggerNotifyDestroyObject(%d);", object_id);
}

// ====================================================================================================================
// DebuggerNotifySetAddObject():  Send notification to the debugger of an object's new membership.
// ====================================================================================================================
void CScriptContext::DebuggerNotifySetAddObject(uint32 parent_id, uint32 object_id, bool8 owned)
{
    // -- sanity check
    int32 debugger_session = 0;
    if (parent_id == 0 || object_id == 0 || !IsDebuggerConnected(debugger_session))
        return;

    // -- send the entry
    SocketManager::SendCommandf("DebuggerNotifySetAddObject(%d, %d, %s);", parent_id, object_id,
                                owned ? "true" : "false");
}

// ====================================================================================================================
// DebuggerNotifySetRemoveObject():  Send notification to the debugger of an object's discontinued membership.
// ====================================================================================================================
void CScriptContext::DebuggerNotifySetRemoveObject(uint32 parent_id, uint32 object_id)
{
    // -- sanity check
    int32 debugger_session = 0;
    if (parent_id == 0 || object_id == 0 || !IsDebuggerConnected(debugger_session))
        return;

    // -- send the entry
    SocketManager::SendCommandf("DebuggerNotifySetRemoveObject(%d, %d);", parent_id, object_id);
}

// ====================================================================================================================
// DebuggerListObjects():  Instead of printing the hierarchy of objects, this method send the entries to the debugger.
// ====================================================================================================================
void CScriptContext::DebuggerListObjects(uint32 parent_id, uint32 object_id)
{
    // -- ensure we have a debugger connected
	int32 debugger_session = 0;
    if (!IsDebuggerConnected(debugger_session))
        return;

    // -- see if we're supposed to list all objects
    if (object_id == 0)
    {
        CObjectEntry* oe = GetObjectDictionary()->First();
        while (oe)
        {
            // -- if we're listing all objects, then we only iterate through the
            if (oe->GetObjectGroup() == NULL)
            {
                DebuggerListObjects(0, oe->GetID());
            }

            // -- next object
            oe = GetObjectDictionary()->Next();
        }

        // -- notify the debugger we've completed sending the list of objects
        SocketManager::SendExec(Hash("DebuggerListObjectsComplete"));
    }

    // -- else we have a specific object to dump
    else
    {
        CObjectEntry* oe = FindObjectEntry(object_id);
        if (oe)
        {
            // -- if we found the object entry, send it to the debugger
            DebuggerNotifyCreateObject(oe);

            // -- if we have a parent_id, then notify the debugger of the membership
            if (parent_id != 0)
            {
                DebuggerNotifySetAddObject(parent_id, oe->GetID(), oe->GetGroupID() == parent_id);
            }

            // -- if the object is an object set, recursively send its children
            static uint32 object_set_hash = Hash("CObjectSet");
            if (oe->HasNamespace(object_set_hash))
            {
                CObjectSet* object_set = static_cast<CObjectSet*>(FindObject(oe->GetID()));
                if (object_set)
                {
                    uint32 child_id = object_set->First();
                    while (child_id != 0)
                    {
                        DebuggerListObjects(oe->GetID(), child_id);
                        child_id = object_set->Next();
                    }
                }
            }
        }
    }
}

// ====================================================================================================================
// DebuggerInspectObject():  Send the object members and methods to the debugger.
// ====================================================================================================================
void CScriptContext::DebuggerInspectObject(uint32 object_id)
{
    // -- ensure we have a debugger connected
	int32 debugger_session = 0;
    if (!IsDebuggerConnected(debugger_session) || object_id == 0)
        return;

    // -- see if we're supposed to list all objects
    CObjectEntry* oe = FindObjectEntry(object_id);
    if (oe)
    {
        // -- send a dump of the object to the debugger
        DebuggerSendObjectMembers(NULL, object_id);
    }
}

// ====================================================================================================================
// DebuggerListSchedules():  Send the connected debugger, a dump of the current pending schedules.
// ====================================================================================================================
void CScriptContext::DebuggerListSchedules()
{
    // -- ensure we have a debugger connected
	int32 debugger_session = 0;
    if (!IsDebuggerConnected(debugger_session))
        return;

    CScriptContext* script_context = TinScript::GetContext();
    script_context->GetScheduler()->DebuggerListSchedules();
}

// --------------------------------------------------------------------------------------------------------------------
static const int32 k_maxIdentifierStackSize = 8;
int32 ParseIdentifierStack(const char* input_str, char identifierStack[kMaxNameLength][k_maxIdentifierStackSize],
                           int32& last_token_offset)
{
    // -- sanity check
    if (input_str == nullptr || input_str[0] == '\0' || identifierStack == nullptr)
        return (0);

    // -- copy the string
    char input_buf[kMaxTokenLength];
    const char* input_buf_start = input_buf;
    strcpy_s(input_buf, input_str);
    int identifier_count = 0;
    last_token_offset = -1;

    // -- start at the end of the input string, and look for 
    char* input_ptr = &input_buf[strlen(input_str)];
    char* input_end = input_ptr;
    while (true)
    {
        // -- find the beginning of the current identifier
        while (input_ptr > input_buf && IsIdentifierChar(*(char*)(input_ptr - 1), true))
            --input_ptr;

        // -- at this point, input_ptr points to the start of the identifier, and input_end is the null terminator
        // -- if we have an empty identifier, or a string too long for an identifier, we're done
        if (input_ptr == input_end || (int32)strlen(input_ptr) >= kMaxNameLength)
            break;

        // -- copy the identifier to the stack
        strcpy_s(identifierStack[identifier_count++], kMaxNameLength, input_ptr);

        // -- set the last token offset
        if (last_token_offset < 0)
            last_token_offset = (int32)kPointerDiffUInt32(input_ptr, input_buf_start);

        // -- if our stack is full, or we've reached the start of our string, we're done
        if (identifier_count >= k_maxIdentifierStackSize || input_ptr == input_buf)
            break;

        // -- back up one char, so we can scan for the next (previous) identifier
        --input_ptr;

        // -- back up past white space
        while (input_ptr > input_buf && *input_ptr <= 0x20)
            --input_ptr;

        // -- feels like a hack, but check for the specific keywords 'create' or 'create_local'
        if (identifier_count == 1)
        {
            // -- note:  inputPtr will be pointing at the last character in the string, so the lenghts are off by 1
            int32 create_length = (int32)strlen("create") - 1;
            int32 createlocal_length = (int32)strlen("createlocal") - 1;
            if (kPointerDiffUInt32(input_ptr, input_buf) >= (uint32)create_length &&
                !strncmp(input_ptr - create_length, "create", create_length + 1))
            {
                strcpy_s(identifierStack[identifier_count++], kMaxNameLength, "create");
                break;
            }
            else if (kPointerDiffUInt32(input_ptr, input_buf) >= (uint32)createlocal_length &&
                     !strncmp(input_ptr - createlocal_length, "create_local", createlocal_length + 1))
            {
                strcpy_s(identifierStack[identifier_count++], kMaxNameLength, "create_local");
                break;
            }
        }

        // -- if we found anything other than an object dereference, we're done
        if (*input_ptr != '.')
            break;

        // -- if we did find a decimal, then we must have a preceeding identifier,
        // -- or we've got a non-tab-completable expression
        if (input_ptr == input_buf)
            return (0);

        // -- back up before the decimal
        --input_ptr;

        // -- back up past white space again
        while (input_ptr > input_buf && *input_ptr <= 0x20)
            --input_ptr;

        // -- if we didn't find an identifier char, we're still missing a preceeding identifier
        if (!IsIdentifierChar(*input_ptr, true))
            return (0);

        // -- set the input_end, and terminate
        input_end = input_ptr + 1;
        *input_end = '\0';
    }

    // -- return the number of identifiers we found
    return (identifier_count);
}

// --------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------
// struct tTabCompleteEntry:  Helper struct for tab completion, storing either a matching var name, or function name
// --------------------------------------------------------------------------------------------------------------------
struct tTabCompleteEntry
{
    const char* tab_string;
    CFunctionEntry* func_entry;
    CVariableEntry* var_entry;

    void Set(const char* name, CFunctionEntry* fe, CVariableEntry* ve)
    {
        tab_string = name;
        func_entry = fe;
        var_entry = ve;
    }
};

// --------------------------------------------------------------------------------------------------------------------
// TabCompleteFunctionTable():  Populate the list of tab completion entries with matching function names
// --------------------------------------------------------------------------------------------------------------------
bool8 TabCompleteFunctionTable(const char* partial_function_name, int32 partial_length,
                               tFuncTable& function_table, tTabCompleteEntry* tab_complete_list, int32& entry_count,
                               int32 max_count)
{
    // -- populate and send a function assist entry
    bool table_is_full = false;
    CFunctionEntry* function_entry = function_table.First();
    while (function_entry && !table_is_full)
    {
        const char* func_name = TinScript::UnHash(function_entry->GetHash());
        if (func_name != nullptr && !_strnicmp(partial_function_name, func_name, partial_length))
        {
            // -- ensure the function hasn't already been added
            // -- (derived registered classes all have their own version of ListMembers() and ListMethods())
            bool already_added = false;
            for (int i = 0; i < entry_count; ++i)
            {
                if (tab_complete_list[i].func_entry != nullptr &&
                    tab_complete_list[i].func_entry->GetHash() == function_entry->GetHash())
                {
                    already_added = true;
                    break;
                }
            }

            // -- add the function
            if (!already_added)
                tab_complete_list[entry_count++].Set(TinScript::UnHash(function_entry->GetHash()),
                                                     function_entry, nullptr);

            // -- break if we're full
            if (entry_count >= max_count)
            {
                table_is_full = true;
                break;
            }
        }

        // -- next function
        function_entry = function_table.Next();
    }

    // -- return true if the list is full
    return (table_is_full);
}

// --------------------------------------------------------------------------------------------------------------------
// TabCompleteFunctionTable():  Populate the list of tab completion entries with matching variable names
// --------------------------------------------------------------------------------------------------------------------
bool8 TabCompleteVarTable(const char* partial_function_name, int32 partial_length,
                          tVarTable& var_table, tTabCompleteEntry* tab_complete_list, int32& entry_count,
                          int32 max_count)
{
    // -- populate and send a function assist entry
    bool table_is_full = false;
    CVariableEntry* var_entry = var_table.First();
    while (var_entry && !table_is_full)
    {
        const char* func_name = TinScript::UnHash(var_entry->GetHash());
        if (func_name != nullptr && !_strnicmp(partial_function_name, func_name, partial_length))
        {
            // -- ensure the function hasn't already been added
            // -- (derived registered classes all have their own version of ListMembers() and ListMethods())
            bool already_added = false;
            for (int i = 0; i < entry_count; ++i)
            {
                if (tab_complete_list[i].var_entry != nullptr &&
                    tab_complete_list[i].var_entry->GetHash() == var_entry->GetHash())
                {
                    already_added = true;
                    break;
                }
            }

            // -- add the variable
            if (!already_added)
                tab_complete_list[entry_count++].Set(TinScript::UnHash(var_entry->GetHash()),
                                                      nullptr, var_entry);

            // -- break if we're full
            if (entry_count >= max_count)
            {
                table_is_full = true;
                break;
            }
        }

        // -- next function
        var_entry = var_table.Next();
    }

    // -- return true if the list is full
    return (table_is_full);
}

// --------------------------------------------------------------------------------------------------------------------
// TabCompleteKeywordCreate():  If the previous token was 'create', then we create based on available namespaces.
// --------------------------------------------------------------------------------------------------------------------
bool8 TabCompleteKeywordCreate(const char* partial_function_name, int32 partial_length,
                               tTabCompleteEntry* tab_complete_list, int32& entry_count,
                               int32 max_count)
{
    bool table_is_full = false;
    CHashTable<CNamespace>* namespaces = TinScript::GetContext()->GetNamespaceDictionary();
    if (namespaces != nullptr)
    {
        CNamespace* current_namespace = namespaces->First();
        while (current_namespace != nullptr)
        {
            const char* namespace_name = TinScript::UnHash(current_namespace->GetHash());
            if (namespace_name != nullptr && !_strnicmp(partial_function_name, namespace_name, partial_length))
            {
                // -- ensure the function hasn't already been added
                // -- (derived registered classes all have their own version of ListMembers() and ListMethods())
                bool already_added = false;
                for (int i = 0; i < entry_count; ++i)
                {
                    if (tab_complete_list[i].tab_string == namespace_name)
                    {
                        already_added = true;
                        break;
                    }
                }

                // -- add the variable
                if (!already_added)
                    tab_complete_list[entry_count++].Set(namespace_name, nullptr, nullptr);

                // -- break if we're full
                if (entry_count >= max_count)
                {
                    table_is_full = true;
                    break;
                }
            }

            // -- next namespace
            current_namespace = namespaces->Next();
        }
    }

    // -- return true if the table is full
    return (table_is_full);
}

// ====================================================================================================================
// TabComplete():  Return the next available command, given the partial input string
// ====================================================================================================================
bool CScriptContext::TabComplete(const char* partial_input, int32& ref_tab_complete_index,
                                 int32& out_name_offset, const char*& tab_result, CFunctionEntry*& fe,
                                 CVariableEntry*& ve)
{
    // -- sanity check
    if (partial_input == nullptr)
        return (false);

    // -- send the function entries through the entire hierarchy (or just the global if requested)
    tFuncTable* function_table = NULL;
    CNamespace* current_namespace = NULL;

    // -- set up the pointer to the start of the partial input
    out_name_offset = 0;
    const char* partial_function_ptr = partial_input;
    if (partial_function_ptr == nullptr || partial_function_ptr[0] == '\0')
        return (false);

    // -- parse the input, separating it into the chain of identifiers
    char identifier_stack[kMaxNameLength][k_maxIdentifierStackSize];
    int32 identifier_count = ParseIdentifierStack(partial_input, identifier_stack, out_name_offset);

    // -- if we weren't able to parse the string into identifiers, we're done
    if (identifier_count == 0)
        return (false);

    // -- set the pointers for the partial input
    partial_function_ptr = identifier_stack[0];
    int32 partial_length = (int32)strlen(partial_function_ptr);

    // -- ensure we have a non-empty partial string
    if (partial_function_ptr[0] == '\0')
        return (nullptr);

    // -- populate the list of function and member names, etc...
    const int32 max_count = 256;
    int32 entry_count = 0;
    tTabCompleteEntry tab_complete_list[max_count];
    bool8 list_is_full = false;

    // -- special case, if we're tab completing on "create"
    bool tabcomplete_handled = false;
    if (identifier_count == 2 && (!strcmp(identifier_stack[1], "create") ||
                                  !strcmp(identifier_stack[1], "create_local")))
    {
        tabcomplete_handled = true;
        list_is_full = TabCompleteKeywordCreate(partial_function_ptr, partial_length, tab_complete_list, entry_count,
                                                max_count);
    }

    // -- see if this is a global function or an method
    CObjectEntry* oe = nullptr;
    uint32 object_id = 0;

    // -- if we haven't handled the completion (e.g. through the keyword create completion)
    if (!tabcomplete_handled)
    {
        // -- if there were multiple identifiers, then we need to find the last object entry in the chain
        for (int stack_index = identifier_count - 1; stack_index >= 1; --stack_index)
        {
            // -- cache the prev_oe
            CObjectEntry* prev_oe = oe;
            oe = nullptr;

            // -- if this is the first identifier in the chain, it must be either an object ID, or a global variable
            if (stack_index == identifier_count - 1)
            {
                object_id = (uint32)atoi(identifier_stack[stack_index]);
                if (object_id > 0)
                    oe = TinScript::GetContext()->FindObjectEntry(object_id);
                else
                {
                    // -- find the variable
                    CVariableEntry* ve_0 = GetGlobalNamespace()->GetVarTable()->FindItem(Hash(identifier_stack[stack_index]));
                    if (ve_0 == nullptr || ve_0->GetType() != eVarType::TYPE_object)
                        return (false);

                    // -- get the variable value, and search for an object with that ID
                    object_id = *(uint32*)(ve_0->GetValueAddr(nullptr));
                    oe = TinScript::GetContext()->FindObjectEntry(object_id);
                }
            }

            // -- otherwise, it's an object member of the previous object
            else
            {
                CVariableEntry* oe_member = prev_oe->GetVariableEntry(Hash(identifier_stack[stack_index]));
                if (oe_member == nullptr || oe_member->GetType() != TYPE_object)
                    return (false);

                // -- find the variable for which this member refers
                object_id = *(uint32*)(oe_member->GetValueAddr(prev_oe->GetAddr()));
                oe = TinScript::GetContext()->FindObjectEntry(object_id);
            }

            // -- if oe is still null, we're done
            if (oe == nullptr)
                return (false);
        }

        // -- if we don't have an object, populate with keywords, global functions, and global var names
        if (oe == nullptr)
        {
            int32 keyword_count = 0;
            const char** keyword_list = GetReservedKeywords(keyword_count);
            for (int32 i = 0; i < keyword_count; ++i)
            {
                if (!_strnicmp(partial_function_ptr, keyword_list[i], partial_length))
                {
                    // -- add the function
                    tab_complete_list[entry_count++].Set(keyword_list[i], nullptr, nullptr);
                }
            }

            // -- also tab complete with global variables
            list_is_full = TabCompleteVarTable(partial_function_ptr, partial_length, *(GetGlobalNamespace()->GetVarTable()),
                tab_complete_list, entry_count, max_count);

            // -- populate the list with matching function names
            function_table = GetGlobalNamespace()->GetFuncTable();
            list_is_full = TabCompleteFunctionTable(partial_function_ptr, partial_length, *function_table,
                tab_complete_list, entry_count, max_count);
        }

        // -- else we have an object - populate with dynamic var names, and through the hierarchy
        // -- of namespaces, each derived member and method name
        else
        {
            tVarTable* dynamic_vars = oe->GetDynamicVarTable();
            if (dynamic_vars != nullptr)
            {
                list_is_full = TabCompleteVarTable(partial_function_ptr, partial_length, *dynamic_vars,
                    tab_complete_list, entry_count, max_count);
            }

            current_namespace = oe->GetNamespace();
            function_table = current_namespace->GetFuncTable();

            // -- lots of places to exit from
            while (function_table && !list_is_full)
            {
                // -- populate the list with matching function names
                list_is_full = TabCompleteFunctionTable(partial_function_ptr, partial_length, *function_table,
                    tab_complete_list, entry_count, max_count);

                // -- add the object members
                tVarTable* var_table = current_namespace->GetVarTable();
                if (var_table != nullptr)
                {
                    list_is_full = TabCompleteVarTable(partial_function_ptr, partial_length, *var_table,
                        tab_complete_list, entry_count, max_count);
                }

                // -- get the next namespace and function table
                current_namespace = current_namespace->GetNext();
                if (current_namespace)
                    function_table = current_namespace->GetFuncTable();
                else
                    function_table = NULL;
            }
        }
    }

    // -- if we didn't find anything, we're done
    if (entry_count == 0)
        return (false);

    // -- if the number of functions is > 1, sort the list
    if (entry_count > 1)
    {
        auto function_sort = [](const void* a, const void* b) -> int
        {
            tTabCompleteEntry* entry_a = (tTabCompleteEntry*)a;
            tTabCompleteEntry* entry_b = (tTabCompleteEntry*)b;
            int result = _stricmp(entry_a->tab_string, entry_b->tab_string);
            return (result);
        };

        qsort(tab_complete_list, entry_count, sizeof(tTabCompleteEntry), function_sort);
    }

    // -- update the index
    ref_tab_complete_index = (++ref_tab_complete_index) % entry_count;

    // -- return the next function entry
    tab_result = tab_complete_list[ref_tab_complete_index].tab_string;
    fe = tab_complete_list[ref_tab_complete_index].func_entry;
    ve = tab_complete_list[ref_tab_complete_index].var_entry;

    return (true);
}

// ====================================================================================================================
// AddThreadCommand():  This enqueues a command, to be process during the normal update
// ====================================================================================================================
bool8 CScriptContext::AddThreadCommand(const char* command)
{
    // -- sanity check
    if (!command || !command[0])
        return (true);

    // -- we need to wrap access to the command buffer in a thread mutex, to prevent simultaneous access
    bool8 success = true;
    mThreadLock.Lock();

    // -- ensure the thread buf pointer is initialized
    if (mThreadBufPtr == NULL)
    {
        mThreadBufPtr = mThreadExecBuffer;
        mThreadExecBuffer[0] = '\0';
    }

    // -- ensure we've got room
    uint32 cmdLength = (int32)strlen(command);
    uint32 lengthRemaining = kThreadExecBufferSize - kPointerDiffUInt32(mThreadBufPtr, mThreadExecBuffer);
    if (lengthRemaining < cmdLength)
    {
        // -- no need to assert - the socket will re-enqueue the command after the buffer has been processed
        //ScriptAssert_(this, 0, "<internal>", -1, "Error - AddThreadCommand():  buffer length exceeded.\n");
        success = false;
    }
    else
    {
        SafeStrcpy(mThreadBufPtr, lengthRemaining, command, lengthRemaining);
        mThreadBufPtr += cmdLength;
    }

    // -- unlock the thread
    mThreadLock.Unlock();

    return (success);
}

// ====================================================================================================================
// ProcessThreadCommands():  Called during the normal update, to process commands received from an different thread
// ====================================================================================================================
void CScriptContext::ProcessThreadCommands()
{
    // -- if there's nothing to process, we're done
    if (mThreadBufPtr == NULL && m_socketCommandList == nullptr)
        return;

    // -- we need to wrap access to the command buffer in a thread mutex, to prevent simultaneous access
    mThreadLock.Lock();

    // -- reset the bufPtr
    bool has_script_commands = (mThreadBufPtr != NULL);
    mThreadBufPtr = NULL;

    // -- because the act of executing the command could trigger a breakpoint, we need to unlock this thread
    // -- *before* executing the command, or the run/step command will never actually be received.
    // -- this means, we copy current buffer so we can begin filling it again from the socket thread
    char local_exec_buffer[kThreadExecBufferSize];
    if (has_script_commands)
    {
        int32 bytes_to_copy = (int32)strlen(mThreadExecBuffer) + 1;
        memcpy(local_exec_buffer, mThreadExecBuffer, bytes_to_copy);
    }

    // -- the current queue of socket commands (created directly) need to be inserted into the scheduler as well
    while (m_socketCommandList != nullptr)
    {
        CScheduler::CCommand* socket_command = m_socketCommandList;
        m_socketCommandList = socket_command->mNext;

        // -- if the dispatch time is 0, execute the socket_command immediately
        if (socket_command->mDispatchTime == 0)
        {
            ExecuteScheduledFunction(this, socket_command->mObjectID, 0, socket_command->mFuncHash,
                                     socket_command->mFuncContext);

            // -- delete the command
            // $$$TZA if the execution fails - since this is from a socket, not sure we want to assert...
            TinFree(socket_command);
        }
        else
        {
            GetScheduler()->InsertCommand(socket_command);
        }
    }

    // -- unlock the thread
    mThreadLock.Unlock();

    // -- execute the buffer
    if (has_script_commands)
    {
        // -- if we're at a debug break, we want to execute the command as a watch expression
        // -- this will allow us to, e.g., print members and local vars from the currently executing method
        bool handled = false;
        if (mDebuggerBreakFuncCallStack != nullptr)
        {
            CDebuggerWatchExpression watch_expression(-1, false, false, nullptr, local_exec_buffer, false);
            handled = InitWatchExpression(watch_expression, true, *mDebuggerBreakFuncCallStack, mDebuggerWatchStackOffset);
            if (handled)
            {
                handled = EvalWatchExpression(watch_expression, true, *mDebuggerBreakFuncCallStack,
                                             *mDebuggerBreakExecStack, mDebuggerWatchStackOffset);
            }
        }

        // -- if we haven't handled the command yet, execute it in the global scope
        if (!handled)
        {
            ExecCommand(local_exec_buffer);
        }
    }
}

// ====================================================================================================================
// BeginThreadExec():  This uses a mutex to lock the queued commands, and creates a CScheduler::CCommand.
// ====================================================================================================================
bool8 CScriptContext::BeginThreadExec(uint32 func_hash)
{
    // -- ensure we have a script context (that we're not shutting down), and the func_hash is valid
    if (GetGlobalNamespace()->GetFuncTable()->FindItem(func_hash) == nullptr)
    {
        TinPrint(this, "Error - CScriptContext::BeginThreadExec(): unable to find function hash: 0x%x\n"
                       "If remote called SocketExec(), remember it's SocketExec(hash('MyFunction'), args...);\n\n", func_hash);
        return (false);
    }

    // -- if we're already creating a thread command, assert
    if (m_socketCurrentCommand != nullptr)
    {
        ScriptAssert_(this, 0, "<internal>", -1, "Error - CScriptContext::BeginThreadExec(): socket exec command already being constructed for function hash: 0x%x\n", func_hash);
        return (false);
    }

    // -- we to wrap access to the linked list of commands in a thread mutex, to prevent the socket thread from
    // -- queuing, while the main thread is processing
    mThreadLock.Lock();

    m_socketCurrentCommand = GetScheduler()->RemoteScheduleCreate(func_hash);

    // -- so far, so good
    return (true);
}

// ====================================================================================================================
// AddThreadExecParam():  Add a parameter to the thread command, returns 'true' if the parameter type was correct.
// ====================================================================================================================
bool8 CScriptContext::AddThreadExecParam(eVarType param_type, void* value)
{
    // -- sanity check
    if (m_socketCurrentCommand == nullptr || param_type == TYPE_void || value == nullptr)
    {
        // -- note:  we return 'true', since we dont' want to plug up traffic with RemoteSignature() calls
        ScriptAssert_(this, 0, "<internal>", -1, "Error - unable to construct a socket command\n");
        return (true);
    }

    // -- convert the value to the required type
    int32 current_param = m_socketCurrentCommand->mFuncContext->GetParameterCount();
    CFunctionEntry* fe = GetGlobalNamespace()->GetFuncTable()->FindItem(m_socketCurrentCommand->mFuncHash);
    CVariableEntry* fe_param = current_param < fe->GetContext()->GetParameterCount()
                               ? fe->GetContext()->GetParameter(current_param)
                               : nullptr;
    if (fe_param == nullptr)
    {
        ScriptAssert_(this, 0, "<internal>", -1, "Error - invalid parameter for function: %s()\n",
                      UnHash(m_socketCurrentCommand->mFuncHash));
        return (true);
    }

    // -- see if the parameter type matches
    bool8 paramTypeMatches = (param_type == fe_param->GetType());

    // -- if the param_type is a string, then the value* is a pointer to the (soon to be destructed) received packet
    // -- we need to add the string to the string table, and use the hash value, like any other internal string value
    uint32 string_hash = 0;
    if (param_type == eVarType::TYPE_string)
    {
        // -- bump the ref count - we need to ensure the value doesn't disappear before the schedule is executed
        // -- note:  the string should be cleaned up *after* the schedule has been executed - double check!
        // -- note:  if the parameter is meant to be a string, then we increment the ref count - otherwise,
        // -- it'll be converted, and the string is no longer needed
        string_hash = Hash((const char*)value);
        GetStringTable()->AddString((const char*)value, -1, string_hash, paramTypeMatches);
        value = (void*)(&string_hash);
    }

    // -- convert the received data to the type required by the function
    void* convert_addr = TypeConvert(this, param_type, value, fe_param->GetType());
    if (convert_addr == nullptr)
    {
        ScriptAssert_(this, 0, "<internal>", -1, "Error - invalid parameter for function: %s()\n",
                      UnHash(m_socketCurrentCommand->mFuncHash));
        return (true);
    }

    // -- add the parameter to the schedule context
    // note:  we'll be executing this function on the MainThread, as it's via a remote SocketExec()
    char param_name[4] = "_px";
    param_name[2] = '0' + current_param;
    if (!m_socketCurrentCommand->mFuncContext->AddParameter(param_name, Hash(param_name),
                                                            fe_param->GetType(), 1, current_param, 0, true))
    {
        ScriptAssert_(this, 0, "<internal>", -1, "Error - invalid parameter for function: %s()\n",
                      UnHash(m_socketCurrentCommand->mFuncHash));
        return (true);
    }

    // -- set the parameter value (use the literal, so strings are actual strings, not hash values)
    CVariableEntry* ve = m_socketCurrentCommand->mFuncContext->GetParameter(current_param);
    ve->SetValue(nullptr, convert_addr, 0);

    // -- return if the parameter type matches
    return (paramTypeMatches);
}

// ====================================================================================================================
// QueueThreadExec():  All command parameters have been set - add the current command to the end of the list.
// ====================================================================================================================
void CScriptContext::QueueThreadExec()
{
    // -- if we're already creating a thread command, assert
    if (m_socketCurrentCommand == nullptr)
    {
        ScriptAssert_(this, 0, "<internal>", -1, "Error - socket exec command does not exist\n");
        return;
    }

    // -- add the m_socketCurrentCommand to the end of the list
    CScheduler::CCommand* prev_command = m_socketCommandList;
    while (prev_command != nullptr && prev_command->mNext != nullptr)
        prev_command = prev_command->mNext;

    // -- add the command to the end of the list
    m_socketCurrentCommand->mNext = nullptr;
    if (prev_command == nullptr)
        m_socketCommandList = m_socketCurrentCommand;
    else
        prev_command->mNext = m_socketCurrentCommand;

    // -- clear the current command
    m_socketCurrentCommand = nullptr;

    // -- unlock the thread
    mThreadLock.Unlock();
}

// -- Debugger Registration -------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------
// DebuggerSetConnected():  toggle whether an application connected through the SocketManager is a debugger
// --------------------------------------------------------------------------------------------------------------------
void DebuggerSetConnected(bool8 connected)
{
    // -- ensure we have a script context
    CScriptContext* script_context = GetContext();
    if (!script_context)
        return;

    // -- this must be threadsafe - only ProcessThreadCommands should ever lead to this function
    script_context->SetDebuggerConnected(connected);
}

// --------------------------------------------------------------------------------------------------------------------
// DebuggerAddBreakpoint():  Add a breakpoint for the given file/line
// --------------------------------------------------------------------------------------------------------------------
void DebuggerAddBreakpoint(const char* filename, int32 line_number, bool8 break_enabled, const char* condition,
                           const char* trace, bool8 trace_on_cond)
{
    // -- ensure we have a script context
    CScriptContext* script_context = GetContext();
    if (!script_context)
        return;

    // -- this must be threadsafe - only ProcessThreadCommands should ever lead to this function
    script_context->AddBreakpoint(filename, line_number, break_enabled, condition, trace, trace_on_cond);
}

// --------------------------------------------------------------------------------------------------------------------
// DebuggerRemoveBreakpoint():  Remove a breakpoint for the given file/line
// --------------------------------------------------------------------------------------------------------------------
void DebuggerRemoveBreakpoint(const char* filename, int32 line_number)
{
    // -- ensure we have a script context
    CScriptContext* script_context = GetContext();
    if (!script_context)
        return;

    // -- this must be threadsafe - only ProcessThreadCommands should ever lead to this function
    script_context->RemoveBreakpoint(filename, line_number);
}

// --------------------------------------------------------------------------------------------------------------------
// DebuggerRemoveAllBreakpoints():  Remove all breakpoint for the given file
// --------------------------------------------------------------------------------------------------------------------
void DebuggerRemoveAllBreakpoints(const char* filename)
{
    // -- ensure we have a script context
    CScriptContext* script_context = GetContext();
    if (!script_context)
        return;

    // -- this must be threadsafe - only ProcessThreadCommands should ever lead to this function
    script_context->RemoveAllBreakpoints(filename);
}

// --------------------------------------------------------------------------------------------------------------------
// DebuggerForceBreak():  Force the VM to break on the next executed statement.
// --------------------------------------------------------------------------------------------------------------------
void DebuggerForceBreak()
{
    // -- ensure we have a script context
    CScriptContext* script_context = GetContext();
    if (!script_context)
        return;

    // -- this must be threadsafe - only ProcessThreadCommands should ever lead to this function
    script_context->SetForceBreak(0);
}

// --------------------------------------------------------------------------------------------------------------------
// DebuggerBreakStep():  When execution is halted from hitting a breakpoint, step to the next statement
// --------------------------------------------------------------------------------------------------------------------
void DebuggerBreakStep(bool8 step_over, bool8 step_out)
{
    // -- ensure we have a script context
    CScriptContext* script_context = GetContext();
    if (!script_context)
        return;

    // -- this must be threadsafe - only ProcessThreadCommands should ever lead to this function
    script_context->SetBreakActionStep(true, step_over, step_out);
}

// --------------------------------------------------------------------------------------------------------------------
// DebuggerBreakRun():  When execution is halted from hitting a breakpoint, continue running
// --------------------------------------------------------------------------------------------------------------------
void DebuggerBreakRun()
{
    // -- ensure we have a script context
    CScriptContext* script_context = GetContext();
    if (!script_context)
        return;

    // -- this must be threadsafe - only ProcessThreadCommands should ever lead to this function
    script_context->SetBreakActionRun(true);
}

// --------------------------------------------------------------------------------------------------------------------
// DebuggerForceExecToLineNumber():  at a breakpoint, force the VM to resume execution at a *different* line number
// --------------------------------------------------------------------------------------------------------------------
void DebuggerForceExecToLineNumber(int32 line_number)
{
    // -- ensure we have a script context
    CScriptContext* script_context = GetContext();
    if (!script_context)
        return;

    // -- this must be threadsafe - only ProcessThreadCommands should ever lead to this function
    script_context->DebuggerForceExecToLineNumber(line_number);
}

// --------------------------------------------------------------------------------------------------------------------
// DebuggerSetWatchStackOffset():  Sets the selected callstack level for evaluating watch expressions (0 == top)
// --------------------------------------------------------------------------------------------------------------------
void DebuggerSetWatchStackOffset(int32 stack_offset)
{
    // -- ensure we have a script context
    CScriptContext* script_context = GetContext();
    if (!script_context)
        return;

    script_context->mDebuggerWatchStackOffset = stack_offset;
}

// --------------------------------------------------------------------------------------------------------------------
// DebuggerAddVariableWatch():  Add a variable watch - requires an ID, and an expression.
// --------------------------------------------------------------------------------------------------------------------
void DebuggerAddVariableWatch(int32 request_id, const char* variable_watch, bool breakOnWrite)
{
    // -- ensure we have a script context
    CScriptContext* script_context = GetContext();
    if (!script_context)
        return;

	// -- ensure we have a valid request_id and an expression
	if (request_id <= 0 || !variable_watch || !variable_watch[0])
		return;

    // -- this must be threadsafe - only ProcessThreadCommands should ever lead to this function
    script_context->AddVariableWatch(request_id, variable_watch, breakOnWrite, NULL);
}

// --------------------------------------------------------------------------------------------------------------------
// DebuggerModifyVariableWatch():  Modifies a variable, using the same mechanism as setting a variable watch.
// --------------------------------------------------------------------------------------------------------------------
void DebuggerModifyVariableWatch(int32 request_id, const char* variable_watch, const char* new_value)
{
    // -- ensure we have a script context
    CScriptContext* script_context = GetContext();
    if (!script_context)
        return;

	// -- ensure we have a valid request_id and an expression
	if (!variable_watch || !variable_watch[0])
		return;

    // -- this must be threadsafe - only ProcessThreadCommands should ever lead to this function
    script_context->AddVariableWatch(request_id, variable_watch, false, new_value);
}

// --------------------------------------------------------------------------------------------------------------------
// DebuggerToggleVarWatch():  Toggle whether we break on write for a given variable
// --------------------------------------------------------------------------------------------------------------------
void DebuggerToggleVarWatch(int32 watch_request_id, uint32 object_id, int32 var_name_hash, bool8 breakOnWrite,
                            const char* condition, const char* trace, bool8 trace_on_cond)
{
    // -- ensure we have a script context
    CScriptContext* script_context = GetContext();
    if (!script_context)
        return;

	// -- ensure we have a valid request_id and an expression
	if (watch_request_id <= 0)
		return;

    // -- this must be threadsafe - only ProcessThreadCommands should ever lead to this function
    script_context->ToggleVarWatch(watch_request_id, object_id, var_name_hash, breakOnWrite, condition, trace,
                                   trace_on_cond);
}

// --------------------------------------------------------------------------------------------------------------------
// DebuggerListObjects():  Send the connected debugger, a dump of all the object entries.
// --------------------------------------------------------------------------------------------------------------------
void DebuggerListObjects(int32 root_object_id)
{
    // -- ensure we have a script context
    CScriptContext* script_context = GetContext();
    if (!script_context)
        return;

    // -- ensure we're connected
    int32 debugger_session = 0;
    if (!script_context->IsDebuggerConnected(debugger_session))
        return;

    // -- send the list of objects
    script_context->DebuggerListObjects(0, root_object_id);
}

// --------------------------------------------------------------------------------------------------------------------
// DebuggerInspectObject():  Send the connected debugger, a dump of an object's members and methods.
// --------------------------------------------------------------------------------------------------------------------
void DebuggerInspectObject(int32 object_id)
{
    // -- ensure we have a script context
    CScriptContext* script_context = GetContext();
    if (!script_context)
        return;

    // -- ensure we're connected
    int32 debugger_session = 0;
    if (!script_context->IsDebuggerConnected(debugger_session))
        return;

    // -- send the list of objects
    script_context->DebuggerInspectObject(object_id);
}

// --------------------------------------------------------------------------------------------------------------------
// DebuggerListSchedules():  Send the connected debugger, a dump of the current pending schedules.
// --------------------------------------------------------------------------------------------------------------------
void DebuggerListSchedules()
{
    // -- ensure we have a script context
    CScriptContext* script_context = GetContext();
    if (!script_context)
        return;

    // -- ensure we're connected
    int32 debugger_session = 0;
    if (!script_context->IsDebuggerConnected(debugger_session))
        return;

    // -- send the list of objects
    script_context->DebuggerListSchedules();
}

// --------------------------------------------------------------------------------------------------------------------
// DebuggerRequestFunctionAssist():  Send the connected debugger, a list of all functions for the given object
// --------------------------------------------------------------------------------------------------------------------
void DebuggerRequestFunctionAssist(int32 object_id)
{
    // -- ensure we have a script context
    CScriptContext* script_context = GetContext();
    if (!script_context)
        return;

    // -- ensure we're connected
    int32 debugger_session = 0;
    if (!script_context->IsDebuggerConnected(debugger_session))
        return;

    // -- send the list of objects
    script_context->DebuggerRequestFunctionAssist(object_id);

    // -- notify the debugger we've completed the function assist request
    SocketManager::SendExec(Hash("DebuggerFunctionAssistComplete"));
}

// --------------------------------------------------------------------------------------------------------------------
// DebuggerRequestNamespaceAssist():  Send the connected debugger, a list of all functions for the namespace
// --------------------------------------------------------------------------------------------------------------------
void DebuggerRequestNamespaceAssist(int32 ns_hash)
{
    // -- ensure we have a script context
    CScriptContext* script_context = GetContext();
    if (!script_context)
        return;

    // -- ensure we're connected
    int32 debugger_session = 0;
    if (!script_context->IsDebuggerConnected(debugger_session))
        return;

    // -- send the list of objects
    script_context->DebuggerRequestNamespaceAssist(ns_hash);

    // -- notify the debugger we've completed the function assist request
    SocketManager::SendExec(Hash("DebuggerFunctionAssistComplete"));
}

// --------------------------------------------------------------------------------------------------------------------
// DebuggerRequestTabComplete():  Receive a partial input string, and send back a tab completed one
// --------------------------------------------------------------------------------------------------------------------
void DebuggerRequestTabComplete(int32 request_id, const char* partial_input, int32 tab_complete_index)
{
    // -- ensure we have a script context
    CScriptContext* script_context = GetContext();
    if (!script_context)
        return;

    // -- ensure we're connected
    int32 debugger_session = 0;
    if (!script_context->IsDebuggerConnected(debugger_session))
        return;

    // -- methods for tab completion
    int32 tab_string_offset = 0;
    const char* tab_result = nullptr;
    CFunctionEntry* fe = nullptr;
    CVariableEntry* ve = nullptr;
    if (script_context->TabComplete(partial_input, tab_complete_index, tab_string_offset, tab_result, fe, ve))
    {
        // -- build the function prototype string
        char prototype_string[TinScript::kMaxTokenLength];

        // -- see if we are to preserve the preceeding part of the tab completion buf
        if (tab_string_offset > 0)
            strncpy_s(prototype_string, partial_input, tab_string_offset);

        // -- eventually, the tab completion will fill in the prototype arg types...
        if (fe != nullptr)
        {
            // -- if we have parameters (more than 1, since the first parameter is always the return value)
            if (fe->GetContext()->GetParameterCount() > 1)
                sprintf_s(&prototype_string[tab_string_offset], TinScript::kMaxTokenLength - tab_string_offset, "%s(", tab_result);
            else
                sprintf_s(&prototype_string[tab_string_offset], TinScript::kMaxTokenLength - tab_string_offset, "%s()", tab_result);
        }
        else
            sprintf_s(&prototype_string[tab_string_offset], TinScript::kMaxTokenLength - tab_string_offset, "%s", tab_result);

        // -- send the tab completed string back to the debugger
        SocketManager::SendCommandf("DebuggerNotifyTabComplete(%d, `%s`, %d);", request_id, prototype_string, tab_complete_index);
    }
}

// --------------------------------------------------------------------------------------------------------------------
// DebuggerRequestStringUnhash():  Query the connected target for the actual string value from a hash
// --------------------------------------------------------------------------------------------------------------------
void DebuggerRequestStringUnhash(uint32 string_hash)
{
    // -- sanity check
    if (TinScript::GetContext() == nullptr)
        return;

    //-- ensure we find a result
    const char* string_value = TinScript::GetContext()->GetStringTable()->FindString(string_hash);
    if (string_value == nullptr || string_value[0] == '\0')
        return;

    // -- notify the debugger of the result
    char hash_as_string[16];
    sprintf_s(hash_as_string, "%d", string_hash);
    SocketManager::SendExec(Hash("DebuggerNotifyStringUnhash"), hash_as_string, string_value);
}

// -------------------------------------------------------------------------------------------------------------------
// -- Registration
REGISTER_FUNCTION(DebuggerSetConnected, DebuggerSetConnected);
REGISTER_FUNCTION(DebuggerAddBreakpoint, DebuggerAddBreakpoint);
REGISTER_FUNCTION(DebuggerRemoveBreakpoint, DebuggerRemoveBreakpoint);
REGISTER_FUNCTION(DebuggerRemoveAllBreakpoints, DebuggerRemoveAllBreakpoints);

REGISTER_FUNCTION(DebuggerForceBreak, DebuggerForceBreak);
REGISTER_FUNCTION(DebuggerBreakStep, DebuggerBreakStep);
REGISTER_FUNCTION(DebuggerBreakRun, DebuggerBreakRun);

REGISTER_FUNCTION(DebuggerForceExecToLineNumber, DebuggerForceExecToLineNumber);

REGISTER_FUNCTION(DebuggerAddVariableWatch, DebuggerAddVariableWatch);
REGISTER_FUNCTION(DebuggerToggleVarWatch, DebuggerToggleVarWatch);
REGISTER_FUNCTION(DebuggerModifyVariableWatch, DebuggerModifyVariableWatch);
REGISTER_FUNCTION(DebuggerSetWatchStackOffset, DebuggerSetWatchStackOffset);

REGISTER_FUNCTION(DebuggerRequestStringUnhash, DebuggerRequestStringUnhash);

REGISTER_FUNCTION(DebuggerListObjects, DebuggerListObjects);
REGISTER_FUNCTION(DebuggerInspectObject, DebuggerInspectObject);

REGISTER_FUNCTION(DebuggerListSchedules, DebuggerListSchedules);
REGISTER_FUNCTION(DebuggerRequestFunctionAssist, DebuggerRequestFunctionAssist);
REGISTER_FUNCTION(DebuggerRequestNamespaceAssist, DebuggerRequestNamespaceAssist);

REGISTER_FUNCTION(DebuggerRequestTabComplete, DebuggerRequestTabComplete);

// == class CThreadMutex ==============================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CThreadMutex::CThreadMutex()
    : mIsLocked(false)
{
    mThreadMutex = new std::recursive_mutex;
}

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CThreadMutex::~CThreadMutex()
{
    // -- for now, don't destroy the mutex, as it could still be busy...  
    // if the context is *properly* destroyed, we should be able to explicitly ensure the thread mutex is not in use
    mThreadMutex = nullptr;
}

// ====================================================================================================================
// Lock():  Lock access to the following structure/code from any other thread, until Unlocked()
// ====================================================================================================================
void CThreadMutex::Lock()
{
    // -- do nothing, if we're in the middle of shutting down
    if (mThreadMutex == nullptr)
        return;

    mIsLocked = true;
    //printf("[0x%x] Thread locked: %s\n", (unsigned int)this, mIsLocked ? "true" : "false");
    mThreadMutex->lock();
}

// ====================================================================================================================
// Unlock():  Restore access to the previous structure/code, to any other thread
// ====================================================================================================================
void CThreadMutex::Unlock()
{
    // -- do nothing, if we're in the middle of shutting down
    if (mThreadMutex == nullptr)
        return;

    mThreadMutex->unlock();
    mIsLocked = false;
    //printf("[0x%x] Thread locked: %s\n", (unsigned int)this, mIsLocked ? "true" : "false");
}

// == class CDebuggerWatchExpression ==================================================================================

// --------------------------------------------------------------------------------------------------------------------
// -- statics
int CDebuggerWatchExpression::gWatchExpressionID = 1;

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CDebuggerWatchExpression::CDebuggerWatchExpression(int32 line_number, bool8 isConditional, bool8 break_enabled,
                                                   const char* condition, const char* trace, bool8 trace_on_condition)
{
    mLineNumber = line_number;
    mIsEnabled = break_enabled;
    mIsConditional = isConditional;
    SafeStrcpy(mConditional, sizeof(mConditional), condition, kMaxNameLength);
    SafeStrcpy(mTrace, sizeof(mTrace), trace, kMaxNameLength);
    mTraceOnCondition = trace_on_condition;
    mWatchFunctionEntry = NULL;
    mTraceFunctionEntry = NULL;
}

// ====================================================================================================================
// Destructor
// ====================================================================================================================
CDebuggerWatchExpression::~CDebuggerWatchExpression()
{
    // -- if we had been initialized, we need to destroy the function entry and codeblock
    // -- which will happen, by clearing the expression
    // -- during shutdown, the Global Namespace is the first to be destroyed, and it contains the watch/conditional
    // function definitions...
    CScriptContext* script_context = TinScript::GetContext();
    CNamespace* global_ns = script_context != nullptr ? TinScript::GetContext()->GetGlobalNamespace() : nullptr;
    if (global_ns != nullptr)
	{
		if (mWatchFunctionEntry)
		{
			CCodeBlock* codeblock = mWatchFunctionEntry->GetCodeBlock();
            if (codeblock != nullptr)
                codeblock->RemoveFunction(mWatchFunctionEntry);
            global_ns->GetFuncTable()->RemoveItem(mWatchFunctionEntry->GetHash());
            TinFree(mWatchFunctionEntry);
        }

		if (mTraceFunctionEntry)
		{
			CCodeBlock* codeblock = mTraceFunctionEntry->GetCodeBlock();
			if (codeblock != nullptr)
				codeblock->RemoveFunction(mTraceFunctionEntry);
            global_ns->GetFuncTable()->RemoveItem(mTraceFunctionEntry->GetHash());
            TinFree(mTraceFunctionEntry);
        }
	}
}

// ====================================================================================================================
// SetExpression():  Given a watch structure, return true if we actually have something to evaluate.
// ====================================================================================================================
void CDebuggerWatchExpression::SetAttributes(bool break_enabled, const char* new_conditional, const char* new_trace,
                                             bool trace_on_condition)
{
    // -- no empty strings
    if (!new_conditional)
        new_conditional = "";
    if (!new_trace)
        new_trace = "";

    // -- set the bools
    mIsEnabled = break_enabled;
    mTraceOnCondition = trace_on_condition;

    // -- if the conditional has changed, and the previous had been compiled, we need to delete it
    if (strcmp(mConditional, new_conditional) != 0)
    {
        if (mWatchFunctionEntry)
        {
            // -- to delete a function, remove it from it's namespace, and then deleting it will automatically
            // -- remove it from whatever codeblock owned it...
            CCodeBlock* codeblock = mWatchFunctionEntry->GetCodeBlock();
			if (codeblock != nullptr)
				codeblock->RemoveFunction(mWatchFunctionEntry);
			TinScript::GetContext()->GetGlobalNamespace()->GetFuncTable()->RemoveItem(mWatchFunctionEntry->GetHash());
            TinFree(mWatchFunctionEntry);
            mWatchFunctionEntry = NULL;
        }

        // -- the first time this is needed, it'll be evaluated
        SafeStrcpy(mConditional, sizeof(mConditional), new_conditional, kMaxNameLength);
    }

    // -- same for the trace
    if (strcmp(mTrace, new_trace) != 0)
    {
        if (mTraceFunctionEntry)
        {
            CCodeBlock* codeblock = mTraceFunctionEntry->GetCodeBlock();
            if (codeblock != nullptr)
                codeblock->RemoveFunction(mTraceFunctionEntry);
			TinScript::GetContext()->GetGlobalNamespace()->GetFuncTable()->RemoveItem(mTraceFunctionEntry->GetHash());
            TinFree(mTraceFunctionEntry);
            mTraceFunctionEntry = NULL;
        }

        // -- if we're currently at the breakpoint that we're adding a trace expression, we (essentially)
        // want that trace expression to evaluate now (not at the next time we hit the line)
        if (new_trace[0] != '\0')
        {
            mTraceIsUpdated = true;
        }

        // -- the first time this is needed, it'll be evaluated
        SafeStrcpy(mTrace, sizeof(mTrace), new_trace, kMaxNameLength);
    }
}

} // TinScript

REGISTER_SCRIPT_CLASS_BEGIN(CScriptObject, VOID)
REGISTER_SCRIPT_CLASS_END()

// ====================================================================================================================
// EOF
// ====================================================================================================================
