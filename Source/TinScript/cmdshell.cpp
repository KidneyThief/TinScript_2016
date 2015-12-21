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
// cmdshell.cpp
// ------------------------------------------------------------------------------------------------

// -- system includes
#include "stdafx.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "windows.h"
#include "conio.h"

// -- TinScript includes
#include "TinScript.h"
#include "TinRegistration.h"

// -- includes
#include "cmdshell.h"

// ====================================================================================================================
// AssertHandler():  default assert handler - simply uses printf
// returns false if we should break
// ====================================================================================================================
bool8 CmdShellAssertHandler(TinScript::CScriptContext* script_context, const char* condition,
                            const char* file, int32 linenumber, const char* fmt, ...) {
    if (!script_context->IsAssertStackSkipped() || script_context->IsAssertEnableTrace())
    {
        if (!script_context->IsAssertStackSkipped())
        {
            TinPrint(script_context, "*************************************************************\n");
        }
        else
        {
            TinPrint(script_context, "\n");
        }

        if (linenumber >= 0)
        {
            TinPrint(script_context, "Assert(%s) file: %s, line %d:\n", condition, file, linenumber + 1);
        }
        else
        {
            TinPrint(script_context, "Exec Assert(%s):\n", condition);
        }

        va_list args;
        va_start(args, fmt);
        char msgbuf[2048];
        vsprintf_s(msgbuf, 2048, fmt, args);
        va_end(args);
        TinPrint(script_context, msgbuf);

        if (!script_context->IsAssertStackSkipped())
        {
            TinPrint(script_context, "*************************************************************\n");

            // -- see if we should break, trace (dump the rest of the assert stack), or skip
            bool assert_trace = false;
            bool assert_break = false;

            int32 debugger_session = 0;
            if (script_context->IsDebuggerConnected(debugger_session))
            {
                // $$$TZA trace, or simply skip?
                assert_trace = false;
            }
            else
            {
                TinPrint(script_context, "Press 'b' to break, 't' to trace, otherwise skip...\n");
                char ch = getchar();
                if (ch == 'b')
                    assert_break = true;
                else if (ch == 't')
                    assert_trace = true;
            }

            // -- handle the result
            if (assert_break)
            {
                return false;
            }
            else if (assert_trace)
            {
                script_context->SetAssertStackSkipped(true);
                script_context->SetAssertEnableTrace(true);
                return true;
            }
            else
            {
                script_context->SetAssertStackSkipped(true);
                script_context->SetAssertEnableTrace(false);
                return true;
            }
        }
    }

    // -- handled - return true so we don't break
    return true;
}


// ====================================================================================================================
// Constructor
// ====================================================================================================================
CCmdShell::CCmdShell()
{
    // -- initialize the mHistory indicies
    mHistoryFull = false;
    mHistoryIndex = -1;
    mHistoryLastIndex = -1;

    // -- initialize the mHistory buffers
    for (int32 i = 0; i < kMaxHistory; ++i)
        *mHistory[i] = '\0';

    // -- initialize the input
    mInputPtr = mConsoleInputBuf;
    *mInputPtr = '\0';

    // -- initialize the command buf
    mCommandBuf[0] = '\0';

    // -- print the initial prompt
    RefreshConsoleInput(true, "");
}

// ====================================================================================================================
// RefreshConsoleInput():  Clears out any stale text, and refreshes the display.
// ====================================================================================================================
void CCmdShell::RefreshConsoleInput(bool8 display_prompt, const char* new_input_string)
{
    // -- whatever was on in the buffer needs to be deleted
    int input_len = strlen(mConsoleInputBuf);
    for (int i = 0; i < input_len; ++i)
    {
        // -- print a space over the "last" character
        printf("\b \b");
    }

    // -- if we have a new input string, copy it
    if (new_input_string)
        TinScript::SafeStrcpy(mConsoleInputBuf, new_input_string, TinScript::kMaxTokenLength);

    // -- if we're supposed to display the prompt, draw it before the input buf
    if (display_prompt)
        printf("\nConsole => ");

    printf(mConsoleInputBuf);
}

// ====================================================================================================================
// Update():  Called every frame, returns a const char* if there's a command to be processed
// ====================================================================================================================
const char* CCmdShell::Update()
{
    // -- initialize the return value
    const char* return_value = NULL;

    // -- see if we hit a key
    if (_kbhit())
    {
        // -- read the next key
        bool8 special_key = false;
        char c = _getch();
        if (c == -32) {
            special_key = true;
            c = _getch();
        }

        // -- esc
        if (!special_key && c == 27) {
            int input_len = strlen(mConsoleInputBuf);
            for (int i = 0; i < input_len; ++i)
            {
                // -- print a space over the "last" character
                printf("\b \b");
            }

            // -- reset the input pointer, and set it to an empty string
            mInputPtr = mConsoleInputBuf;
            *mInputPtr = '\0';

            // -- reset the mHistory
            mHistoryIndex = -1;
        }

        // -- uparrow
        else if (special_key && c == 72) {
            int32 oldhistory = mHistoryIndex;
            if (mHistoryIndex < 0)
                mHistoryIndex = mHistoryLastIndex;
            else if (mHistoryLastIndex > 0) {
                if (mHistoryFull)
                    mHistoryIndex = (mHistoryIndex + kMaxHistory - 1) % kMaxHistory;
                else
                    mHistoryIndex = (mHistoryIndex + mHistoryLastIndex) % (mHistoryLastIndex + 1);
            }

            // -- see if we actually changed
            if (mHistoryIndex != oldhistory && mHistoryIndex >= 0)
            {
                // -- update the input buf with the new string
                RefreshConsoleInput(false, mHistory[mHistoryIndex]);

                // -- set the "new character" input ptr to the end of the buf
                mInputPtr = &mConsoleInputBuf[strlen(mConsoleInputBuf)];
            }
        }

        // -- downarrow
        else if (special_key && c == 80) {
            int32 oldhistory = mHistoryIndex;
            if (mHistoryIndex < 0)
                mHistoryIndex = mHistoryLastIndex;
            else if (mHistoryLastIndex > 0) {
                if (mHistoryFull)
                    mHistoryIndex = (mHistoryIndex + 1) % kMaxHistory;
                else
                    mHistoryIndex = (mHistoryIndex + 1) % (mHistoryLastIndex + 1);
            }

            // -- see if we actually changed
            if (mHistoryIndex != oldhistory && mHistoryIndex >= 0) {
                // -- update the input buf with the new string
                RefreshConsoleInput(false, mHistory[mHistoryIndex]);

                // -- set the "new character" input ptr to the end of the buf
                mInputPtr = &mConsoleInputBuf[strlen(mConsoleInputBuf)];
            }
        }

        // -- backspace keypress
        else if (!special_key && c == 8 && mInputPtr > mConsoleInputBuf) {
            *--mInputPtr = '\0';
            printf("\b \b");
        }

        // -- return keypress
        else if (!special_key && c == 13) {
            // -- echo the input and execute it
            *mInputPtr = '\0';
            RefreshConsoleInput(false);
            printf("\n>> %s\n", mConsoleInputBuf);

            // -- add this to the mHistory buf
            const char* historyptr = (mHistoryLastIndex < 0) ? NULL : mHistory[mHistoryLastIndex];
            if (mConsoleInputBuf[0] != '\0' && (!historyptr ||
                                                strcmp(historyptr, mConsoleInputBuf) != 0)) {
                mHistoryFull = mHistoryFull || mHistoryLastIndex == kMaxHistory - 1;
                mHistoryLastIndex = (mHistoryLastIndex + 1) % kMaxHistory;
                TinScript::SafeStrcpy(mHistory[mHistoryLastIndex], mConsoleInputBuf, TinScript::kMaxTokenLength);
            }
            mHistoryIndex = -1;

            // -- copy the input buf to the command buf, and return it
            TinScript::SafeStrcpy(mCommandBuf, mConsoleInputBuf, TinScript::kMaxTokenLength);
            return_value = mCommandBuf;

            // -- clear the input, and set the input ptr
            RefreshConsoleInput(false, "");
            mInputPtr = mConsoleInputBuf;
        }

        // ignore any other non-printable character
        else if (!special_key && (uint32)c >= 0x20) {
            RefreshConsoleInput();
            *mInputPtr++ = c;
            *mInputPtr = '\0';
            printf("%c", c);
        }
    }

    // -- return the result - only valid on the frame the return key is pressed
    return (return_value);
};

// ====================================================================================================================
// EOF
// ====================================================================================================================
