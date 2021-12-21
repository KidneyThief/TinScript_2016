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
// TinQTObjectInspectWin.cpp : A list view of the members and editable values for an object.
// ====================================================================================================================

#include <QListWidget>
#include <QLabel>
#include <QScroller>
#include <QScrollArea>
#include <QPushButton>

#include "TinScript.h"
#include "TinRegistration.h"

#include "TinQTConsole.h"
#include "TinQTSourceWin.h"
#include "TinQTBreakpointsWin.h"
#include "TinQTObjectBrowserWin.h"
#include "TinQTObjectInspectWin.h"
#include "mainwindow.h"

// == CObjectInspectEntry =============================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CObjectInspectEntry::CObjectInspectEntry(CDebugObjectInspectWin* parent) : QWidget()
{
    mParent = parent;
    mNameLabel = NULL;
    mName[0] = '\0';
    mNameHash = 0;
    mType = TinScript::TYPE_void;
    mValue = NULL;
}

// ====================================================================================================================
// Deconstructor
// ====================================================================================================================
CObjectInspectEntry::~CObjectInspectEntry()
{
    // -- delete the elements
    delete mNameLabel;
    delete mTypeLabel;
    if (mValue)
        delete mValue;
}

// ====================================================================================================================
// Initialize():  Populates the layout with the elements for this 
// ====================================================================================================================
void CObjectInspectEntry::Initialize(const TinScript::CDebuggerWatchVarEntry& debugger_entry)
{
    // -- get the current number of entries added to this window
    int count = mParent->GetEntryCount() + 1;

    QSize parentSize = mParent->size();
    int newWidth = parentSize.width();
	mParent->GetContent()->setGeometry(0, CConsoleWindow::TitleHeight(), newWidth, (count + 2) * CConsoleWindow::TextEditHeight());

    mNameLabel = new QLabel(debugger_entry.mVarName);
    TinScript::SafeStrcpy(mName, sizeof(mName), debugger_entry.mVarName, TinScript::kMaxNameLength);
    mNameHash = debugger_entry.mVarHash;

    // -- if this is not a namespace, add the line edit
    if (debugger_entry.mType != TinScript::TYPE_void)
    {
        mType = debugger_entry.mType;
        mTypeLabel = new QLabel(TinScript::GetRegisteredTypeName(debugger_entry.mType));
        mValue = new SafeLineEdit();
        mValue->setText(debugger_entry.mValue);
        mValue->setMinimumWidth(160);

        // -- hook up the line edit
        QObject::connect(mValue, SIGNAL(returnPressed()), this, SLOT(OnReturnPressed()));
    }
    else
    {
        mTypeLabel = new QLabel("namespace");
    }

    // -- add this to the window
    mParent->GetLayout()->addWidget(mTypeLabel, count, 0, 1, 1);
    mParent->GetLayout()->addWidget(mNameLabel, count, 1, 1, 1);
    mTypeLabel->setFixedHeight(CConsoleWindow::TextEditHeight());
    mNameLabel->setFixedHeight(CConsoleWindow::TextEditHeight());
    if (mValue)
    {
        mParent->GetLayout()->addWidget(mValue, count, 2, 1, 2);
        mValue->setFixedHeight(CConsoleWindow::TextEditHeight());
    }
    mParent->AddEntry(this);

    mParent->GetContent()->updateGeometry();
    mParent->ExpandToParentSize();
}

// ====================================================================================================================
// SetValue():  Update the button text.
// ====================================================================================================================
void CObjectInspectEntry::SetValue(const char* new_value)
{
    if (!new_value)
        new_value = "";
    
    // -- set the value
    if (mValue)
        mValue->setText(new_value ? new_value : "");
}

// ====================================================================================================================
// OnReturnPressed():  Slot hooked up to the line edit, to be executed when the return is pressed.
// ====================================================================================================================
void CObjectInspectEntry::OnReturnPressed()
{
    // -- create the command, by inserting the slider value as the first parameter
    char command_buf[TinScript::kMaxTokenLength];
    sprintf_s(command_buf, "%d.%s = `%s`;", mParent->GetObjectID(), mName, mValue->GetStringValue());

    bool8 is_connected = CConsoleWindow::GetInstance()->IsConnected();
    if (is_connected)
    {
        // -- for sliders, we need to embed the value, so the command is only the function name
        ConsolePrint(0, "%s%s\n", kConsoleSendPrefix, command_buf);
        SocketManager::SendCommand(command_buf);
    }
}

// == CDebugObjectInspectWin ==========================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CDebugObjectInspectWin::CDebugObjectInspectWin(uint32 object_id, const char* object_identifier, QWidget* parent)
    : QWidget(parent)
    , mObjectID(object_id)
{
    TinScript::SafeStrcpy(mWindowName, sizeof(mWindowName), object_identifier, TinScript::kMaxNameLength);
    mScrollArea = new QScrollArea(this);
    mScrollContent = new QWidget(mScrollArea);
    mLayout = new QGridLayout(mScrollContent);
    mLayout->setColumnStretch(2, 1);
    mScrollArea->setWidget(mScrollContent);
    mScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    mScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    // -- add the refresh button
    mRefreshButton = new QPushButton("Refresh");
    mRefreshButton->setFixedHeight(CConsoleWindow::TextEditHeight());
    mLayout->addWidget(mRefreshButton, 0, 0);
    QObject::connect(mRefreshButton, SIGNAL(clicked()), this, SLOT(OnButtonRefreshPressed()));

    const char* object_derivation =
        CConsoleWindow::GetInstance()->GetDebugObjectBrowserWin()->GetObjectDerivation(object_id);
    QLabel* derivation_label = new QLabel("Derivation:");
    derivation_label->setFixedHeight(CConsoleWindow::TextEditHeight());
    QLabel* derivation_content = new QLabel(object_derivation);
    derivation_content->setFixedHeight(CConsoleWindow::TextEditHeight());
    mLayout->addWidget(derivation_label, 0, 1);
    mLayout->addWidget(derivation_content, 0, 2);

    ExpandToParentSize();
}

// ====================================================================================================================
// Destructor
// ====================================================================================================================
CDebugObjectInspectWin::~CDebugObjectInspectWin()
{
}

// ====================================================================================================================
// AddEntry():  Adds an entry to the map of all entries owned by the window.
// ====================================================================================================================
void CDebugObjectInspectWin::AddEntry(CObjectInspectEntry* entry)
{
    if (entry)
    {
        mEntryMap.insert(entry->GetHash(), entry);
    }
}

// ====================================================================================================================
// SetEntryValue():  Given an entry ID, update the DebugEntry's value.
// ====================================================================================================================
void CDebugObjectInspectWin::SetEntryValue(const TinScript::CDebuggerWatchVarEntry& debugger_entry)
{
    // -- see if the entry is in the map
    if (debugger_entry.mVarHash > 0)
    {
        if (mEntryMap.contains(debugger_entry.mVarHash))
        {
            CObjectInspectEntry* entry = mEntryMap[debugger_entry.mVarHash];
            entry->SetValue(debugger_entry.mValue);
        }

        // -- otherwise we need to create the entry
        else
        {
            CObjectInspectEntry* entry = new CObjectInspectEntry(this);
            entry->Initialize(debugger_entry);
            mEntryMap.insert(debugger_entry.mVarHash, entry);
        }
    }
}

// ====================================================================================================================
// OnButtonRefreshPressed():  Called when the refresh button is pressed
// ====================================================================================================================
void CDebugObjectInspectWin::OnButtonRefreshPressed()
{
    // -- send the request to re-populate the inspector for the window's object
    if (SocketManager::IsConnected())
    {
        SocketManager::SendCommandf("DebuggerInspectObject(%d);", mObjectID);
    }
}

// ------------------------------------------------------------------------------------------------
#include "TinQTObjectInspectWinMoc.cpp"

// ------------------------------------------------------------------------------------------------
// eof
// ------------------------------------------------------------------------------------------------
