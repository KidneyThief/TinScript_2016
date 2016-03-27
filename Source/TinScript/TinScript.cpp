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

// -- includes
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#ifdef WIN32
    #include "windows.h"
    #include "conio.h"
    #include "direct.h"
#endif

#include "integration.h"

#include "TinHash.h"
#include "TinParse.h"
#include "TinCompile.h"
#include "TinExecute.h"
#include "TinNamespace.h"
#include "TinScheduler.h"
#include "TinObjectGroup.h"
#include "TinStringTable.h"
#include "TinRegistration.h"
#include "TinOpExecFunctions.h"

#include "socket.h"

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

const char* CScriptContext::kGlobalNamespace = "_global";
uint32 CScriptContext::kGlobalNamespaceHash = Hash(CScriptContext::kGlobalNamespace);

// -- this is a *thread* variable, each thread can reference a separate context
_declspec(thread) CScriptContext* gThreadContext = NULL;

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
    CScriptContext* script_context = GetContext();
    assert(script_context != NULL);
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
// ExecScript():  Executes a text file containing script code
// ====================================================================================================================
bool8 ExecScript(const char* filename)
{
    CScriptContext* script_context = GetContext();
    assert(script_context != NULL);
    return (script_context->ExecScript(filename, true, true));
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

REGISTER_FUNCTION_P1(Compile, CompileScript, bool8, const char*);
REGISTER_FUNCTION_P1(Exec, ExecScript, bool8, const char*);
REGISTER_FUNCTION_P1(Include, IncludeScript, bool8, const char*);

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
int NullPrintHandler(const char*, ...)
{
    return (0);
}

// == CScriptContext ==================================================================================================

// ====================================================================================================================
// ResetAssertStack():  Allows the next assert to trace it's own (error) path
// ====================================================================================================================
void CScriptContext::ResetAssertStack()
{
    mAssertEnableTrace = false;
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
    if (gThreadContext)
    {
		// -- we clear the thread context first, so destructors can tell we're
		// -- shutting down
		CScriptContext* currentContext = gThreadContext;
		gThreadContext = NULL;
		TinFree(currentContext);
    }
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

    // -- set the thread local singleton
    gThreadContext = this;

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
    mAssertEnableTrace = false;

    // -- initialize the namespaces dictionary, and all object dictionaries
    InitializeDictionaries();

    // -- create the global namespace for this context
    mGlobalNamespace = FindOrCreateNamespace(NULL, true);

    // -- register functions, each to their namespace
    CRegFunctionBase* regfunc = CRegFunctionBase::gRegistrationList;
    while (regfunc != NULL)
    {
        regfunc->Register(this);
        regfunc = regfunc->GetNext();
    }

    // -- register globals
    CRegisterGlobal::RegisterGlobals(this);

    // -- initialize the scheduler
    mScheduler = TinAlloc(ALLOC_SchedCmd, CScheduler, this);

    // -- initialize the master object list
    mMasterMembershipList = TinAlloc(ALLOC_ObjectGroup, CMasterMembershipList, this, kMasterMembershipTableSize);

    // -- initialize the code block hash table
    mCodeBlockList = TinAlloc(ALLOC_HashTable, CHashTable<CCodeBlock>, kGlobalFuncTableSize);

    // -- initialize the scratch buffer index
    mScratchBufferIndex = 0;

    // -- debugger members
	mDebuggerSessionNumber = 0;
    mDebuggerConnected = false;
    mDebuggerActionForceBreak = false;
    mDebuggerActionStep = false;
    mDebuggerActionStepOver = false;
    mDebuggerActionStepOut = false;
    mDebuggerActionRun = true;

	mDebuggerBreakLoopGuard = false;
	mDebuggerBreakFuncCallStack = NULL;
	mDebuggerBreakExecStack = NULL;
	mDebuggerVarWatchRequestID = 0;

    // -- initialize the thread command
    mThreadBufPtr = NULL;
}

// ====================================================================================================================
// InitializeDictionaries():  Create the dictionaries, namespace, object, etc... and perform the startup registration.
// ====================================================================================================================
void CScriptContext::InitializeDictionaries()
{
    // -- allocate the dictinary to store creation functions
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
                    LinkNamespaces(newnamespace, parentnamespace);
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
    // -- cleanup the namespace context
    // -- note:  the global namespace is owned by the namespace dictionary
    // -- within the context - it'll be automatically cleaned up
    ShutdownDictionaries();

    // -- cleanup all related codeblocks
    // -- by deleting the namespace dictionaries, all codeblocks should now be unused
    CCodeBlock::DestroyUnusedCodeBlocks(mCodeBlockList);
    assert(mCodeBlockList->IsEmpty());
    TinFree(mCodeBlockList);

    // -- clean up the scheduleer
    TinFree(mScheduler);

    // -- cleanup the membership list
    TinFree(mMasterMembershipList);

    // -- clean up the string table
    TinFree(mStringTable);

    // -- if this is the MainThread context, shutdown types
    if (mIsMainThread)
    {
        ShutdownTypes();
    }
}

void CScriptContext::ShutdownDictionaries()
{
    // -- delete the Namespace dictionary
    if (mNamespaceDictionary)
    {
        mNamespaceDictionary->DestroyAll();
        TinFree(mNamespaceDictionary);
    }

    // -- delete the Object dictionaries
    if (mObjectDictionary)
    {
        mObjectDictionary->DestroyAll();
        TinFree(mObjectDictionary);
    }

    // -- objects will have been destroyed above, so simply clear this hash table
    if (mAddressDictionary)
    {
        mAddressDictionary->RemoveAll();
        TinFree(mAddressDictionary);
    }

    if (mNameDictionary)
    {
        mNameDictionary->RemoveAll();
        TinFree(mNameDictionary);
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
// SaveStringTable():  Write the string table to a file.
// ====================================================================================================================
void SaveStringTable(const char* filename)
{
    // -- ensure we have a valid filename
    if (!filename || !filename[0])
        filename = GetStringTableName();

    // -- get the context for this thread
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context)
        return;

    const CHashTable<CStringTable::tStringEntry>* string_table =
        script_context->GetStringTable()->GetStringDictionary();

    if (!string_table)
        return;

  	// -- open the file
	FILE* filehandle = NULL;
	int32 result = fopen_s(&filehandle, gStringTableFileName, "wb");
	if (result != 0)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - unable to write file %s\n", filename);
		return;
    }

	if (!filehandle)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - unable to write file %s\n", filename);
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
            ScriptAssert_(script_context, 0, "<internal>", -1, "Error - unable to write file %s\n", filename);
            return;
        }

        // -- write the string length
        sprintf_s(tempbuf, kMaxTokenLength, "%04d: ", length);
        count = (int32)fwrite(tempbuf, sizeof(char), 6, filehandle);
        if (count != 6)
        {
            fclose(filehandle);
            ScriptAssert_(script_context, 0, "<internal>", -1, "Error - unable to write file %s\n", filename);
            return;
        }

        // -- write the string
        count = (int32)fwrite(string, sizeof(char), length, filehandle);
        if (count != length)
        {
            fclose(filehandle);
            ScriptAssert_(script_context, 0, "<internal>", -1, "Error - unable to write file %s\n", filename);
            return;
        }

        // -- write the eol
        count = (int32)fwrite("\r\n", sizeof(char), 2, filehandle);
        if (count != 2)
        {
            fclose(filehandle);
            ScriptAssert_(script_context, 0, "<internal>", -1, "Error - unable to write file %s\n", filename);
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
void LoadStringTable(const char* filename)
{
    // -- ensure we have a valid filename
    if (!filename || !filename[0])
        filename = GetStringTableName();

    // -- get the context for this thread
    CScriptContext* script_context = TinScript::GetContext();
    if (!script_context)
        return;

  	// -- open the file
	FILE* filehandle = NULL;
	int32 result = fopen_s(&filehandle, filename, "rb");
	if (result != 0)
		return;

	if (!filehandle)
    {
        ScriptAssert_(script_context, 0, "<internal>", -1, "Error - unable to read file %s\n", filename);
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
            ScriptAssert_(script_context, 0, "<internal>", -1, "Error - unable to read file: %s\n", filename);
            return;
        }
        tempbuf[count] = '\0';
        sscanf_s(tempbuf, "%04d: ", &length);

        // -- read the string
        count = (int32)fread(string, sizeof(char), length, filehandle);
        if (ferror(filehandle) || count != length)
        {
            fclose(filehandle);
            ScriptAssert_(script_context, 0, "<internal>", -1, "Error - unable to read file: %s\n", filename);
            return;
        }
        string[length] = '\0';

        // -- read the eol
        count = (int32)fread(tempbuf, sizeof(char), 2, filehandle);
        if (ferror(filehandle) || count != 2)
        {
            fclose(filehandle);
            ScriptAssert_(script_context, 0, "<internal>", -1, "Error - unable to read file: %s\n", filename);
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
bool8 GetLastWriteTime(const char* filename, FILETIME& writetime)
{
    if (!filename || !filename[0])
        return (false);

    // -- convert the filename to a wchar_t array
	/*
    int32 length = (int32)strlen(filename);
    wchar_t wfilename[kMaxNameLength];
    for(int32 i = 0; i < length + 1; ++i)
        wfilename[i] = filename[i];
	*/

	HANDLE hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

    // Retrieve the file times for the file.
    FILETIME ftCreate, ftAccess;
    bool success = true;
    if (hFile == INVALID_HANDLE_VALUE ||  !GetFileTime(hFile, &ftCreate, &ftAccess, &writetime))
    {
        success = false;
    }

    // -- close the file
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    return (success);
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

    // -- copy the root name
    uint32 length = kPointerDiffUInt32(extptr, filename);
    SafeStrcpy(binfilename, filename, maxnamelength);
    SafeStrcpy(&binfilename[length], ".tso", maxnamelength - length);

    return true;
}

// ====================================================================================================================
// NeedToCompile():  Returns 'true' if the source file needs to be compiled.
// ====================================================================================================================
bool8 NeedToCompile(const char* filename, const char* binfilename)
{
    // -- get the filetime for the original script
    // -- if fail, then we have nothing to compile
    FILETIME scriptft;
    if (!GetLastWriteTime(filename, scriptft))
        return (false);

    // -- get the filetime for the binary file
    // -- if fail, we need to compile
    FILETIME binft;
    if (!GetLastWriteTime(binfilename, binft))
        return true;

    // -- if the binft is more recent, then we don't need to compile
    if (CompareFileTime(&binft, &scriptft) < 0)
        return true;
    else
    {

    // -- if we don't need to compile, then if we're forcing compilation anyways,
    // -- we only force it on files that aren't already loaded
#if FORCE_COMPILE
        uint32 filename_hash = Hash(filename, -1, false);
        CCodeBlock* already_executed = GetContext()->GetCodeBlockList()->FindItem(filename_hash);
        return (!already_executed);
#endif
        return false;
    }
}

// ====================================================================================================================
// CompileScript():  Compile a source script.
// ====================================================================================================================
CCodeBlock* CScriptContext::CompileScript(const char* filename)
{
    // -- get the name of the output binary file
    char binfilename[kMaxNameLength];
    if (!GetBinaryFileName(filename, binfilename, kMaxNameLength))
    {
        ScriptAssert_(this, 0, "<internal>", -1, "Error - invalid script filename: %s\n", filename ? filename : "");
        return NULL;
    }

    // -- compile the source
    CCodeBlock* codeblock = ParseFile(this, filename);
    if (codeblock == NULL)
    {
        ScriptAssert_(this, 0, "<internal>", -1, "Error - unable to parse file: %s\n", filename);
        return NULL;
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
// ExecScript():  Execute a script, compiles if necessary.
// ====================================================================================================================
bool8 CScriptContext::ExecScript(const char* filename, bool8 must_exist, bool8 re_exec)
{
    char binfilename[kMaxNameLength];
    if (!GetBinaryFileName(filename, binfilename, kMaxNameLength))
    {
        if (must_exist)
        {
            ScriptAssert_(this, 0, "<internal>", -1, "Error - invalid script filename: %s\n",
                          filename ? filename : "");
            ResetAssertStack();
        }
        return false;
    }

    CCodeBlock* codeblock = NULL;

    bool8 needtocompile = NeedToCompile(filename, binfilename);
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
            uint32 filename_hash = Hash(filename, -1, false);
            CCodeBlock* already_executed = GetCodeBlockList()->FindItem(filename_hash);
            if (already_executed)
            {
                return (true);
            }
        }

        bool8 old_version = false;
        codeblock = LoadBinary(this, filename, binfilename, must_exist, old_version);

        // -- if we have an old version, recompile
        if (!codeblock && old_version)
        {
            codeblock = CompileScript(filename);
        }
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
        bool error = false;
        char* cwdBuffer = _getcwd(NULL, 0);
        if (cwdBuffer == NULL)
        {
            error = true;
            cwdBuffer = ".";
        }

        // -- send the command
        DebuggerCurrentWorkingDir(cwdBuffer);

        // -- if we successfully got the current working directory, we need to free the buffer
        if (!error)
            delete [] cwdBuffer;

        // -- now notify the debugger of all the codeblocks loaded
        CCodeBlock* code_block = GetCodeBlockList()->First();
        while (code_block)
        {
            if (code_block->GetFilenameHash() != Hash("<stdin>"))
                DebuggerCodeblockLoaded(code_block->GetFilenameHash());
            code_block = GetCodeBlockList()->Next();
        }
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
void CScriptContext::AddBreakpoint(const char* filename, int32 line_number, bool8 break_enabled,
                                   const char* conditional, const char* trace, bool8 trace_on_condition)
{
    // -- sanity check
    if (!filename || !filename[0])
        return;

    // -- find the code block within the thread
    uint32 filename_hash = Hash(filename);
    CCodeBlock* code_block = GetCodeBlockList()->FindItem(filename_hash);
    if (! code_block)
        return;

    // -- add the breakpoint
    int32 actual_line = code_block->AddBreakpoint(line_number, break_enabled, conditional, trace, trace_on_condition);

    // -- if the actual breakable line doesn't match the request, notify the debugger
    if (actual_line != line_number)
    {
        DebuggerBreakpointConfirm(filename_hash, line_number, actual_line);
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

    // -- find the code block within the thread
    uint32 filename_hash = Hash(filename);
    CCodeBlock* code_block = GetCodeBlockList()->FindItem(filename_hash);
    if (! code_block)
        return;

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
    // -- this is usually set to false when a breakpoint is hit, and then remotely set to true by the debugger
    mDebuggerActionForceBreak = false;
    mDebuggerActionStep = torf;
    mDebuggerActionStepOver = torf ? step_over : false;
    mDebuggerActionStepOut = torf ? step_out : false;

	// -- clear the var watch requst ID - it'll be set on the next write if necessary
	mDebuggerVarWatchRequestID = 0;
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
    watch_entry.mStackLevel = -1;

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
	SafeStrcpy(watch_entry.mVarName, UnHash(ve.GetHash()), kMaxNameLength);
	gRegisteredTypeToString[ve.GetType()](value_addr, watch_entry.mValue, kMaxNameLength);

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
		if (DebuggerFindStackTopVar(this, var_hash, found_variable, ve))
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
            found_variable.mStackLevel = -1;

			// -- fill in the watch entry
			found_variable.mFuncNamespaceHash = 0;
			found_variable.mFunctionHash = 0;
			found_variable.mFunctionObjectID = 0;
			found_variable.mObjectID = 0;
			found_variable.mNamespaceHash = 0;

			// -- type, name, and value string, and array size
			found_variable.mType = TYPE_object;
			SafeStrcpy(found_variable.mVarName, UnHash(oe->GetNameHash()), kMaxNameLength);
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
			bool8 found_token = GetToken(next_token);
			if (found_token && next_token.type == TOKEN_PERIOD && oe != NULL)
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
			else if (found_token)
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
                found_variable.mStackLevel = -1;

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
                    found_variable.mStackLevel = -1;

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

    // -- we require a callstack to evaluate a variable watch as a complete expression
	if (!mDebuggerBreakFuncCallStack || !mDebuggerBreakExecStack)
		return;

    // -- watch expressions can handle a the complete syntax, but are unable to set a break on a variable entry
    CDebuggerWatchExpression watch_expression(false, false, variable_watch, NULL, false);
    bool result = InitWatchExpression(watch_expression, false, *mDebuggerBreakFuncCallStack);

    // -- if we were successful initializing the expression (e.g. a codeblock and function were created
    if (result)
    {
        // -- if evaluating the watch was successful
        result = EvalWatchExpression(watch_expression, false, *mDebuggerBreakFuncCallStack, *mDebuggerBreakExecStack);
        if (result)
        {
            // -- if we're able to retrieve the return value
            eVarType returnType = TYPE_void;
            void* returnValue = NULL;
            if (GetFunctionReturnValue(returnValue, returnType))
            {
    	        CDebuggerWatchVarEntry watch_result;

		        watch_result.mWatchRequestID = request_id;
                watch_result.mStackLevel = -1;

		        // -- fill in the watch entry
		        watch_result.mFuncNamespaceHash = 0;
		        watch_result.mFunctionHash = 0;
		        watch_result.mFunctionObjectID = 0;
		        watch_result.mObjectID = 0;
		        watch_result.mNamespaceHash = 0;

		        // -- type, name, and value string, and array size
		        watch_result.mType = returnType;
		        SafeStrcpy(watch_result.mVarName, variable_watch, kMaxNameLength);
                gRegisteredTypeToString[returnType](returnValue, watch_result.mValue, kMaxNameLength);
                watch_result.mArraySize = 1;

		        watch_result.mVarHash = Hash(variable_watch);
		        watch_result.mVarObjectID = 0;

                // -- if the type is an object, see if it actually exists
                if (returnType == TYPE_object)
                {
                    // -- ensure the object actually exists
                    CObjectEntry* oe = FindObjectEntry(*(uint32*)returnValue);
                    if (oe)
                        watch_result.mVarObjectID = oe->GetID();
                }

			    // -- send the response
			    DebuggerSendWatchVariable(&watch_result);

			    // -- if the result is an object, then send the complete object
			    if (watch_result.mType == TYPE_object && watch_result.mVarObjectID > 0)
			    {
				    DebuggerSendObjectMembers(&watch_result, watch_result.mVarObjectID);
			    }
            }
        }
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
                                          CFunctionCallStack& call_stack)
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

    // -- find the function we're currently executing
    int32 stacktop = 0;
    CFunctionEntry* cur_function = NULL;
    CObjectEntry* cur_object = NULL;
    cur_function = call_stack.GetExecuting(cur_object, stacktop);

    // -- make sure we've got a valid function
    if (!cur_function)
        return (false);

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
    codeblock->smFuncDefinitionStack->Push(fe, cur_object, 0);

    // -- add a funcdecl node, and set its left child to be the statement block
    // -- for fun, use the watch_id as the line number - to find it while debugging
    CFuncDeclNode* funcdeclnode = TinAlloc(ALLOC_TreeNode, CFuncDeclNode, codeblock, root->next,
                                           watch_id, watch_name, (int32)strlen(watch_name), "", 0, 0);

    // -- the body of our watch function, is to simply return the given expression
    // -- parsing and returning the expression will also identify the type for us
    // -- note:  trace expressions are evaluated verbatim
    char expr_result[kMaxTokenLength];
    if (use_trace)
        SafeStrcpy(expr_result, expression, kMaxTokenLength);
    else
        sprintf_s(expr_result, "return (%s);", expression);

    // -- several steps to go through - any failures will require us to clean up and return false
    bool8 success = true;

    // -- now we've got a temporary function with exactly the same set of local variables
    // -- see if we can parse the expression
	tReadToken parsetoken(expr_result, 0);
    success == !success || !ParseStatementBlock(codeblock, funcdeclnode->leftchild, parsetoken, false);

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
                                          CFunctionCallStack& cur_call_stack, CExecStack& cur_exec_stack)
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

    // -- find the function we're currently executing
    int32 stacktop = 0;
    CFunctionEntry* cur_function = NULL;
    CObjectEntry* cur_object = NULL;
    cur_function = cur_call_stack.GetExecuting(cur_object, stacktop);

    // -- make sure we've got a valid function
    if (!cur_function)
        return (false);

    // -- create the stack used to execute the function
	CExecStack execstack;
    CFunctionCallStack funccallstack;

    // -- push the function entry onto the call stack
    funccallstack.Push(watch_function, cur_object, 0);

    // -- create space on the execstack for the local variables
    int32 localvarcount = watch_function->GetContext()->CalculateLocalVarStackSize();
    execstack.Reserve(localvarcount * MAX_TYPE_SIZE);

    // -- copy the local values from the currently executing function, to stack
    CVariableEntry* cur_ve = cur_function->GetLocalVarTable()->First();
    while (cur_ve)
    {
        void* dest_stack_addr = execstack.GetStackVarAddr(0, cur_ve->GetStackOffset());
        void* cur_stack_addr = cur_exec_stack.GetStackVarAddr(stacktop, cur_ve->GetStackOffset());

        void* var_addr = cur_ve->GetAddr(NULL);
        memcpy(dest_stack_addr, cur_stack_addr, kMaxTypeSize);
        cur_ve = cur_function->GetLocalVarTable()->Next();
    }

    // -- call the function
    funccallstack.BeginExecution();
    bool8 result = CodeBlockCallFunction(watch_function, NULL, execstack, funccallstack, false);

    // -- if we executed succesfully...
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
    }

    // -- return the result
    return (result);
}

// ====================================================================================================================
// EvaluateWatchExpression():  Used by the debugger for variable watches.
// ====================================================================================================================
bool8 CScriptContext::EvaluateWatchExpression(const char* expression, bool8 conditional)
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
    cur_function = mDebuggerBreakFuncCallStack->GetExecuting(cur_object, stacktop);

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
                CFunctionCallStack funccallstack;

                // -- push the function entry onto the call stack
                funccallstack.Push(fe, NULL, 0);

                // -- create space on the execstack, if this is a script function
                int32 localvarcount = fe->GetContext()->CalculateLocalVarStackSize();
                execstack.Reserve(localvarcount * MAX_TYPE_SIZE);

                // -- copy the local values onto the stack
                CVariableEntry* temp_ve = temp_context->GetLocalVarTable()->First();
                while (temp_ve)
                {
                    void* stack_addr = execstack.GetStackVarAddr(0, temp_ve->GetStackOffset());
                    void* var_addr = temp_ve->GetAddr(NULL);
                    memcpy(stack_addr, var_addr, kMaxTypeSize);
                    temp_ve = temp_context->GetLocalVarTable()->Next();
                }

                // -- call the function
                funccallstack.BeginExecution();
                bool8 result = CodeBlockCallFunction(fe, NULL, execstack, funccallstack, false);

                // -- if we executed succesfully...
                if (result)
                {
                    eVarType returnType;
                    void* returnValue = execstack.Pop(returnType);
                    if (returnValue)
                    {
                        char resultString[kMaxNameLength];
                        gRegisteredTypeToString[returnType](returnValue, resultString, kMaxNameLength);
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
		if (!DebuggerFindStackTopVar(this, var_name_hash, found_variable, ve))
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
// DebuggerCurrentWorkingDir():  Use the packet type DATA, and notify the debugger of our current working directory
// ====================================================================================================================
void CScriptContext::DebuggerCurrentWorkingDir(const char* cwd)
{
    if (!cwd)
        cwd = "./";

    // -- calculate the size of the data
    int32 total_size = 0;

    // -- first int32 will be identifying this data packet
    total_size += sizeof(int32);

    // -- we're sending the file name as a atring, since the debugger may not be able to unhash it yet
    int strLength = (int32)strlen(cwd) + 1;
    total_size += strLength;

    // -- declare a header
    // -- note, if we ever implement a request/acknowledge approach, we can use the mID field
    SocketManager::tPacketHeader header(k_PacketVersion, SocketManager::tPacketHeader::DATA, total_size);

    // -- create the packet (null data, as we'll fill in the data directly into the packet)
    SocketManager::tDataPacket* newPacket = SocketManager::CreateDataPacket(&header, NULL);
    if (!newPacket)
    {
        TinPrint(this, "Error - DebuggerCurrentWorkingDir():  unable to send\n");
        return;
    }

    // -- initialize the ptr to the data buffer
    int32* dataPtr = (int32*)newPacket->mData;

    // -- write the identifier - defined in the debugger constants near the top of TinScript.h
    *dataPtr++ = k_DebuggerCurrentWorkingDirPacketID;

    // -- write the string
    SafeStrcpy((char*)dataPtr, cwd, strLength);

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
    SafeStrcpy((char*)dataPtr, filename, strLength);

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
                                           uint32* namespace_array,uint32* func_array,
                                           uint32* linenumber_array, int array_size)
{
    // -- calculate the size of the data
    int32 total_size = 0;

    // -- first int32 will be identifying this data packet
    total_size += sizeof(int32);

    // -- second int32 will be the array size
    total_size += sizeof(int32);

    // -- finally, we have a uint32 for each of codeblock, objid, namespace, function, and line number
    // -- note:  the debugger suppors a callstack of up to 32, so our max packet size is about 640 bytes
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
	*dataPtr++ = watch_var_entry->mStackLevel;

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
    SafeStrcpy((char*)dataPtr, watch_var_entry->mVarName, nameLength);
    dataPtr += (nameLength / 4);

    // -- write the value string length
    *dataPtr++ = valueLength;

    // -- write the value string
    SafeStrcpy((char*)dataPtr, watch_var_entry->mValue, valueLength);
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
        watch_entry.mStackLevel = callingFunction ? callingFunction->mStackLevel : -1;

        watch_entry.mFuncNamespaceHash = callingFunction ? callingFunction->mFuncNamespaceHash : 0;
        watch_entry.mFunctionHash = callingFunction ? callingFunction->mFunctionHash : 0;
        watch_entry.mFunctionObjectID = callingFunction ? callingFunction->mFunctionObjectID : 0;

        watch_entry.mObjectID = object_id;
        watch_entry.mNamespaceHash = Hash("self");

        // -- TYPE_void marks this as a namespace label, and set the object's name as the value
        watch_entry.mType = TYPE_void;
        SafeStrcpy(watch_entry.mVarName, "self", kMaxNameLength);
        SafeStrcpy(watch_entry.mValue, oe->GetName(), kMaxNameLength);

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
        ns_entry.mStackLevel = callingFunction ? callingFunction->mStackLevel : -1;

        ns_entry.mFuncNamespaceHash = callingFunction ? callingFunction->mFuncNamespaceHash : 0;
        ns_entry.mFunctionHash = callingFunction ? callingFunction->mFunctionHash : 0;
        ns_entry.mFunctionObjectID = callingFunction ? callingFunction->mFunctionObjectID : 0;

        ns_entry.mObjectID = object_id;
        ns_entry.mNamespaceHash = ns->GetHash();

        // -- TYPE_void marks this as a namespace label
        ns_entry.mType = TYPE_void;
        SafeStrcpy(ns_entry.mVarName, UnHash(ns->GetHash()), kMaxNameLength);
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
		member_entry.mStackLevel = callingFunction ? callingFunction->mStackLevel : -1;

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
        SafeStrcpy(member_entry.mVarName, UnHash(member->GetHash()), kMaxNameLength);

        // -- copy the value, as a string (to a max length)
        gRegisteredTypeToString[member->GetType()](member->GetAddr(oe->GetAddr()), member_entry.mValue,
                                                   kMaxNameLength);

        // -- fill in the cached members
        member_entry.mVarHash = member->GetHash();
        member_entry.mVarObjectID = 0;
        if (member->GetType() == TYPE_object)
        {
             member_entry.mVarObjectID = *(uint32*)(member->GetAddr(oe->GetAddr()));
        }

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

    // -- send the length of the assert message, including EOL, and 4-byte aligned
    *dataPtr++ = msgLength;

    // -- write the message string
    SafeStrcpy((char*)dataPtr, assert_msg, msgLength);
    dataPtr += (msgLength / 4);

    // -- send the codeblock hash
    *dataPtr++ = codeblock_hash;

    // -- send the line number
    *dataPtr++ = line_number;

    // -- send the packet
    SocketManager::SendDataPacket(newPacket);
}

// ====================================================================================================================
// DebuggerSendPrint():  Send a print message to the debugger (usually to echo the local output)
// ====================================================================================================================
void CScriptContext::DebuggerSendPrint(const char* fmt, ...)
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

    // -- send the length of the assert message, including EOL, and 4-byte aligned
    *dataPtr++ = msgLength;

    // -- write the message string
    SafeStrcpy((char*)dataPtr, msg_buf, msgLength);
    dataPtr += (msgLength / 4);

    // -- send the packet
    SocketManager::SendDataPacket(newPacket);
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
        // -- populate and send a function assist entry
        CFunctionEntry* function_entry = function_table->First();
        while (function_entry)
        {
            CDebuggerFunctionAssistEntry entry;
            entry.mIsObjectEntry = false;
            entry.mObjectID = object_id;
            entry.mNamespaceHash = current_namespace ? current_namespace->GetHash() : 0;
            entry.mFunctionHash = function_entry->GetHash();
            SafeStrcpy(entry.mFunctionName, function_entry->GetName(), kMaxNameLength);

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

            // -- send the entry
            DebuggerSendFunctionAssistEntry(entry);

            // -- get the next
            function_entry = function_table->Next();
        }

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
// DebuggerSendFunctionAssistEntry():  Send a a function and it's parameter list to the debugger
// ====================================================================================================================
void CScriptContext::DebuggerSendFunctionAssistEntry(const CDebuggerFunctionAssistEntry& function_assist_entry)
{
    // -- calculate the size of the data
    int32 total_size = 0;

    // -- first int32 will be identifying this data packet
    total_size += sizeof(int32);

	// -- object ID
	total_size += sizeof(int32);

	// -- namespace hash
	total_size += sizeof(int32);

	// -- function hash
	total_size += sizeof(int32);

    // -- calculate the length of the function name, including EOL, and 4-byte aligned
    int32 nameLength = (int32)strlen(function_assist_entry.mFunctionName) + 1;
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
    total_size += (sizeof(int32) * function_assist_entry.mParameterCount);;

    // -- parameter name hash (x parameter count)
    total_size += (sizeof(int32) * function_assist_entry.mParameterCount);;

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

	// -- object ID
	*dataPtr++ = function_assist_entry.mObjectID;

	// -- namespace hash
	*dataPtr++ = function_assist_entry.mNamespaceHash;

	// -- function hash
	*dataPtr++ = function_assist_entry.mFunctionHash;

    // -- send the function name length
    *dataPtr++ = nameLength;

    // -- write the message string
    SafeStrcpy((char*)dataPtr, function_assist_entry.mFunctionName, nameLength);
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

    // -- send the entry
    SocketManager::SendCommandf("DebuggerNotifyCreateObject(%d, `%s`, `%s`);", oe->GetID(),
                                oe->GetNameHash() != 0 ? oe->GetName() : "", derivation_buf);
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

// ====================================================================================================================
// AddThreadCommand():  This enqueues a command, to be process during the normal update
// ====================================================================================================================
// -- Thread commands are only supported in WIN32
#ifdef WIN32
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
    uint32 lengthRemaining = kThreadExecBufferSize - ((uint32)mThreadBufPtr - (uint32)mThreadExecBuffer);
    if (lengthRemaining < cmdLength)
    {
        // -- no need to assert - the socket will re-enqueue the command after the buffer has been processed
        //ScriptAssert_(this, 0, "<internal>", -1, "Error - AddThreadCommand():  buffer length exceeded.\n");
        success = false;
    }
    else
    {
        SafeStrcpy(mThreadBufPtr, command, lengthRemaining);
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
    if (mThreadBufPtr == NULL)
        return;

    // -- we need to wrap access to the command buffer in a thread mutex, to prevent simultaneous access
    mThreadLock.Lock();

    // -- reset the bufPtr
    mThreadBufPtr = NULL;

    // -- because the act of executing the command could trigger a breakpoint, we need to unlock this thread
    // -- *before* executing the command, or the run/step command will never actually be received.
    // -- this means, we copy current buffer so we can begin filling it again from the socket thread
    char local_exec_buffer[kThreadExecBufferSize];
    int32 bytes_to_copy = (int32)strlen(mThreadExecBuffer) + 1;
    memcpy(local_exec_buffer, mThreadExecBuffer, bytes_to_copy);

    // -- unlock the thread
    mThreadLock.Unlock();

    // -- execute the buffer
    ExecCommand(local_exec_buffer);
}

#endif // WIN32

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

    // -- as we've just received the request, send the initial "clear" response
    SocketManager::SendCommand("DebuggerClearObjectBrowser();");

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
}

// -------------------------------------------------------------------------------------------------------------------
// -- Registration
REGISTER_FUNCTION_P1(DebuggerSetConnected, DebuggerSetConnected, void, bool8);
REGISTER_FUNCTION_P6(DebuggerAddBreakpoint, DebuggerAddBreakpoint, void, const char*, int32, bool8, const char*, const char*, bool8);
REGISTER_FUNCTION_P2(DebuggerRemoveBreakpoint, DebuggerRemoveBreakpoint, void, const char*, int32);
REGISTER_FUNCTION_P1(DebuggerRemoveAllBreakpoints, DebuggerRemoveAllBreakpoints, void, const char*);

REGISTER_FUNCTION_P0(DebuggerForceBreak, DebuggerForceBreak, void);
REGISTER_FUNCTION_P2(DebuggerBreakStep, DebuggerBreakStep, void, bool8, bool8);
REGISTER_FUNCTION_P0(DebuggerBreakRun, DebuggerBreakRun, void);

REGISTER_FUNCTION_P3(DebuggerAddVariableWatch, DebuggerAddVariableWatch, void, int32, const char*, bool8);
REGISTER_FUNCTION_P7(DebuggerToggleVarWatch, DebuggerToggleVarWatch, void, int32, uint32, int32, bool8, const char*, const char*, bool8);
REGISTER_FUNCTION_P3(DebuggerModifyVariableWatch, DebuggerModifyVariableWatch, void, int32, const char*, const char*);

REGISTER_FUNCTION_P1(DebuggerListObjects, DebuggerListObjects, void, int32);
REGISTER_FUNCTION_P1(DebuggerInspectObject, DebuggerInspectObject, void, int32);

REGISTER_FUNCTION_P0(DebuggerListSchedules, DebuggerListSchedules, void);
REGISTER_FUNCTION_P1(DebuggerRequestFunctionAssist, DebuggerRequestFunctionAssist, void, int32);

// == class CThreadMutex ==============================================================================================
// -- CThreadMutex is only functional in WIN32

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CThreadMutex::CThreadMutex()
    : mIsLocked(false)
{
    #ifdef WIN32
        mThreadMutex = CreateMutex(NULL, false, NULL);
    #endif
}

// ====================================================================================================================
// Lock():  Lock access to the following structure/code from any other thread, until Unlocked()
// ====================================================================================================================
void CThreadMutex::Lock()
{
    mIsLocked = true;
    //printf("[0x%x] Thread locked: %s\n", (unsigned int)this, mIsLocked ? "true" : "false");
    #ifdef WIN32
        WaitForSingleObject(mThreadMutex, INFINITE);
    #endif
}

// ====================================================================================================================
// Unlock():  Restore access to the previous structure/code, to any other thread
// ====================================================================================================================
void CThreadMutex::Unlock()
{
    #ifdef WIN32
        ReleaseMutex(mThreadMutex);
    #endif
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
CDebuggerWatchExpression::CDebuggerWatchExpression(bool8 isConditional, bool8 break_enabled, const char* condition,
                                                   const char* trace, bool8 trace_on_condition)
{
    mIsEnabled = break_enabled;
    mIsConditional = isConditional;
    SafeStrcpy(mConditional, condition, kMaxNameLength);
    SafeStrcpy(mTrace, trace, kMaxNameLength);
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
	if (TinScript::GetContext() != nullptr)
	{
		SetAttributes(false, "", "", false);
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
            TinScript::GetContext()->GetGlobalNamespace()->GetFuncTable()->RemoveItem(mWatchFunctionEntry->GetHash());
            TinFree(mWatchFunctionEntry);
            CCodeBlock::DestroyCodeBlock(codeblock);
            mWatchFunctionEntry = NULL;
        }

        // -- the first time this is needed, it'll be evaluated
        SafeStrcpy(mConditional, new_conditional, kMaxNameLength);
    }

    // -- same for the trace
    if (strcmp(mTrace, new_trace) != 0)
    {
        if (mTraceFunctionEntry)
        {
            CCodeBlock* codeblock = mTraceFunctionEntry->GetCodeBlock();
            TinScript::GetContext()->GetGlobalNamespace()->GetFuncTable()->RemoveItem(mTraceFunctionEntry->GetHash());
            TinFree(mTraceFunctionEntry);
            CCodeBlock::DestroyCodeBlock(codeblock);
            mTraceFunctionEntry = NULL;
        }

        // -- the first time this is needed, it'll be evaluated
        SafeStrcpy(mTrace, new_trace, kMaxNameLength);
    }
}

} // TinScript

// ====================================================================================================================
// class CScriptObject:  Default registered object, to provide a base class for scripting.
// ====================================================================================================================
// -- generic object - has absolutely no functionality except to serve as something to
// -- instantiate, that you can name, and then implement methods on the namespace

class CScriptObject
{
    public:
        CScriptObject() { dummy = 0; }
        virtual ~CScriptObject() {}

        DECLARE_SCRIPT_CLASS(CScriptObject, VOID);

    private:
        int64 dummy;
};

IMPLEMENT_SCRIPT_CLASS_BEGIN(CScriptObject, VOID)
IMPLEMENT_SCRIPT_CLASS_END()

// ====================================================================================================================
// EOF
// ====================================================================================================================
