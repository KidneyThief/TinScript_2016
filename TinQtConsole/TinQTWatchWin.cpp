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

CWatchEntry::CWatchEntry(const TinScript::CDebuggerWatchVarEntry& debugger_entry, bool isObject, bool breakOnWrite)
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
        // -- set the text
        setText(0, mDebuggerEntry.mVarName);

		if (mDebuggerEntry.mType != TinScript::TYPE_void)
			setText(1, TinScript::GetRegisteredTypeName(mDebuggerEntry.mType));
		else
			setText(1, "");

        const char* value = mDebuggerEntry.mValue;
        if (mDebuggerEntry.mType == TinScript::TYPE_object && mDebuggerEntry.mVarObjectID == 0)
        {
            value = "<invalid>";
        }
        setText(2, value);
    }
}

CWatchEntry::~CWatchEntry()
{
}

void CWatchEntry::UpdateType(TinScript::eVarType type)
{
	mDebuggerEntry.mType = type;
	if (mDebuggerEntry.mType != TinScript::TYPE_void)
		setText(1, TinScript::GetRegisteredTypeName(mDebuggerEntry.mType));
	else
		setText(1, "");
}

void CWatchEntry::UpdateValue(const char* new_value)
{
    if (!new_value)
        new_value = "";

    // -- if it's of type object, it's could be a variable that was uninitialized - re-cache the object ID
    if (mDebuggerEntry.mType == TinScript::TYPE_object && mDebuggerEntry.mVarObjectID == 0)
    {
        new_value = "<invalid>";
    }

    strcpy_s(mDebuggerEntry.mValue, new_value);
    setText(2, mDebuggerEntry.mValue);
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
            entry->UpdateType(watch_var_entry.mType);

            // -- if the type is a variable, copy the object ID as well
            if (watch_var_entry.mType == TinScript::TYPE_object)
            {
                // -- if we're changing objects, we need to delete the children
                if (entry->mDebuggerEntry.mVarObjectID != watch_var_entry.mVarObjectID)
                    RemoveWatchVarChildren(entry_index);

                // -- set the new object ID
                entry->mDebuggerEntry.mVarObjectID = watch_var_entry.mVarObjectID;
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
    addTopLevelItem(new_entry);
    mWatchList.insert(0, new_entry);
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
            if (entry->mDebuggerEntry.mFuncNamespaceHash == watch_var_entry.mFuncNamespaceHash &&
                entry->mDebuggerEntry.mFunctionHash == watch_var_entry.mFunctionHash &&
                entry->mDebuggerEntry.mFunctionObjectID == watch_var_entry.mFunctionObjectID &&
                entry->mDebuggerEntry.mType == watch_var_entry.mType && 
                entry->mDebuggerEntry.mVarHash == watch_var_entry.mVarHash &&
                (entry->mDebuggerEntry.mStackLevel == watch_var_entry.mStackLevel ||
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

    // -- if we did not find the entry for the current callstack...
    uint32 top_func_ns_hash = 0;
    uint32 top_func_hash = 0;
    uint32 top_func_object_id = 0;
    int32 top_stack_index = CConsoleWindow::GetInstance()->GetDebugCallstackWin()->
                                                           GetTopStackEntry(top_func_ns_hash, top_func_hash,
                                                                            top_func_object_id);

    if (!found_callstack_entry && watch_var_entry.mStackLevel == top_stack_index && !update_only)
    {
        // -- see if this is for the 
        CWatchEntry* new_entry = new CWatchEntry(watch_var_entry);
        addTopLevelItem(new_entry);
        mWatchList.append(new_entry);

        // -- see if this entry should be hidden
        bool hidden = (watch_var_entry.mStackLevel != current_stack_index && watch_var_entry.mWatchRequestID == 0);
        if (hidden)
        {
            new_entry->setHidden(hidden);
        }
    }
}

void CDebugWatchWin::AddObjectMemberEntry(const TinScript::CDebuggerWatchVarEntry& watch_var_entry, bool update_only)
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
            if (entry->mDebuggerEntry.mObjectID == 0 &&
                entry->mDebuggerEntry.mType == TinScript::TYPE_object &&
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

        // -- if we didn't find a label, and we're only permitted to update, then we're done
        if (member_entry == NULL && update_only)
        {
            return;
        }

        // -- if we didn't find a label, add one
        if (member_entry == NULL)
        {
            CWatchEntry* ns = new CWatchEntry(watch_var_entry);
            obj_entry->addChild(ns);

            if (entry_index == mWatchList.size())
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

            member_entry->UpdateValue(watch_var_entry.mValue);
        }
    }
}

// ====================================================================================================================
// AddVariableWatch():  Dynamically add a watch to be updated by the debugger
// ====================================================================================================================
void CDebugWatchWin::AddVariableWatch(const char* variableWatch, bool breakOnWrite)
{
	if (!variableWatch || !variableWatch[0])
		return;

    // -- ensure this window is the top level (visable in front of the other docked widgets)
    parentWidget()->show();
    parentWidget()->raise();

	// -- the variable name for a watch is the expression requested
	TinScript::CDebuggerWatchVarEntry new_watch;

	// -- set the request ID, so if/when we receive an update from the target, we'll know what it
	// -- is in response to
	new_watch.mWatchRequestID = gVariableWatchRequestID++;
    new_watch.mStackLevel = -1;

	new_watch.mFuncNamespaceHash = 0;
    new_watch.mFunctionHash = 0;
    new_watch.mFunctionObjectID = 0;

    new_watch.mObjectID = 0;
    new_watch.mNamespaceHash = 0;

	// -- we *hope* the target can identify the expression and fill in the type
    new_watch.mType = TinScript::TYPE_void;

	// -- var name holds the expresion
	TinScript::SafeStrcpy(new_watch.mVarName, variableWatch, TinScript::kMaxNameLength);

	// -- we also *hope* the target can fill in a value for us as well
    new_watch.mValue[0] = '\0';

	// -- also to be filled in by a response from the target
	new_watch.mVarHash = 0;
    new_watch.mVarObjectID = 0;

	// -- we're allowed *anything* including duplicates when adding variable watches
	CWatchEntry* new_entry = new CWatchEntry(new_watch, breakOnWrite);
    addTopLevelItem(new_entry);
    mWatchList.append(new_entry);

	// -- send the request to the target, if we're currently in a break point
    if (CConsoleWindow::GetInstance()->mBreakpointHit)
    {
        SocketManager::SendCommandf("DebuggerAddVariableWatch(%d, `%s`, %s);",
                                    new_watch.mWatchRequestID, variableWatch,
                                    breakOnWrite ? "true" : "false");
        new_entry->mRequestSent = true;
    }
}

// ====================================================================================================================
// CreateSelectedWatch():  Dynamically add a watch to be updated by the debugger
// ====================================================================================================================
void CDebugWatchWin::CreateSelectedWatch()
{
    CWatchEntry* cur_item = static_cast<CWatchEntry*>(currentItem());
    if (cur_item && cur_item->mDebuggerEntry.mType != TinScript::TYPE_void)
    {
        // -- create the string to use as a watch
        char watch_string[TinScript::kMaxNameLength];
        char cur_value[TinScript::kMaxNameLength];

        // -- see if we have a variable or a member
        if (cur_item->mDebuggerEntry.mObjectID > 0)
            sprintf_s(watch_string, "%d.%s", cur_item->mDebuggerEntry.mObjectID, cur_item->mDebuggerEntry.mVarName);
        else
            strcpy_s(watch_string, cur_item->mDebuggerEntry.mVarName);

        // -- set the current value
        strcpy_s(cur_value, cur_item->mDebuggerEntry.mValue);

        CConsoleWindow::GetInstance()->GetMainWindow()->CreateVariableWatch(cur_item->mDebuggerEntry.mWatchRequestID,
                                                                            watch_string, cur_value);
    }
}

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

// ------------------------------------------------------------------------------------------------
void CDebugWatchWin::ClearWatchWin()
{
    mWatchList.clear();
    clear();
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
    int32 object_id = parent_entry->mDebuggerEntry.mVarObjectID;

    // -- remove all children (if we had any)
    if (object_id > 0)
    {
        // -- remove the children from the Qt list
        while (parent_entry->childCount() > 0)
            parent_entry->removeChild(parent_entry->child(0));

        // -- removethe children from the watch list
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
}

// ------------------------------------------------------------------------------------------------
void CDebugWatchWin::NotifyWatchVarEntry(TinScript::CDebuggerWatchVarEntry* watch_var_entry, bool update_only)
{
	// -- sanity check
    if (!watch_var_entry)
        return;

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

    // -- if we're adding a namespace label
    if (watch_var_entry->mObjectID > 0)
    {
        AddObjectMemberEntry(*watch_var_entry, update_only);
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
        if (entry->mDebuggerEntry.mWatchRequestID == watch_var_entry->mWatchRequestID &&
            entry->mDebuggerEntry.mObjectID == 0)
		{
            // -- if this response is for the main (top level) request, update the value
            if (watch_var_entry->mObjectID == 0)
            {
                // -- if the type used to be an object, and the new type is something else, we
                // -- need to remove the children
                if (entry->mDebuggerEntry.mType == TinScript::TYPE_object &&
                    watch_var_entry->mType != TinScript::TYPE_object)
                {
                    RemoveWatchVarChildren(entry_index);
                }

                // -- update the type (it may have been undetermined)
                entry->UpdateType(watch_var_entry->mType);

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

			    // -- update the value
			    entry->UpdateValue(watch_var_entry->mValue);
            }

            // -- otherwise, if this entry is a matching object, then this is a member of that object
            else if (entry->mDebuggerEntry.mType == TinScript::TYPE_object &&
                     entry->mDebuggerEntry.mVarObjectID == watch_var_entry->mObjectID)
            {
                NotifyVarWatchMember(entry_index, watch_var_entry);
            }

			// -- we're done
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
    if (entry_index == mWatchList.size())
        mWatchList.append(ns);
    else
        mWatchList.insert(mWatchList.begin() + entry_index, ns);
}

// ====================================================================================================================
// NotifyBreakpointHit():  Notification that the callstack has been updated, all watch entries are complete
// ====================================================================================================================
void CDebugWatchWin::NotifyBreakpointHit()
{
	// -- loop through any dynamic watches, and re-request the value
    int entry_index = 0;
    while (entry_index < mWatchList.size())
    {
        CWatchEntry* entry = mWatchList.at(entry_index);
		if (entry->mDebuggerEntry.mWatchRequestID > 0)
		{
			// -- send the request to the target
            // -- note:  we don't flag it for break unless the request hasn't yet been sent
			SocketManager::SendCommandf("DebuggerAddVariableWatch(%d, `%s`, %s);",
                                        entry->mDebuggerEntry.mWatchRequestID, entry->mDebuggerEntry.mVarName,
                                        !entry->mRequestSent && entry->mBreakOnWrite ? "true" : "false");
            entry->mRequestSent = true;
		}

		// -- next entry
		++entry_index;
	}
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
                // -- remove this entry
                mWatchList.removeAt(entry_index);

                // -- if this is an object, we need to delete its children as well
                if (entry->mDebuggerEntry.mType == TinScript::TYPE_object && entry->childCount() > 0)
                {
                    // -- ensure we have a valid object ID
                    CWatchEntry* child_entry = static_cast<CWatchEntry*>(entry->child(0));
                    uint32 object_id = child_entry->mDebuggerEntry.mObjectID;

                    // -- clear the children
                    while (entry->childCount() > 0)
                        entry->removeChild(entry->child(0));

                    // -- now starting from the next index, delete the children of this object
                    while (entry_index < mWatchList.size())
                    {
                        CWatchEntry* child_entry = mWatchList.at(entry_index);
                        if (entry->mDebuggerEntry.mObjectID == object_id)
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

                // -- now delete the entry
                delete entry;
                removed = true;
            }

            // -- otherwise, see if we need to hide the items
            else
            {
                bool hidden = (entry->mDebuggerEntry.mStackLevel != current_stack_index &&
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
            // -- we're only allowed to delete top level entries (no children), and only from dyanic watches
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
