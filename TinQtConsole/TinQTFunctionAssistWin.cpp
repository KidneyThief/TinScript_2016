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
// TinQTFunctionAssistWin.cpp:  A window to provide an incremental search for functions/methods
// ====================================================================================================================

// -- Qt includes
#include <qgridlayout.h>
#include <qlist.h>
#include <qlabel.h>
#include <QKeyEvent>

// -- TinScript includes
#include "TinScript.h"
#include "TinRegistration.h"
#include "socket.h"

// -- includes
#include "TinQTConsole.h"
#include "TinQTSourceWin.h"
#include "TinQTObjectBrowserWin.h"
#include "TinQTFunctionAssistWin.h"

// == class CDebugFunctionAssistWin ===================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CDebugFunctionAssistWin::CDebugFunctionAssistWin(QWidget* parent)
    : QWidget(parent)
{
    // -- create the grid layout, to coordinate the input, function list, object ID, and parameter list
    QGridLayout* main_layout = new QGridLayout(this);

    // -- create the input components
    QWidget* input_widget = new QWidget(this);
    input_widget->setFixedHeight(48);
    input_widget->setMinimumWidth(80);
    QHBoxLayout* input_layout = new QHBoxLayout(input_widget);
    mFunctionInput = new CFunctionAssistInput(this, input_widget);

    input_layout->addWidget(new QLabel("Search:"));
    input_layout->addWidget(mFunctionInput, 1);

    // -- create the object identifier
    QWidget* identifier_widget = new QWidget(this);
    identifier_widget->setFixedHeight(kFontHeight * 2 + kButtonSpace * 3);
    identifier_widget->setMinimumWidth(80);
    QGridLayout* identifier_layout = new QGridLayout(identifier_widget);
    mObjectIndentifier = new QLabel("<global scope>", identifier_widget);
	mObjectIndentifier->setFixedHeight(kFontHeight);
	QPushButton* method_button = new QPushButton("Method", identifier_widget);
	QPushButton* browse_button = new QPushButton("Object", identifier_widget);
    QPushButton* created_button = new QPushButton("Created", identifier_widget);
    method_button->setFixedHeight(kFontHeight);
    browse_button->setFixedHeight(kFontHeight);
    created_button->setFixedHeight(kFontHeight);

    identifier_layout->addWidget(mObjectIndentifier, 0, 0, 1, 3);
	identifier_layout->addWidget(method_button, 1, 0, 1, 1);
    identifier_layout->addWidget(browse_button, 1, 1, 1, 1);
    identifier_layout->addWidget(created_button, 1, 2, 1, 1);

    // -- create the function list
    mFunctionList = new CFunctionAssistList(this, this);

    // -- create the parameter list
    mParameterList = new CFunctionParameterList(this);

    // add the 4x pieces to the main layout
    main_layout->addWidget(input_widget, 0, 0);
    main_layout->addWidget(identifier_widget, 0, 1);
    main_layout->addWidget(mFunctionList, 1, 0);
    main_layout->addWidget(mParameterList, 1, 1);

    main_layout->setRowStretch(0, 1);
    main_layout->setColumnStretch(1, 1);

    // -- ensure we start with a clean search
	mSelectedFunctionHash = 0;
	mSearchObjectID = 0;
    mFilterString[0] = '\0';

    // -- hook up the method and browse button
	QObject::connect(method_button, SIGNAL(clicked()), this, SLOT(OnButtonMethodPressed()));
	QObject::connect(browse_button, SIGNAL(clicked()), this, SLOT(OnButtonBrowsePressed()));
    QObject::connect(created_button, SIGNAL(clicked()), this, SLOT(OnButtonCreatedPressed()));
}

// ====================================================================================================================
// Deconstructor
// ====================================================================================================================
CDebugFunctionAssistWin::~CDebugFunctionAssistWin()
{
    ClearSearch();
}

// ====================================================================================================================
// NotifyCodeblockLoaded():  When a codeblock has been loaded, we should re-query, in case of new function definitions. 
// ====================================================================================================================
void CDebugFunctionAssistWin::NotifyCodeblockLoaded(uint32 codeblock_hash)
{
    // -- only request from the target for a valid ID
    if (mSearchObjectID >= 0 && SocketManager::IsConnected())
    {
        SocketManager::SendCommandf("DebuggerRequestFunctionAssist(%d);", mSearchObjectID);
    }
}

// ====================================================================================================================
// ClearSearch():  Cleans up all data associated with a search.
// ====================================================================================================================
void CDebugFunctionAssistWin::ClearSearch()
{
    // -- clear the parameter and function list
	mSelectedFunctionHash = 0;
	mParameterList->Clear();
    mFunctionList->Clear();

    // -- clear the function entry map
    QList<uint32>& key_list = mFunctionEntryMap.keys();
    for (int i = 0; i < key_list.size(); ++i)
    {
        TinScript::CDebuggerFunctionAssistEntry* entry = mFunctionEntryMap[key_list[i]];
        if (entry)
            delete entry;
    }
    mFunctionEntryMap.clear();

    // -- clear the object entry map
    QList<uint32>& object_id_list = mObjectEntryMap.keys();
    for (int i = 0; i < object_id_list.size(); ++i)
    {
        TinScript::CDebuggerFunctionAssistEntry* entry = mFunctionEntryMap[object_id_list[i]];
        if (entry)
            delete entry;
    }
    mObjectEntryMap.clear();
}

// ====================================================================================================================
// NotifyFunctionAssistEntry():  The result received from the target, of our request for a function list
// ====================================================================================================================
void CDebugFunctionAssistWin::NotifyFunctionAssistEntry(const TinScript::CDebuggerFunctionAssistEntry& assist_entry)
{
    // -- ensure this is for our current search objectc
    if (assist_entry.mObjectID != mSearchObjectID)
        return;

    // -- if this entry is already in the map, we're done
    if (mFunctionEntryMap.contains(assist_entry.mFunctionHash))
        return;

    // -- make a copy of the received entry, and add it to the map
    TinScript::CDebuggerFunctionAssistEntry* new_entry = new TinScript::CDebuggerFunctionAssistEntry();
    memcpy(new_entry, &assist_entry, sizeof(TinScript::CDebuggerFunctionAssistEntry));
    mFunctionEntryMap.insert(assist_entry.mFunctionHash, new_entry);

    // -- update the filtered display (given the new entry - we'll see if it needs to be added to the results)
    UpdateSearchNewEntry(assist_entry.mFunctionHash);
}

// ====================================================================================================================
// UpdateSearchNewEntry():  Called when the filter hasn't changed, but we've received new entries
// ====================================================================================================================
void CDebugFunctionAssistWin::UpdateSearchNewEntry(uint32 function_hash)
{
    // -- get the entry from the map
    TinScript::CDebuggerFunctionAssistEntry* entry = mFunctionEntryMap[function_hash];
    if (entry)
    {
        if (FunctionContainsFilter(entry->mFunctionName))
        {
            mFunctionList->DisplayEntry(entry);
        }
    }
}

// ====================================================================================================================
// StringContainsFilter():  See if the filter is contained anywhere within the given string.
// ====================================================================================================================
bool CDebugFunctionAssistWin::StringContainsFilter(const char* string, bool& exact_match, bool& new_object_search)
{
    // -- initialize the results
    exact_match = false;
    new_object_search = false;

    // -- loop to see if the filter is contained anywhere
    bool first_pass = true;
    const char* substr_ptr = string;
    while (*substr_ptr != '\0')
    {
        bool temp_exact_match = false;
        bool temp_new_object_search = false;
        bool result = StringContainsFilterImpl(substr_ptr, mFilterString, temp_exact_match, temp_new_object_search);

        // -- update the flags
        if (first_pass)
        {
            first_pass = false;
            exact_match = temp_exact_match;
            new_object_search = temp_new_object_search;
        }

        // -- if we found a matching substring, return true
        if (result)
            return (true);

        // -- otherwise, keep looking
        else
            ++substr_ptr;
    }

    // -- not contained
    return (false);
}

// ====================================================================================================================
// FunctionContainsFilter():  See if the filter is contained within the given string, not including the object id.
// ====================================================================================================================
bool CDebugFunctionAssistWin::FunctionContainsFilter(const char* string)
{
    // -- if the filter contains a period, we want to filter based on the method string
    const char* use_filter_ptr = strchr(mFilterString, '.');
    if (use_filter_ptr != NULL)
        ++use_filter_ptr;
    else
        use_filter_ptr = mFilterString;

    // -- loop to see if the filter is contained anywhere
    const char* substr_ptr = string;
    while (*substr_ptr != '\0')
    {
        bool temp_exact_match = false;
        bool temp_new_object_search = false;
        bool result = StringContainsFilterImpl(substr_ptr, use_filter_ptr, temp_exact_match, temp_new_object_search);

        // -- if we found a matching substring, return true
        if (result)
            return (true);

        // -- otherwise, keep looking
        else
            ++substr_ptr;
    }

    // -- not contained
    return (false);

}

// ====================================================================================================================
// void UpdateFilter():  Update the search string, and filter the function list.
// ====================================================================================================================
void CDebugFunctionAssistWin::UpdateFilter(const char* filter)
{
    if (!filter)
        filter = "";

    // -- trim the whitespace from the front
    const char* filter_ptr = filter;
    while (*filter_ptr <= 0x20)
        ++filter_ptr;

    // -- call the string filter method - we don't actually care about the return value,
    // we only care if they're an exact match, or if we have to completely reset the search
    bool exact_match = false;
    bool new_object_search = false;
    StringContainsFilter(filter_ptr, exact_match, new_object_search);

    // -- if our search object is invalid, then it's by definition, a new search every time the filter changes
    if (mSearchObjectID == 0)
    {
        new_object_search = true;
        exact_match = false;
    }

    // -- if we have an exact match, we're done
    if (exact_match)
        return;

    // -- copy the new search filter
    TinScript::SafeStrcpy(mFilterString, filter, TinScript::kMaxNameLength);

    // -- if we have a new object id, we need to re-issue the function assist request
    if (new_object_search)
    {
        // -- find the new object (or switching back to the global scope)
        int32 object_id = 0;

        // -- see if we have an to search
        char* period_str = strchr(mFilterString, '.');
        if (period_str)
        {
            // -- if the string is a valid number
            char object_string[TinScript::kMaxNameLength];
            TinScript::SafeStrcpy(object_string, mFilterString, TinScript::kMaxNameLength);
            period_str = strchr(object_string, '.');
            *period_str = '\0';

            // -- see if the string already represents a valid object ID
            object_id = TinScript::Atoi(object_string);

            // -- if it was not, see if it's the name of an object, from which we can get the ID
            if (object_id == 0)
            {
                object_id = CConsoleWindow::GetInstance()->GetDebugObjectBrowserWin()->FindObjectByName(object_string);
            }

            // -- if we still didn't find an object, set the object_id to -1, invalid
            if (object_id == 0)
            {
                object_id = -1;
            }
        }

        // -- set the search scope label
        if (object_id < 0)
            mObjectIndentifier->setText("<invalid>");
        else if (object_id == 0)
            mObjectIndentifier->setText("<global scope>");
        else
        {
            const char* object_identifier =
                CConsoleWindow::GetInstance()->GetDebugObjectBrowserWin()->GetObjectIdentifier(object_id);
            char buf[TinScript::kMaxNameLength];
            if (!object_identifier || !object_identifier[0])
                sprintf_s(buf, "Object: [%d]", object_id);
            else
                sprintf_s(buf, "Object: %s", object_identifier);
            mObjectIndentifier->setText(buf);
        }

        // -- if we found an object, issue the function query
        if (object_id != mSearchObjectID)
        {
            // -- clear the search, and set the new (possibly invalid) object ID
            ClearSearch();
            mSearchObjectID = object_id;

            // -- only request from the target for a valid ID
            if (mSearchObjectID >= 0 && SocketManager::IsConnected())
            {
                SocketManager::SendCommandf("DebuggerRequestFunctionAssist(%d);", mSearchObjectID);
            }

            // -- if the search object is 0, we're in the global space, so the search window will double
            // -- as a way to search for objects by name as well
            if (mSearchObjectID == 0)
            {
                QList<uint32> object_id_list;
                CConsoleWindow::GetInstance()->GetDebugObjectBrowserWin()->PopulateObjectIDList(object_id_list);
                for (int i = 0; i < object_id_list.size(); ++i)
                {
                    // -- make a copy of the received entry, and add it to the map
                    uint32 display_object_id = object_id_list[i];
                    const char* object_name = CConsoleWindow::GetInstance()->GetDebugObjectBrowserWin()->
                                              GetObjectName(display_object_id);
                    if (object_name && object_name[0])
                    {
                        TinScript::CDebuggerFunctionAssistEntry* new_entry =
                            new TinScript::CDebuggerFunctionAssistEntry();
                        new_entry->mIsObjectEntry = true;
                        new_entry->mObjectID = display_object_id;
                        new_entry->mNamespaceHash = 0;
                        new_entry->mFunctionHash = 0;
                        TinScript::SafeStrcpy(new_entry->mFunctionName, object_name, TinScript::kMaxNameLength);
                        new_entry->mParameterCount = 0;

                        // -- add the object to the object map
                        mObjectEntryMap.insert(new_entry->mObjectID, new_entry);

                        // -- see if we need to display it
                        if (FunctionContainsFilter(new_entry->mFunctionName))
                        {
                            mFunctionList->DisplayEntry(new_entry);
                        }
                    }
                }
            }
        }
    }

    // -- else, loop through and see which entries must be toggled
    else
    {
        // -- list objects first
        QList<uint32>& object_id_list = mObjectEntryMap.keys();
        for (int i = 0; i < object_id_list.size(); ++i)
        {
            TinScript::CDebuggerFunctionAssistEntry* entry = mObjectEntryMap[object_id_list[i]];
            if (entry)
            {
                if (FunctionContainsFilter(entry->mFunctionName))
                {
                    mFunctionList->DisplayEntry(entry);
                }
                else
                {
                    mFunctionList->FilterEntry(entry);
                }
            }
        }

        QList<uint32>& key_list = mFunctionEntryMap.keys();
        for (int i = 0; i < key_list.size(); ++i)
        {
            TinScript::CDebuggerFunctionAssistEntry* entry = mFunctionEntryMap[key_list[i]];
            if (entry)
            {
                if (FunctionContainsFilter(entry->mFunctionName))
                {
                    mFunctionList->DisplayEntry(entry);
                }
                else
                {
                    mFunctionList->FilterEntry(entry);
                }
            }
        }
    }
}

// ====================================================================================================================
// void SetAssistObjectID():  Focuses the function assist window, initializing it to the given object
// ====================================================================================================================
void CDebugFunctionAssistWin::SetAssistObjectID(uint32 object_id)
{
    // -- focus the input
    show();
    raise();
    mFunctionInput->setFocus();

    // -- if the object_id is different from the current search object ID, reset the input text
    if (object_id != mSearchObjectID)
    {
		// -- clear the selected function hash
		mSelectedFunctionHash = 0;

		if (object_id == 0)
        {
            mFunctionInput->setText("");
            UpdateFilter("");
        }
        else
        {
            char buf[32];
            sprintf_s(buf, "%d.", object_id);
            mFunctionInput->setText(buf);
            UpdateFilter(buf);
        }
    }
}

// ====================================================================================================================
// NotifyFunctionClicked():  Selecting a function entry populates the parameter list.
// ====================================================================================================================
void CDebugFunctionAssistWin::NotifyFunctionClicked(TinScript::CDebuggerFunctionAssistEntry* list_entry)
{
	// -- clear the selected function
	mSelectedFunctionHash = 0;

    // -- clicking on an object does nothing
    if (!list_entry || list_entry->mIsObjectEntry)
        return;

    if (!mFunctionEntryMap.contains(list_entry->mFunctionHash))
        return;

	// -- cache the selected function hash, and populate the parameter list
	mSelectedFunctionHash = list_entry->mFunctionHash;
    TinScript::CDebuggerFunctionAssistEntry* assist_entry = mFunctionEntryMap[list_entry->mFunctionHash];
    mParameterList->Populate(assist_entry);
}

// ====================================================================================================================
// NotifyFunctionDoubleClicked():  Activating a function entry issues a command string to the Console Input.
// ====================================================================================================================
void CDebugFunctionAssistWin::NotifyFunctionDoubleClicked(TinScript::CDebuggerFunctionAssistEntry* list_entry)
{
	// -- clear the selected function
	mSelectedFunctionHash = 0;

	// -- ensure we have a valid search
    if (mSearchObjectID == 0)
        return;

    if (list_entry->mIsObjectEntry)
    {
        // -- on double-click, set the filter to be the "<object_name>."
        char new_filter[TinScript::kMaxNameLength];
        sprintf_s(new_filter, "%d.", list_entry->mObjectID);
        mFunctionInput->setText(new_filter);
        UpdateFilter(new_filter);
    }
    else
    {
        if (!mFunctionEntryMap.contains(list_entry->mFunctionHash))
            return;

		// -- cache the selected function hash, and populate the parameter list
		mSelectedFunctionHash = list_entry->mFunctionHash;
		TinScript::CDebuggerFunctionAssistEntry* assist_entry = mFunctionEntryMap[list_entry->mFunctionHash];
        mParameterList->Populate(assist_entry);

        // -- create the command string, and send it to the console input
        char buf[TinScript::kMaxTokenLength];
        int length_remaining = TinScript::kMaxTokenLength;
        if (mSearchObjectID > 0)
            sprintf_s(buf, "%d.%s(", mSearchObjectID, assist_entry->mFunctionName);
        else
            sprintf_s(buf, "%s(", assist_entry->mFunctionName);

        int length = strlen(buf);
        length_remaining -= length;
        char* buf_ptr = &buf[length];

        // -- note:  we want the cursor to be placed at the beginning of the parameter list
        int cursor_pos = length;

        // -- fill in the parameters (starting with 1, as we don't include the return value)
        for (int i = 1; i < assist_entry->mParameterCount; ++i)
        {
            if (i != 1)
                sprintf_s(buf_ptr, length_remaining, ", %s", TinScript::GetRegisteredTypeName(assist_entry->mType[i]));
            else
                strcpy_s(buf_ptr, length_remaining, TinScript::GetRegisteredTypeName(assist_entry->mType[i]));

            // -- update the buf pointer,and the length remaining
            length = strlen(buf_ptr);
            length_remaining -= length;
            buf_ptr += strlen(buf_ptr);
        }

        // -- complete the command
        strcpy_s(buf_ptr, length_remaining, ");");

        // -- sedn the command
        CConsoleWindow::GetInstance()->GetInput()->SetText(buf, cursor_pos);
        CConsoleWindow::GetInstance()->GetInput()->setFocus();
    }
}

// ====================================================================================================================
// OnButtonMethodPressed():  FOr the search object ID, find the file/line implementation of the selected method.
// ====================================================================================================================
void CDebugFunctionAssistWin::OnButtonMethodPressed()
{
	// -- if we have a selected function
	if (mSelectedFunctionHash != 0 && mFunctionEntryMap.contains(mSelectedFunctionHash))
	{
		TinScript::CDebuggerFunctionAssistEntry* entry = mFunctionEntryMap[mSelectedFunctionHash];
		CConsoleWindow::GetInstance()->GetDebugSourceWin()->SetSourceView(entry->mCodeBlockHash, entry->mLineNumber);
	}
}

// ====================================================================================================================
// OnButtonCreatedPressed():  For the search object ID, find the file/line where the object was created.
// ====================================================================================================================
void CDebugFunctionAssistWin::OnButtonCreatedPressed()
{
    CConsoleWindow::GetInstance()->GetDebugObjectBrowserWin()->DisplayCreatedFileLine(mSearchObjectID);
}

// ====================================================================================================================
// OnButtonBrowsePressed():  Finds the search object ID, and selects it in the ObjectBrowser window.
// ====================================================================================================================
void CDebugFunctionAssistWin::OnButtonBrowsePressed()
{
    CConsoleWindow::GetInstance()->GetDebugObjectBrowserWin()->SetSelectedObject(mSearchObjectID);
}

// --------------------------------------------------------------------------------------------------------------------
// StringContainsFilterImpl():  Performs a single pass to see if the strings match from the beginning.
// --------------------------------------------------------------------------------------------------------------------
bool CDebugFunctionAssistWin::StringContainsFilterImpl(const char* string, const char* filter, bool& exact_match,
                                                       bool& new_object_search)
{
    // -- initialize the return values
    exact_match = false;
    new_object_search = false;

    // -- see if we can compare (insensitive) the strings
    const char* string_ptr = string;
    const char* filter_ptr = filter;

    // -- we want to check periods, to see if either wants to dereference an object
    const char* string_period = NULL;
    const char* filter_period = NULL;

    // -- see if the strings are different
    while (true)
    {
        // -- if both strings are terminated, they're exact
        if (*string_ptr == '\0' && *filter_ptr == '\0')
        {
            exact_match = true;
            return (true);
        }

        // -- get the lower case character from each string
        char string_char = *string_ptr;
        if (string_char >= 'A' && string_char <= 'Z')
            string_char = 'a' + (string_char - 'A');
        char filter_char = *filter_ptr;
        if (filter_char >= 'A' && filter_char <= 'Z')
            filter_char = 'a' + (filter_char - 'A');

        // -- see if either is a period
        if (string_char == '.')
            string_period = string_ptr;
        if (filter_char == '.')
            filter_period = filter_ptr;

        // -- if both characters are the same (case insensitive), continue the search
        if (string_char == filter_char)
        {
            ++string_ptr;
            ++filter_ptr;
        }

        // -- strings are different
        else
        {
            // -- if either string is different, in that one contains a period, then we've got a new object id
            if ((!string_period && filter_period) || (string_period && !filter_period))
                new_object_search = true;

            // -- else if we haven't found a period in either string, then the only way we have a new object search
            // is if we've already hit a difference before reaching the period.
            else if (!string_period && !filter_period)
            {
                new_object_search = (strchr(string_ptr, '.') != NULL || strchr(filter_ptr, '.') != NULL);
            }

            // -- if the string was first to terminate, then the string does contain the filter
            return (filter_char == '\0' && string_char != '\0');
        }
    }

    // -- unreachable
    return (false);
}

// == class CFunctionAssistInput ======================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CFunctionAssistInput::CFunctionAssistInput(CDebugFunctionAssistWin* owner, QWidget* parent)
    : QLineEdit(parent)
    , mOwner(owner)
{
    // -- q&d history implementation
    mHistoryFull = false;
    mHistoryIndex = -1;
    mHistoryLastIndex = -1;
    for(int i = 0; i < kMaxHistory; ++i)
        mHistory[i].text[0] = '\0';
}

// ====================================================================================================================
// keyPressEvent():  Handles key input events, modifying the search filter string.
// ====================================================================================================================
void CFunctionAssistInput::keyPressEvent(QKeyEvent * event)
{
    if (!event)
        return;

    // -- up arrow
    if (event->key() == Qt::Key_Up)
    {
        int32 oldhistory = mHistoryIndex;
        if(mHistoryIndex < 0)
            mHistoryIndex = mHistoryLastIndex;
        else if(mHistoryLastIndex > 0)
        {
            if (mHistoryFull)
                mHistoryIndex = (mHistoryIndex + kMaxHistory - 1) % kMaxHistory;
            else
                mHistoryIndex = (mHistoryIndex + mHistoryLastIndex) % (mHistoryLastIndex + 1);
        }

        // -- see if we actually changed
        if (mHistoryIndex != oldhistory && mHistoryIndex >= 0)
        {
            setText(mHistory[mHistoryIndex].text);
        }
    }

    // -- down arrow
    else if (event->key() == Qt::Key_Down)
    {
        int32 oldhistory = mHistoryIndex;
        if(mHistoryIndex < 0)
            mHistoryIndex = mHistoryLastIndex;
        else if(mHistoryLastIndex > 0)
        {
            if (mHistoryFull)
                mHistoryIndex = (mHistoryIndex + 1) % kMaxHistory;
            else
                mHistoryIndex = (mHistoryIndex + 1) % (mHistoryLastIndex + 1);
        }

        // -- see if we actually changed
        if (mHistoryIndex != oldhistory && mHistoryIndex >= 0)
        {
            setText(mHistory[mHistoryIndex].text);
        }
    }

    // -- esc
    else if (event->key() == Qt::Key_Escape)
    {
        setText("");
        mHistoryIndex = -1;
    }

    else
    {
        QLineEdit::keyPressEvent(event);
    }

    // -- get the current text, see if our search string has changed
    QByteArray text_input = text().toUtf8();
    const char* search_text = text_input.data();
    mOwner->UpdateFilter(search_text);
}

// == class CFunctionListEntry ========================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CFunctionListEntry::CFunctionListEntry(TinScript::CDebuggerFunctionAssistEntry* _entry, QTreeWidget* _owner)
    : QTreeWidgetItem(_owner)
    , mFunctionAssistEntry(_entry)
{
    if (_entry->mIsObjectEntry)
    {
        const char* object_identifier =
            CConsoleWindow::GetInstance()->GetDebugObjectBrowserWin()->GetObjectIdentifier(_entry->mObjectID);
        setText(0, object_identifier);

        // -- set the function name
        char sort_buf[TinScript::kMaxNameLength];
        sort_buf[0] = ' ';
        TinScript::SafeStrcpy(&sort_buf[1], _entry->mFunctionName, TinScript::kMaxNameLength - 1);
        setText(1, sort_buf);
    }
    else
    {
        // -- set the namespace
        if (_entry->mNamespaceHash != 0)
            setText(0, TinScript::UnHash(_entry->mNamespaceHash));
        else
            setText(0, "");

        // -- set the function name
        setText(1, _entry->mFunctionName);
    }

    // -- all new entries begin hidden
    setHidden(true);
}

// ====================================================================================================================
// Deconstructor
// ====================================================================================================================
CFunctionListEntry::~CFunctionListEntry()
{
}

// == class CFunctionAssistList =======================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CFunctionAssistList::CFunctionAssistList(CDebugFunctionAssistWin* owner, QWidget* parent)
    : QTreeWidget(parent)
    , mOwner(owner)
{
    setColumnCount(2);
    setItemsExpandable(false);
    setExpandsOnDoubleClick(false);

    // -- set the header
  	QTreeWidgetItem* header = new QTreeWidgetItem();
	header->setText(0,QString("Namespace"));
	header->setText(1,QString("Function"));
	setHeaderItem(header);

    // -- connect up both the clicked and double clicked slots
    QObject::connect(this, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(OnDoubleClicked(QTreeWidgetItem*)));
    QObject::connect(this, SIGNAL(itemClicked(QTreeWidgetItem*, int)), this, SLOT(OnClicked(QTreeWidgetItem*)));
}

// ====================================================================================================================
// Deconstructor
// ====================================================================================================================
CFunctionAssistList::~CFunctionAssistList()
{
}

// ====================================================================================================================
// FindEntry():  Returns true if the given function is currently visible in the list
// ====================================================================================================================
CFunctionListEntry* CFunctionAssistList::FindEntry(TinScript::CDebuggerFunctionAssistEntry* assist_entry)
{
    
    for (int i = 0; i < mFunctionList.size(); ++i)
    {
        CFunctionListEntry* entry = mFunctionList[i];
        if (assist_entry->mIsObjectEntry && entry->mFunctionAssistEntry->mObjectID == assist_entry->mObjectID)
            return (entry);
        else if (!assist_entry->mIsObjectEntry &&
                 entry->mFunctionAssistEntry->mFunctionHash == assist_entry->mFunctionHash)
            return (entry);
    }

    // -- not found
    return (NULL);
}

// ====================================================================================================================
// DisplayEntry():  Unhide or create an list entry for the given function.
// ====================================================================================================================
void CFunctionAssistList::DisplayEntry(TinScript::CDebuggerFunctionAssistEntry* assist_entry)
{
    // -- sanity check
    if (!assist_entry)
        return;

    // -- find the entry (or create it, if needed)
    CFunctionListEntry* entry = FindEntry(assist_entry);
    if (!entry)
    {
        entry = new CFunctionListEntry(assist_entry, this);
        sortItems(1, Qt::AscendingOrder);
        mFunctionList.append(entry);
    }

    // -- make the entry visible
    entry->setHidden(false);
}

// ====================================================================================================================
// FilterEntry():  Ensure the given function is hidden.
// ====================================================================================================================
void CFunctionAssistList::FilterEntry(TinScript::CDebuggerFunctionAssistEntry* assist_entry)
{
    // -- sanity check
    if (!assist_entry)
        return;

    // -- find the entry - set it hidden if it exists
    CFunctionListEntry* entry = FindEntry(assist_entry);
    if (entry)
        entry->setHidden(true);
}

// ====================================================================================================================
// FilterEntry():  Ensure the given function is hidden.
// ====================================================================================================================
void CFunctionAssistList::Clear()
{
    while (mFunctionList.size() > 0)
    {
        CFunctionListEntry* entry = mFunctionList[0];
        mFunctionList.removeAt(0);
        delete entry;
    }
}

void CFunctionAssistList::OnClicked(QTreeWidgetItem* item)
{
    CFunctionListEntry* entry = static_cast<CFunctionListEntry*>(item);
    mOwner->NotifyFunctionClicked(entry->mFunctionAssistEntry);
}

void CFunctionAssistList::OnDoubleClicked(QTreeWidgetItem* item)
{
    CFunctionListEntry* entry = static_cast<CFunctionListEntry*>(item);
    mOwner->NotifyFunctionDoubleClicked(entry->mFunctionAssistEntry);
}

// == class CFunctionParameterEntry ========================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CFunctionParameterEntry::CFunctionParameterEntry(TinScript::eVarType var_type, bool is_array, const char* _name,
                                                 QTreeWidget* _owner)
    : QTreeWidgetItem(_owner)
{
    const char* type_name = TinScript::GetRegisteredTypeName(var_type);
    char type_buf[TinScript::kMaxNameLength];
    sprintf_s(type_buf, "%s%s", type_name, is_array ? "[]" : "");
    setText(0, type_buf);
    setText(1, _name);
}

// ====================================================================================================================
// Deconstructor
// ====================================================================================================================
CFunctionParameterEntry::~CFunctionParameterEntry()
{
}

// == class CFunctionParameterList =======================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CFunctionParameterList::CFunctionParameterList(QWidget* parent)
    : QTreeWidget(parent)
{
    setColumnCount(2);
    setItemsExpandable(false);
    setExpandsOnDoubleClick(false);

    // -- set the header
  	QTreeWidgetItem* header = new QTreeWidgetItem();
	header->setText(0,QString("Type"));
	header->setText(1,QString("Name"));
	setHeaderItem(header);
}

// ====================================================================================================================
// Deconstructor
// ====================================================================================================================
CFunctionParameterList::~CFunctionParameterList()
{
}

// ====================================================================================================================
// Clear():  Clears the array of paramters.
// ====================================================================================================================
void CFunctionParameterList::Clear()
{
    while (mParameterList.size() > 0)
    {
        CFunctionParameterEntry* entry = mParameterList[0];
        delete entry;
        mParameterList.removeAt(0);
    }
}

// ====================================================================================================================
// Populate():  Given the function entry, extract the list of parameters and populate the view.
// ====================================================================================================================
void CFunctionParameterList::Populate(TinScript::CDebuggerFunctionAssistEntry* assist_entry)
{
    Clear();
    if (!assist_entry)
        return;

    for (int i = 0; i < assist_entry->mParameterCount; ++i)
    {
        TinScript::eVarType var_type = assist_entry->mType[i];
        bool is_array = assist_entry->mIsArray[i];
        const char* var_name = TinScript::UnHash(assist_entry->mNameHash[i]);
        CFunctionParameterEntry* new_parameter = new CFunctionParameterEntry(var_type, is_array, var_name, this);
        mParameterList.append(new_parameter);
    }
}

// --------------------------------------------------------------------------------------------------------------------
#include "TinQTFunctionAssistWinMoc.cpp"

// ====================================================================================================================
// eof
// ====================================================================================================================
