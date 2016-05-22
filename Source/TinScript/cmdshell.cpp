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

// -- statics ---------------------------------------------------------------------------------------------------------
CCmdShell* CCmdShell::sm_cmdShell = nullptr;

// ====================================================================================================================
// CmdShellPrintf():  Default printf handler.
// ====================================================================================================================
int CmdShellPrintf(const char* fmt, ...)
{
    // -- notify the shell we're about to print
    if (CCmdShell::GetInstance() != nullptr)
        CCmdShell::GetInstance()->NotifyPrintStart();

    // -- print the message
    va_list args;
    va_start(args, fmt);
    char buffer[1024];
    vsprintf_s(buffer, 1024, fmt, args);
    va_end(args);
    printf(buffer);

    // -- notify the shell we're about to print
    if (CCmdShell::GetInstance() != nullptr)
        CCmdShell::GetInstance()->NotifyPrintEnd();

    return (0);
}

// ====================================================================================================================
// CmdShellAssertHandler():  Default assert handler - returns false if we should break.
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
    // -- set the singleton
    sm_cmdShell = this;

    mCurrentLineIsPrompt = false;
    mRefreshPrompt = false;

    // -- initialize the mHistory indicies
    mHistoryFull = false;
    mHistoryIndex = -1;
    mHistoryLastIndex = -1;

    // -- initialize the mHistory buffers
    for (int32 i = 0; i < kMaxHistory; ++i)
        *mHistory[i] = '\0';

    // -- initialize the cursor
    m_screenHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    _CONSOLE_SCREEN_BUFFER_INFO console_info;
    if (GetConsoleScreenBufferInfo(m_screenHandle, &console_info))
    {
        m_screenCursorPos = console_info.dwCursorPosition;
        m_screenSize = console_info.dwMaximumWindowSize;
    }

    mConsoleInputBuf[0] = '\0';
    m_cursorOffset = 0;

    // -- initialize the tab completion members
    mTabCompletionIndex = -1;
    mTabCompletionBuf[0] = '\0';

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
// DeleteCharactersFromDisplay():  Removes character from the end of the display (including word wrap)
// ====================================================================================================================
void CCmdShell::DeleteCharactersFromDisplay(int32 count)
{
    if (mConsoleInputBuf[0] == '\0' || count <= 0)
        return;

    // -- set the screen cursor position to the end of the display string, without assigning the m_cursorOffset
    SetCursorPosition(-1);
    COORD cursor_pos;
    COORD cursor_window_size;
    _CONSOLE_SCREEN_BUFFER_INFO console_info;
    if (GetConsoleScreenBufferInfo(m_screenHandle, &console_info))
    {
        cursor_pos = console_info.dwCursorPosition;
        cursor_window_size = console_info.dwMaximumWindowSize;
    }

    // -- delete charactes from the end of the display string
    for (int i = 0; i < count; ++i)
    {
        // -- see if we need to set the cursor to the line above
        if (--cursor_pos.X < 0)
        {
            cursor_pos.X = cursor_window_size.X - 1;
            if (cursor_pos.Y > 0)
                --cursor_pos.Y;

            // -- update the cursor poition
            SetConsoleCursorPosition(m_screenHandle, cursor_pos);

            // - print a space to erase the character
            printf(" ");

            // -- and again, reset the cursor position
            SetConsoleCursorPosition(m_screenHandle, cursor_pos);
        }

        // -- else simply back up and write a space over the last character
        else
            printf("\b \b");

        // -- we deleted a character, so decrement the offset
        --m_cursorOffset;
    }
}

// ====================================================================================================================
// SetCursorPosition():  Converts ths character index to the console x,y cursor position.
// ====================================================================================================================
void CCmdShell::SetCursorPosition(int32 pos)
{
    // -- if the pos is -1, set the cursor to the end of the string
    int32 count = (int32)strlen(mConsoleInputBuf);
    m_cursorOffset = pos < 0 || pos >= count ? count : pos;

    // -- calculate the actual cursor position
    int32 line_x = (m_screenCursorPos.X + m_cursorOffset) % m_screenSize.X;
    int32 line_y = m_screenCursorPos.Y + (m_screenCursorPos.X + m_cursorOffset) / m_screenSize.X;
    COORD newScreenPos;
    newScreenPos.X = line_x;
    newScreenPos.Y = line_y;
    SetConsoleCursorPosition(m_screenHandle, newScreenPos);
}

// ====================================================================================================================
// InsertCharacterAtCursor():  Inserts a character at the cursor position.
// ====================================================================================================================
void CCmdShell::InsertCharacterAtCursor(char c)
{
    // -- get the character count, and pointers - update the end of the buffer if needed
    int32 count = (int32)strlen(mConsoleInputBuf);
    char* append_ptr = &mConsoleInputBuf[count];
    char* input_ptr = &mConsoleInputBuf[m_cursorOffset];

    // -- ensure we've got room
    if (count >= TinScript::kMaxTokenLength - 1)
        return;

    // -- ensure the string is null-terminated after inserting a character 
    *(append_ptr + 1) = '\0';
    while (append_ptr >= input_ptr)
    {
        *(append_ptr + 1) = *append_ptr;
        --append_ptr;
    }

    // -- set the character
    *input_ptr++ = c;

    // -- refresh the display
    printf(&mConsoleInputBuf[m_cursorOffset]);

    // -- update the offset, and cursor
     SetCursorPosition(m_cursorOffset + 1);
}

// ====================================================================================================================
// DeleteCharacterAtCursor():  Deletes the character at the cursor position.
// ====================================================================================================================
void CCmdShell::DeleteCharacterAtCursor()
{
    // -- remove the characters from the display
    int32 count = (int32)strlen(mConsoleInputBuf);
    int32 remove_count = count - m_cursorOffset;
    DeleteCharactersFromDisplay(remove_count);

    // -- get the character count, and pointers - update the end of the buffer if needed
    char* input_ptr = &mConsoleInputBuf[m_cursorOffset];
    while (*input_ptr != '\0')
        *input_ptr++ = *(input_ptr + 1);

    // -- if the cursor offset is not at the end of the string, we need to re-print 
    if (m_cursorOffset < count)
    {
        printf(&mConsoleInputBuf[m_cursorOffset]);
        SetCursorPosition(m_cursorOffset);
    }
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

    // -- if we're supposed to re-display the entire prompt
    if (display_prompt)
    {
        // -- whatever was on in the buffer needs to be deleted, by backing up, printing a space over the last char
        int input_len = strlen(mConsoleInputBuf);
        DeleteCharactersFromDisplay(input_len);

        // -- print the prompt 
        printf("\nConsole => ");

        // -- cache the new screen cursor position
        _CONSOLE_SCREEN_BUFFER_INFO console_info;
        if (GetConsoleScreenBufferInfo(m_screenHandle, &console_info))
            m_screenCursorPos = console_info.dwCursorPosition;

        // -- update the input string, if given
        if (new_input_string)
            TinScript::SafeStrcpy(mConsoleInputBuf, new_input_string, TinScript::kMaxTokenLength);

        // -- display the input string
        printf(mConsoleInputBuf);

        // -- ensure the cursor position is up to date
        SetCursorPosition(-1);
    }

    // -- otherwise, only refresh the difference between the old and new strings
    else if (new_input_string)
    {
        int32 count = (int32)strlen(mConsoleInputBuf);
        DeleteCharactersFromDisplay(count);

        // -- update the new input string
        TinScript::SafeStrcpy(mConsoleInputBuf, new_input_string, TinScript::kMaxTokenLength);

        // -- print the new string
        printf(new_input_string);

        // -- update the cursor
        SetCursorPosition(-1);
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

// --------------------------------------------------------------------------------------------------------------------
// TabCompleteKeywordCreate():  If the previous token was 'create', then we create based on available namespaces.
// --------------------------------------------------------------------------------------------------------------------
bool8 CCmdShell::TabCompleteHistory(const char* partial_function_name, int32& ref_tab_complete_index,
                                    const char*& tab_result)
{
    // -- sanity check
    if (partial_function_name == nullptr)
        return (false);

    // -- we're tab-completing history if the first non-white character is a '!'
    const char* partial_ptr = partial_function_name;
    while (*partial_ptr != '\0' && *partial_ptr < 0x20)
        ++partial_ptr;

    // -- if the first non-zero character is a '!', we'll try to tab-complete based on the history
    if (*partial_ptr != '!')
        return (false);

    // -- skip past the '!', and the following whitespace
    ++partial_ptr;
    while (*partial_ptr != '\0' && *partial_ptr < 0x20)
        ++partial_ptr;

    // -- if we have nothing to compare, we're done
    if (*partial_ptr == '\0')
        return (false);

    int32 partial_length = (int32)strlen(partial_ptr);

    // -- loop through the history in reverse order
    int32 matching_history_count = 0;
    const char* matching_history[kMaxHistory];
    int history_count = mHistoryFull ? kMaxHistory : mHistoryLastIndex + 1;
    for (int32 i = 0; i < history_count; ++i)
    {
        // -- look through the history buf from most recent and back
        int32 index = (mHistoryLastIndex + kMaxHistory - i) % kMaxHistory;
        if (!_strnicmp(partial_ptr, mHistory[index], partial_length))
            matching_history[matching_history_count++] = mHistory[index];
    }

    // -- if we found no matching history, we're done
    if (matching_history_count == 0)
        return (false);

    // -- update the index
    ref_tab_complete_index = (++ref_tab_complete_index) % matching_history_count;

    // -- return the matching history entry
    tab_result = matching_history[ref_tab_complete_index];

    return (true);
}

// ====================================================================================================================
// Update():  Called every frame, returns a const char* if there's a command to be processed
// ====================================================================================================================
const char* CCmdShell::Update()
{
    // -- see if we should refresh the prompt
    if (mRefreshPrompt)
    {
        // -- cache the new screen cursor position
        _CONSOLE_SCREEN_BUFFER_INFO console_info;
        if (GetConsoleScreenBufferInfo(m_screenHandle, &console_info))
            m_screenCursorPos = console_info.dwCursorPosition;

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
        if (c == -32)
        {
            special_key = true;
            c = _getch();
        }

        // -- esc
        if (!special_key && c == 27)
        {
            // -- clear the display and buffer
            RefreshConsoleInput(false, "");

            // -- reset the mHistory
            mHistoryIndex = -1;

            // -- clear the tab completion
            mTabCompletionBuf[0] = '\0';
        }

        // -- tab (complete)
        else if (!special_key && c == 9)
        {
            // -- see if we should initialize the tab completion buffer
            if (mTabCompletionBuf[0] == '\0')
            {
                TinScript::SafeStrcpy(mTabCompletionBuf, mConsoleInputBuf, TinScript::kMaxTokenLength);
                mTabCompletionIndex = -1;
            }

            // -- see if we can find a complete a function - first, tab completing history
            // -- and second, tab completing the string in context
            int32 tab_string_offset = 0;
            const char* tab_result = nullptr;
            TinScript::CFunctionEntry* fe = nullptr;
            TinScript::CVariableEntry* ve = nullptr;
            if (TabCompleteHistory(mTabCompletionBuf, mTabCompletionIndex, tab_result))
            {
                RefreshConsoleInput(false, tab_result);
            }

            else if (TinScript::GetContext()->TabComplete(mTabCompletionBuf, mTabCompletionIndex, tab_string_offset, tab_result, fe, ve))
            {
                // -- update the input buf with the new string
                int32 tab_complete_length = (int32)strlen(tab_result);

                // -- build the function prototype string
                char prototype_string[TinScript::kMaxTokenLength];

                // -- see if we are to preserve the preceeding part of the tab completion buf
                if (tab_string_offset > 0)
                    strncpy_s(prototype_string, mTabCompletionBuf, tab_string_offset);

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

                RefreshConsoleInput(false, prototype_string);
            }
        }

        // -- home
        else if (special_key && c == 71)
        {
            SetCursorPosition(0);
        }

        // -- end
        else if (special_key && c == 79)
        {
            SetCursorPosition(-1);
        }

        // -- leftarrow
        else if (special_key && c == 75)
        {
            if (m_cursorOffset > 0)
                SetCursorPosition(m_cursorOffset - 1);
        }

        // -- rightarrow
        else if (special_key && c == 77)
        {
            SetCursorPosition(m_cursorOffset + 1);
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
                // -- clear the display and buffer
                RefreshConsoleInput(false, "");

                // -- update the input buf with the new string
                RefreshConsoleInput(false, mHistory[mHistoryIndex]);

                // -- clear the tab completion
                mTabCompletionBuf[0] = '\0';
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
                // -- clear the display and buffer
                RefreshConsoleInput(false, "");

                // -- update the input buf with the new string
                RefreshConsoleInput(false, mHistory[mHistoryIndex]);

                // -- clear the tab completion
                mTabCompletionBuf[0] = '\0';
            }
        }

        // -- backspace keypress
        else if (!special_key && c == 8)
        {
            if (m_cursorOffset > 0)
            {
                --m_cursorOffset;
                DeleteCharacterAtCursor();

                // -- clear the tab completion
                mTabCompletionBuf[0] = '\0';
            }
        }

        // -- delete keypress
        else if (special_key && c == 83)
        {
            int32 count = (int32)strlen(mConsoleInputBuf);
            if (m_cursorOffset < count)
            {
                DeleteCharacterAtCursor();

                // -- clear the tab completion
                mTabCompletionBuf[0] = '\0';
            }
        }

        // -- return keypress
        else if (!special_key && c == 13)
        {
            // -- echo the input and execute it
            NotifyPrintStart();
            printf(">> %s\n", mConsoleInputBuf);
            NotifyPrintEnd();

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
            mConsoleInputBuf[0] = '\0';

            // -- clear the tab completion
            mTabCompletionBuf[0] = '\0';
        }

        // ignore any other non-printable character
        else if (!special_key && (uint32)c >= 0x20)
        {
            //RefreshConsoleInput();
            //*mInputPtr++ = c;
            //*mInputPtr = '\0';
            //printf("%c", c);
            InsertCharacterAtCursor(c);

            // -- clear the tab completion
            mTabCompletionBuf[0] = '\0';
        }
    }

    // -- return the result - only valid on the frame the return key is pressed
    return (return_value);
};

// ====================================================================================================================
// EOF
// ====================================================================================================================
