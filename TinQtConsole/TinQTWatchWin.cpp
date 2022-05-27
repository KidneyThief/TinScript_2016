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
// TinQTWatchWin.cpp
//

#include <QLineEdit>
#include <QKeyEvent>

#include "TinScript.h"
#include "TinRegistration.h"

#include "TinQTConsole.h"
#include "TinQTBreakpointsWin.h"
#include "TinQTWatchWin.h"
#include "mainwindow.h"

// --------------------------------------------------------------------------------------------------------------------
// -- statics
int CDebugWatchWin::gVariableWatchRequestID = 1;  // zero is "not a dynamic var watch"

// ------------------------------------------------------------------------------------------------
// CWatchEntry

CWatchEntry::CWatchEntry(const TinScript::CDebuggerWatchVarEntry& debugger_entry, bool breakOnWrite)
    : QTreeWidgetItem()
    , mBreakOnWrite(breakOnWrite)
    , mRequestSent(false)
{
    // -- copy the debugger entry
    memcpy(&mDebuggerEntry, &debugger_entry, sizeof(TinScript::CDebuggerWatchVarEntry));

    // -- the text is displayed differently...
    // -- if this is a namespace label
    if (mDebuggerEntry.mObjectID > 0 && mDebuggerEntry.mNamespaceHash > 0 &&
        mDebuggerEntry.mType == TinScript::TYPE_void)
    {
        setText(0, mDebuggerEntry.mVarName);
        setText(1, "Namespace");
        setText(2, mDebuggerEntry.mValue);
    }

    // -- otherwise, it's a real entry
    else
    {
        UpdateDisplay();
    }
}

CWatchEntry::~CWatchEntry()
{
}

void CWatchEntry::UpdateType(TinScript::eVarType type, int32 array_size)
{
	mDebuggerEntry.mType = type;
    mDebuggerEntry.mArraySize = array_size;
    UpdateDisplay();
}

void CWatchEntry::UpdateArrayVar(uint32 var_array_id, int32 array_size)
{
    mDebuggerEntry.mSourceVarID = var_array_id;
    mDebuggerEntry.mArraySize = array_size;
    UpdateDisplay();
}

void CWatchEntry::UpdateValue(const char* new_value)
{
    if (!new_value)
        new_value = "";

    strcpy_s(mDebuggerEntry.mValue, new_value);
    UpdateDisplay();
}

void CWatchEntry::UpdateDisplay()
{
    // -- set the text
    setText(0, mDebuggerEntry.mVarName);

    // -- array variable entries don't have values - their "children" do...
    if (mDebuggerEntry.mArraySize <= 1)
    {
        if (mDebuggerEntry.mType != TinScript::TYPE_void)
            setText(1, TinScript::GetRegisteredTypeName(mDebuggerEntry.mType));
        else
            setText(1, "");

        const char* value = mDebuggerEntry.mValue;
        if (mDebuggerEntry.mType == TinScript::TYPE_object && mDebuggerEntry.mVarObjectID == 0)
        {
            value = "<invalid>";
        }

        // -- if this is set to value_text above, setText() had better be making a copy!
        setText(2, value);
    }
    // -- to display an array, the "type" is array, and the "value" is the arraytype[size] (e.g.  int[5])
    else
    {
        setText(1, "<array>");
        char array_type_size[32];
        sprintf_s(array_type_size, "%s[%d]", TinScript::GetRegisteredTypeName(mDebuggerEntry.mType),
            mDebuggerEntry.mArraySize);
        setText(2, array_type_size);
    }
}

// ------------------------------------------------------------------------------------------------
// CDebugWatchWin
CDebugWatchWin::CDebugWatchWin(QWidget* parent) : QTreeWidget(parent)
{
    setColumnCount(3);
    setItemsExpandable(true);
    setExpandsOnDoubleClick(true);

    // -- set the header
  	mHeaderItem = new QTreeWidgetItem();
	mHeaderItem->setText(0,QString("Variable"));
	mHeaderItem->setText(1,QString("Type"));
	mHeaderItem->setText(2,QString("Value"));
	setHeaderItem(mHeaderItem);
}

CDebugWatchWin::~CDebugWatchWin()
{
    delete mHeaderItem;
    mWatchList.clear();
    clear();
}

void CDebugWatchWin::UpdateReturnValueEntry(const TinScript::CDebuggerWatchVarEntry& watch_var_entry)
{
    // -- find the current "return value" entry
    static uint32 _return_hash = TinScript::Hash("__return");
    int entry_index = 0;
    while (entry_index < mWatchList.size())
    {
        CWatchEntry* entry = mWatchList.at(entry_index);
        if (entry->mDebuggerEntry.mFuncNamespaceHash == 0 &&
            entry->mDebuggerEntry.mFunctionHash == 0 &&
            entry->mDebuggerEntry.mFunctionObjectID == 0 &&
            entry->mDebuggerEntry.mVarHash == _return_hash)
        {
            // -- update the type and value, including clearing children if needed
            if (entry->mDebuggerEntry.mType == TinScript::TYPE_object &&
                watch_var_entry.mType != TinScript::TYPE_object)
            {
                RemoveWatchVarChildren(entry_index);
            }

            // -- update the type (it may have been undetermined)
            entry->UpdateType(watch_var_entry.mType, watch_var_entry.mArraySize);

            // -- if the type is a variable, copy the object ID as well
            if (watch_var_entry.mType == TinScript::TYPE_object)
            {
                // -- if we're changing objects, we need to delete the children
                if (entry->mDebuggerEntry.mVarObjectID != watch_var_entry.mVarObjectID)
                    RemoveWatchVarChildren(entry_index);

                // -- set the new object ID
                entry->mDebuggerEntry.mVarObjectID = watch_var_entry.mVarObjectID;
            }

            // -- if the watch is an array, then we're going to be receiving the array entries
            if (entry->mDebuggerEntry.mArraySize > 1)
            {
                entry->UpdateArrayVar(entry->mDebuggerEntry.mSourceVarID, entry->mDebuggerEntry.mArraySize);
            }

			// -- update the value
			entry->UpdateValue(watch_var_entry.mValue);

            // -- and we're done
            return;
        }
        else
        {
            ++entry_index;
        }
    }

    // -- we didn't already find it - add it
    CWatchEntry* new_entry = new CWatchEntry(watch_var_entry);
    new_entry->mIsTopLevel = true;
    addTopLevelItem(new_entry);
    mWatchList.insert(0, new_entry);
}

bool CDebugWatchWin::IsTopLevelEntry(const CWatchEntry& watch_var_entry)
{
    // -- ensure this is a top level watch
    return (watch_var_entry.mIsTopLevel);
}

void CDebugWatchWin::AddTopLevelEntry(const TinScript::CDebuggerWatchVarEntry& watch_var_entry, bool update_only)
{
    // -- find out what function call is currently selected on the stack
    uint32 cur_func_ns_hash = 0;
    uint32 cur_func_hash = 0;
    uint32 cur_func_object_id = 0;
    int32 current_stack_index = CConsoleWindow::GetInstance()->GetDebugCallstackWin()->
                                                               GetSelectedStackEntry(cur_func_ns_hash, cur_func_hash,
                                                                                     cur_func_object_id);
    if (current_stack_index < 0)
    {
        return;
    }

    // -- also get the full execution stack depth, and calculate the selected "depth from the bottom"
    // all watch entry "stack depths" are from the bottom, not the top
    uint32 top_func_ns_hash = 0;
    uint32 top_func_hash = 0;
    uint32 top_func_object_id = 0;
    int32 execution_stack_depth = CConsoleWindow::GetInstance()->GetDebugCallstackWin()->
                                                                 GetTopStackEntry(top_func_ns_hash, top_func_hash,
                                                                                  top_func_object_id);

    int32 selected_depth_from_bottom = execution_stack_depth - current_stack_index - 1;

    // -- "_return" is special, as it's the value returned by th last function call, and not
    // -- part of any individual stack
    static uint32 _return_hash = TinScript::Hash("__return");
    if (watch_var_entry.mFuncNamespaceHash == 0 && watch_var_entry.mFunctionHash == 0 && 
        watch_var_entry.mFunctionObjectID == 0 && watch_var_entry.mVarHash == _return_hash)
    {
        UpdateReturnValueEntry(watch_var_entry);
        return;
    }

    // -- loop through the watch entries, and every instance of matching watch entry,
    // -- update the value (if it's not an object)
    bool found_callstack_entry = false;
    int entry_index = 0;
    while (entry_index < mWatchList.size())
    {
        // -- in order to find a matching entry, *either* the entry needs to be
        // -- a local variable with a matching stack index:
        // -- no objectID (not a member), and matching func_ns, func_hash, object_id *and* the type and varhash
        CWatchEntry* entry = mWatchList.at(entry_index);
        if (entry->mDebuggerEntry.mObjectID == 0)
        {
            // -- see if this entry matches an existing top level stack variable
            // note:  we might be able to simplify all of this with the mSourceVarID,
            // but the stack offset is still relevant...
            // a recursive call using an array param will have the same mSourceVarID, but different stack offsets
            if (entry->mDebuggerEntry.mFuncNamespaceHash == watch_var_entry.mFuncNamespaceHash &&
                entry->mDebuggerEntry.mFunctionHash == watch_var_entry.mFunctionHash &&
                entry->mDebuggerEntry.mFunctionObjectID == watch_var_entry.mFunctionObjectID &&
                entry->mDebuggerEntry.mType == watch_var_entry.mType && 
                entry->mDebuggerEntry.mVarHash == watch_var_entry.mVarHash &&
                entry->mDebuggerEntry.mSourceVarID == watch_var_entry.mSourceVarID &&
                (entry->mDebuggerEntry.mStackOffsetFromBottom == watch_var_entry.mStackOffsetFromBottom ||
                 entry->mDebuggerEntry.mWatchRequestID > 0))
            {
                // -- update the value (if it's not a label)
                if (entry->mDebuggerEntry.mType != TinScript::TYPE_void)
                {
                    // -- if the entry is for an object, update the object ID as well
                    if (entry->mDebuggerEntry.mType == TinScript::TYPE_object)
                    {
                        // -- if we're changing our object ID, clear the old members out
                        if (entry->mDebuggerEntry.mVarObjectID != watch_var_entry.mVarObjectID)
                            RemoveWatchVarChildren(entry_index);

                        // -- update the object ID
                        entry->mDebuggerEntry.mVarObjectID = watch_var_entry.mVarObjectID;
                    }

                    // -- if the watch is an array, then we're going to be receiving the array entries
                    if (entry->mDebuggerEntry.mArraySize > 1)
                    {
                        entry->UpdateArrayVar(entry->mDebuggerEntry.mSourceVarID, entry->mDebuggerEntry.mArraySize);
                    }

                    // -- update the value (text label)
                    entry->UpdateValue(watch_var_entry.mValue);
                }

                // -- set the bool - we found the matching entry
                found_callstack_entry = true;
            }
        }

        // -- increment the index
        ++entry_index;
    }

    if (!found_callstack_entry && !update_only)
    {
        CWatchEntry* new_entry = new CWatchEntry(watch_var_entry);
        new_entry->mIsTopLevel = true;
        addTopLevelItem(new_entry);
        mWatchList.append(new_entry);
        bool hidden = (watch_var_entry.mStackOffsetFromBottom != selected_depth_from_bottom &&
                       watch_var_entry.mWatchRequestID == 0);
        if (hidden)
        {
            new_entry->setHidden(hidden);
        }
    }
}

void CDebugWatchWin::AddObjectMemberEntry(const TinScript::CDebuggerWatchVarEntry& watch_var_entry)
{
    // -- find out what function call is currently selected on the stack
    uint32 cur_func_ns_hash = 0;
    uint32 cur_func_hash = 0;
    uint32 cur_func_object_id = 0;
    int32 current_stack_index = CConsoleWindow::GetInstance()->GetDebugCallstackWin()->
                                                               GetSelectedStackEntry(cur_func_ns_hash, cur_func_hash,
                                                                                     cur_func_object_id);
    if (current_stack_index < 0)
    {
        return;
    }

    // -- loop through the watch entries, and every instance of a watch entry for the given object ID, ensure
    // -- it has a label
    int entry_index = 0;
    while (entry_index < mWatchList.size())
    {
        // -- find the object entry
        CWatchEntry* obj_entry = NULL;
        while (entry_index < mWatchList.size())
        {
            CWatchEntry* entry = mWatchList.at(entry_index);
            if (entry->mDebuggerEntry.mType == TinScript::TYPE_object &&
                entry->mDebuggerEntry.mVarObjectID == watch_var_entry.mObjectID)
            {
                // -- increment the index - we want to start looking for the member/label after the object entry
                ++entry_index;

                obj_entry = entry;
                break;
            }

            // -- not yet found - increment the index
            else
                ++entry_index;
        }

        // -- if we did not find the entry, we have no parent entry to add a namespace label
        if (!obj_entry)
            break;

        // -- otherwise, now see if we have a label 
        CWatchEntry* member_entry = NULL;
        while (entry_index < mWatchList.size())
        {
            CWatchEntry* entry = mWatchList.at(entry_index);
            if (entry->mDebuggerEntry.mObjectID == watch_var_entry.mObjectID &&
                entry->mDebuggerEntry.mType == watch_var_entry.mType &&
                (entry->mDebuggerEntry.mType != TinScript::TYPE_void ||
                 entry->mDebuggerEntry.mNamespaceHash == watch_var_entry.mNamespaceHash) &&
                entry->mDebuggerEntry.mVarHash == watch_var_entry.mVarHash)
            {
                member_entry = entry;
                break;
            }

            // -- else if we've moved on to a different object, we're done
            else if (entry->mDebuggerEntry.mObjectID != watch_var_entry.mObjectID)
            {
                break;
            }

            // -- not yet found - increment the index
            else
                ++entry_index;
        }

        // -- if we didn't find a label, add one
        if (member_entry == NULL)
        {
            CWatchEntry* ns = new CWatchEntry(watch_var_entry);
            obj_entry->addChild(ns);

            if (entry_index >= mWatchList.size())
            {
                mWatchList.append(ns);
            }
            else
            {
                mWatchList.insert(mWatchList.begin() + entry_index, ns);
            }

            // -- now see if the label should be visible
            // -- either it's not from a function call, or it's from the current callstack function call
            if (!(watch_var_entry.mFuncNamespaceHash == 0 ||
                    (watch_var_entry.mFuncNamespaceHash == cur_func_ns_hash &&
                    watch_var_entry.mFunctionHash == cur_func_hash &&
                    watch_var_entry.mFunctionObjectID == cur_func_object_id)))
            {
                ns->setHidden(true);
            }

            // -- we want to increment the index, to account for the inserted entry
            ++entry_index;
        }

        // -- otherwise, simply update it's value
        else if (member_entry->mDebuggerEntry.mType != TinScript::TYPE_void)
        {
            // -- if the entry is for an object, update the object ID as well
            if (member_entry->mDebuggerEntry.mType == TinScript::TYPE_object)
            {
                // -- if we're changing our object ID, clear the old members out
                if (member_entry->mDebuggerEntry.mVarObjectID != watch_var_entry.mVarObjectID)
                    RemoveWatchVarChildren(entry_index);

                // -- update the object ID
                member_entry->mDebuggerEntry.mVarObjectID = watch_var_entry.mVarObjectID;
            }

            // -- if the watch is an array, then we're going to be receiving the array entries
            if (member_entry->mDebuggerEntry.mArraySize > 1)
            {
                member_entry->UpdateArrayVar(member_entry->mDebuggerEntry.mSourceVarID,
                                             member_entry->mDebuggerEntry.mArraySize);
            }

            member_entry->UpdateValue(watch_var_entry.mValue);
        }
    }
}

// ====================================================================================================================
// FindTopLevelWatchEntry():  Finds a watch entry for the given expression that is *not* a child of another entry
// ====================================================================================================================
CWatchEntry* CDebugWatchWin::FindTopLevelWatchEntry(const char* watch_expr)
{
    // -- sanity check
    if (watch_expr == nullptr || watch_expr[0] == '\0')
        return (nullptr);

    // -- find out what function call is currently selected on the stack
    uint32 cur_func_ns_hash = 0;
    uint32 cur_func_hash = 0;
    uint32 cur_func_object_id = 0;
    int32 current_stack_index = CConsoleWindow::GetInstance()->GetDebugCallstackWin()->
                                                               GetSelectedStackEntry(cur_func_ns_hash,cur_func_hash,
                                                                                     cur_func_object_id);
    
    int32 entry_index = 0;
    while (entry_index < mWatchList.size())
    {
        // -- ensure this is a top level watch (mObjectID == 0)
        // $$$TZA we'll need to check other things like watches for array entries...
        CWatchEntry* entry = mWatchList.at(entry_index);
        if (entry->mDebuggerEntry.mObjectID == 0)
        {
            if (_stricmp(entry->mDebuggerEntry.mVarName, watch_expr) == 0)
            {
                return (entry);
            }
        }
        ++entry_index;
    }

    // -- not found
    return (nullptr);
}

// ====================================================================================================================
// AddVariableWatch():  Dynamically add a watch to be updated by the debugger
// ====================================================================================================================
void CDebugWatchWin::AddVariableWatch(const char* variableWatch, bool breakOnWrite)
{
	if (!variableWatch || !variableWatch[0])
		return;

    // -- before we create a new variable watch, see if we have one that already matches exactly
    CWatchEntry* found = FindTopLevelWatchEntry(variableWatch);
    if (found)
    {
        found->mBreakOnWrite = breakOnWrite;
        ResendVariableWatch(found, true);
        setCurrentItem(found);
        return;
    }

    // -- ensure this window is the top level (visible in front of the other docked widgets)
    parentWidget()->show();
    parentWidget()->raise();

	// -- the variable name for a watch is the expression requested
	TinScript::CDebuggerWatchVarEntry new_watch;

	// -- set the request ID, so if/when we receive an update from the target, we'll know what it
	// -- is in response to
	new_watch.mWatchRequestID = gVariableWatchRequestID++;
    new_watch.mStackOffsetFromBottom = -1;

	new_watch.mFuncNamespaceHash = 0;
    new_watch.mFunctionHash = 0;
    new_watch.mFunctionObjectID = 0;

    new_watch.mObjectID = 0;
    new_watch.mNamespaceHash = 0;

	// -- we *hope* the target can identify the expression and fill in the type
    new_watch.mType = TinScript::TYPE_void;

	// -- var name holds the expression
	TinScript::SafeStrcpy(new_watch.mVarName, sizeof(new_watch.mVarName), variableWatch, kMaxNameLength);

	// -- we also *hope* the target can fill in a value for us as well
    new_watch.mValue[0] = '\0';

	// -- also to be filled in by a response from the target
	new_watch.mVarHash = 0;
    new_watch.mVarObjectID = 0;

	// -- we're allowed *anything* including duplicates when adding variable watches
	CWatchEntry* new_entry = new CWatchEntry(new_watch, breakOnWrite);
    addTopLevelItem(new_entry);
    new_entry->mIsTopLevel = true;
    mWatchList.append(new_entry);

    // -- send the request to the target
    // note:  complex (e.g. function call) expressions can't be evaluated unless at a breakpoint
    ResendVariableWatch(new_entry, true);

    // -- as this is a new (or duplicated) watch, we want to see it in the window
    setCurrentItem(new_entry);
}

// ====================================================================================================================
// ResendVariableWatch():  (Re)send the debugger command for a given watch entry
// ====================================================================================================================
void CDebugWatchWin::ResendVariableWatch(CWatchEntry* watch_entry, bool allow_break_on_write)
{
    // -- sanity check
    if (watch_entry == nullptr)
        return;

    // -- send the request to the target, if we're currently in a break point
    SocketManager::SendCommandf("DebuggerAddVariableWatch(%d, `%s`, `%s`);",
                                watch_entry->mDebuggerEntry.mWatchRequestID,
                                watch_entry->mDebuggerEntry.mVarName,
                                allow_break_on_write && watch_entry->mBreakOnWrite ? "true" : "false");
    watch_entry->mRequestSent = true;
}

// ====================================================================================================================
// GetSelectedWatchExpression():  if the watch window is focused, we use its selection to populate the new var watch
// ====================================================================================================================
bool CDebugWatchWin::GetSelectedWatchExpression(int32& out_use_watch_id, char* out_watch_string, int32 max_expr_length,
                                                char* out_value_string, int32 max_value_length)
{
    CWatchEntry* cur_item = static_cast<CWatchEntry*>(currentItem());
    if (cur_item && cur_item->mDebuggerEntry.mType != TinScript::TYPE_void)
    {
        out_use_watch_id = cur_item->mDebuggerEntry.mWatchRequestID;

        // -- see if we have a variable or a member
        if (cur_item->mDebuggerEntry.mObjectID > 0)
            snprintf(out_watch_string, max_expr_length, "%d.%s", cur_item->mDebuggerEntry.mObjectID,cur_item->mDebuggerEntry.mVarName);
        else
            snprintf(out_watch_string, max_expr_length, cur_item->mDebuggerEntry.mVarName);

        // -- set the current value
        snprintf(out_value_string, max_value_length, "%s", cur_item->mDebuggerEntry.mValue);

        return true;
    }

    // -- failed
    return false;
}

// ====================================================================================================================
// CreateSelectedWatch():  Dynamically add a watch to be updated by the debugger
// ====================================================================================================================
/*
void CDebugWatchWin::CreateSelectedWatch()
{
    CWatchEntry* cur_item = static_cast<CWatchEntry*>(currentItem());
    if (cur_item && cur_item->mDebuggerEntry.mType != TinScript::TYPE_void)
    {
        // -- create the string to use as a watch
        char watch_string[kMaxNameLength];
        char cur_value[kMaxNameLength];

        // -- see if we have a variable or a member
        if (cur_item->mDebuggerEntry.mObjectID > 0)
            sprintf_s(watch_string, "%d.%s", cur_item->mDebuggerEntry.mObjectID, cur_item->mDebuggerEntry.mVarName);
        else
            snprintf(watch_string, sizeof(watch_string), "%s", cur_item->mDebuggerEntry.mVarName);

        // -- set the current value
        snprintf(cur_value, sizeof(cur_value), "%s", cur_item->mDebuggerEntry.mValue);

        CConsoleWindow::GetInstance()->GetMainWindow()->CreateVariableWatch(cur_item->mDebuggerEntry.mWatchRequestID,
                                                                            watch_string, cur_value);
    }
}
*/

// ====================================================================================================================
// GetSelectedObjectID(): Used for creating an ObjectInspector, if the selected enty is a variable of type object.
// ====================================================================================================================
uint32 CDebugWatchWin::GetSelectedObjectID()
{
    CWatchEntry* cur_item = static_cast<CWatchEntry*>(currentItem());
    if (cur_item && cur_item->mDebuggerEntry.mType != TinScript::TYPE_void)
    {
        // -- see if we have a variable or a member
        if (cur_item->mDebuggerEntry.mVarObjectID > 0)
            return (cur_item->mDebuggerEntry.mVarObjectID);
    }

    // -- no current objects selected
    return (0);
}

// ====================================================================================================================
// NotifyOnConnect():  Initialization when the IDE becomes connected to the target
// ====================================================================================================================
void CDebugWatchWin::NotifyOnConnect()
{
    // -- on connect, we want to clear all autos (leave the user watches)
    ClearWatchWin(false);
}

// ====================================================================================================================
// ClearWatchWin():  Clears the display and the array of watches
// ====================================================================================================================
void CDebugWatchWin::ClearWatchWin(bool clear_user_watches)
{
    // -- if we're clearing user watches, then we're clearing everything!
    if (clear_user_watches)
    {
        mWatchList.clear();
        clear();
    }

    // -- otherwise we need to remove only autos
    else
    {
        // -- loop through all watches
        int entry_index = 0;
        while (entry_index < mWatchList.size())
        {
            bool removed = false;
            CWatchEntry* entry = mWatchList.at(entry_index);

            // -- we only remove the non-requested watches...
            // note:  it should be impossible while iterating, to reach a non-topLevelEntry
            if (entry->mDebuggerEntry.mWatchRequestID <= 0 && IsTopLevelEntry(*entry))
            {
                // -- remove the children of this entry
                RemoveWatchVarChildren(entry_index);

                // -- remove this entry
                mWatchList.removeAt(entry_index);

                // -- now delete the entry (and leave the entry_index)
                delete entry;
            }
            else
            {
                ++entry_index;
            }
        }
    }
}

// ====================================================================================================================
// RemoveEntryChildren():  Used when an object variable watch points at a different object
// ====================================================================================================================
void CDebugWatchWin::RemoveWatchVarChildren(int32 object_entry_index)
{
    // -- sanity check
    if (object_entry_index < 0 || object_entry_index >= mWatchList.size())
        return;
    CWatchEntry* parent_entry = mWatchList.at(object_entry_index);

    // -- remove the children from the Qt list
    while (parent_entry->childCount() > 0)
        parent_entry->removeChild(parent_entry->child(0));

    // -- remove all children if the parent watch entry is for an object
    int32 object_id = parent_entry->mDebuggerEntry.mVarObjectID;
    if (object_id > 0)
    {
        // -- remove the children from the watch list
        int entry_index = object_entry_index + 1;
        while (entry_index < mWatchList.size())
        {
            CWatchEntry* child_entry = mWatchList.at(entry_index);
            if (child_entry->mDebuggerEntry.mObjectID == object_id)
            {
                mWatchList.removeAt(entry_index);
                delete child_entry;
            }

            // -- otherwise we're done
            else
            {
                break;
            }
        }
    }

    // -- else if the entry is for an array
    // $$$TZA We need a better way to identify children of a parent watch - this is redundant code...
    else
    {
        int32 var_array_id = parent_entry->mDebuggerEntry.mSourceVarID;
        if (var_array_id > 0 && parent_entry->mDebuggerEntry.mArraySize > 1)
        {
            // -- remove the children from the watch list
            int entry_index = object_entry_index + 1;
            while (entry_index < mWatchList.size())
            {
                CWatchEntry* child_entry = mWatchList.at(entry_index);
                if (child_entry->mDebuggerEntry.mSourceVarID == var_array_id)
                {
                    mWatchList.removeAt(entry_index);
                    delete child_entry;
                }

                // -- otherwise we're done
                else
                {
                    break;
                }
            }
        }
    }
}

// ====================================================================================================================
// NotifyWatchVarEntry():  received from the connected target, an auto or watch expression response
// ====================================================================================================================
void CDebugWatchWin::NotifyWatchVarEntry(TinScript::CDebuggerWatchVarEntry* watch_var_entry, bool update_only)
{
	// -- sanity check
    if (!watch_var_entry)
        return;

    // -- if we're adding a namespace label
    if (watch_var_entry->mObjectID > 0)
    {
        AddObjectMemberEntry(*watch_var_entry);
    }

    // -- else see if we're adding a top level entry
    else if (watch_var_entry->mObjectID == 0 && watch_var_entry->mType != TinScript::TYPE_void)
    {
        AddTopLevelEntry(*watch_var_entry, update_only);
    }
}

// ====================================================================================================================
// NotifyVarWatchResponse():  Update received from the target in response to a dynamic variable watch.
// ====================================================================================================================
void CDebugWatchWin::NotifyVarWatchResponse(TinScript::CDebuggerWatchVarEntry* watch_var_entry)
{
	// -- sanity check
	if (!watch_var_entry || watch_var_entry->mWatchRequestID <= 0)
        return;

	// -- find the entry
    int entry_index = 0;
    while (entry_index < mWatchList.size())
    {
		CWatchEntry* entry = mWatchList.at(entry_index);
        if (entry->mDebuggerEntry.mWatchRequestID == watch_var_entry->mWatchRequestID)
		{

            // -- if this entry is a matching object, then this is a member of that object
            // note:  mVarObjectID is the value of the object...
            // a watch entry with an mObjectID means it's a member of that value...
            if (entry->mDebuggerEntry.mType == TinScript::TYPE_object &&
                watch_var_entry->mObjectID > 0 && entry->mDebuggerEntry.mVarObjectID == watch_var_entry->mObjectID)
            {
                NotifyVarWatchMember(entry_index, watch_var_entry);
            }

            // -- else this response is the value of a top level watch request
            else
            {
                // -- update the type if undetermined
                if (entry->mDebuggerEntry.mType == TinScript::eVarType::TYPE_void)
                {
                    entry->UpdateType(watch_var_entry->mType, watch_var_entry->mArraySize);
                }

                // -- if the type used to be an object, and the new type is something else, we
                // -- need to remove the children
                if (entry->mDebuggerEntry.mType == TinScript::TYPE_object &&
                    watch_var_entry->mType != TinScript::TYPE_object)
                {
                    RemoveWatchVarChildren(entry_index);
                }

                // -- watch entries are contextual - copy the source of the variable
                entry->mDebuggerEntry.mFuncNamespaceHash = watch_var_entry->mFuncNamespaceHash;
                entry->mDebuggerEntry.mFunctionHash = watch_var_entry->mFunctionHash;
                entry->mDebuggerEntry.mFunctionObjectID = watch_var_entry->mFunctionObjectID;
                entry->mDebuggerEntry.mVarHash = watch_var_entry->mVarHash;

                // -- if the type is a variable, copy the object ID as well
                if (watch_var_entry->mType == TinScript::TYPE_object)
                {
                    // -- if we're changing objects, we need to delete the children
                    if (entry->mDebuggerEntry.mVarObjectID != watch_var_entry->mVarObjectID)
                        RemoveWatchVarChildren(entry_index);

                    // -- set the new object ID
                    entry->mDebuggerEntry.mVarObjectID = watch_var_entry->mVarObjectID;
                }

                // -- if the watch is an array, then we're going to be receiving the array entries
                if (watch_var_entry->mArraySize > 1)
                {
                    entry->UpdateArrayVar(watch_var_entry->mSourceVarID, watch_var_entry->mArraySize);
                }

			    // -- update the value
			    entry->UpdateValue(watch_var_entry->mValue);
            }

			// -- we're done (each watch request can only ever have one response)
			return;
		}

        // -- next entry
        ++entry_index;
	}
}

// ====================================================================================================================
// NotifyVarWatchMember():  A member update received from the target in response to a dynamic variable watch.
// ====================================================================================================================
void CDebugWatchWin::NotifyVarWatchMember(int32 parent_entry_index, TinScript::CDebuggerWatchVarEntry* watch_var_entry)
{
	// -- sanity check
    if (!watch_var_entry || watch_var_entry->mWatchRequestID <= 0 || watch_var_entry->mObjectID <= 0 ||
        parent_entry_index < 0 || parent_entry_index >= mWatchList.size())
    {
        return;
    }

	CWatchEntry* parent_entry = mWatchList.at(parent_entry_index);
    int entry_index = parent_entry_index + 1;
    while (entry_index < mWatchList.size())
    {
		CWatchEntry* entry = mWatchList.at(entry_index);
        if (entry->mDebuggerEntry.mWatchRequestID == watch_var_entry->mWatchRequestID &&
            watch_var_entry->mObjectID == parent_entry->mDebuggerEntry.mVarObjectID)
		{
            // -- see if this for the same member
            if (entry->mDebuggerEntry.mVarHash == watch_var_entry->mVarHash)
            {
			    // -- update the value, and we're done
			    entry->UpdateValue(watch_var_entry->mValue);
                return;
            }
		}
        else
        {
            break;
        }

        // -- next entry
        ++entry_index;
	}

    // -- if we'd have found an entry, we'd have updated it - at this point we need to insert an entry
    // -- right before entry_index
    CWatchEntry* ns = new CWatchEntry(*watch_var_entry);
    parent_entry->addChild(ns);
    if (entry_index >= mWatchList.size())
        mWatchList.append(ns);
    else
        mWatchList.insert(mWatchList.begin() + entry_index, ns);
}

// ====================================================================================================================
// NotifyArrayEntry():  an array entry has been received... this is essentially a child of an array var
// ====================================================================================================================
void CDebugWatchWin::NotifyArrayEntry(int32 watch_request_id, int32 stack_offset_bottom, uint32 array_var_id,
                                      int32 array_index, const char* value_str)
{
    // -- loop through all watch entries, each parent, and ensure it has a child
    int entry_index = 0;
    while (entry_index < mWatchList.size())
    {
        // this is the parent if either we have a matching non-zero request ID, or
        // the varID and the stack offset are the same
        CWatchEntry* parent_entry = mWatchList.at(entry_index);
        if (parent_entry->mDebuggerEntry.mArraySize > 1 &&
            parent_entry->mDebuggerEntry.mWatchRequestID == watch_request_id &&
            (watch_request_id > 0 ||
             (parent_entry->mDebuggerEntry.mStackOffsetFromBottom == stack_offset_bottom &&
              parent_entry->mDebuggerEntry.mSourceVarID == array_var_id)))
        {
            // -- now we see if there's a child with the same index
            char array_entry_name[32];
            sprintf_s(array_entry_name, sizeof(array_entry_name), "[%d]", array_index);

            bool found_child = false;
            int parent_index = entry_index;
            ++entry_index;
            while (entry_index < mWatchList.size())
            {
                // -- make sure the child belongs to the same array variable
                CWatchEntry* child = mWatchList.at(entry_index);

                // -- same as above - either this is an array entry for a requested watch,
                // or it's for the same array variable at the same stack offset
                if (watch_request_id > 0 ||
                    (child->mDebuggerEntry.mStackOffsetFromBottom == stack_offset_bottom &&
                        child->mDebuggerEntry.mSourceVarID == array_var_id))
                {
                    // -- see if it's the child we're looking for
                    if (!strcmp(child->mDebuggerEntry.mVarName, array_entry_name))
                    {
                        found_child = true;
                        TinScript::SafeStrcpy(child->mDebuggerEntry.mValue, sizeof(child->mDebuggerEntry.mValue),
                            value_str);
                        child->UpdateDisplay();
                        break;
                    }
                    // -- else check the next child
                    else
                    {
                        ++entry_index;
                    }
                }
                // -- else we've past the entries for this array
                else
                {
                    break;
                }
            }

            // -- if we didn't find the child, add one
            // note:  entry_index will be the entry of the next var, so we want to insert here
            if (!found_child)
            {
                // -- create the new array entry
                TinScript::CDebuggerWatchVarEntry array_entry;
                array_entry.mWatchRequestID = watch_request_id;
                array_entry.mStackOffsetFromBottom = stack_offset_bottom;
                array_entry.mSourceVarID = parent_entry->mDebuggerEntry.mSourceVarID;
                sprintf_s(array_entry.mVarName, sizeof(array_entry.mVarName), "%s", array_entry_name);
                sprintf_s(array_entry.mValue, sizeof(array_entry.mValue), "%s", value_str);

                // -- add to the parent
                CWatchEntry* array_child_entry = new CWatchEntry(array_entry);
                parent_entry->addChild(array_child_entry);

                // -- insert into the watch list
                if (entry_index >= mWatchList.size())
                    mWatchList.append(array_child_entry);
                else
                    mWatchList.insert(mWatchList.begin() + entry_index, array_child_entry);
            }
        }

        // -- next entry
        // note:  if we found the child, entry_index will be that child, but we're only searching for mArraySize > 1
        // entries, so we'll skip the rest of the children.
        // -- if we didn't find the child, we appended, or inserted at entry_index, so we need to increment anyways
        entry_index++;
    }
}

// ====================================================================================================================
// ResendAllUserWatches():  resend all variable watches that were added by the user
// ====================================================================================================================
void CDebugWatchWin::ResendAllUserWatches()
{
    // -- loop through any dynamic watches, and re-request the value
    int entry_index = 0;
    while (entry_index < mWatchList.size())
    {
        CWatchEntry* entry = mWatchList.at(entry_index);
        if (entry->mDebuggerEntry.mWatchRequestID > 0)
        {
            // -- send the request to the target
            // -- note:  we only send top level watches, as their children will be populated as expected
            if (IsTopLevelEntry(*entry))
            {
                ResendVariableWatch(entry);
            }
        }

        // -- next entry
        ++entry_index;
    }
}

// ====================================================================================================================
// NotifyBreakpointHit():  Notification that the callstack has been updated, all watch entries are complete
// ====================================================================================================================
void CDebugWatchWin::NotifyBreakpointHit()
{
    // -- when a breakpoint is hit, we want to re-query all the manual watch values
    // the rest (locals, etc...) are sent automatically
    ResendAllUserWatches();
}

// ====================================================================================================================
// NotifyNewCallStack():  Called when the callstack has changed, so we can verify/purge invalid watches
// ====================================================================================================================
void CDebugWatchWin::NotifyUpdateCallstack(bool breakpointHit)
{
    // -- get the stack window
    CDebugCallstackWin* stackWindow = CConsoleWindow::GetInstance()->GetDebugCallstackWin();
    if (!stackWindow)
        return;

    uint32 cur_func_ns_hash = 0;
    uint32 cur_func_hash = 0;
    uint32 cur_func_object_id = 0;
    int32 current_stack_index = stackWindow->GetSelectedStackEntry(cur_func_ns_hash, cur_func_hash,
                                                                   cur_func_object_id);
    if (current_stack_index < 0)
    {
        return;
    }

    // -- also get the full execution stack depth, and calculate the selected "depth from the bottom"
    // all watch entry "stack depths" are from the bottom, not the top
    uint32 top_func_ns_hash = 0;
    uint32 top_func_hash = 0;
    uint32 top_func_object_id = 0;
    int32 execution_stack_depth = CConsoleWindow::GetInstance()->GetDebugCallstackWin()->
        GetTopStackEntry(top_func_ns_hash, top_func_hash,
            top_func_object_id);

    int32 selected_depth_from_bottom = execution_stack_depth - current_stack_index - 1;

    // -- loop through all watches
    int entry_index = 0;
    while (entry_index < mWatchList.size())
    {
        // -- if the watch has a calling function, see if that calling function is still valid in the current stack
        // -- note:  only loop through top level entries, as members are automatically deleted when the object entry is
        bool removed = false;
        CWatchEntry* entry = mWatchList.at(entry_index);
        if (entry->mDebuggerEntry.mObjectID == 0 && entry->mDebuggerEntry.mFunctionHash != 0)
        {
            int32 stack_index = stackWindow->ValidateStackEntry(entry->mDebuggerEntry.mFuncNamespaceHash,
                                                                entry->mDebuggerEntry.mFunctionHash,
                                                                entry->mDebuggerEntry.mFunctionObjectID);
            if (stack_index < 0)
            {
                // -- remove the children of this entry
                RemoveWatchVarChildren(entry_index);

                // -- remove this entry
                mWatchList.removeAt(entry_index);

                // -- now delete the entry
                delete entry;
                removed = true;
            }

            // -- otherwise, see if we need to hide the items
            else
            {
                bool hidden = (entry->mDebuggerEntry.mStackOffsetFromBottom != selected_depth_from_bottom &&
                               entry->mDebuggerEntry.mWatchRequestID == 0);
                entry->setHidden(hidden);

                // -- if this entry is an object, we need to hide all of its children
                for (int i = 0; i < entry->childCount(); ++i)
                {
                    QTreeWidgetItem* child = entry->child(i);
                    child->setHidden(hidden);
                }
            }
        }

        // -- if we didn't remove the entry, we need to bump the index
        if (!removed)
            ++entry_index;
    }
}

// --------------------------------------------------------------------------------------------------------------------
// keyPressEvent():  Handler for key presses, when a watch window is in focus
// --------------------------------------------------------------------------------------------------------------------
void CDebugWatchWin::keyPressEvent(QKeyEvent* event)
{
    if (!event)
        return;

    // -- delete the selected, if we have a selected
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace)
    {
        CWatchEntry* cur_item = static_cast<CWatchEntry*>(currentItem());
        if (cur_item)
        {
            // -- we're only allowed to delete top level entries (no children), and only from dynamic watches
            if (cur_item->mDebuggerEntry.mWatchRequestID > 0 && cur_item->mDebuggerEntry.mObjectID == 0)
            {
                // -- find the index for the entry
                int entry_index = 0;
                for (entry_index = 0; entry_index < mWatchList.size(); ++entry_index)
                {
                    if (mWatchList.at(entry_index) == cur_item)
                        break;
                }

                // -- if we found our entry, ensure we remove any children
                if (entry_index < mWatchList.size())
                {
                    RemoveWatchVarChildren(entry_index);
                }

                // -- now remove the item and delete it
                mWatchList.removeAt(entry_index);
                delete cur_item;
            }
        }
        return;
    }

    // -- pass to the base class for handling
    QTreeWidget::keyPressEvent(event);
}

// --------------------------------------------------------------------------------------------------------------------
#include "TinQTWatchWinMoc.cpp"

// ====================================================================================================================
// eof
// ====================================================================================================================
