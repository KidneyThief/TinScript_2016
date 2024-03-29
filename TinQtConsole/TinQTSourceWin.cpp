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
// TinQTConsole.cpp : Defines the entry point for the console application.
//

#include <QListWidget>

#include "TinScript.h"
#include "TinRegistration.h"

#include "TinQTConsole.h"
#include "TinQTSourceWin.h"
#include "TinQTBreakpointsWin.h"
#include "mainwindow.h"

// --------------------------------------------------------------------------------------------------------------------
// -- statics
char CDebugSourceWin::mDebuggerDir[kMaxArgLength];

// ------------------------------------------------------------------------------------------------
char* ReadFileAllocBuf(const char* filename) {

	// -- open the file
	FILE* filehandle = NULL;
	if(filename) {
		 int32 result = fopen_s(&filehandle, filename, "r");
		 if (result != 0)
			 return NULL;
	}
	if(!filehandle)
		return NULL;

	// -- get the size of the file
	int32 result = fseek(filehandle, 0, SEEK_END);
	if(result != 0) {
		fclose(filehandle);
		return NULL;
	}
	int32 filesize = ftell(filehandle);
	if(filesize <= 0) {
		fclose(filehandle);
		return NULL;
	}
	fseek(filehandle, 0, SEEK_SET);

	// -- allocate a buffer and read the file into it (will null terminate)
	char* filebuf = (char*)TinAllocArray(ALLOC_FileBuf, char, filesize + 1);
	fseek(filehandle, 0, SEEK_SET);
	int32 bytesread = fread(filebuf, 1, filesize, filehandle);

    // $$$TZA for some reason, my text file is taking more space on disk than what is actually read...
	//if (bytesread != filesize) {
	if (bytesread <= 0) {
		delete[] filebuf;
		fclose(filehandle);
		return NULL;
	}
	filebuf[bytesread] = '\0';

	// -- close the file before we leave
	fclose(filehandle);

	// -- success
	return filebuf;
}

// ------------------------------------------------------------------------------------------------
CSourceLine::CSourceLine(const char* source_text, int line_number) : QListWidgetItem()
{

    QFont font;
    font.setFamily("Consolas");
    font.setFixedPitch(true);
    setFont(font);

    if (source_text == nullptr)
        source_text = "";

    // -- leave room for a 5x digit line number
    char clean_buf[256];
    snprintf(clean_buf, sizeof(clean_buf), "%5d", line_number + 1);
    char* clean_ptr = &clean_buf[strlen(clean_buf)];

    // -- put a tab's worth of spaces between the line number and the text
    *clean_ptr++ = ' ';
    *clean_ptr++ = ' ';
    *clean_ptr++ = ' ';
    *clean_ptr++ = ' ';

    // -- lets see if we can clean up the preceding spaces/tabs, hardcoding all tabs to 4x spaces
    const char* text_ptr = source_text;
    while (*text_ptr == ' ' || *text_ptr == '\t')
    {
        if (*text_ptr == ' ')
        {
            *clean_ptr++ = ' ';
        }
        else
        {
            for (int i = 0; i < 4; ++i)
                *clean_ptr++ = ' ';
        }
        ++text_ptr;
    }
    *clean_ptr = '\0';

    // -- the actual text we want then, begins with the clean_buf, and appends the rest
    QString line_text(clean_buf);
    line_text.append(text_ptr);

    setText(line_text);
    mLineNumber = line_number;
    mBreakpointSet = eBreakpointStatus::None;
    mIsPC = false;

    // -- update the icon (no breakpoint, not pc)
    UpdateIcon();
}

// --------------------------------------------------------------------------------------------------------------------
// UpdateIcon():  QListWidgetItem's only allow a single icon, so we've made 6 to cover all combinations of BP and PC
// --------------------------------------------------------------------------------------------------------------------
void CSourceLine::UpdateIcon()
{
    // -- kinda clunky - could put this in a table
    if (mBreakpointSet == eBreakpointStatus::None)
    {
        if (mIsPC)
        {
            QPixmap line_pixmap("resource/Source_blank_pc");
            QIcon line_icon(line_pixmap);
            setIcon(line_icon);
        }
        else
        {
            QPixmap line_pixmap("resource/Source_blank_blank");
            QIcon line_icon(line_pixmap);
            setIcon(line_icon);
        }
    }
    else if (mBreakpointSet == eBreakpointStatus::Disabled)
    {
        if (mIsPC)
        {
            QPixmap line_pixmap("resource/Source_breakoff_pc");
            QIcon line_icon(line_pixmap);
            setIcon(line_icon);
        }
        else
        {
            QPixmap line_pixmap("resource/Source_breakoff_blank");
            QIcon line_icon(line_pixmap);
            setIcon(line_icon);
        }
    }
    else
    {
        if (mIsPC)
        {
            QPixmap line_pixmap("resource/Source_breakon_pc");
            QIcon line_icon(line_pixmap);
            setIcon(line_icon);
        }
        else
        {
            QPixmap line_pixmap("resource/Source_breakon_blank");
            QIcon line_icon(line_pixmap);
            setIcon(line_icon);
        }
    }
}

// ------------------------------------------------------------------------------------------------
CDebugSourceWin::CDebugSourceWin(QWidget* parent) : QListWidget(parent)
{
    QFont font;
    font.setFamily("Consolas");
    font.setFixedPitch(true);
    font.setBold(false);
    setFont(font);

    // -- the source art for the icons is 48 x 24, so we'll scale that to the current font height
    float font_height = CConsoleWindow::FontHeight();
    float icon_scale = font_height / 24.0f;
    setIconSize(QSize(48 * icon_scale, 24 * icon_scale));

    mCurrentCodeblockHash = 0;
    mCurrentLineNumber = -1;

    // -- initialize the debugger directory
    mDebuggerDir[0] = '\0';

    mViewLineNumber = 0;
}

CDebugSourceWin::~CDebugSourceWin() {
    // -- clear any old text
    clear();
    while(mSourceText.size() > 0) {
        CSourceLine* line_item = mSourceText.at(0);
        mSourceText.removeAt(0);
        delete line_item;
    }
}

void CDebugSourceWin::NotifyCurrentDir(const char* cwd, const char* exe_dir)
{
    if (!cwd)
        cwd = "./";

    // -- get the length - ensure we don't copy some randomly long directory
    int length = strlen(cwd);
    if (length >= kMaxArgLength - 2)
    {
        mDebuggerDir[0] = '\0';
        return;
    }

    // -- copy the cwd
    snprintf(mDebuggerDir, sizeof(mDebuggerDir), "%s", cwd);

    // -- ensure the directory ends in a '/'
    if (mDebuggerDir[length - 1] != '/' && mDebuggerDir[length - 1] != '\\')
    {
        mDebuggerDir[length] = '/';
        mDebuggerDir[length + 1] = '\0';
    }

    // -- because communication is remote, we must be sure our string table is up to date with our target's
    TinScript::LoadStringTable(mDebuggerDir);

    // -- we don't have any need to store the target executable directory, but it may contain
    // a string table as well
    if (_stricmp(cwd, exe_dir) != 0)
    {
        TinScript::LoadStringTable(exe_dir);
    }
}

bool CDebugSourceWin::OpenSourceFile(const char* file_name, bool reload)
{
    // -- sanity check
    if(!file_name || !file_name[0])
        return (false);

    // -- see if we actually need to reload this file
    char full_path_buf[kMaxArgLength * 2];
    const char* full_path = GetFullPath(file_name, full_path_buf, kMaxArgLength * 2);
    uint32 filehash = TinScript::Hash(full_path);
    if (filehash == mCurrentCodeblockHash && !reload)
    {
        parentWidget()->raise();
        return (true);
    }
    return (OpenFullPathFile(full_path, reload));
}

bool CDebugSourceWin::OpenFullPathFile(const char* full_path, bool reload)
{
    const char* fileNamePtr = GetFileName(full_path);
    uint32 filehash = TinScript::Hash(full_path);
    char* filebuf = ReadFileAllocBuf(full_path);
    if (filebuf)
    {
        // -- set the file line edit
        CConsoleWindow::GetInstance()->GetFileLineEdit()->setText(fileNamePtr);

        // -- reset the current hash
        mCurrentCodeblockHash = 0;
        mCurrentLineNumber = -1;

        // -- clear any old text
        clear();
        mSourceText.clear();

        // -- set the hash
        mCurrentCodeblockHash = filehash;

        // --read each line of the document, and add it to the 
        int line_number = 1;
        char* filebufptr = filebuf;
        char* eol = strchr(filebufptr, '\n');
        while(eol)
        {
            *eol = '\0';
            line_number++;

            CSourceLine* list_item = new CSourceLine(filebufptr, mSourceText.size());
            addItem(list_item);
            mSourceText.append(list_item);

            filebufptr = eol + 1;
            eol = strchr(filebufptr, '\n');
        }

        if (filebufptr[0] != '\n')
        {
            CSourceLine* list_item = new CSourceLine(filebufptr, mSourceText.size());
            addItem(list_item);
            mSourceText.append(list_item);
        }

        // -- delete the buffer
        delete [] filebuf;
    }

    // -- unable to open file
    else
    {
        return (false);
    }

    // -- notify the break points window, so we can transmit all breakpoints for this file
    CConsoleWindow::GetInstance()->GetDebugBreakpointsWin()->NotifySourceFile(filehash);

    // -- ensure the source window is shown
    parentWidget()->raise();

    // -- success
    return (true);
}

// ------------------------------------------------------------------------------------------------
bool CDebugSourceWin::SetSourceView(uint32 codeblock_hash, int32 line_number, bool update_history)
{
    const char* full_path = TinScript::UnHash(codeblock_hash);
    if (full_path)
    {
        bool result = OpenSourceFile(full_path);
        if (result)
        {
            // -- set the selected line
            if (line_number >= 0 && line_number < mSourceText.size())
            {
                mSourceText.at(line_number)->setSelected(true);
                if (line_number < mSourceText.size() - 6)
                    scrollToItem(mSourceText.at(line_number + 5));
                else
                    scrollToItem(mSourceText.at(mSourceText.size() - 1));

                if (line_number >= 5)
                    scrollToItem(mSourceText.at(line_number - 5));
                else
                    scrollToItem(mSourceText.at(0));

                scrollToItem(mSourceText.at(line_number));

                // -- cache the line we're viewing
                mViewLineNumber = line_number;


                // -- update the file history
                if (update_history)
                {
                    UpdateHistory(codeblock_hash, line_number);
                }
            }
        }
        return (result);
    }
    return (false);
}

// ------------------------------------------------------------------------------------------------
void CDebugSourceWin::SetCurrentPC(uint32 codeblock_hash, int32 line_number) {
    const char* full_path = TinScript::UnHash(codeblock_hash);
    if (full_path)
    {
        bool result = OpenSourceFile(full_path);
        if (result)
        {
            // -- if we have a current line number, and it's different, we need to clear it
            if (mCurrentLineNumber >= 0 && mCurrentLineNumber != line_number)
            {
                // -- find the CSourceLine for the actual_line
                CSourceLine* source_line = mSourceText.at(mCurrentLineNumber);
                source_line->mIsPC = false;
                source_line->UpdateIcon();

                QFont font;
                font.setBold(false);
                source_line->setFont(font);
            }

            // -- now set the new current line
            if (line_number >= 0 && line_number < mSourceText.size())
            {
                mCurrentLineNumber = line_number;
                CSourceLine* source_line = mSourceText.at(mCurrentLineNumber);
                source_line->mIsPC = true;
                source_line->UpdateIcon();

                QFont font;
                font.setBold(true);
                source_line->setFont(font);

                // -- set the selected line
                SetSourceView(codeblock_hash, line_number);
            }
        }
    }
}

// ------------------------------------------------------------------------------------------------
// OnForceExecuteLineNumber():  While at a breakpoint, this will request the target
// manually set the next instruction to the line selected in the source view... extremely unsafe!
// ------------------------------------------------------------------------------------------------
void CDebugSourceWin::OnForceExecuteLineNumber()
{
    QList<QListWidgetItem*>	selected_items = selectedItems();
    if (selected_items.size() != 1)
        return;

    // -- see if we can find the line number
    bool found = false;
    int32 selected_line_number = 0;
    for (const auto& it : mSourceText)
    {
        if (it == selected_items[0])
        {
            found = true;
            break;
        }

        ++selected_line_number;
    }

    if (!found)
        return;

    static uint32 func_hash = TinScript::Hash("DebuggerForceExecToLineNumber");
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", selected_line_number);
    SocketManager::SendExec(func_hash, buf);

    // -- this is semi-fake, since we've not verified that the PC will actually be here!
    SetCurrentPC(mCurrentCodeblockHash, selected_line_number);
}

// ------------------------------------------------------------------------------------------------
void CDebugSourceWin::GoToLineNumber(int32 line_number)
{
    // -- validate the line number
    // -- note:  in code lines are counted from 0, but when coming from the user, we count from 1
    if (line_number < 1 || line_number >= mSourceText.size())
        return;

    // -- set the source view (line number - 1 to match the zero based array offset)
    SetSourceView(mCurrentCodeblockHash, line_number - 1);
}

// ------------------------------------------------------------------------------------------------
void CDebugSourceWin::FindInFile(const char* search_string)
{
    // -- ensure we have a valid search string
    if (!search_string || !search_string[0])
        return;

    // -- ensure we have something to search
    int32 line_count = mSourceText.size();
    if (line_count <= 0)
        return;

    // -- we'll use the QString class to do the search
    QString search(search_string);

    // -- start searching from the line after the current line
    int found_index = -1;
    int start_index = (mViewLineNumber + 1) % line_count;
    for (int i = 0; i < line_count; ++i)
    {
        int line_index = (i + start_index) % line_count;
        CSourceLine* line = mSourceText.at(line_index);
        if (line->text().contains(search, Qt::CaseInsensitive))
        {
            found_index = line_index;
            break;
        }
    }

    // -- if we found our line, set the source view
    char result_msg[64];
    if (found_index >= 0)
    {
        SetSourceView(mCurrentCodeblockHash, found_index);
        if (found_index < start_index)
        {
            snprintf(result_msg, sizeof(result_msg), "found: %d  wrapped", found_index + 1);
        }
        else
        {
            snprintf(result_msg, sizeof(result_msg), "found: %d", found_index + 1);
        }
    }
    else
    {
        snprintf(result_msg, sizeof(result_msg), "%s", "not found");
        CConsoleWindow::GetInstance()->GetFindResult()->setText("not found");
    }

    // -- set the result message
    CConsoleWindow::GetInstance()->GetFindResult()->setText(result_msg);
}

// ------------------------------------------------------------------------------------------------
void CDebugSourceWin::OnDoubleClicked(QListWidgetItem * item) {
    CSourceLine* source_line = static_cast<CSourceLine*>(item);
    int actual_line = -1;
    bool set_breakpoint = false;
    if (source_line->mBreakpointSet == CSourceLine::eBreakpointStatus::None)
    {
        CConsoleWindow::GetInstance()->ToggleBreakpoint(mCurrentCodeblockHash,
                                                        source_line->mLineNumber, true, true);
    }
    else
    {
        CConsoleWindow::GetInstance()->ToggleBreakpoint(mCurrentCodeblockHash,
                                                        source_line->mLineNumber, false, false);
    }
}

void CDebugSourceWin::ToggleBreakpoint(uint32 codeblock_hash, int32 line_number,
                                       bool add, bool enable) {
    // -- sanity check
    if(line_number < 0 || line_number >= mSourceText.size())
        return;

    // -- ignore, if the current source view is a different file
    if(codeblock_hash != mCurrentCodeblockHash)
        return;

    // -- find the CSourceLine for the actual_line
    CSourceLine* actual_source_line = mSourceText.at(line_number);
    actual_source_line->mBreakpointSet = add && enable ? CSourceLine::eBreakpointStatus::Enabled :
                                         add ? CSourceLine::eBreakpointStatus::Disabled :
                                         CSourceLine::eBreakpointStatus::None;
    actual_source_line->UpdateIcon();
}

void CDebugSourceWin::NotifyCodeblockLoaded(uint32 codeblock_hash)
{
    // -- get the matching filename
    const char* filename = TinScript::UnHash(codeblock_hash);

    // -- open the file in the source window, unless it's already open
    if (mCurrentCodeblockHash == 0 && mCurrentCodeblockHash != codeblock_hash)
    {
        // -- get the file name, and open the file
        if (OpenSourceFile(filename, true))
        {
            SetSourceView(codeblock_hash, 0);
        }
    }

    // -- add an action to the main menu
    char fullPath[kMaxArgLength];
    int length = snprintf(fullPath, sizeof(fullPath), "%s%s", mDebuggerDir, filename);
    if (length >= kMaxArgLength)
        return;

    // -- add an entry to the Scripts menu
    CConsoleWindow::GetInstance()->GetMainWindow()->AddScriptOpenAction(fullPath);

    // -- remove the entry from the Compile menu
    CConsoleWindow::GetInstance()->GetMainWindow()->RemoveScriptCompileAction(fullPath);
}

void CDebugSourceWin::NotifyCodeblockLoaded(const char* full_path)
{
    // -- sanity check
    if (!full_path || !full_path[0])
        return;

    // -- get the codeblock_hash
    uint32 codeblock_hash = TinScript::Hash(full_path);

    // -- do nothing if we've already got a file open,
    // -- unless we're reloading the current file
    if (mCurrentCodeblockHash == 0 && mCurrentCodeblockHash != codeblock_hash)
    {
        // -- open the file
        if (OpenSourceFile(full_path, true))
        {
            SetSourceView(codeblock_hash, 0);
        }
    }

    // -- add an entry to the Scripts menu
    CConsoleWindow::GetInstance()->GetMainWindow()->AddScriptOpenAction(full_path);
    CConsoleWindow::GetInstance()->GetMainWindow()->RemoveScriptCompileAction(full_path);
}

// ====================================================================================================================
// NotifySourceStatus():  Notification that the source file is out of date (modified and/or contains errors)
// ====================================================================================================================
void CDebugSourceWin::NotifySourceStatus(const char* source_full_path, bool has_error)
{
    // -- sanity check
    if (source_full_path == nullptr || source_full_path[0] == '\0')
        return;

    // -- if this is the file we're currently displaying, reload it
    uint32 codeblock_hash = TinScript::Hash(source_full_path);
    if (codeblock_hash == mCurrentCodeblockHash)
    {
        OpenSourceFile(source_full_path, true);
    }

    // -- if we already know this file needs to be recompiled, we don't need to spam additional warning
    if (CConsoleWindow::GetInstance()->GetMainWindow()->HasScriptCompileAction(source_full_path))
        return;

    // -- Update the "*** COMPILE" menu, which is (essentially) a list of files that
    // need to be (possibly fixed and) re-executed
    CConsoleWindow::GetInstance()->GetMainWindow()->AddScriptCompileAction(source_full_path, has_error);

    // -- if there are no errors, and this file is what we're currently displaying, we  want to reload the file,
    // with a warning to update any breakpoints, and re-execute it
    const char* file_name = CDebugSourceWin::GetFileName(source_full_path);
    if (codeblock_hash == 0 || file_name == nullptr || file_name[0] == '\0')
    {
        return;
    }

    // -- if we have an error, display a warning
    if (has_error)
    {
        ConsolePrint(1, "Source file contains error(s) and should be fixed and re-executed: %s\n", file_name);
    }
    else
    {
        if (codeblock_hash == mCurrentCodeblockHash)
        {
            ConsolePrint(1, "Source file has been modified and may need to be re-executed: %s\n", file_name);
        }
        if (CConsoleWindow::GetInstance()->GetDebugBreakpointsWin()->HasBreakpoint(codeblock_hash))
        {
            ConsolePrint(1, "Breakpoint(s) may need to be adjusted for modified file: %s\n", file_name);
        }
    }
}

// ====================================================================================================================
// UpdateHistory():  track the opening of a source file so we can move forward/back in the source view history
// ====================================================================================================================
void CDebugSourceWin::UpdateHistory(uint32 codeblock_hash, int32 line_number)
{
    // -- sanity check
    if (codeblock_hash == 0 || line_number < 0)
        return;

    // if we're not at the end of the history, then we need to pop the recent history to the current index
    if (mHistoryIndex >= 0 && mHistoryIndex < (int32)mHistoryCodeBlock.size() - 1)
    {
        int32 num_to_pop = mHistoryCodeBlock.size() - mHistoryIndex - 1;
        for (int32 i = 0; i < num_to_pop; ++i)
        {
            mHistoryCodeBlock.pop_back();
            mHistoryLineNumber.pop_back();
        }
    }

    // -- push back the new file/line location into the history, and set the index to the history end
    mHistoryCodeBlock.push_back(codeblock_hash);
    mHistoryLineNumber.push_back(line_number);
    mHistoryIndex = mHistoryCodeBlock.size() - 1;

    // -- update the buttons
    CConsoleWindow::GetInstance()->GetSourcePrevButton()->setDisabled(mHistoryIndex <= 0);
    CConsoleWindow::GetInstance()->GetSourceNextButton()->setDisabled(mHistoryIndex >= (int32)mHistoryCodeBlock.size() - 1);
}

// ====================================================================================================================
// OpenHistoryPrevious():  Opens the previous file/line in the history buffer
// ====================================================================================================================
void CDebugSourceWin::OpenHistoryPrevious()
{
    if (mHistoryIndex > 0)
    {
        mHistoryIndex--;
        SetSourceView(mHistoryCodeBlock[mHistoryIndex], mHistoryLineNumber[mHistoryIndex], false);

        // -- update the buttons
        CConsoleWindow::GetInstance()->GetSourcePrevButton()->setDisabled(mHistoryIndex <= 0);
        CConsoleWindow::GetInstance()->GetSourceNextButton()->setDisabled(mHistoryIndex >= (int32)mHistoryCodeBlock.size() - 1);
    }
}

// ====================================================================================================================
// OpenHistoryNext():  Opens the next file/line in the history buffer
// ====================================================================================================================
void CDebugSourceWin::OpenHistoryNext()
{
    if (mHistoryIndex < (int32)mHistoryCodeBlock.size() - 1)
    {
        mHistoryIndex++;
        SetSourceView(mHistoryCodeBlock[mHistoryIndex], mHistoryLineNumber[mHistoryIndex], false);

        // -- update the buttons
        CConsoleWindow::GetInstance()->GetSourcePrevButton()->setDisabled(mHistoryIndex <= 0);
        CConsoleWindow::GetInstance()->GetSourceNextButton()->setDisabled(mHistoryIndex >= (int32)mHistoryCodeBlock.size() - 1);
    }
}

// --------------------------------------------------------------------------------------------------------------------
// GetFullPath():  Return the complete full path, prepending the directory to the file name (if necessary)
// --------------------------------------------------------------------------------------------------------------------
const char* CDebugSourceWin::GetFullPath(const char* in_file_name, char* out_full_path, int in_max_length)
{
    // -- sanity check
    if (in_file_name == nullptr || out_full_path == nullptr || in_max_length <= 0)
        return (in_file_name);

    // -- if there's no current working directory, or our in_file_name is *already* prepended, simply copy
    if (mDebuggerDir[0] == '\0' ||
        _strnicmp(in_file_name, mDebuggerDir, strlen(mDebuggerDir)) == 0)
    {
        TinScript::SafeStrcpy(out_full_path, in_max_length, in_file_name);
        return (out_full_path);
    }

    // -- get the full path name, by pre-pending the current working directory (if required)
    int fn_length = strlen(in_file_name);
    int dir_length = strlen(mDebuggerDir);
    if (fn_length + dir_length > in_max_length)
    {
        return in_file_name;
    }

    TinScript::SafeStrcpy(out_full_path,in_max_length,mDebuggerDir);
    TinScript::SafeStrcpy(&out_full_path[dir_length], in_max_length - dir_length,in_file_name);

    return out_full_path;
}

// --------------------------------------------------------------------------------------------------------------------
// GetFileName():  Return just the file name, given a full path.
// --------------------------------------------------------------------------------------------------------------------
const char* CDebugSourceWin::GetFileName(const char* fullPath)
{
    if (!fullPath)
        return ("");

    // -- the filename (which must match the target's filename exactly), is the string remaining
    // -- after we strip off the mDebuggerDir
    const char* file_name_ptr = fullPath;
    const char* debug_dir_ptr = mDebuggerDir;

    // -- loop through until we find one of the strings is different.
    while (true)
    {
        // -- we're done, if one of the strings has ended
        if (*file_name_ptr == '\0' || *debug_dir_ptr == '\0')
            break;

        // -- see if both ptrs are file separators
        if (*debug_dir_ptr == '/' || *debug_dir_ptr == '\\')
        {
            if (*file_name_ptr != '/' && *file_name_ptr != '\\')
                break;
        }

        // -- see if both ptrs are the same alphabetical character (compare lower cases)
        char file_char = *file_name_ptr;
        if (file_char >= 'A' && file_char <= 'Z')
            file_char = 'a' + (file_char - 'A');
        char dir_char = *debug_dir_ptr;
        if (dir_char >= 'A' && dir_char <= 'Z')
            dir_char = 'a' + (dir_char - 'A');

        // -- if the two characters are not the same, we're done
        if (file_char != dir_char)
            break;

        // -- otherwise, advance both pointers
        ++file_name_ptr;
        ++debug_dir_ptr;
    }

    // -- return the result
    return (file_name_ptr);
}

// ------------------------------------------------------------------------------------------------
#include "TinQTSourceWinMoc.cpp"

// ------------------------------------------------------------------------------------------------
// eof
// ------------------------------------------------------------------------------------------------
