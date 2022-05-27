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

// -- system includes
#include <string>
#include <sstream>

// -- Qt includes
#include <qgridlayout.h>
#include <qlist.h>
#include <qlabel.h>
#include <QKeyEvent>
#include <QToolTip>
#include <QCheckBox>

// -- TinScript includes
#include "TinScript.h"
#include "TinRegistration.h"
#include "socket.h"

// -- includes
#include "TinQTConsole.h"
#include "TinQTSourceWin.h"
#include "TinQTObjectBrowserWin.h"
#include "TinQTFunctionAssistWin.h"
#include "TinQtPreferences.h"

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
    identifier_widget->setFixedHeight(CConsoleWindow::TextEditHeight() * 3);
    identifier_widget->setMinimumWidth(80);
    QGridLayout* identifier_layout = new QGridLayout(identifier_widget);

    mObjectIndentifier = new QLabel("<global scope>", identifier_widget);

    mObjectFilterCB = new QCheckBox("Objects", this);
    bool checkbox_enabled = TinPreferences::GetInstance()->GetValue("FA_FilterObjects", true);
    mObjectFilterCB->setChecked(checkbox_enabled);

    mNamespaceFilterCB = new QCheckBox("Namespaces", this);
    checkbox_enabled = TinPreferences::GetInstance()->GetValue("FA_FilterNamespaces", true);
    mNamespaceFilterCB->setChecked(checkbox_enabled);

    mFunctionFilterCB = new QCheckBox("Functions", this);
    checkbox_enabled = TinPreferences::GetInstance()->GetValue("FA_FilterFunctions", true);
    mFunctionFilterCB->setChecked(checkbox_enabled);

    mHierarchyFilterCB = new QCheckBox("Hierarchy", this);
    checkbox_enabled = TinPreferences::GetInstance()->GetValue("FA_FilterHierarchy", true);
    mHierarchyFilterCB->setChecked(checkbox_enabled);

	mObjectIndentifier->setFixedHeight(CConsoleWindow::FontHeight());
	QPushButton* show_api_button = new QPushButton("Show Signature", identifier_widget);
    QPushButton* show_origin_button = new QPushButton("Show Object Creation", identifier_widget);
    QPushButton* console_input_button = new QPushButton("Copy To Input", identifier_widget);
    show_api_button->setFixedHeight(CConsoleWindow::TextEditHeight());
    show_origin_button->setFixedHeight(CConsoleWindow::TextEditHeight());
    console_input_button->setFixedHeight(CConsoleWindow::TextEditHeight());

    int column = 0;
    identifier_layout->addWidget(mObjectIndentifier, 0, column++);
    identifier_layout->addWidget(new QLabel(""), 0, column++);
    identifier_layout->addWidget(mObjectFilterCB, 0, column++);
    identifier_layout->addWidget(mNamespaceFilterCB, 0, column++);
    identifier_layout->addWidget(mFunctionFilterCB, 0, column++);
    identifier_layout->addWidget(mHierarchyFilterCB, 0, column++);

    column = 2;
	identifier_layout->addWidget(show_api_button, 1, column++);
    identifier_layout->addWidget(show_origin_button, 1, column++);
    identifier_layout->addWidget(console_input_button, 1, column++);

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
    mSelectedNamespaceHash = 0;
	mSelectedFunctionHash = 0;
    mSelectedObjectID = 0;
	mSearchObjectID = 0;
    mSearchNamespaceHash = 0;
    mFilterString[0] = '\0';

    // -- hook up the button signals
    QObject::connect(mObjectFilterCB, SIGNAL(clicked()), this, SLOT(OnCBFilterObjectsPressed()));
    QObject::connect(mNamespaceFilterCB, SIGNAL(clicked()), this, SLOT(OnCBFilterNamespacesPressed()));
    QObject::connect(mFunctionFilterCB, SIGNAL(clicked()), this, SLOT(OnCBFilterFunctionsPressed()));
    QObject::connect(mHierarchyFilterCB, SIGNAL(clicked()), this, SLOT(OnCBFilterHierarchyPressed()));

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
    mSelectedNamespaceHash = 0;
	mSelectedFunctionHash = 0;
    mSelectedObjectID = 0;
	mParameterList->Clear();
    mFunctionList->Clear();

    // -- clear the function entry map
    for (auto iter : mFunctionEntryList)
    {
        delete iter;
    }
    mFunctionEntryList.clear();
}

// ====================================================================================================================
// FindFunctionEntry():  find given an existing (incomping packet) entry
// ====================================================================================================================
TinScript::CDebuggerFunctionAssistEntry*
    CDebugFunctionAssistWin::FindFunctionEntry(const TinScript::CDebuggerFunctionAssistEntry& in_entry)
{
    return (FindFunctionEntry(in_entry.mEntryType, in_entry.mObjectID, in_entry.mNamespaceHash,
                              in_entry.mFunctionHash));
}


// ====================================================================================================================
// FunctionEntryExists(): find given the type and relevent hash values of an entry
// ====================================================================================================================
TinScript::CDebuggerFunctionAssistEntry*
    CDebugFunctionAssistWin::FindFunctionEntry(TinScript::eFunctionEntryType entry_type, uint32 obj_id, uint32 ns_hash,
                                               uint32 func_hash)
{
    for (auto iter : mFunctionEntryList)
    {
        // -- types must match
        if (iter->mEntryType != entry_type)
            continue;

        switch (entry_type)
        {
            case TinScript::eFunctionEntryType::Namespace:
            {
                if (iter->mNamespaceHash == ns_hash)
                    return iter;
            }
            break;

            case TinScript::eFunctionEntryType::Object:
            {
                if (iter->mObjectID == obj_id)
                    return iter;
            }
            break;

            case TinScript::eFunctionEntryType::Function:
            {
                if (iter->mNamespaceHash == ns_hash && iter->mFunctionHash == func_hash)
                {
                    return iter;
                }
            }
            break;
        }
    }

    // -- not found
    return (nullptr);
}

// ====================================================================================================================
// NotifyFunctionAssistEntry():  The result received from the target, of our request for a function list
// ====================================================================================================================
void CDebugFunctionAssistWin::NotifyFunctionAssistEntry(const TinScript::CDebuggerFunctionAssistEntry& assist_entry)
{
    // -- make sure we have no duplicated
    if (FindFunctionEntry(assist_entry) != nullptr)
        return;

    // -- make a copy of the received entry, and add it to our list
    TinScript::CDebuggerFunctionAssistEntry* new_entry = new TinScript::CDebuggerFunctionAssistEntry();
    memcpy(new_entry, &assist_entry, sizeof(TinScript::CDebuggerFunctionAssistEntry));

    mFunctionEntryList.append(new_entry);

    // -- update the filtered display (given the new entry - we'll see if it needs to be added to the display list)
    UpdateSearchNewEntry(new_entry);
}

// ====================================================================================================================
// NotifyListObjectsComplete():  Called when DebuggerListObjects() has completed sending the complete list
// ====================================================================================================================
void CDebugFunctionAssistWin::NotifyListObjectsComplete()
{
    UpdateFilter(mFilterString, true);
}

// ====================================================================================================================
// NotifyFunctionAssistComplete():  Called when any function assist request has received all response packets
// ====================================================================================================================
void CDebugFunctionAssistWin::NotifyFunctionAssistComplete()
{
    mFunctionList->sortItems(1, Qt::AscendingOrder);

    // -- pull the text directly from the function input
    if (mFunctionInput != nullptr)
    {
        QString filter = mFunctionInput->GetInputString();
        QByteArray text_input = filter.toUtf8();
        const char* search_text = text_input.data();
        if (search_text != nullptr && search_text[0] != '\0')
        {
            UpdateFilter(search_text, false);
        }
    }
}

// ====================================================================================================================
// UpdateSearchNewEntry():  Called when the filter hasn't changed, but we've received new entries
// ====================================================================================================================
void CDebugFunctionAssistWin::UpdateSearchNewEntry(TinScript::CDebuggerFunctionAssistEntry* in_entry)
{
    // -- sanity check
    if (in_entry == nullptr)
        return;

    // -- get our filter flags
    bool filter_by_objects = TinPreferences::GetInstance()->GetValue("FA_FilterObjects", true);
    bool filter_by_namespaces = TinPreferences::GetInstance()->GetValue("FA_FilterNamespaces", true);
    bool filter_by_functions = TinPreferences::GetInstance()->GetValue("FA_FilterFunctions", true);
    bool filter_by_hierarchy = TinPreferences::GetInstance()->GetValue("FA_FilterHierarchy", true);

    // -- see if our new entry passes the types of filters enabled (and of course, our filter string)
    if (((filter_by_functions && in_entry->mEntryType == TinScript::eFunctionEntryType::Function) ||
         (filter_by_namespaces && in_entry->mEntryType == TinScript::eFunctionEntryType::Namespace) ||
         (filter_by_objects && in_entry->mEntryType == TinScript::eFunctionEntryType::Object)) &&
        FilterStringCompare(in_entry->mSearchName))
    {
        mFunctionList->DisplayEntry(in_entry);
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
// FilterStringCompare():  See if the filter is contained within the given string, not including the object id.
// ====================================================================================================================
bool CDebugFunctionAssistWin::FilterStringCompare(const char* string)
{
    // -- there are two uses of the filter here - one, for a method when a '.' for a preceeding object id
    // and the second, when we're searching for (multiple) space delineated substrings
    std::vector<std::string> search_strings;
    const char* method_search = strchr(mFilterString, '.');
    if (method_search != nullptr)
    {
        ++method_search;
        search_strings.push_back(method_search);
    }

    // -- if we're not doing a method search, we need to split the filter string into (space delineated) substrings
    else
    {
        if (mFilterString[0] == '\0')
        {
            search_strings.push_back("");
        }
        else
        {
            std::istringstream f(mFilterString);
            std::string s;
            while (getline(f, s, ' '))
            {
                if (s.size() > 0)
                {
                    search_strings.push_back(s);
                }
            }
        }
    }

    // -- loop to see if the filter is contained anywhere within the string
    // note:  if the filter contains multiple substrings, then a match on any one returns true
    for (const auto& it : search_strings)
    {
        const char* substr_ptr = string;
        while (*substr_ptr != '\0')
        {
            bool temp_exact_match = false;
            bool temp_new_object_search = false;
            bool result = StringContainsFilterImpl(substr_ptr, it.c_str(), temp_exact_match, temp_new_object_search);

            // -- if we found a matching substring, return true
            if (result)
                return (true);

            // -- otherwise, keep looking
            else
                ++substr_ptr;
        }
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

    // -- if the current filter is empty, and the new filter is also empty
    // (e.g.  we pressed return/del/backspace)
    // then we reset the search object and namespace (fresh search)
    if (filter[0] == '\0' && mFilterString[0] == '\0')
    {
        mSearchNamespaceHash = 0;
        mSearchObjectID = 0;
    }

    // -- if we've cleared the search, we're forcing a refresh, and resetting the search
    // -- note:  if we have a search object, the filter string is prepended with the <objectid>.XXX
    // but if we're searching a namespace (after a double click), the filter string will be empty,
    // and we want to continue searching that namespace
    if (mSearchNamespaceHash == 0 && (filter_ptr == nullptr || filter_ptr[0] == '\0'))
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
    if ((mSearchObjectID == 0 && mSearchNamespaceHash == 0) || force_refresh)
    {
        new_object_search = true;
        exact_match = false;
    }

    // -- if we have an exact match, we're done
    if (exact_match)
        return;

    // -- get our filter flags
    bool filter_by_objects = TinPreferences::GetInstance()->GetValue("FA_FilterObjects", true);
    bool filter_by_namespaces = TinPreferences::GetInstance()->GetValue("FA_FilterNamespaces", true);
    bool filter_by_functions = TinPreferences::GetInstance()->GetValue("FA_FilterFunctions", true);
    bool filter_by_hierarchy = TinPreferences::GetInstance()->GetValue("FA_FilterHierarchy", true);

    // -- copy the new search filter
    TinScript::SafeStrcpy(mFilterString, sizeof(mFilterString), filter, kMaxNameLength);

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
            char object_string[kMaxNameLength];
            TinScript::SafeStrcpy(object_string, sizeof(object_string), mFilterString, kMaxNameLength);
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
            char buf[kMaxNameLength];
            if (!object_identifier || !object_identifier[0])
                snprintf(buf, sizeof(buf), "Object: [%d]", object_id);
            else
                snprintf(buf, sizeof(buf), "Object: %s", object_identifier);
            mObjectIndentifier->setText(buf);
        }

        // -- if we found an object, issue the function query
        if (object_id != mSearchObjectID || force_refresh)
        {
            // -- clear the search, and set the new (possibly invalid) object ID
            ClearSearch();
            mSearchObjectID = object_id;
            mSearchNamespaceHash = 0;

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
                        // -- from the ObjectBrowser, add an entry to the FunctionAssistWin, for that object
                        TinScript::CDebuggerFunctionAssistEntry* new_entry =
                            FindFunctionEntry(TinScript::eFunctionEntryType::Object, display_object_id, 0, 0);
                        if (new_entry == nullptr)
                        {
                            new_entry = new TinScript::CDebuggerFunctionAssistEntry();
                            new_entry->mEntryType = TinScript::eFunctionEntryType::Object;
                            new_entry->mObjectID = display_object_id;
                            new_entry->mNamespaceHash = 0;
                            new_entry->mFunctionHash = 0;
                            TinScript::SafeStrcpy(new_entry->mSearchName, sizeof(new_entry->mSearchName), object_name,
                                                  kMaxNameLength);
                            new_entry->mParameterCount = 0;

                            // -- add the object to the object map
                            mFunctionEntryList.append(new_entry);
                        }

                        // -- see if we need to display it
                        if (FilterStringCompare(new_entry->mSearchName))
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
        // -- if the filter itself is a simple number, we want to include objects with that ID in the list
        int32 filter_string_obj_id = TinScript::Atoi(mFilterString, -1);

        for (auto iter : mFunctionEntryList)
        {
            bool should_display = false;
            if (filter_by_objects && mSearchObjectID == 0 && filter_string_obj_id > 0 && iter->mObjectID == filter_string_obj_id)
            {
                should_display = true;
            }

            // -- this is a function
            else if (((filter_by_functions && iter->mEntryType == TinScript::eFunctionEntryType::Function) ||
                      (filter_by_namespaces && iter->mEntryType == TinScript::eFunctionEntryType::Namespace) ||
                      (filter_by_objects && iter->mEntryType == TinScript::eFunctionEntryType::Object)) &&
                     FilterStringCompare(iter->mSearchName))
            {
                should_display = true;
            }

            // -- else if the iter is an object, and we're not searching on a specific object
            // check the derivation and origin for the search string
            else if (filter_by_objects && filter_by_hierarchy && mSearchObjectID == 0 && filter_string_obj_id == 0 &&
                     iter->mEntryType == TinScript::eFunctionEntryType::Object && iter->mObjectID > 0)
            {
                const char* object_derivation =
                    CConsoleWindow::GetInstance()->GetDebugObjectBrowserWin()->GetObjectDerivation(iter->mObjectID);
                if (object_derivation != nullptr)
                {
                    if (FilterStringCompare(object_derivation))
                    {
                        should_display = true;
                    }
                }

                // -- check the origin
                if (!should_display)
                {
                    const char* object_origin =
                        CConsoleWindow::GetInstance()->GetDebugObjectBrowserWin()->GetObjectOrigin(iter->mObjectID);
                    if (FilterStringCompare(object_origin))
                    {
                        should_display = true;
                    }
                }
            }

            if (should_display)
            {
                mFunctionList->DisplayEntry(iter);
            }
            else
            {
                mFunctionList->FilterEntry(iter);
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
        assist_entry = FindFunctionEntry(TinScript::eFunctionEntryType::Function, mSelectedObjectID,
                                         mSelectedNamespaceHash, mSelectedFunctionHash);
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
        mSelectedNamespaceHash = 0;
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
            snprintf(buf, sizeof(buf), "%d.", object_id);
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
// NotifyAssistEntryClicked():  Selecting an assist entry populates the parameter list, if the entry was for a function
// ====================================================================================================================
void CDebugFunctionAssistWin::NotifyAssistEntryClicked(TinScript::CDebuggerFunctionAssistEntry* list_entry)
{
	// -- clear the selected function
    mSelectedNamespaceHash = 0;
	mSelectedFunctionHash = 0;
    mSelectedObjectID = 0;

    // -- if we don't have a matching entry in the list, we're done
    // note:  this is confusing, so to explain:
    // we have two lists - the mFunctionDisplayList (displayed by the QWidget), and
    // the mFunctionEntryList - the "master" list of all entries received...  the
    // mFunctionDisplayList contains a subset of duplicates from the mFunctionEntryList
    if (!list_entry || FindFunctionEntry(*list_entry) == nullptr)
        return;
    
    if (list_entry->mEntryType == TinScript::eFunctionEntryType::Function)
    {
	    // -- cache the selected function hash, and populate the parameter list
        mSelectedNamespaceHash = list_entry->mNamespaceHash;
	    mSelectedFunctionHash = list_entry->mFunctionHash;
        mSelectedObjectID = list_entry->mObjectID;
    }
    else if (list_entry->mEntryType == TinScript::eFunctionEntryType::Object)
    {
        mSelectedNamespaceHash = 0;
        mSelectedFunctionHash = 0;
        mSelectedObjectID = list_entry->mObjectID;
    }
    else if(list_entry->mEntryType == TinScript::eFunctionEntryType::Namespace)
    {
        mSelectedNamespaceHash = list_entry->mNamespaceHash;
        mSelectedFunctionHash = 0;
        mSelectedObjectID = 0;
    }

    // -- will clear the panel and update the header,
    // and if we actually selected a function, will populate the parameters
    DisplayFunctionSignature();
}

// ====================================================================================================================
// NotifyAssistEntryDoubleClicked():  Activating an assist entry issues a command string to the Console Input.
// ====================================================================================================================
void CDebugFunctionAssistWin::NotifyAssistEntryDoubleClicked(TinScript::CDebuggerFunctionAssistEntry* list_entry)
{
	// -- clear the selected function
    mSelectedNamespaceHash = 0;
	mSelectedFunctionHash = 0;
    mSelectedObjectID = 0;

    // -- just in case...
    if (FindFunctionEntry(*list_entry) == nullptr)
        return;

    if (list_entry->mEntryType == TinScript::eFunctionEntryType::Object)
    {
        // -- on double-click, set the filter to be the "<object_name>."
        char new_filter[kMaxNameLength];
        snprintf(new_filter, sizeof(new_filter), "%d.", list_entry->mObjectID);
        mFunctionInput->setText(new_filter);
        UpdateFilter(new_filter);

        if (mSearchObjectID > 0)
        {
            CConsoleWindow::GetInstance()->GetDebugObjectBrowserWin()->SetSelectedObject(mSearchObjectID);
        }
    }
    else if (list_entry->mEntryType == TinScript::eFunctionEntryType::Function)
    {
		// -- cache the selected function hash, and populate the parameter list
        mSelectedNamespaceHash = list_entry->mNamespaceHash;
		mSelectedFunctionHash = list_entry->mFunctionHash;
		TinScript::CDebuggerFunctionAssistEntry* assist_entry =
            FindFunctionEntry(TinScript::eFunctionEntryType::Function, list_entry->mObjectID,
                              mSelectedNamespaceHash, mSelectedFunctionHash);

        // -- open the function implementation in the source view
        if (assist_entry != nullptr)
        {
            CConsoleWindow::GetInstance()->GetDebugSourceWin()->SetSourceView(assist_entry->mCodeBlockHash,
                                                                              assist_entry->mLineNumber - 1);
        }
    }
    else if (list_entry->mEntryType == TinScript::eFunctionEntryType::Namespace)
    {
        // -- update the label
        mObjectIndentifier->setText(QString("Namespace: ").append(
            CConsoleWindow::GetInstance()->UnhashOrRequest(list_entry->mNamespaceHash)));

        // -- if the entry we clicked is a namespace, not a specific function or object
        // we want to request the functions registered to that hierarchy
        mSearchNamespaceHash = list_entry->mNamespaceHash;
        mSearchObjectID = 0;
        mFunctionInput->setText("");
        mFilterString[0] = '\0';
        ClearSearch();

        // -- send a message to retrieve all method implemented for the hierarchy starting at our namespace
        if (SocketManager::IsConnected())
        {
            SocketManager::SendCommandf("DebuggerRequestNamespaceAssist(%d);", mSearchNamespaceHash);
        }
    }

    // -- display the function signature - if an object was double-clicked, then this 
    // will clear the panel and update the header
    DisplayFunctionSignature();
}

// ====================================================================================================================
// OnCBFilterObjectsPressed():  Update the filter to to include objects
// ====================================================================================================================
void CDebugFunctionAssistWin::OnCBFilterObjectsPressed()
{
    bool enabled = mObjectFilterCB->isChecked();
    TinPreferences::GetInstance()->SetValue("FA_FilterObjects", enabled);
    UpdateFilter(mFilterString, true);
}

// ====================================================================================================================
// OnCBFilterNamespacesPressed():  Update the filter to to include namespaces
// ====================================================================================================================
void CDebugFunctionAssistWin::OnCBFilterNamespacesPressed()
{
    bool enabled = mNamespaceFilterCB->isChecked();
    TinPreferences::GetInstance()->SetValue("FA_FilterNamespaces", enabled);
    UpdateFilter(mFilterString, true);
}

// ====================================================================================================================
// OnCBFilterNamespacesPressed():  Update the filter to to include functions
// ====================================================================================================================
void CDebugFunctionAssistWin::OnCBFilterFunctionsPressed()
{
    bool enabled = mFunctionFilterCB->isChecked();
    TinPreferences::GetInstance()->SetValue("FA_FilterFunctions", enabled);
    UpdateFilter(mFilterString, true);
}

// ====================================================================================================================
// OnCBFilterHierarchyPressed():  Update the filter to to include functions
// ====================================================================================================================
void CDebugFunctionAssistWin::OnCBFilterHierarchyPressed()
{
    bool enabled = mHierarchyFilterCB->isChecked();
    TinPreferences::GetInstance()->SetValue("FA_FilterHierarchy", enabled);
    UpdateFilter(mFilterString, true);
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
    if (mSearchObjectID == 0 || mSelectedFunctionHash == 0)
    {
        return;
    }

    // -- get the function we want to execute from the console input
    TinScript::CDebuggerFunctionAssistEntry* assist_entry =
        FindFunctionEntry(TinScript::eFunctionEntryType::Function, mSelectedObjectID, mSelectedNamespaceHash,
                          mSelectedFunctionHash);
    if (assist_entry == nullptr)
        return;

    // -- create the command string, and send it to the console input
    char buf[kMaxTokenLength];
    int length_remaining = kMaxTokenLength;
    if(mSearchObjectID > 0)
        snprintf(buf, sizeof(buf), "%d.%s(", mSearchObjectID, assist_entry->mSearchName);
    else
        snprintf(buf, sizeof(buf), "%s(", assist_entry->mSearchName);

    int length = strlen(buf);
    length_remaining -= length;
    char* buf_ptr = &buf[length];

    // -- note:  we want the cursor to be placed at the beginning of the parameter list
    int cursor_pos = length;

    // -- fill in the parameters (starting with 1, as we don't include the return value)
    for (int i = 1; i < assist_entry->mParameterCount; ++i)
    {
        const char* type_name = TinScript::GetRegisteredTypeName(assist_entry->mType[i]);
        if (type_name == nullptr)
        {
            if (mSearchObjectID > 0)
                snprintf(buf, sizeof(buf), "%d.%s() has an invalid signature - REGISTER_METHOD() contains an unregistered param type",
                    mSearchObjectID, assist_entry->mSearchName);
            else
                snprintf(buf, sizeof(buf), "%s() has an invalid signature - REGISTER_FUNCTION() contains an unregistered param type",
                    assist_entry->mSearchName);

            CConsoleWindow::GetInstance()->AddText(0, 0, const_cast<char*>(buf));
            return;
        }

        if (i != 1)
            snprintf(buf_ptr, length_remaining, ", %s", type_name);
        else
            snprintf(buf_ptr, length_remaining, "%s", type_name);

        // -- update the buf pointer,and the length remaining
        length = strlen(buf_ptr);
        length_remaining -= length;
        buf_ptr += strlen(buf_ptr);
    }

    // -- complete the command
    snprintf(buf_ptr, length_remaining,"%s", ");");

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

// ====================================================================================================================
// GetSelectedWatchExpression():  the "watch expression dialog" needs to be initialized from whatever is selected
// ====================================================================================================================
bool CDebugFunctionAssistWin::GetSelectedWatchExpression(int32& out_use_watch_id,
                                                         char* out_watch_string, int32 max_expr_length,
                                                         char* out_value_string, int32 max_value_length)
{
    out_use_watch_id = 0;
    *out_watch_string = '\0';
    *out_value_string = '\0';

    uint32 assist_object_id = GetAssistObjectID();
    if (assist_object_id > 0)
        snprintf(out_watch_string, max_expr_length, "%d", assist_object_id);

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
// GetInputString():  get the text to use for the Function Assist search
// ====================================================================================================================
QString CFunctionAssistInput::GetInputString() const
{
    return text();
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

    // -- don't do anything on a modifier key press
    if (event->key() != Qt::Key_CapsLock && event->key() != Qt::Key_Shift && event->key() != Qt::Key_Alt &&
        event->key() != Qt::Key_Control)
    {
        // -- get the current text, see if our search string has changed
        QByteArray text_input = text().toUtf8();
        const char* search_text = text_input.data();
        mOwner->UpdateFilter(search_text);
    }
}

// == class CFunctionListEntry ========================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CFunctionListEntry::CFunctionListEntry(TinScript::CDebuggerFunctionAssistEntry* _entry, QTreeWidget* _owner)
    : QTreeWidgetItem(_owner)
    , mFunctionAssistEntry(_entry)
{
    // -- sanity check
    if (_entry == nullptr)
    {
        setHidden(true);
        return;
    }

    if (_entry->mEntryType == TinScript::eFunctionEntryType::Object)
    {
        const char* object_identifier =
            CConsoleWindow::GetInstance()->GetDebugObjectBrowserWin()->GetObjectIdentifier(_entry->mObjectID);
        setText(0, object_identifier);

        // -- set string for the "function" column, which is the derivation for objects
        const char* object_derivation = CConsoleWindow::GetInstance()->GetDebugObjectBrowserWin()->GetObjectDerivation(_entry->mObjectID);
        setText(1, object_derivation);

        // -- set the source to denote an object
        setText(2, QString("<object>"));
    }
    else if (_entry->mEntryType == TinScript::eFunctionEntryType::Function)
    {
        // -- set the namespace
        if (_entry->mNamespaceHash != 0)
            setText(0, CConsoleWindow::GetInstance()->UnhashOrRequest(_entry->mNamespaceHash));
        else
            setText(0, "");

        // -- set the function name, appended with parenthesis for readability
        setText(1, QString(_entry->mSearchName).append("()"));

        // -- set the source, C++, or try to find the actual script
        if (_entry->mCodeBlockHash == 0)
        {
            setText(2, QString("[C++]"));
        }
        else
        {
            const char* full_path = CConsoleWindow::GetInstance()->UnhashOrRequest(_entry->mCodeBlockHash);
            const char* file_name = CConsoleWindow::GetInstance()->GetDebugSourceWin()->GetFileName(full_path);
            if (file_name != nullptr)
            {
                setText(2, QString(file_name).append(" @ ").append(QString::number(_entry->mLineNumber)));
            }
        }
    }
    else if (_entry->mEntryType == TinScript::eFunctionEntryType::Namespace)
    {
        // -- set the namespace
        if (_entry->mNamespaceHash != 0)
            setText(0, CConsoleWindow::GetInstance()->UnhashOrRequest(_entry->mNamespaceHash));
        else
            setText(0,"");

        // -- for namespaces, we want to clear the function 
        setText(1, "");
        setText(2, QString("<namespace>"));
    }

    // -- all new entries begin hidden
    setHidden(true);
}

// ====================================================================================================================
// Deconstructor
// ====================================================================================================================
CFunctionListEntry::~CFunctionListEntry()
{
    // note:  emphasizing that this pointer is owned by CDebugFunctionAssistWin::mFunctionEntryList
    mFunctionAssistEntry = nullptr;
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
	header->setText(1,QString("Function / Derivation"));
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
    // -- sanity check
    if (assist_entry == nullptr)
        return nullptr;

    for (int i = 0; i < mFunctionList.size(); ++i)
    {
        CFunctionListEntry* entry = mFunctionList[i];

        switch (assist_entry->mEntryType)
        {
            case TinScript::eFunctionEntryType::Object:
            {
                if (entry->mFunctionAssistEntry->mEntryType == TinScript::eFunctionEntryType::Object &&
                    entry->mFunctionAssistEntry->mObjectID == assist_entry->mObjectID)
                {
                    return (entry);
                }
            }
            break;

            case TinScript::eFunctionEntryType::Function:
            {
                if (entry->mFunctionAssistEntry->mEntryType == TinScript::eFunctionEntryType::Function &&
                    entry->mFunctionAssistEntry->mFunctionHash == assist_entry->mFunctionHash)
                {
                    return (entry);
                }
            }
            break;

            case TinScript::eFunctionEntryType::Namespace:
            {
                if(entry->mFunctionAssistEntry->mEntryType == TinScript::eFunctionEntryType::Namespace &&
                    entry->mFunctionAssistEntry->mNamespaceHash == assist_entry->mNamespaceHash)
                {
                    return (entry);
                }
            }
            break;
        }
    }

    // -- not found
    return (nullptr);
}

// ====================================================================================================================
// DisplayEntry():  Unhide or create an list entry for the given function.
// ====================================================================================================================
bool CFunctionAssistList::DisplayEntry(TinScript::CDebuggerFunctionAssistEntry* in_entry)
{
    // -- sanity check
    if (in_entry == nullptr)
        return false;

    // -- find the entry (or create it, if needed)
    bool visibility_changed = false;
    CFunctionListEntry* entry = FindEntry(in_entry);
    if (!entry)
    {
        entry = new CFunctionListEntry(in_entry, this);
        mFunctionList.append(entry);
        visibility_changed = true;
    }

    // -- make the entry visible
    visibility_changed = visibility_changed || entry->isHidden();
    entry->setHidden(false);

    // -- if the visibility of any element changes, we'll need to resort
    return visibility_changed;
}

// ====================================================================================================================
// FilterEntry():  Ensure the given function is hidden.
// ====================================================================================================================
bool CFunctionAssistList::FilterEntry(TinScript::CDebuggerFunctionAssistEntry* assist_entry)
{
    // -- sanity check
    if (assist_entry == nullptr)
        return false;

    // -- find the entry - set it hidden if it exists
    bool visibility_changed = false;
    CFunctionListEntry* entry = FindEntry(assist_entry);
    if (entry)
    {
        visibility_changed = !entry->isHidden();
        entry->setHidden(true);
    }

    // -- if the visibility of any element changes, we'll need to resort
    return visibility_changed;
}

// ====================================================================================================================
// Clear():  empties the FunctionEntryList that is used for display
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

// ====================================================================================================================
// operator<():  used for sorting the actual visible entries
// ====================================================================================================================
bool CFunctionListEntry::operator<(const QTreeWidgetItem& other) const
{
    const CFunctionListEntry& other_entry = static_cast<const CFunctionListEntry&>(other);

    CDebugFunctionAssistWin* assist_win = CConsoleWindow::GetInstance()->GetDebugFunctionAssistWin();
    uint32 selected_obj_id = assist_win ? assist_win->GetAssistObjectID() : 0;

    // sort by object, then functions, then namespaces
    if (mFunctionAssistEntry->mEntryType == other_entry.mFunctionAssistEntry->mEntryType)
    {
        switch (mFunctionAssistEntry->mEntryType)
        {
            // objects are sorted by name
            case TinScript::eFunctionEntryType::Object:
                return (QString(mFunctionAssistEntry->mSearchName) < QString(other_entry.mFunctionAssistEntry->mSearchName));

            // -- functions are sorted within the same namespace only
            case TinScript::eFunctionEntryType::Function:
                if (mFunctionAssistEntry->mNamespaceHash == other_entry.mFunctionAssistEntry->mNamespaceHash)
                {
                    return (QString(mFunctionAssistEntry->mSearchName) < QString(other_entry.mFunctionAssistEntry->mSearchName));
                }
                else
                {
                    return false;
                }

            // -- namespaces are sorted, if we're searching the global context only!
            // otherwise, namespace entries are in order or the selected object's hierarchy, as the packets are received
            case TinScript::eFunctionEntryType::Namespace:
                if (selected_obj_id == 0)
                {
                    return (QString(mFunctionAssistEntry->mSearchName) < QString(other_entry.mFunctionAssistEntry->mSearchName));
                }
                else
                {
                    return false;
                }
				
			default:
				return false;
        }
    }

    // -- objects first
    if (mFunctionAssistEntry->mEntryType == TinScript::eFunctionEntryType::Object)
        return true;

    else if (other_entry.mFunctionAssistEntry->mEntryType == TinScript::eFunctionEntryType::Object)
        return false;

    // -- then functions
    else if (mFunctionAssistEntry->mEntryType == TinScript::eFunctionEntryType::Function)
        return true;

    else if (other_entry.mFunctionAssistEntry->mEntryType == TinScript::eFunctionEntryType::Function)
        return false;

    return false;
}

// ====================================================================================================================
// OnClicked():  slot executed when an entry in the assist window is clicked
// ====================================================================================================================
void CFunctionAssistList::OnClicked(QTreeWidgetItem* item)
{
    CFunctionListEntry* entry = static_cast<CFunctionListEntry*>(item);
    mOwner->NotifyAssistEntryClicked(entry->mFunctionAssistEntry);

    if (entry != nullptr && entry->mFunctionAssistEntry != nullptr && entry->mFunctionAssistEntry->mHelpString[0] != '\0')
    {
        QToolTip::showText(QCursor::pos(), QString(entry->mFunctionAssistEntry->mHelpString));
    }
}

// ====================================================================================================================
// OnDoubleClicked():  slot executed when an entry in the assist window is double-clicked
// ====================================================================================================================
void CFunctionAssistList::OnDoubleClicked(QTreeWidgetItem* item)
{
    CFunctionListEntry* entry = static_cast<CFunctionListEntry*>(item);
    mOwner->NotifyAssistEntryDoubleClicked(entry->mFunctionAssistEntry);
}

// == class CFunctionParameterEntry ========================================================================================

// ====================================================================================================================
// constructor:  For an entry representing a variable (type, name, array, value).
// ====================================================================================================================
CFunctionParameterEntry::CFunctionParameterEntry(TinScript::eVarType var_type, bool is_array, const char* _name,
                                                 uint32* default_value, QTreeWidget* _owner)
    : QTreeWidgetItem(_owner)
{
    mIsOriginEntry = false;

    const char* type_name = TinScript::GetRegisteredTypeName(var_type);
    char type_buf[kMaxNameLength];
    snprintf(type_buf, sizeof(type_buf), "%s%s", type_name, is_array ? "[]" : "");
    setText(0, type_buf);
    setText(1, _name);

    // -- see if we have a default value
    // -- this is weird, but we can use the tool's integration of TinScript to convert from the default value to
    // a string format
    if (default_value != nullptr)
    {
        const char* value_as_string = TinScript::CRegDefaultArgValues::GetDefaultValueAsString(var_type, default_value, true);
        if (value_as_string != nullptr)
        {
            // -- wrap in quotes, if needed
            if (var_type == TinScript::TYPE_string)
                setText(2, QString("`").append(value_as_string).append("`"));
            else
                setText(2, value_as_string);
        }
    }
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
    mHeader->setText(2, QString("Default"));
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
        const char* var_name = CConsoleWindow::GetInstance()->UnhashOrRequest(assist_entry->mNameHash[i]);
        // note:  parameter 0 is the return value, and cannot have a default value
        uint32* default_value = i > 0 && assist_entry->mHasDefaultValues ? assist_entry->mDefaultValue[i] : nullptr;
        CFunctionParameterEntry* new_parameter = new CFunctionParameterEntry(var_type, is_array, var_name,
                                                                             default_value, this);
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
        const char* full_path = CConsoleWindow::GetInstance()->UnhashOrRequest(file_hash_array[i]);
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
