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
// cmdshell.h
// ------------------------------------------------------------------------------------------------

// -- TinScript includes
#include "TinScript.h"
#include "TinRegistration.h"

// ====================================================================================================================
// Default Assert Handler
// ====================================================================================================================
bool8 CmdShellAssertHandler(TinScript::CScriptContext* script_context, const char* condition,
                            const char* file, int32 linenumber, const char* fmt, ...);

// ====================================================================================================================
// class CCmdShell:  simple input out command shell
// ====================================================================================================================
class CCmdShell
{
public:
    // -- constructor / destructor
    CCmdShell();
    virtual ~CCmdShell() { }

    // -- clears out stale text, and refreshes the prompt
    void RefreshConsoleInput(bool8 display_prompt = false, const char* new_input_string = NULL);

    // -- allows the console to handle a preceeding newline 
    void NotifyPrintStart();
    void NotifyPrintEnd();

    // -- update should be called every frame - returns the const char* of the current command
    const char* Update();

private:
    // -- constants
    static const int32 kMaxHistory = 64;

    // -- history members
    bool8 mHistoryFull;
    int32 mHistoryIndex;
    int32 mHistoryLastIndex;
    char mHistory[TinScript::kMaxTokenLength][kMaxHistory];

    bool8 mCurrentLineIsPrompt;
    bool8 mRefreshPrompt;

    // -- input members
    char mConsoleInputBuf[TinScript::kMaxTokenLength];
    char* mInputPtr;

    // -- tab completion members
    int32 mTabCompletionIndex;
    char mTabCompletionBuf[TinScript::kMaxTokenLength];

    // -- command entry buffer
    char mCommandBuf[TinScript::kMaxTokenLength];
};

// ====================================================================================================================
// EOF
// ====================================================================================================================
