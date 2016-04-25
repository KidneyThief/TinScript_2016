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
    mCurrentLineIsPrompt = false;
    mRefreshPrompt = false;

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
// SubstringLength(): Compares two strings, and returns the number of characters in common.
// ====================================================================================================================
int32 SubstringLength(const char* string_a, const char* string_b)
{
    // -- if one of the two strings is not specified, they have 0 characters in common
    if (!string_a || !string_b)
        return (0);

    int32 substr_count = 0;
    const char* str_a_ptr = string_a;
    const char* str_b_ptr = string_b;

    while (*str_a_ptr != '\0' && *str_b_ptr != '\0' && *str_a_ptr == *str_b_ptr)
    {
        ++substr_count;
        ++str_a_ptr;
        ++str_b_ptr;
    }

    return (substr_count);
}

// ====================================================================================================================
// DeleteLastChar():  Removes the last character from the buffer, and updates the prompt (including word wrap).
// ====================================================================================================================
void DeleteLastchar()
{
    HANDLE cursor_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD cursor_pos;
    COORD cursor_window_size;
    _CONSOLE_SCREEN_BUFFER_INFO console_info;
    if (GetConsoleScreenBufferInfo(cursor_handle, &console_info))
    {
        cursor_pos = console_info.dwCursorPosition;
        cursor_window_size = console_info.dwMaximumWindowSize;
    }

    // -- see if we need to set the cursor to the line above
    if (--cursor_pos.X < 0)
    {
        cursor_pos.X = cursor_window_size.X - 1;
        if (cursor_pos.Y > 0)
            --cursor_pos.Y;

        // -- update the cursor poition
        SetConsoleCursorPosition(cursor_handle, cursor_pos);

        // - print a space to erase the character
        printf(" ");

        // -- and again, reset the cursor position
        SetConsoleCursorPosition(cursor_handle, cursor_pos);
    }

    // -- else simply back up and write a space over the last character
    else
        printf("\b \b");
}

// ====================================================================================================================
// RefreshConsoleInput():  Clears out any stale text, and refreshes the display.
// ====================================================================================================================
void CCmdShell::RefreshConsoleInput(bool8 display_prompt, const char* new_input_string)
{
    // -- if the prompt is to be displayed, but the new_input_string isn't "new", we're done
    const char* test_input_string = new_input_string != nullptr ? new_input_string : "";
    bool8 is_new_string = strcmp(mConsoleInputBuf, test_input_string) != 0;
    if (display_prompt && !is_new_string && mConsoleInputBuf[0] == '\0' && mCurrentLineIsPrompt)
    {
        return;
    }

    //SetConsoleCursorPosition()
    HANDLE cursor_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD cursor_pos;
    COORD cursor_window_size;
    _CONSOLE_SCREEN_BUFFER_INFO console_info;
    if (GetConsoleScreenBufferInfo(cursor_handle, &console_info))
    {
        cursor_pos = console_info.dwCursorPosition;
        cursor_window_size = console_info.dwMaximumWindowSize;
        //printf("TEST: %d, %d out of %d", cursor_pos.X, cursor_pos.Y, cursor_window_size.X);
    }

    // -- if we're supposed to re-display the entire prompt
    if (display_prompt)
    {
        // -- whatever was on in the buffer needs to be deleted, by backing up, printing a space over the last char
        int input_len = strlen(mConsoleInputBuf);
        for (int i = 0; i < input_len; ++i)
            DeleteLastchar();

        // -- print the prompt 
        printf("\nConsole => ");

        // -- update the input string, if given
        if (new_input_string)
            TinScript::SafeStrcpy(mConsoleInputBuf, new_input_string, TinScript::kMaxTokenLength);

        // -- display the input string
        printf(mConsoleInputBuf);
    }

    // -- otherwise, only refresh the difference between the old and new strings
    else if (new_input_string)
    {
        int32 substr_length = SubstringLength(mConsoleInputBuf, new_input_string);
        int32 length_current = (int32)strlen(mConsoleInputBuf);
        int32 length_new = (int32)strlen(new_input_string);

        // -- if the new string doesn't add to the existing, remove the trailing characters
        if (substr_length < length_current)
        {
            int32 remove_count = length_current - substr_length;
            for (int i = 0; i < remove_count; ++i)
                DeleteLastchar();
        }

        // -- if there are additional characters to print, print them now
        if (length_new > substr_length)
            printf(&new_input_string[substr_length]);

        // -- update the new input string
        TinScript::SafeStrcpy(mConsoleInputBuf, new_input_string, TinScript::kMaxTokenLength);
    }

    // -- set the bool, so the next printed output is on a new line
    mCurrentLineIsPrompt = mCurrentLineIsPrompt || display_prompt;
}

// ====================================================================================================================
// NotifyPrintStart():  Tracks whether we need to preceed the output with a newline
// ====================================================================================================================
void CCmdShell::NotifyPrintStart()
{
    if (mCurrentLineIsPrompt)
    {
        printf("\n");
        mCurrentLineIsPrompt = false;
    }
}

// ====================================================================================================================
// NotifyCommandOutput():  Commands reflected to the output handle their own newlines.
// ====================================================================================================================
void CCmdShell::NotifyPrintEnd()
{
    // -- refresh the prompt next update
    mRefreshPrompt = true;
}

// ====================================================================================================================
// Update():  Called every frame, returns a const char* if there's a command to be processed
// ====================================================================================================================
const char* CCmdShell::Update()
{
    // -- see if we should refresh the prompt
    if (mRefreshPrompt)
    {
        RefreshConsoleInput(true);
        mRefreshPrompt = false;
    }

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
        if (!special_key && c == 27)
        {
            // -- delete al characters from the current promp
            int input_len = strlen(mConsoleInputBuf);
            for (int i = 0; i < input_len; ++i)
                DeleteLastchar();

            // -- reset the input pointer, and set it to an empty string
            mInputPtr = mConsoleInputBuf;
            *mInputPtr = '\0';

            // -- reset the mHistory
            mHistoryIndex = -1;
        }

        // -- uparrow
        else if (special_key && c == 72)
        {
            int32 oldhistory = mHistoryIndex;
            if (mHistoryIndex < 0)
                mHistoryIndex = mHistoryLastIndex;
            else if (mHistoryLastIndex > 0)
            {
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
        else if (special_key && c == 80)
        {
            int32 oldhistory = mHistoryIndex;
            if (mHistoryIndex < 0)
                mHistoryIndex = mHistoryLastIndex;
            else if (mHistoryLastIndex > 0)
            {
                if (mHistoryFull)
                    mHistoryIndex = (mHistoryIndex + 1) % kMaxHistory;
                else
                    mHistoryIndex = (mHistoryIndex + 1) % (mHistoryLastIndex + 1);
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

        // -- backspace keypress
        else if (!special_key && c == 8 && mInputPtr > mConsoleInputBuf)
        {
            *--mInputPtr = '\0';
            DeleteLastchar();
        }

        // -- return keypress
        else if (!special_key && c == 13)
        {
            // -- echo the input and execute it
            *mInputPtr = '\0';
            RefreshConsoleInput(false);
            NotifyPrintStart();
            printf(">> %s\n", mConsoleInputBuf);

            // -- add this to the mHistory buf
            const char* historyptr = (mHistoryLastIndex < 0) ? NULL : mHistory[mHistoryLastIndex];
            if (mConsoleInputBuf[0] != '\0' && (!historyptr ||
                                                strcmp(historyptr, mConsoleInputBuf) != 0))
            {
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
        else if (!special_key && (uint32)c >= 0x20)
        {
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
