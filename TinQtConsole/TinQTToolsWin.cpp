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
// TinQTToolsWin.cpp : A list view of tool widgets used to conveniently submit commands to a target application.
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
#include "TinQTToolsWin.h"
#include "mainwindow.h"

// --------------------------------------------------------------------------------------------------------------------
// -- statics
int32 CDebugToolEntry::gToolsWindowElementIndex = 0;
QMap<int32, CDebugToolEntry*> CDebugToolsWin::gDebugToolEntryMap;
QMap<uint32, CDebugToolEntry*> CDebugToolsWin::gDebugToolEntryNamedMap;

// == CDebugToolEntry =================================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CDebugToolEntry::CDebugToolEntry(CDebugToolsWin* parent) : QWidget()
{
    mEntryID = 0;
    mEntryNameHash = 0;
    mParent = parent;
}

// ====================================================================================================================
// Deconstructor
// ====================================================================================================================
CDebugToolEntry::~CDebugToolEntry()
{
    // -- delete the elements
    delete mName;
    delete mDescription;

    // -- remove this entry from the global map
    if (CDebugToolsWin::gDebugToolEntryMap[mEntryID] == this)
    {
        CDebugToolsWin::gDebugToolEntryMap[mEntryID] = NULL;
    }

    // -- remove this entry from the named map
    if (mEntryNameHash != 0 && CDebugToolsWin::gDebugToolEntryNamedMap[mEntryNameHash] == this)
    {
        CDebugToolsWin::gDebugToolEntryNamedMap.remove(mEntryNameHash);
    }
}

// ====================================================================================================================
// Initialize():  Populates the layout with the elements for this 
// ====================================================================================================================
int32 CDebugToolEntry::Initialize(const char* name, const char* description, QWidget* element)
{
    // -- ensure we don't initialize multiple times
    if (mEntryID > 0)
        return (mEntryID);

    mEntryID = ++gToolsWindowElementIndex;

    // -- get the current number of entries added to this window
    int count = mParent->GetEntryCount();

    QSize parentSize = mParent->size();
    int newWidth = parentSize.width();
    setMinimumHeight(CConsoleWindow::TextEditHeight());
	mParent->GetContent()->setGeometry(0, CConsoleWindow::TitleHeight(), newWidth, (count + 2) * CConsoleWindow::TextEditHeight());

    mName = new QLabel(name);
    mDescription = new QLabel(description);

    // -- add this to the window
    mParent->GetLayout()->addWidget(mName, count, 0, 1, 1);
    mParent->GetLayout()->addWidget(element, count, 1, 1, 1);
    mParent->GetLayout()->addWidget(mDescription, count, 2, 1, 1);
    mParent->AddEntry(this);

    mParent->GetContent()->updateGeometry();
    mParent->ExpandToParentSize();

    // -- add the entry to the global map
    CDebugToolsWin::gDebugToolEntryMap[mEntryID] = this;

    // -- add the entry to the named map
    char hash_string[kMaxNameLength];
    snprintf(hash_string, sizeof(hash_string), "%s::%s", mParent != nullptr ? mParent->GetWindowName() : "<unnamed>", name);
    mEntryNameHash = TinScript::Hash(hash_string);
    if (!CDebugToolsWin::gDebugToolEntryNamedMap.contains(mEntryNameHash))
        CDebugToolsWin::gDebugToolEntryNamedMap[mEntryNameHash] = this;
    else
        mEntryNameHash = 0;

    return (mEntryID);
}

// ====================================================================================================================
// SetDescription():  Update the description of the CDebugToolEntry
// ====================================================================================================================
void CDebugToolEntry::SetDescription(const char* new_description)
{
    if (!new_description)
        new_description = "";
    mDescription->setText(new_description);
}

// == CDebugToolMessage ===============================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CDebugToolMessage::CDebugToolMessage(const char* message, CDebugToolsWin* parent) : CDebugToolEntry(parent)
{
    mMessage = new QLabel(message);
    Initialize("", "", mMessage);
}

// ====================================================================================================================
// Deconstructor
// ====================================================================================================================
CDebugToolMessage::~CDebugToolMessage()
{
    delete mMessage;
}

// ====================================================================================================================
// SetValue():  Update the message text
// ====================================================================================================================
void CDebugToolMessage::SetValue(const char* new_value)
{
    if (!new_value)
        new_value = "";

    mMessage->setText(new_value);
}

// == CDebugToolButton ==================================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CDebugToolButton::CDebugToolButton(const char* name, const char* description, const char* value, const char* command,
                                   CDebugToolsWin* parent)
    : CDebugToolEntry(parent)
{
    // -- copy the command
    TinScript::SafeStrcpy(mCommand, sizeof(mCommand), command, kMaxTokenLength);

    // -- create the button
    mButton = new QPushButton(value);
    Initialize(name, description, mButton);

    // -- hook up the button
    QObject::connect(mButton, SIGNAL(clicked()), this, SLOT(OnButtonPressed()));
};

// ====================================================================================================================
// Deconstructor
// ====================================================================================================================
CDebugToolButton::~CDebugToolButton()
{
    delete mButton;
}

// ====================================================================================================================
// SetValue():  Update the button text.
// ====================================================================================================================
void CDebugToolButton::SetValue(const char* new_value)
{
    if (!new_value)
        new_value = "";
    mButton->setText(new_value);
}

// ====================================================================================================================
// OnButtonPressed():  Slot hooked up to the button, to execute the command when pressed.
// ====================================================================================================================
void CDebugToolButton::OnButtonPressed()
{
    bool8 is_connected = CConsoleWindow::GetInstance()->IsConnected();
    if (is_connected)
    {
        ConsolePrint(0, "%s%s\n", kConsoleSendPrefix, mCommand);
        SocketManager::SendCommandf(mCommand);
    }
    else
    {
        ConsolePrint(0, "%s%s\n", kLocalSendPrefix, mCommand);
        TinScript::ExecCommand(mCommand);
    }
}

// == CDebugToolSlider ================================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CDebugToolSlider::CDebugToolSlider(const char* name, const char* description, int32 min_value, int32 max_value,
                                   int32 cur_value, const char* command, CDebugToolsWin* parent)

    : CDebugToolEntry(parent)
{
    // -- copy the command
    TinScript::SafeStrcpy(mCommand, sizeof(mCommand), command, kMaxTokenLength);

    // -- create the button
    mSlider = new QSlider(Qt::Horizontal);
    mSlider->setRange(min_value, max_value);
    mSlider->setValue(cur_value);
    mSlider->setMinimumWidth(160);
    mSlider->setTickPosition(QSlider::TicksBelow);
    mSlider->setTickInterval((max_value - min_value) / 10);
    Initialize(name, description, mSlider);

    // -- hook up the button
    QObject::connect(mSlider, SIGNAL(sliderReleased()), this, SLOT(OnSliderReleased()));
};

// ====================================================================================================================
// Deconstructor
// ====================================================================================================================
CDebugToolSlider::~CDebugToolSlider()
{
    delete mSlider;
}

// ====================================================================================================================
// SetValue():  Update the button text.
// ====================================================================================================================
void CDebugToolSlider::SetValue(const char* new_value)
{
    if (!new_value)
        new_value = "";

    int32 int_value = TinScript::Atoi(new_value);
    mSlider->setValue(int_value);
}

// ====================================================================================================================
// OnSliderReleased():  Slot hooked up to the slider, to be executed when the slider is released.
// ====================================================================================================================
void CDebugToolSlider::OnSliderReleased()
{
    // -- create the command, by inserting the slider value as the first parameter
    char command_buf[kMaxTokenLength];
    if (!mCommand[0])
        snprintf(command_buf, sizeof(command_buf), "Print(%d);", mSlider->value());
    else
        snprintf(command_buf, sizeof(command_buf), "%s(%d);", mCommand, mSlider->value());

    bool8 is_connected = CConsoleWindow::GetInstance()->IsConnected();
    if (is_connected)
    {
        // -- for sliders, we need to embed the value, so the command is only the function name
        ConsolePrint(0, "%s%s\n", kConsoleSendPrefix, command_buf);
        SocketManager::SendCommandf(command_buf);
    }
    else
    {
        ConsolePrint(0, "%s%s\n", kLocalSendPrefix, command_buf);
        TinScript::ExecCommand(command_buf);
    }
}

// == CDebugToolTextEdit ==============================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CDebugToolTextEdit::CDebugToolTextEdit(const char* name, const char* description, const char* cur_value,
                                       const char* command, CDebugToolsWin* parent)

    : CDebugToolEntry(parent)
{
    // -- copy the command
    TinScript::SafeStrcpy(mCommand, sizeof(mCommand), command, kMaxTokenLength);

    // -- create the button
    mLineEdit = new SafeLineEdit();
    mLineEdit->setText(cur_value ? cur_value : "");
    mLineEdit->setMinimumWidth(160);
    Initialize(name, description, mLineEdit);

    // -- hook up the button
    QObject::connect(mLineEdit, SIGNAL(returnPressed()), this, SLOT(OnReturnPressed()));
};

// ====================================================================================================================
// Deconstructor
// ====================================================================================================================
CDebugToolTextEdit::~CDebugToolTextEdit()
{
    delete mLineEdit;
}

// ====================================================================================================================
// SetValue():  Update the button text.
// ====================================================================================================================
void CDebugToolTextEdit::SetValue(const char* new_value)
{
    if (!new_value)
        new_value = "";

    mLineEdit->setText(new_value ? new_value : "");
}

// ====================================================================================================================
// OnReturnPressed():  Slot hooked up to the line edit, to be executed when the return is pressed.
// ====================================================================================================================
void CDebugToolTextEdit::OnReturnPressed()
{
    // -- create the command, by inserting the slider value as the first parameter
    char command_buf[kMaxTokenLength];
    if (!mCommand[0])
        snprintf(command_buf, sizeof(command_buf), "Print(`%s`);", mLineEdit->GetStringValue());
    else
        snprintf(command_buf, sizeof(command_buf), "%s(`%s`);", mCommand, mLineEdit->GetStringValue());

    bool8 is_connected = CConsoleWindow::GetInstance()->IsConnected();
    if (is_connected)
    {
        // -- for sliders, we need to embed the value, so the command is only the function name
        ConsolePrint(0, "%s%s\n", kConsoleSendPrefix, command_buf);
        SocketManager::SendCommandf(command_buf);
    }
    else
    {
        ConsolePrint(0, "%s%s\n", kLocalSendPrefix, command_buf);
        TinScript::ExecCommand(command_buf);
    }
}

// == CDebugToolCheckBox ==============================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CDebugToolCheckBox::CDebugToolCheckBox(const char* name, const char* description, bool cur_value,
                                       const char* command, CDebugToolsWin* parent)

    : CDebugToolEntry(parent)
{
    // -- copy the command
    TinScript::SafeStrcpy(mCommand, sizeof(mCommand), command, kMaxTokenLength);

    // -- create the button
    mCheckBox = new QCheckBox();
    mCheckBox->setCheckState(cur_value ? Qt::Checked : Qt::Unchecked);
    Initialize(name, description, mCheckBox);

    // -- hook up the button
    QObject::connect(mCheckBox, SIGNAL(clicked()), this, SLOT(OnClicked()));
};

// ====================================================================================================================
// Deconstructor
// ====================================================================================================================
CDebugToolCheckBox::~CDebugToolCheckBox()
{
    delete mCheckBox;
}

// ====================================================================================================================
// SetValue():  Update the button text.
// ====================================================================================================================
void CDebugToolCheckBox::SetValue(const char* new_value)
{
    bool bool_value = false;
    TinScript::StringToBool(TinScript::GetContext(), &bool_value, (char*)new_value);
    mCheckBox->setCheckState(bool_value ? Qt::Checked : Qt::Unchecked);
}

// ====================================================================================================================
// OnClicked():  Slot hooked up to the line edit, to be executed when the return is pressed.
// ====================================================================================================================
void CDebugToolCheckBox::OnClicked()
{
    // -- create the command, by inserting the slider value as the first parameter
    bool value = (mCheckBox->checkState() == Qt::Checked);
    char command_buf[kMaxTokenLength];
    if (!mCommand[0])
        snprintf(command_buf, sizeof(command_buf), "Print(%s);", value ? "true" : "false");
    else
        snprintf(command_buf, sizeof(command_buf), "%s(%s);", mCommand, value ? "true" : "false");

    bool8 is_connected = CConsoleWindow::GetInstance()->IsConnected();
    if (is_connected)
    {
        // -- for sliders, we need to embed the value, so the command is only the function name
        ConsolePrint(0, "%s%s\n", kConsoleSendPrefix, command_buf);
        SocketManager::SendCommandf(command_buf);
    }
    else
    {
        ConsolePrint(0, "%s%s\n", kLocalSendPrefix, command_buf);
        TinScript::ExecCommand(command_buf);
    }
}

// == CDebugToolsWin ==================================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CDebugToolsWin::CDebugToolsWin(const char* tools_name, QWidget* parent) : QWidget(parent)
{
    TinScript::SafeStrcpy(mWindowName, sizeof(mWindowName), tools_name, kMaxNameLength);
    mScrollArea = new QScrollArea(this);
    mScrollContent = new QWidget(mScrollArea);
    mLayout = new QGridLayout(mScrollContent);
    mLayout->setColumnStretch(2, 1);
    mScrollArea->setWidget(mScrollContent);
    mScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    mScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ExpandToParentSize();
}

// ====================================================================================================================
// Destructor
// ====================================================================================================================
CDebugToolsWin::~CDebugToolsWin()
{
}

// ====================================================================================================================
// ClearAll():  Delete all elements,from this window - allows the window to be repopulated
// ====================================================================================================================
void CDebugToolsWin::ClearAll()
{
    for (int i = 0; i < mEntryList.size(); ++i)
    {
        CDebugToolEntry* entry = mEntryList[i];
        delete entry;
    }
    mEntryList.clear();

    // -- re-create the layout, which should clean up oall the widgets parented to it
    delete mLayout;
    mLayout = new QGridLayout(mScrollContent);
    mLayout->setColumnStretch(2, 1);
}

// ====================================================================================================================
// AddMessage():  Adds a gui entry of type "message" to the ToolPalette window
// ====================================================================================================================
int32 CDebugToolsWin::AddMessage(const char* message)
{
    // -- create the message entry
    CDebugToolMessage* new_entry = new CDebugToolMessage(message, this);
    if (new_entry)
        return (new_entry->GetEntryID());

    // -- failed to create the message
    return (0);
}

// ====================================================================================================================
// AddButton():  Adds a gui entry of type "button" to the ToolPalette window
// ====================================================================================================================
int32 CDebugToolsWin::AddButton(const char* name, const char* description, const char* value, const char* command)
{
    // -- create the message entry
    CDebugToolButton* new_entry = new CDebugToolButton(name, description, value, command, this);
    if (new_entry)
        return (new_entry->GetEntryID());

    // -- failed to create the message
    return (0);
}

// ====================================================================================================================
// AddSlider():  Adds a gui entry of type "slider" to the ToolPalette window
// ====================================================================================================================
int32 CDebugToolsWin::AddSlider(const char* name, const char* description, int32 min_value, int32 max_value,
                                int32 cur_value, const char* command)
{
    // -- create the message entry
    CDebugToolSlider* new_entry = new CDebugToolSlider(name, description, min_value, max_value, cur_value,
                                                       command, this);
    if (new_entry)
        return (new_entry->GetEntryID());

    // -- failed to create the message
    return (0);
}

// ====================================================================================================================
// AddTextEdit():  Adds a gui entry of type "text edit" to the ToolPalette window
// ====================================================================================================================
int32 CDebugToolsWin::AddTextEdit(const char* name, const char* description, const char* cur_value,
                                  const char* command)
{
    // -- create the message entry
    CDebugToolTextEdit* new_entry = new CDebugToolTextEdit(name, description, cur_value, command, this);
    if (new_entry)
        return (new_entry->GetEntryID());

    // -- failed to create the message
    return (0);
}

// ====================================================================================================================
// AddCheckBox():  Adds a gui entry of type "text edit" to the ToolPalette window
// ====================================================================================================================
int32 CDebugToolsWin::AddCheckBox(const char* name, const char* description, bool cur_value, const char* command)
{
    // -- create the message entry
    CDebugToolCheckBox* new_entry = new CDebugToolCheckBox(name, description, cur_value, command, this);
    if (new_entry)
        return (new_entry->GetEntryID());

    // -- failed to create the message
    return (0);
}

// ====================================================================================================================
// SetEntryDescription():  Given an entry name, update the DebugEntry's description.
// ====================================================================================================================
void CDebugToolsWin::SetEntryDescription(const char* entry_name, const char* new_description)
{
    uint32 name_hash = TinScript::Hash(entry_name);
    if (name_hash != 0 && gDebugToolEntryNamedMap.contains(name_hash))
    {
        CDebugToolEntry* entry = gDebugToolEntryNamedMap[name_hash];
        entry->SetDescription(new_description);
    }
}

// ====================================================================================================================
// SetEntryDescription():  Given an entry ID, update the DebugEntry's description.
// ====================================================================================================================
void CDebugToolsWin::SetEntryDescription(int32 entry_id, const char* new_description)
{
    if (gDebugToolEntryMap.contains(entry_id))
    {
        CDebugToolEntry* entry = gDebugToolEntryMap[entry_id];
        entry->SetDescription(new_description);
    }
}

// ====================================================================================================================
// SetEntryValue():  Given an entry name, update the DebugEntry's value.
// ====================================================================================================================
void CDebugToolsWin::SetEntryValue(const char* entry_name, const char* new_value)
{
    uint32 name_hash = TinScript::Hash(entry_name);
    if (name_hash != 0 && gDebugToolEntryNamedMap.contains(name_hash))
    {
        CDebugToolEntry* entry = gDebugToolEntryNamedMap[name_hash];
        entry->SetValue(new_value);
    }
}

// ====================================================================================================================
// SetEntryValue():  Given an entry ID, update the DebugEntry's value.
// ====================================================================================================================
void CDebugToolsWin::SetEntryValue(int32 entry_id, const char* new_value)
{
    if (gDebugToolEntryMap.contains(entry_id))
    {
        CDebugToolEntry* entry = gDebugToolEntryMap[entry_id];
        entry->SetValue(new_value);
    }
}

// ------------------------------------------------------------------------------------------------
#include "TinQTToolsWinMoc.cpp"

// ------------------------------------------------------------------------------------------------
// eof
// ------------------------------------------------------------------------------------------------
