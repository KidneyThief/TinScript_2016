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
// TinQTConsole.cpp : Defines the entry point32 for the console application.
//

#include <QDockWidget>
#include <QKeyEvent>

#include "TinScript.h"
#include "TinRegistration.h"

#include "TinQTConsole.h"
#include "TinQTSourceWin.h"
#include "TinQTBreakpointsWin.h"
#include "TinQTWatchWin.h"

// ------------------------------------------------------------------------------------------------
CBreakpointEntry::CBreakpointEntry(uint32 codeblock_hash, int32 line_number, QListWidget* owner)
    : QListWidgetItem("", owner)
{
    // -- store the file/line members
    mCodeblockHash = codeblock_hash;
    mLineNumber = line_number;

    // -- clear the var watch members
    mWatchRequestID = 0;
    mWatchVarObjectID = 0;
    mWatchVarNameHash = 0;

    // -- clear the condition members
    mCondition[0] = '\0';
    mConditionEnabled = false;

    // -- clear the trace members
    mTracePoint[0] = '\0';
    mTraceEnabled = false;
    mTraceOnCondition = false;

    setFlags(flags() | Qt::ItemIsUserCheckable);

    // -- we're manually managing the checked state
    mChecked = true;
    SetCheckedState(mChecked, mConditionEnabled);
};

CBreakpointEntry::CBreakpointEntry(int32 watch_request_id, uint32 var_object_id, uint32 var_name_hash, QListWidget* owner)
    : QListWidgetItem("", owner)
{
    // -- clear the file/line members
    mCodeblockHash = 0;
    mLineNumber = 0;

    // -- set the var watch members
    mWatchRequestID = watch_request_id;
    mWatchVarObjectID = var_object_id;
    mWatchVarNameHash = var_name_hash;

    // -- clear the condition members
    mCondition[0] = '\0';
    mConditionEnabled = false;

    // -- clear the trace members
    mTracePoint[0] = '\0';
    mTraceEnabled = false;
    mTraceOnCondition = false;

    setFlags(flags() | Qt::ItemIsUserCheckable);

    // -- we're manually managing the checked state
    mChecked = true;
    SetCheckedState(mChecked, mConditionEnabled);
};

CBreakpointEntry::~CBreakpointEntry()
{
}

void CBreakpointEntry::SetCheckedState(bool8 enabled, bool8 hasCondition)
{
    if (enabled)
    {
        if (hasCondition)
            setCheckState(Qt::PartiallyChecked);
        else
            setCheckState(Qt::Checked);
    }
    else
        setCheckState(Qt::Unchecked);
}

void CBreakpointEntry::UpdateLabel(uint32 codeblock_hash, int32 line_number)
{
    // -- ensure we're not updating the label for a variable watch
    if (mWatchRequestID > 0)
        return;

    char linebuf[256];
    char spaces[6] = "     ";
    int32 space_count = line_number >= 1e5 ? 0 :
                        line_number >= 1e4 ? 1 :
                        line_number >= 1e3 ? 2 :
                        line_number >= 1e2 ? 3 :
                        line_number >= 1e1 ? 4 : 5;
    spaces[space_count] = '\0';

    // -- fill in the condition and trace lables
    bool condition_enabled = mConditionEnabled && mCondition[0];
    char condition_buf[TinScript::kMaxNameLength];
    if (condition_enabled)
        sprintf_s(condition_buf, "cond: [ %s ]", condition_enabled ? mCondition : "");
    else
        condition_buf[0] = '\0';

    bool trace_enabled = mTraceEnabled && mTracePoint[0];
    bool trace_on_condition = condition_enabled && trace_enabled && mTraceOnCondition;
    char tracepoint_buf[TinScript::kMaxNameLength];
    if (trace_on_condition)
        sprintf_s(tracepoint_buf, "c-trace: [ %s ]", mTracePoint);
    else if (trace_enabled)
        sprintf_s(tracepoint_buf, "trace: [ %s ]", mTracePoint);
    else
        tracepoint_buf[0] = '\0';

    // -- note:  all line numbers are stored accurately (0 based), but displayed +1, to match text editors
    sprintf_s(linebuf, 256, "%s : %s%d    %s    %s", TinScript::UnHash(codeblock_hash), spaces, line_number + 1,
              condition_buf, tracepoint_buf);

    // -- set the text in the QWidget
    setText(linebuf);
}

void CBreakpointEntry::UpdateLabel(int32 watch_request_id, uint32 var_object_id, uint32 var_name_hash)
{
    // -- fill in the condition and trace lables
    bool condition_enabled = mConditionEnabled && mCondition[0];
    char condition_buf[TinScript::kMaxNameLength];
    if (condition_enabled)
        sprintf_s(condition_buf, "cond: [ %s ]", condition_enabled ? mCondition : "");
    else
        condition_buf[0] = '\0';

    bool trace_enabled = mTraceEnabled && mTracePoint[0];
    bool trace_on_condition = condition_enabled && trace_enabled && mTraceOnCondition;
    char tracepoint_buf[TinScript::kMaxNameLength];
    if (trace_on_condition)
        sprintf_s(tracepoint_buf, "c-trace: [ %s ]", mTracePoint);
    else if (trace_enabled)
        sprintf_s(tracepoint_buf, "trace: [ %s ]", mTracePoint);
    else
        tracepoint_buf[0] = '\0';

    // -- set the text in the QWidget
    char label_buf[TinScript::kMaxNameLength];
    if (mWatchVarObjectID > 0)
        sprintf_s(label_buf, 256, "_watch:  %d.%s    %s    %s", mWatchVarObjectID, TinScript::UnHash(mWatchVarNameHash),
                  condition_buf, tracepoint_buf);
    else
        sprintf_s(label_buf, 256, "_watch:  %s   %s    %s", TinScript::UnHash(mWatchVarNameHash), condition_buf,
                  tracepoint_buf);

    // -- set the text in the QWidget
    setText(label_buf);
}

void CDebugBreakpointsWin::OnClicked(QListWidgetItem* item)
{
    CBreakpointEntry* breakpoint = static_cast<CBreakpointEntry*>(item);

    // -- manage the checked state
    Qt::CheckState state = breakpoint->checkState();

    // -- we're using "partial checked" to indicate
    // -- if the state is "fully checked", and we have a condition, then the user must have
    // -- clicked on it the actual check box, and we want to toggle it off
    bool enabled = (state != Qt::Unchecked);
    if (state == Qt::Checked && breakpoint->mChecked && breakpoint->mConditionEnabled)
        enabled = false;

    breakpoint->mChecked = enabled;
    breakpoint->SetCheckedState(enabled, breakpoint->mConditionEnabled);

    // -- if this is a file/line breakpoint, toggle it (affects the source view as well)
    if (breakpoint->mWatchRequestID == 0)
    {
        CConsoleWindow::GetInstance()->ToggleBreakpoint(breakpoint->mCodeblockHash,
                                                        breakpoint->mLineNumber, true, enabled);
    }

    // -- otherwise, send the toggle message directly
    else
    {
        // -- fill in the condition and trace lables
        bool condition_enabled = breakpoint->mConditionEnabled && breakpoint->mCondition[0];
        bool trace_enabled = breakpoint->mTraceEnabled && breakpoint->mTracePoint[0];
        bool trace_on_condition = condition_enabled && trace_enabled && breakpoint->mTraceOnCondition;

        // -- note:  If the trace is enabled, then we don't *break* on the variable being written, but we do
        // -- execute the trace expression
        SocketManager::SendCommandf("DebuggerToggleVarWatch(%d, %d, %d, %s, '%s', '%s', %s);",
                                    breakpoint->mWatchRequestID, breakpoint->mWatchVarObjectID,
                                    breakpoint->mWatchVarNameHash, breakpoint->mChecked ? "true" : "false",
                                    condition_enabled ? breakpoint->mCondition : "",
                                    trace_enabled ? breakpoint->mTracePoint : "",
                                    trace_on_condition ? "true" : "false");
    }
}

void CDebugBreakpointsWin::OnDoubleClicked(QListWidgetItem* item)
{
    CBreakpointEntry* breakpoint = static_cast<CBreakpointEntry*>(item);

    // -- open the source, to the filename
    if (breakpoint->mWatchRequestID == 0)
    {
        CConsoleWindow::GetInstance()->GetDebugSourceWin()->SetSourceView(breakpoint->mCodeblockHash,
                                                                          breakpoint->mLineNumber);
    }
}

void CDebugBreakpointsWin::keyPressEvent(QKeyEvent* event)
{
    if (!event)
        return;

    // -- delete the selected, if we have a selected
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace)
    {
        CBreakpointEntry* cur_item = static_cast<CBreakpointEntry*>(currentItem());
        if (cur_item)
        {
            if (cur_item->mWatchRequestID == 0)
            {
                CConsoleWindow::GetInstance()->ToggleBreakpoint(cur_item->mCodeblockHash, cur_item->mLineNumber,
                                                                false, false);
            }
            else
            {
                SocketManager::SendCommandf("DebuggerToggleVarWatch(%d, %d, %d, false, ``, ``, false);",
                                            cur_item->mWatchRequestID, cur_item->mWatchVarObjectID,
                                            cur_item->mWatchVarNameHash);

                // -- find and remove the breakpoint
                int32 found_index = -1;
                CBreakpointEntry* breakpoint = NULL;
                for(int32 i = 0; i < mBreakpoints.size(); ++i)
                {
                    CBreakpointEntry* temp = mBreakpoints.at(i);
                    if (temp->mWatchRequestID == cur_item->mWatchRequestID)
                    {
                        breakpoint = temp;
                        found_index = i;
                        break;
                    }
                }

                if (found_index >= 0)
                {
                    mBreakpoints.removeAt(found_index);
                    delete breakpoint;
                }
            }
        }
        return;
    }

    QListWidget::keyPressEvent(event);
}

CDebugBreakpointsWin::CDebugBreakpointsWin(QWidget* parent) : QListWidget(parent)
{
}

CDebugBreakpointsWin::~CDebugBreakpointsWin() {
    while(mBreakpoints.size() > 0) {
        CBreakpointEntry* temp = mBreakpoints.at(0);
        mBreakpoints.removeAt(0);
        delete temp;
    }
}

void CDebugBreakpointsWin::ToggleBreakpoint(uint32 codeblock_hash, int32 line_number, bool addOrRemove)
{
    // -- see if the breakpoint already exists
    int32 found_index = -1;
    CBreakpointEntry* breakpoint = NULL;
    for(int32 i = 0; i < mBreakpoints.size(); ++i)
    {
        CBreakpointEntry* temp = mBreakpoints.at(i);
        if (temp->mWatchRequestID == 0 && temp->mCodeblockHash == codeblock_hash && temp->mLineNumber == line_number)
        {
            breakpoint = temp;
            found_index = i;
            break;
        }
    }

    // -- if we found it, but we're supposed to delete it...
    const char* filename = TinScript::UnHash(codeblock_hash);
    if (!addOrRemove && breakpoint)
    {
        mBreakpoints.removeAt(found_index);
        delete breakpoint;

        // -- send the remove command as well
        SocketManager::SendCommandf("DebuggerRemoveBreakpoint('%s', %d);", filename, line_number);

        // -- and we're done
        return;
    }

    // -- not found - create it
    if (addOrRemove && !breakpoint)
    {
        breakpoint = new CBreakpointEntry(codeblock_hash, line_number, this);
        breakpoint->UpdateLabel(codeblock_hash, line_number);
        mBreakpoints.append(breakpoint);
        sortItems();
    }

    // -- send the message
    // -- fill in the condition and trace lables
    bool condition_enabled = breakpoint->mConditionEnabled && breakpoint->mCondition[0];
    bool trace_enabled = breakpoint->mTraceEnabled && breakpoint->mTracePoint[0];
    bool trace_on_condition = condition_enabled && trace_enabled && breakpoint->mTraceOnCondition;

    // -- note:  If the trace is enabled, then we don't *break* on hitting the breakpoint, but we do
    // -- execute the trace expression
    if (breakpoint->mChecked || trace_enabled)
    {
        SocketManager::SendCommandf("DebuggerAddBreakpoint('%s', %d, '%s', '%s', '%s', '%s');", filename, line_number,
                                    breakpoint->mChecked ? "true" : "false",
                                    condition_enabled ? breakpoint->mCondition : "",
                                    trace_enabled ? breakpoint->mTracePoint : "",
                                    trace_on_condition ? "true" : "false");
    }
    else
        SocketManager::SendCommandf("DebuggerRemoveBreakpoint('%s', %d);", filename, line_number);
}

// ====================================================================================================================
// SetCurrentBreakpoint():  Set the current breakpoint, when we hit a file/line break.
// ====================================================================================================================
void CDebugBreakpointsWin::SetCurrentBreakpoint(uint32 codeblock_hash, int32 line_number)
{
    int32 found_index = -1;
    CBreakpointEntry* breakpoint = NULL;
    for (int32 i = 0; i < mBreakpoints.size(); ++i)
    {
        CBreakpointEntry* temp = mBreakpoints.at(i);
        if (temp->mWatchRequestID == 0 && temp->mCodeblockHash == codeblock_hash && temp->mLineNumber == line_number)
        {
            breakpoint = temp;
            found_index = i;
            break;
        }
    }

    // -- set the current entry (or clear it, if the entry wasn't found)
    setCurrentItem(breakpoint);
}

// ====================================================================================================================
// SetCurrentVarWatch():  Set the current breakpoint, when we hit a variable watch.
// ====================================================================================================================
void CDebugBreakpointsWin::SetCurrentVarWatch(int32 watch_request_id)
{
    int32 found_index = -1;
    CBreakpointEntry* breakpoint = NULL;
    for (int32 i = 0; i < mBreakpoints.size(); ++i)
    {
        CBreakpointEntry* temp = mBreakpoints.at(i);
        if (temp->mWatchRequestID == watch_request_id)
        {
            breakpoint = temp;
            found_index = i;
            break;
        }
    }

    // -- set the current entry (or clear it, if the entry wasn't found)
    setCurrentItem(breakpoint);
}

// ====================================================================================================================
// SetBreakCondition():  Set/modify/disable a condition on the the currently selected break.
// ====================================================================================================================
void CDebugBreakpointsWin::SetBreakCondition(const char* expression, bool8 cond_enabled)
{
    // -- sanity check
    if (!expression)
        expression = "";

    // -- get the current entry, update the expression and enabled members
    CBreakpointEntry* cur_entry = static_cast<CBreakpointEntry*>(currentItem());
    if (cur_entry)
    {
        TinScript::SafeStrcpy(cur_entry->mCondition, sizeof(cur_entry->mCondition), expression, TinScript::kMaxNameLength);
        cur_entry->mConditionEnabled = cond_enabled;

        // -- update the label
        if (cur_entry->mWatchRequestID == 0)
            cur_entry->UpdateLabel(cur_entry->mCodeblockHash, cur_entry->mLineNumber);
        else
        {
            cur_entry->UpdateLabel(cur_entry->mWatchRequestID, cur_entry->mWatchVarObjectID,
                                   cur_entry->mWatchVarNameHash);
        }

        // -- update the check box
        cur_entry->SetCheckedState(cur_entry->mChecked, cur_entry->mConditionEnabled);

        // -- if this is a file/line breakpoint...
        if (cur_entry->mWatchRequestID == 0)
        {
            // -- toggle the breakpoint (which sends the message to the target)
            ToggleBreakpoint(cur_entry->mCodeblockHash, cur_entry->mLineNumber, true);
        }
        // -- otherwise, send the toggle message directly
        else
        {
            // -- fill in the condition and trace lables
            bool condition_enabled = cur_entry->mConditionEnabled && cur_entry->mCondition[0];
            bool trace_enabled = cur_entry->mTraceEnabled && cur_entry->mTracePoint[0];
            bool trace_on_condition = condition_enabled && trace_enabled && cur_entry->mTraceOnCondition;

            // -- note:  If the trace is enabled, then we don't *break* on the variable being written, but we do
            // -- execute the trace expression
            SocketManager::SendCommandf("DebuggerToggleVarWatch(%d, %d, %d, %s, '%s', '%s', %s);",
                                        cur_entry->mWatchRequestID, cur_entry->mWatchVarObjectID,
                                        cur_entry->mWatchVarNameHash, cur_entry->mChecked ? "true" : "false",
                                        condition_enabled ? cur_entry->mCondition : "",
                                        trace_enabled ? cur_entry->mTracePoint : "",
                                        trace_on_condition ? "true" : "false");
        }
    }
}

// ====================================================================================================================
// GetBreakCondition(): Get the break condition for the currently selected break
// ====================================================================================================================
const char* CDebugBreakpointsWin::GetBreakCondition(bool8& enabled)
{
    // -- get the current entry, update the expression and enabled members
    CBreakpointEntry* cur_entry = static_cast<CBreakpointEntry*>(currentItem());
    if (!cur_entry)
        return (NULL);

    enabled = cur_entry->mConditionEnabled;
    return (cur_entry->mCondition);
}

// ====================================================================================================================
// SetTraceExpression():  Set/modify/disable a tracepoint32 on the currently selected break.
// ====================================================================================================================
void CDebugBreakpointsWin::SetTraceExpression(const char* expression, bool8 trace_enabled, bool8 trace_on_condition)
{
    // -- sanity check
    if (!expression)
        expression = "";

    // -- get the current entry, update the expression and enabled members
    CBreakpointEntry* cur_entry = static_cast<CBreakpointEntry*>(currentItem());
    if (cur_entry)
    {
        TinScript::SafeStrcpy(cur_entry->mTracePoint, sizeof(cur_entry->mCondition), expression, TinScript::kMaxNameLength);
        cur_entry->mTraceEnabled = trace_enabled;
        cur_entry->mTraceOnCondition = trace_on_condition;

        // -- update the label
        if (cur_entry->mWatchRequestID == 0)
            cur_entry->UpdateLabel(cur_entry->mCodeblockHash, cur_entry->mLineNumber);
        else
        {
            cur_entry->UpdateLabel(cur_entry->mWatchRequestID, cur_entry->mWatchVarObjectID,
                                   cur_entry->mWatchVarNameHash);
        }

        // -- if this is a file/line breakpoint...
        if (cur_entry->mWatchRequestID == 0)
        {
            // -- toggle the breakpoint (which sends the message to the target)
            ToggleBreakpoint(cur_entry->mCodeblockHash, cur_entry->mLineNumber, true);
        }
        // -- otherwise, send the toggle message directly
        else
        {
            // -- fill in the condition and trace lables
            bool condition_enabled = cur_entry->mConditionEnabled && cur_entry->mCondition[0];
            bool trace_enabled = cur_entry->mTraceEnabled && cur_entry->mTracePoint[0];
            bool trace_on_condition = condition_enabled && trace_enabled && cur_entry->mTraceOnCondition;

            // -- note:  If the trace is enabled, then we don't *break* on the variable being written, but we do
            // -- execute the trace expression
            SocketManager::SendCommandf("DebuggerToggleVarWatch(%d, %d, %d, %s, '%s', '%s', %s);",
                                        cur_entry->mWatchRequestID, cur_entry->mWatchVarObjectID,
                                        cur_entry->mWatchVarNameHash, cur_entry->mChecked ? "true" : "false",
                                        condition_enabled ? cur_entry->mCondition : "",
                                        trace_enabled ? cur_entry->mTracePoint : "",
                                        trace_on_condition ? "true" : "false");
        }
    }
}

// ====================================================================================================================
// GetTraceExpression(): Get the tracepoint32 expression for the currently selected break
// ====================================================================================================================
const char* CDebugBreakpointsWin::GetTraceExpression(bool8& trace_enabled, bool8& trace_on_condition)
{
    // -- get the current entry, update the expression and enabled members
    CBreakpointEntry* cur_entry = static_cast<CBreakpointEntry*>(currentItem());
    if (!cur_entry)
        return (NULL);

    trace_enabled = cur_entry->mTraceEnabled;
    trace_on_condition = cur_entry->mTraceOnCondition;
    return (cur_entry->mTracePoint);
}

void CDebugBreakpointsWin::NotifyCodeblockLoaded(uint32 codeblock_hash)
{
    // -- get the file name
    const char* filename = TinScript::UnHash(codeblock_hash);

    // -- loop through all the existing breakpoints, and set the breakpoints
    for (int32 i = 0; i < mBreakpoints.size(); ++i)
    {
        CBreakpointEntry* breakpoint = mBreakpoints.at(i);
        if (breakpoint->mWatchRequestID == 0 && breakpoint->mCodeblockHash == codeblock_hash)
        {
            // -- see if the breakpoint is enabled
            bool breakpoint_enabled = breakpoint->mChecked;

            // -- notify the source window
            CConsoleWindow::GetInstance()->GetDebugSourceWin()->ToggleBreakpoint(codeblock_hash,
                                                                                 breakpoint->mLineNumber, true,
                                                                                 breakpoint_enabled);

            // -- notify the target, if the breakpoint is enabled
            bool condition_enabled = breakpoint->mConditionEnabled && breakpoint->mCondition[0];
            bool trace_enabled = breakpoint->mTraceEnabled && breakpoint->mTracePoint[0];
            bool trace_on_condition = condition_enabled && trace_enabled && breakpoint->mTraceOnCondition;
            if (breakpoint_enabled || trace_enabled)
            {
                SocketManager::SendCommandf("DebuggerAddBreakpoint('%s', %d, '%s', '%s', '%s', '%s');", filename,
                                            breakpoint->mLineNumber,
                                            breakpoint->mChecked ? "true" : "false",
                                            condition_enabled ? breakpoint->mCondition : "",
                                            trace_enabled ? breakpoint->mTracePoint : "",
                                            trace_on_condition ? "true" : "false");
            }
        }
    }
}

// ====================================================================================================================
// NotifyConfirmBreakpoint():  Notification of the actual breakable line for a requested breakpoint
// ====================================================================================================================
void CDebugBreakpointsWin::NotifyConfirmBreakpoint(uint32 codeblock_hash, int32 line_number, int32 actual_line)
{
    // -- ensure the numbers are different
    if (line_number == actual_line)
        return;

    int32 foundIndex = -1;
    CBreakpointEntry* found = NULL;
    CBreakpointEntry* alreadyExists = NULL;
    for (int32 i = 0; i < mBreakpoints.size(); ++i)
    {
        CBreakpointEntry* breakpoint = mBreakpoints.at(i);
        if (breakpoint->mCodeblockHash == codeblock_hash)
        {
            if (breakpoint->mWatchRequestID == 0 && breakpoint->mLineNumber == line_number)
            {
                found = breakpoint;
                foundIndex = i;
            }

            if (breakpoint->mLineNumber == actual_line)
                alreadyExists = breakpoint;
        }
    }

    // -- if we found our breakpoint, and one doesn't already exist, simply update the line
    if (found)
    {
        // -- clear the breakpoint from the old line
        CConsoleWindow::GetInstance()->GetDebugSourceWin()->ToggleBreakpoint(codeblock_hash, line_number,
                                                                             false, false);

        bool old_enabled = found->mChecked;
        if (!alreadyExists)
        {
            found->mLineNumber = actual_line;
            found->UpdateLabel(codeblock_hash, actual_line);

            // -- update the source window with the new breakpoint location
            CConsoleWindow::GetInstance()->GetDebugSourceWin()->ToggleBreakpoint(codeblock_hash, actual_line,
                                                                                 true, old_enabled);
        }

        // -- otherwise they both exist - simply delete the invalid breakpoint
        else
        {
            bool new_enabled = alreadyExists->mChecked;

            CBreakpointEntry* temp = mBreakpoints.at(foundIndex);
            mBreakpoints.removeAt(foundIndex);
            delete temp;

            // -- update the source window with the new breakpoint location - if either was enabled, choose enabled
            CConsoleWindow::GetInstance()->GetDebugSourceWin()->ToggleBreakpoint(codeblock_hash, actual_line,
                                                                                 true, old_enabled || new_enabled);
        }
    }
}

// ====================================================================================================================
// NotifyConfirmVarWatch():  Received in response to a variable watch request to break on write.
// ====================================================================================================================
void CDebugBreakpointsWin::NotifyConfirmVarWatch(int32 watch_request_id, uint32 watch_object_id, uint32 var_name_hash)
{
    CBreakpointEntry* found = NULL;
    CBreakpointEntry* alreadyExists = NULL;
    for (int32 i = 0; i < mBreakpoints.size(); ++i)
    {
        CBreakpointEntry* breakpoint = mBreakpoints.at(i);
        if (breakpoint->mWatchRequestID == watch_request_id && breakpoint->mWatchVarObjectID == watch_object_id &&
            breakpoint->mWatchVarNameHash == var_name_hash)
        {
            found = breakpoint;
            break;
        }
    }

    // -- if we found our breakpoint, it means we've had a duplicate watch request, simply enable it
    if (found)
    {
        found->SetCheckedState(found->mChecked, found->mConditionEnabled);
    }
    else
    {
        CBreakpointEntry* breakpoint = new CBreakpointEntry(watch_request_id, watch_object_id, var_name_hash, this);
        breakpoint->UpdateLabel(watch_request_id, watch_object_id, var_name_hash);
        mBreakpoints.append(breakpoint);
        sortItems();
    }
}

// ------------------------------------------------------------------------------------------------
void CDebugBreakpointsWin::NotifySourceFile(uint32 filehash)
{
    // -- loop through all the existing breakpoints, and set the breakpoints
    for (int32 i = 0; i < mBreakpoints.size(); ++i)
    {
        CBreakpointEntry* breakpoint = mBreakpoints.at(i);
        if (breakpoint->mCodeblockHash == filehash)
        {
            // -- notify the source window
            bool breakpoint_enabled = breakpoint->mChecked;
            CConsoleWindow::GetInstance()->GetDebugSourceWin()->ToggleBreakpoint(filehash, breakpoint->mLineNumber,
                                                                                 true, breakpoint_enabled);
        }
    }
}

// ====================================================================================================================
// NotifyOnConnect():  Called when the debugger is re-attached, to resend all active breakpoints
// ====================================================================================================================
void CDebugBreakpointsWin::NotifyOnConnect()
{
    // -- we want to re-broadcast all active breakpoints, upon reconnection
    for (int32 i = 0; i < mBreakpoints.size(); ++i)
    {
        CBreakpointEntry* breakpoint = mBreakpoints.at(i);

        bool breakpoint_enabled = breakpoint->mChecked;
        bool condition_enabled = breakpoint->mConditionEnabled && breakpoint->mCondition[0];
        bool trace_enabled = breakpoint->mTraceEnabled && breakpoint->mTracePoint[0];
        bool trace_on_condition = condition_enabled && trace_enabled && breakpoint->mTraceOnCondition;
        if ((breakpoint_enabled || trace_enabled) && breakpoint->mWatchRequestID == 0)
        {
            const char* filename = TinScript::UnHash(breakpoint->mCodeblockHash);
            SocketManager::SendCommandf("DebuggerAddBreakpoint('%s', %d, '%s', '%s', '%s', '%s');", filename,
                                        breakpoint->mLineNumber,
                                        breakpoint->mChecked ? "true" : "false",
                                        condition_enabled ? breakpoint->mCondition : "",
                                        trace_enabled ? breakpoint->mTracePoint : "",
                                        trace_on_condition ? "true" : "false");
        }

        // -- otherwise, variable watches are disabled
        else if (breakpoint->mWatchRequestID > 0)
        {
            breakpoint->setCheckState(Qt::Unchecked);
        }
    }
}

// ------------------------------------------------------------------------------------------------
CCallstackEntry::CCallstackEntry(uint32 codeblock_hash, int32 line_number, uint32 object_id,
                                 uint32 namespace_hash, uint32 function_hash) : QListWidgetItem()
{
    mCodeblockHash = codeblock_hash;
    mLineNumber = line_number;

    mObjectID = object_id;

    mNamespaceHash = namespace_hash;
    mFunctionHash = function_hash;

    char buf[2048];
    sprintf_s(buf, 2048, "[ %d ] %s::%s   %s @ %d", object_id, TinScript::UnHash(namespace_hash),
              TinScript::UnHash(function_hash), TinScript::UnHash(codeblock_hash), line_number);
    setText(buf);
};

CDebugCallstackWin::CDebugCallstackWin(QWidget* parent) : QListWidget(parent)
{
    setWindowTitle(QString("Call Stack"));
}

CDebugCallstackWin::~CDebugCallstackWin() {
    ClearCallstack();
}

void CDebugCallstackWin::ClearCallstack() {
    while(mCallstack.size()) {
        CCallstackEntry* entry = mCallstack.at(0);
        mCallstack.removeAt(0);
        delete entry;
    }
}

void CDebugCallstackWin::NotifyCallstack(uint32* codeblock_array, uint32* objid_array,
                                         uint32* namespace_array, uint32* func_array,
                                         uint32* linenumber_array, int32 array_size) {
    // -- clear the callstack
    ClearCallstack();

    // -- add each entry in the callstack
    for(int32 i = 0; i < array_size; ++i) {
        CCallstackEntry* list_item =
            new CCallstackEntry(codeblock_array[i], linenumber_array[i], objid_array[i],
                                namespace_array[i], func_array[i]);
        addItem(list_item);
        mCallstack.append(list_item);
    }

    // -- if our array is non-empty set the selected to be the top of the stack
    if(mCallstack.size() > 0)
        setCurrentItem(mCallstack.at(0));
}

void CDebugCallstackWin::OnDoubleClicked(QListWidgetItem* item)
{
    CCallstackEntry* stack_entry = static_cast<CCallstackEntry*>(item);
    CConsoleWindow::GetInstance()->GetDebugSourceWin()->SetSourceView(stack_entry->mCodeblockHash,
                                                                      stack_entry->mLineNumber);

    // -- find out which stack index this entry is, and notify the watchvar window
    for (int32 i = 0; i < mCallstack.size(); ++i)
    {
        if (mCallstack.at(i) == stack_entry)
        {
            CConsoleWindow::GetInstance()->GetDebugAutosWin()->NotifyUpdateCallstack(false);
            break;
        }
    }
}

int32 CDebugCallstackWin::GetSelectedStackEntry(uint32& func_ns_hash, uint32& func_hash, uint32& func_obj_id)
{
    if (mCallstack.size() <= 0)
        return (-1);

    QListWidgetItem* cur_item = currentItem();
    int32 stack_index = 0;
    while (stack_index < mCallstack.size() && mCallstack.at(stack_index) != cur_item)
        ++stack_index;

    // -- success
    if (stack_index < mCallstack.size())
    {
        CCallstackEntry* stack_entry = static_cast<CCallstackEntry*>(cur_item);
        func_ns_hash = stack_entry->mNamespaceHash;
        func_hash = stack_entry->mFunctionHash;
        func_obj_id = stack_entry->mObjectID;

        return (mCallstack.size() - stack_index - 1);
    }

    // -- fail
    return (-1);
}

int32 CDebugCallstackWin::GetTopStackEntry(uint32& func_ns_hash, uint32& func_hash, uint32& func_obj_id)
{
    if (mCallstack.size() <= 0)
        return (-1);

    QListWidgetItem* cur_item = mCallstack.at(0);
    CCallstackEntry* stack_entry = static_cast<CCallstackEntry*>(cur_item);
    func_ns_hash = stack_entry->mNamespaceHash;
    func_hash = stack_entry->mFunctionHash;
    func_obj_id = stack_entry->mObjectID;

    return (mCallstack.size() - 1);
}

// ====================================================================================================================
// ValidateStackEntry():  Returns true if there's a current stack entry matching the given function call attributes
// ====================================================================================================================
int32 CDebugCallstackWin::ValidateStackEntry(uint32 func_ns_hash, uint32 func_hash, uint32 func_obj_id)
{
    if (mCallstack.size() <= 0)
        return (-1);

    int32 stack_index = 0;
    while (stack_index < mCallstack.size())
    {
        const CCallstackEntry* callstack_entry = mCallstack.at(stack_index);
        if (callstack_entry && callstack_entry->mNamespaceHash == func_ns_hash &&
            callstack_entry->mFunctionHash == func_hash && callstack_entry->mObjectID == func_obj_id)
        {
            return (mCallstack.size() - stack_index - 1);
        }

        // -- not yet found
        ++stack_index;
    }

    // -- there are no callstack entries matching the watch entry
    return (-1);
}

// ------------------------------------------------------------------------------------------------
#include "TinQTBreakpointsWinMoc.cpp"

// ------------------------------------------------------------------------------------------------
// eof
// ------------------------------------------------------------------------------------------------
