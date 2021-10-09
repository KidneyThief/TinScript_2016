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
    input_widget->setMinimumWidth(80);
    QHBoxLayout* input_layout = new QHBoxLayout(input_widget);
    mFunctionInput = new CFunctionAssistInput(this, input_widget);
    QPushButton* refresh_button = new QPushButton("Refresh",input_widget);
    refresh_button->setFixedWidth(100);

    input_layout->addWidget(new QLabel("Search:"));
    input_layout->addWidget(mFunctionInput, 1);
    input_layout->addWidget(refresh_button, 2);

    // -- create the object identifier
    QWidget* identifier_widget = new QWidget(this);
    identifier_widget->setFixedHeight(CConsoleWindow::TextEditHeight() * 2 + 4);
    identifier_widget->setMinimumWidth(80);
    QGridLayout* identifier_layout = new QGridLayout(identifier_widget);
    mObjectIndentifier = new QLabel("<global scope>", identifier_widget);
	mObjectIndentifier->setFixedHeight(CConsoleWindow::FontHeight());
	QPushButton* show_api_button = new QPushButton("Show Signature", identifier_widget);
    QPushButton* show_origin_button = new QPushButton("Show Object Creation", identifier_widget);
    QPushButton* console_input_button = new QPushButton("Copy To Input", identifier_widget);
    show_api_button->setFixedHeight(CConsoleWindow::TextEditHeight());
    show_origin_button->setFixedHeight(CConsoleWindow::TextEditHeight());
    console_input_button->setFixedHeight(CConsoleWindow::TextEditHeight());

    identifier_layout->addWidget(mObjectIndentifier, 0, 0, 1, 3);
	identifier_layout->addWidget(show_api_button, 1, 0, 1, 1);
    identifier_layout->addWidget(show_origin_button, 1, 1, 1, 1);
    identifier_layout->addWidget(console_input_button, 1, 2, 1, 1);

    identifier_layout->setRowStretch(0, 0);

    // -- create the function list
    mFunctionList = new CFunctionAssistList(this, this);

    mFunctionDisplayLabel = new QLabel("Signature:", this);

    // -- create the parameter list
    mParameterList = new CFunctionParameterList(this);

    // add the 4x pieces to the main layout
    main_layout->addWidget(input_widget, 0, 0);
    main_layout->addWidget(identifier_widget, 1, 0);
    main_layout->addWidget(mFunctionList, 2, 0);
    main_layout->addWidget(mFunctionDisplayLabel, 3, 0);
    main_layout->addWidget(mParameterList, 4, 0);

    // -- ensure we start with a clean search
	mSelectedFunctionHash = 0;
    mSelectedObjectID = 0;
	mSearchObjectID = 0;
    mFilterString[0] = '\0';

    // -- hook up the button signals
	QObject::connect(refresh_button, SIGNAL(clicked()), this, SLOT(OnButtonFilterRefreshPressed()));
    QObject::connect(show_api_button,SIGNAL(clicked()),this,SLOT(OnButtonShowAPIPressed()));
    QObject::connect(show_origin_button, SIGNAL(clicked()), this, SLOT(OnButtonShowOriginPressed()));
    QObject::connect(console_input_button,SIGNAL(clicked()),this,SLOT(OnButtonCopyToConsolePressed()));
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
    mSelectedObjectID = 0;
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
    // -- ensure this is for our current search object
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
// NotifyListObjectsComplete():  Called when DebuggerListObjects() has completed sending the complete list
// ====================================================================================================================
void CDebugFunctionAssistWin::NotifyListObjectsComplete()
{
    UpdateFilter(mFilterString, true);
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
void CDebugFunctionAssistWin::UpdateFilter(const char* filter, bool8 force_refresh)
{
    // -- trim the whitespace from the front
    const char* filter_ptr = filter;
    while (filter_ptr && *filter_ptr > 0x00 && *filter_ptr <= 0x20)
        ++filter_ptr;

    // -- an empty filter means we want to refresh
    if (filter_ptr == nullptr || filter_ptr[0] == '\0')
    {
        filter_ptr = "";
        force_refresh = true;
    }

    // -- call the string filter method - we don't actually care about the return value,
    // we only care if they're an exact match, or if we have to completely reset the search
    bool exact_match = false;
    bool new_object_search = false;
    StringContainsFilter(filter_ptr, exact_match, new_object_search);

    // -- if our search object is invalid, then it's by definition, a new search every time the filter changes
    if (mSearchObjectID == 0 || force_refresh)
    {
        new_object_search = true;
        exact_match = false;
    }

    // -- if we have an exact match, we're done
    if (exact_match)
        return;

    // -- copy the new search filter
    TinScript::SafeStrcpy(mFilterString, sizeof(mFilterString), filter, TinScript::kMaxNameLength);

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
            TinScript::SafeStrcpy(object_string, sizeof(object_string), mFilterString, TinScript::kMaxNameLength);
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
        {
            mObjectIndentifier->setText("<invalid>");
        }
        else if (object_id == 0)
        {
            mObjectIndentifier->setText("<global scope>");
        }
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
        if (object_id != mSearchObjectID || force_refresh)
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
                        TinScript::SafeStrcpy(new_entry->mFunctionName, sizeof(new_entry->mFunctionName), object_name, TinScript::kMaxNameLength);
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

    // -- if we didn't find an object, we're using the global scope
    // -- alternatively, we're continuing an existing with an updated filter
    if (mSearchObjectID == 0 || !new_object_search)
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
// DisplayObjectOrigin():  Populate the "signature/origin" panel with the origin callstack.
// ====================================================================================================================
void CDebugFunctionAssistWin::DisplayObjectOrigin(uint32 obj_id)
{
    // -- update the label
    mFunctionDisplayLabel->setText("Creation Callstack:");

    const uint32* file_hash_array = nullptr;
    const int32* lines_array = nullptr;
    int32 stack_size = CConsoleWindow::GetInstance()->GetDebugObjectBrowserWin()->GetObjectOriginStack(
                           obj_id, file_hash_array, lines_array);

    mParameterList->PopulateWithOrigin(stack_size, file_hash_array, lines_array);
}

// ====================================================================================================================
// DisplayFunctionSignature():  Populate the "signature/origin" panel with the signature of the selected function.
// ====================================================================================================================
void CDebugFunctionAssistWin::DisplayFunctionSignature()
{
    // -- update the label
    mFunctionDisplayLabel->setText("Signature:");

    // -- if not found, we'll at least set the header text
    TinScript::CDebuggerFunctionAssistEntry* assist_entry = nullptr;
    if (mSelectedFunctionHash > 0)
    {
        assist_entry = mFunctionEntryMap[mSelectedFunctionHash];
    }
    mParameterList->PopulateWithSignature(assist_entry);
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
        mSelectedObjectID = 0;

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
// GetAssistObjectID():  returns the assist object if set, otherwise the selected object in the search
// ====================================================================================================================
uint32 CDebugFunctionAssistWin::GetAssistObjectID() const
{
    if (mSearchObjectID > 0)
        return (mSearchObjectID);
    else
        return (mSelectedObjectID);
}

// ====================================================================================================================
// NotifyFunctionClicked():  Selecting a function entry populates the parameter list.
// ====================================================================================================================
void CDebugFunctionAssistWin::NotifyFunctionClicked(TinScript::CDebuggerFunctionAssistEntry* list_entry)
{
	// -- clear the selected function
	mSelectedFunctionHash = 0;
    mSelectedObjectID = 0;

    // -- clicking on an object does nothing
    if (!list_entry)
        return;

    if (!list_entry->mIsObjectEntry && mFunctionEntryMap.contains(list_entry->mFunctionHash))
    {
	    // -- cache the selected function hash, and populate the parameter list
	    mSelectedFunctionHash = list_entry->mFunctionHash;
        mSelectedObjectID = 0;
    }
    else if (list_entry->mIsObjectEntry && mObjectEntryMap.contains(list_entry->mObjectID))
    {
        mSelectedFunctionHash = 0;
        mSelectedObjectID = list_entry->mObjectID;
    }

    // -- will clear the panel and update the header,
    // and if we actually selected a function, will populate the parameters
    DisplayFunctionSignature();
}

// ====================================================================================================================
// NotifyFunctionDoubleClicked():  Activating a function entry issues a command string to the Console Input.
// ====================================================================================================================
void CDebugFunctionAssistWin::NotifyFunctionDoubleClicked(TinScript::CDebuggerFunctionAssistEntry* list_entry)
{
	// -- clear the selected function
	mSelectedFunctionHash = 0;
    mSelectedObjectID = 0;

    if (list_entry->mIsObjectEntry)
    {
        // -- on double-click, set the filter to be the "<object_name>."
        char new_filter[TinScript::kMaxNameLength];
        sprintf_s(new_filter, "%d.", list_entry->mObjectID);
        mFunctionInput->setText(new_filter);
        UpdateFilter(new_filter);

        if (mSearchObjectID > 0)
        {
            CConsoleWindow::GetInstance()->GetDebugObjectBrowserWin()->SetSelectedObject(mSearchObjectID);
        }
    }
    else
    {
        if (!mFunctionEntryMap.contains(list_entry->mFunctionHash))
            return;

		// -- cache the selected function hash, and populate the parameter list
		mSelectedFunctionHash = list_entry->mFunctionHash;
		TinScript::CDebuggerFunctionAssistEntry* assist_entry = mFunctionEntryMap[list_entry->mFunctionHash];

        // -- open the function implementation in the source view
        CConsoleWindow::GetInstance()->GetDebugSourceWin()->SetSourceView(assist_entry->mCodeBlockHash,
                                                                          assist_entry->mLineNumber - 1);
    }

    // -- display the function signature - if an object was double-clicked, then this 
    // will clear the panel and update the header
    DisplayFunctionSignature();
}

// ====================================================================================================================
// OnButtonFilterRefreshPressed():  Update the filter to reflect target-side changes (new methods, objects, etc...)
// ====================================================================================================================
void CDebugFunctionAssistWin::OnButtonFilterRefreshPressed()
{
    UpdateFilter(mFilterString, true);
}

// ====================================================================================================================
// OnButtonMethodPressed():  For the search object ID, find the file/line implementation of the selected method.
// ====================================================================================================================
void CDebugFunctionAssistWin::OnButtonShowAPIPressed()
{
    DisplayFunctionSignature();
}

// ====================================================================================================================
// OnButtonCreatedPressed():  For the search object ID, populate the parameter list with the creation callstack.
// ====================================================================================================================
void CDebugFunctionAssistWin::OnButtonShowOriginPressed()
{
    uint32 assist_object_id = GetAssistObjectID();
    if (assist_object_id > 0)
        DisplayObjectOrigin(assist_object_id);
}

// ====================================================================================================================
// OnButtonCopyToConsolePressed():  Construct the command to execute the selected object.method() in the console.
// ====================================================================================================================
void CDebugFunctionAssistWin::OnButtonCopyToConsolePressed()
{
    if(mSearchObjectID == 0 || mSelectedFunctionHash == 0)
    {
        return;
    }

    // -- get the function we want to execute from the console input
    TinScript::CDebuggerFunctionAssistEntry* assist_entry = mFunctionEntryMap[mSelectedFunctionHash];

    // -- create the command string, and send it to the console input
    char buf[TinScript::kMaxTokenLength];
    int length_remaining = TinScript::kMaxTokenLength;
    if(mSearchObjectID > 0)
        sprintf_s(buf,"%d.%s(",mSearchObjectID,assist_entry->mFunctionName);
    else
        sprintf_s(buf,"%s(",assist_entry->mFunctionName);

    int length = strlen(buf);
    length_remaining -= length;
    char* buf_ptr = &buf[length];

    // -- note:  we want the cursor to be placed at the beginning of the parameter list
    int cursor_pos = length;

    // -- fill in the parameters (starting with 1, as we don't include the return value)
    for(int i = 1; i < assist_entry->mParameterCount; ++i)
    {
        if(i != 1)
            sprintf_s(buf_ptr,length_remaining,", %s",TinScript::GetRegisteredTypeName(assist_entry->mType[i]));
        else
            strcpy_s(buf_ptr,length_remaining,TinScript::GetRegisteredTypeName(assist_entry->mType[i]));

        // -- update the buf pointer,and the length remaining
        length = strlen(buf_ptr);
        length_remaining -= length;
        buf_ptr += strlen(buf_ptr);
    }

    // -- complete the command
    strcpy_s(buf_ptr,length_remaining,");");

    // -- send the command
    CConsoleWindow::GetInstance()->GetInput()->SetText(buf,cursor_pos);
    CConsoleWindow::GetInstance()->GetInput()->setFocus();
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

bool CDebugFunctionAssistWin::GetSelectedWatchExpression(int32& out_use_watch_id,
                                                         char* out_watch_string, int32 max_expr_length,
                                                         char* out_value_string, int32 max_value_length)
{
    out_use_watch_id = 0;
    *out_watch_string = '\0';
    *out_value_string = '\0';

    uint32 assist_object_id = GetAssistObjectID();
    if (assist_object_id > 0)
        sprintf_s(out_watch_string, max_expr_length,"%d", assist_object_id);

    return true;
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
        TinScript::SafeStrcpy(&sort_buf[1], sizeof(sort_buf) - 1, _entry->mFunctionName, TinScript::kMaxNameLength - 1);
        setText(1, sort_buf);
    }
    else
    {
        // -- set the namespace
        if (_entry->mNamespaceHash != 0)
            setText(0, TinScript::UnHash(_entry->mNamespaceHash));
        else
            setText(0, "");

        // -- set the function name, appended with parenthesis for readability
        setText(1, QString(_entry->mFunctionName).append("()"));

        // -- set the source, C++, or try to find the actual script
        if (_entry->mCodeBlockHash == 0)
        {
            setText(2, QString("[C++]"));
        }
        else
        {
            const char* full_path = TinScript::UnHash(_entry->mCodeBlockHash);
            const char* file_name = CConsoleWindow::GetInstance()->GetDebugSourceWin()->GetFileName(full_path);
            if (file_name != nullptr)
            {
                setText(2, QString(file_name).append(" @ ").append(QString::number(_entry->mLineNumber)));
            }
        }
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
    setColumnCount(3);
    setItemsExpandable(false);
    setExpandsOnDoubleClick(false);
    setColumnWidth(0, CConsoleWindow::StringWidth("WWWWWWWWWWWW"));
    setColumnWidth(1,CConsoleWindow::StringWidth("WWWWWWWWWWWWWWWWWWWW"));

    // -- set the header
  	QTreeWidgetItem* header = new QTreeWidgetItem();
	header->setText(0,QString("Namespace"));
	header->setText(1,QString("Function"));
    header->setText(2,QString("Source"));
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
// constructor:  For an entry representing a variable (type, name, array, value).
// ====================================================================================================================
CFunctionParameterEntry::CFunctionParameterEntry(TinScript::eVarType var_type, bool is_array, const char* _name,
                                                 QTreeWidget* _owner)
    : QTreeWidgetItem(_owner)
{
    mIsOriginEntry = false;

    const char* type_name = TinScript::GetRegisteredTypeName(var_type);
    char type_buf[TinScript::kMaxNameLength];
    sprintf_s(type_buf, "%s%s", type_name, is_array ? "[]" : "");
    setText(0, type_buf);
    setText(1, _name);
}

// ====================================================================================================================
// constructor:  For an entry representing an origin filename and line number.
// ====================================================================================================================
CFunctionParameterEntry::CFunctionParameterEntry(const char* _filename, uint32 _file_hash, int32 _line_number,
                                                 QTreeWidget* _owner) : QTreeWidgetItem(_owner)
{
    mIsOriginEntry = true;
    mFileHash = _file_hash;
    mLineNumber = _line_number;

    // -- the display should show a 1-based line number
    QString display_text = QString(_filename).append(" @ ").append(QString::number(_line_number + 1));
    setText(0, display_text);
}

// ====================================================================================================================
// Deconstructor
// ====================================================================================================================
CFunctionParameterEntry::~CFunctionParameterEntry()
{
}

// ====================================================================================================================
// GetOriginFileLine():  Returns the file name and line number, if this is an entry representing the origin
// ====================================================================================================================
bool8 CFunctionParameterEntry::GetOriginFileLine(uint32& out_file_hash, int32& out_line_number)
{
    if (mIsOriginEntry)
    {
        out_file_hash = mFileHash;
        out_line_number = mLineNumber;
    }
    return mIsOriginEntry;
}

// == class CFunctionParameterList ====================================================================================

// ====================================================================================================================
// constructor
// ====================================================================================================================
CFunctionParameterList::CFunctionParameterList(QWidget* parent)
    : QTreeWidget(parent)
{
    setColumnCount(2);
    setItemsExpandable(false);
    setExpandsOnDoubleClick(false);

    // -- set the header
  	mHeader = new QTreeWidgetItem();
	mHeader->setText(0, QString("Type"));
	mHeader->setText(1, QString("Name"));
	setHeaderItem(mHeader);

    // -- connect up both the clicked and double clicked slots
    QObject::connect(this,SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),this,SLOT(OnDoubleClicked(QTreeWidgetItem*)));
}

// ====================================================================================================================
// deconstructor
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
// PopulateWithSignature():  Given the function entry, extract the list of parameters and populate the view.
// ====================================================================================================================
void CFunctionParameterList::PopulateWithSignature(TinScript::CDebuggerFunctionAssistEntry* assist_entry)
{
    Clear();

    // -- set the header labels
    mHeader->setText(0, QString("Type"));
    mHeader->setText(1, QString("Name"));
    setColumnWidth(0, 120);

    // -- the button was pressed, but if we don't actually have a function to populate, we're done
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

// ====================================================================================================================
// PopulateWithOrigin():  For a given object, populate the output with the origin file and line numbers callstack.
// ====================================================================================================================
void CFunctionParameterList::PopulateWithOrigin(int32 stack_size,const uint32* file_hash_array,const int32* line_array)
{
    Clear();

    // -- set the header labels
    mHeader->setText(0,QString("Source File/Line"));
    mHeader->setText(1,QString(""));
    setColumnWidth(0, 400);
	
	// -- sanity check
	if (file_hash_array == nullptr || line_array == nullptr)
		stack_size = 0;

    for (int i = 0; i < stack_size; ++i)
    {
        const char* full_path = TinScript::UnHash(file_hash_array[i]);
        const char* file_name = CConsoleWindow::GetInstance()->GetDebugSourceWin()->GetFileName(full_path);
        CFunctionParameterEntry* new_parameter =
            new CFunctionParameterEntry(file_name, file_hash_array[i], line_array[i], this);
        mParameterList.append(new_parameter);
    }
}

// ====================================================================================================================
// OnDoubleClicked():  If the item is an origin file/line, we open it in the source view
// ====================================================================================================================
void CFunctionParameterList::OnDoubleClicked(QTreeWidgetItem* _item)
{
    CFunctionParameterEntry* entry = static_cast<CFunctionParameterEntry*>(_item);
    if (entry == nullptr)
        return;


    bool8 GetOriginFileLine(QString& out_file_name,int32& out_line_number);
    uint32 file_hash = 0;
    int32 line_number = -1;
    if (entry->GetOriginFileLine(file_hash, line_number))
    {
        CConsoleWindow::GetInstance()->GetDebugSourceWin()->SetSourceView(file_hash, line_number);
    }
}

// --------------------------------------------------------------------------------------------------------------------
#include "TinQTFunctionAssistWinMoc.cpp"

// ====================================================================================================================
// eof
// ====================================================================================================================
