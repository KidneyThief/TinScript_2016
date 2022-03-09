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
// TinQTSchedulesWin.cpp : A list view of the pending schedules.
// ====================================================================================================================

#include <QListWidget>
#include <QLabel>
#include <QScroller>
#include <QScrollArea>
#include <QPushButton>
#include <QCheckBox>

#include "TinScript.h"
#include "TinRegistration.h"

#include "TinQTConsole.h"
#include "TinQTObjectBrowserWin.h"
#include "TinQTSchedulesWin.h"

// == CScheduleEntry ==================================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CScheduleEntry::CScheduleEntry(uint32 sched_id, bool repeat, int32 time_remaining_ms, uint32 object_id,
                               const char* command, CDebugSchedulesWin* parent)
    : QWidget()
    , mScheduleID(sched_id)
    , mParent(parent)
{
    // -- set our height
    setFixedHeight(CConsoleWindow::TextEditHeight());
    // -- get the current number of entries added to this window
    int count = parent->GetEntryCount() + 1;

    QSize parentSize = parent->size();
    int newWidth = parentSize.width();
	parent->GetContent()->setGeometry(0, 20, newWidth, (count + 2) * CConsoleWindow::TextEditHeight());

    // -- kill button (it's partially checked, if this is a repeated schedule)
    mKillButton = new QCheckBox();
    if (repeat)
        mKillButton->setCheckState(Qt::PartiallyChecked);
    QObject::connect(mKillButton, SIGNAL(clicked()), this, SLOT(OnButtonKillPressed()));

    // -- time remaining
    mTimeRemainingLabel = new QLabel();
    SetTimeRemaining(time_remaining_ms);

    // -- schedule ID
    char sched_id_str[32];
    sprintf_s(sched_id_str, "%d", sched_id);
    mScheduleIDLabel = new QLabel(sched_id_str);

    // -- object ID
    if (object_id != 0)
    {
        // -- see if we can get a nicer label for the object
        const char* browser_object = CConsoleWindow::GetInstance()->GetDebugObjectBrowserWin()->GetObjectIdentifier(object_id);
        if (browser_object != NULL)
            mObjectIDLabel = new QLabel(browser_object);
        else
        {
            char object_id_str[32];
            sprintf_s(object_id_str, "[%d]", object_id);
            mObjectIDLabel = new QLabel(object_id_str);
        }
    }
    else
    {
        mObjectIDLabel = new QLabel("");
    }

    // -- command
    mCommandLabel = new QLabel(command);

    // -- populate the layout
    SetLayoutRow(count);
    mParent->GetLayout()->setRowStretch(count - 1, 0);
    mParent->GetLayout()->setRowStretch(count, 1);

    parent->AddEntry(this);
    parent->GetContent()->updateGeometry();
    parent->ExpandToParentSize();
}

// ====================================================================================================================
// Deconstructor
// ====================================================================================================================
CScheduleEntry::~CScheduleEntry()
{
    delete mKillButton;
    delete mTimeRemainingLabel;
    delete mScheduleIDLabel;
    delete mObjectIDLabel;
    delete mCommandLabel;
}

// ====================================================================================================================
// Update():  Updates the time remaining
// ====================================================================================================================
void CScheduleEntry::Update(int32 delta_time_ms)
{
    mTimeRemaining -= delta_time_ms;
    if (mTimeRemaining < 0)
        mTimeRemaining = 0;
    SetTimeRemaining(mTimeRemaining);
}

// ====================================================================================================================
// SetValue():  Update the button text.
// ====================================================================================================================
void CScheduleEntry::SetTimeRemaining(int32 time_remaining_ms)
{
    // -- store the time remaining
    mTimeRemaining = time_remaining_ms;
    if (mTimeRemaining < 0)
        mTimeRemaining = 0;

    char buf[32];
    float time_remaining_float = static_cast<float>(mTimeRemaining) / 1000.0f;
    sprintf_s(buf, "%.2f", time_remaining_float);
    mTimeRemainingLabel->setText(buf);
}

// ====================================================================================================================
// SetLayoutRow():  The list is always sorted by time remaining.
// ====================================================================================================================
void CScheduleEntry::SetLayoutRow(int32 row)
{
    // -- add all GUI elements to the layout
    mParent->GetLayout()->addWidget(mKillButton, row, 0, 1, 1);
    mParent->GetLayout()->addWidget(mTimeRemainingLabel, row, 1, 1, 1);
    mParent->GetLayout()->addWidget(mScheduleIDLabel, row, 2, 1, 1);
    mParent->GetLayout()->addWidget(mObjectIDLabel, row, 3, 1, 1);
    mParent->GetLayout()->addWidget(mCommandLabel, row, 4, 1, 2);
}

// ====================================================================================================================
// OnReturnPressed():  Slot hooked up to the line edit, to be executed when the return is pressed.
// ====================================================================================================================
void CScheduleEntry::OnButtonKillPressed()
{
    // -- create the command, by inserting the slider value as the first parameter
    char command_buf[TinScript::kMaxTokenLength];
    sprintf_s(command_buf, "ScheduleCancel(%d);", mScheduleID);

    bool8 is_connected = CConsoleWindow::GetInstance()->IsConnected();
    if (is_connected)
    {
        // -- for sliders, we need to embed the value, so the command is only the function name
        ConsolePrint(0, "%s%s\n", kConsoleSendPrefix, command_buf);
        SocketManager::SendCommand(command_buf);
    }
}

// == CDebugSchedulesWin ==============================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CDebugSchedulesWin::CDebugSchedulesWin(QWidget* parent)
    : QWidget(parent)
{
    mScrollArea = new QScrollArea(this);
    mScrollContent = new QWidget(mScrollArea);
    mLayout = new QGridLayout(mScrollContent);
    mScrollArea->setWidget(mScrollContent);
    mScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    mScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    mLayout->addWidget(new QLabel("Kill"), 0, 0);
    mLayout->addWidget(new QLabel("Time"), 0, 1);
    mLayout->addWidget(new QLabel("Sched ID"), 0, 2);
    mLayout->addWidget(new QLabel("Object"), 0, 3);
    mLayout->addWidget(new QLabel("Command"), 0, 4);
    mLayout->setRowMinimumHeight(0, CConsoleWindow::TextEditHeight() + 2);
    mLayout->setRowStretch(0, 1);
    mLayout->setColumnStretch(4, 1);

    // -- initialize the time members
    mTargetTimeScale = 1.0f;
}

// ====================================================================================================================
// Destructor
// ====================================================================================================================
CDebugSchedulesWin::~CDebugSchedulesWin()
{
    RemoveAll();
}

// ====================================================================================================================
// AddEntry():  Adds an entry to the map of all entries owned by the window.
// ====================================================================================================================
void CDebugSchedulesWin::AddEntry(CScheduleEntry* entry)
{
    if (entry)
    {
        mEntryMap.insert(entry->GetScheduleID(), entry);
    }
}

// ====================================================================================================================
// AddSchedule():  Give the details of a new schedule.
// ====================================================================================================================
void CDebugSchedulesWin::AddSchedule(uint32 sched_id, bool repeat, int32 time_remaining_ms, uint32 object_id,
                                     const char* command)
{
    CScheduleEntry* entry = mEntryMap[sched_id];
    if (entry == nullptr)
    {
        entry = new CScheduleEntry(sched_id,repeat,time_remaining_ms,object_id,command,this);
        mEntryMap.insert(sched_id,entry);
    }

    // -- update the time remaining
    entry->SetTimeRemaining(time_remaining_ms);

    // -- schedule entries are sorted by time remaining
    SortSchedules();
}

// ====================================================================================================================
// RemoveSchedule():  Notify a schedule has either been canceled or executed.
// ====================================================================================================================
void CDebugSchedulesWin::RemoveSchedule(uint32 sched_id)
{
    if (!mEntryMap.contains(sched_id))
        return;

    CScheduleEntry* entry = mEntryMap[sched_id];
    if (entry)
    {
        mEntryMap.remove(sched_id);
        delete entry;
        SortSchedules();
    }
}

// ====================================================================================================================
// RemoveAll():  Removes all schedule entries
// ====================================================================================================================
void CDebugSchedulesWin::RemoveAll()
{
    // -- delete the entry pointers in the map
    for (auto iter : mEntryMap)
    {
        CScheduleEntry* entry = iter;
        delete entry;
    }

    // -- clear the map
    mEntryMap.clear();
}

// ====================================================================================================================
// ScheduleCompare():  Used for qsort() during SortSchedules().
// ====================================================================================================================
static int ScheduleCompare(const void* a, const void* b)
{
    CScheduleEntry* schedule_a = *(CScheduleEntry**)(a);
    CScheduleEntry* schedule_b = *(CScheduleEntry**)(b);
    return (schedule_a->GetTimeRemaining() - schedule_b->GetTimeRemaining());
}

// ====================================================================================================================
// SortSchedules():  Schedule entries are sorted by time remaining.
// ====================================================================================================================
void CDebugSchedulesWin::SortSchedules()
{
    // -- we use a temporary list to sort and determine the layout row
    QList<uint32>& keys = mEntryMap.keys();
    int count = keys.size();
    CScheduleEntry** sorted_list;
    sorted_list = new CScheduleEntry*[count];

    // -- populate the sorted list
    int index = 0;
    for (int i = 0; i < keys.size(); ++i)
    {
        // $$$TZA How his entry nullptr??
        CScheduleEntry* entry = mEntryMap[keys[i]];
        if (entry != nullptr)
        {
            sorted_list[index++] = entry;
        }
    }

    // -- sort the entries
    qsort(sorted_list, count, sizeof(CScheduleEntry*), ScheduleCompare);

    // -- clear the layout (except the first row)

    // -- loop through and update the layout row
    for (int i = 0; i < count; ++i)
    {
        // -- use the index + 1, to leave room for the Refresh and Heading row.
        sorted_list[i]->SetLayoutRow(i + 1);
    }

    // -- we're done with the temporary list
    delete [] sorted_list;
}

// ====================================================================================================================
// Update():  We use this to attempt to track time, so the time remaining is somewhat accurate.
// ====================================================================================================================
void CDebugSchedulesWin::Update(int32 delta_ms)
{
    // -- go through all entries, and update them
    int32 scaled_delta_ms = mTargetTimeScale == 1.0f
                            ? delta_ms
                            : static_cast<int32>(static_cast<float>(delta_ms) * mTargetTimeScale);
    QList<uint32>& keys = mEntryMap.keys();
    for (int i = 0; i < keys.size(); ++i)
    {
        CScheduleEntry* entry = mEntryMap[keys[i]];
        // $$$TZA Find out why we have an entry in the keys, but not in the map!!
        if (entry != nullptr)
        {
            entry->Update(scaled_delta_ms);
        }
    }
}

// ====================================================================================================================
// NotifyOnConnect():  Called when the debugger's connection to the target is initially confirmed.
// ====================================================================================================================
void CDebugSchedulesWin::NotifyOnConnect()
{
    // -- request a fresh population of the existing objects
    RemoveAll();
    SocketManager::SendCommand("DebuggerListSchedules();");
}

// ====================================================================================================================
// NotifyTargetSimTime():  Notification of the target's timescale, so our reflection of schedules is accurate.
// ====================================================================================================================
void CDebugSchedulesWin::NotifyTargetTimeScale(float target_time_scale)
{
    // -- initialize if required
    mTargetTimeScale = target_time_scale;
}

// ====================================================================================================================
// OnButtonRefreshPressed():  Called when the refresh button is pressed
// ====================================================================================================================
void CDebugSchedulesWin::OnButtonRefreshPressed()
{
    // -- send the request to re-populate the inspector for the window's object
    if (SocketManager::IsConnected())
    {
        RemoveAll();
        SocketManager::SendCommandf("DebuggerListSchedules();");
    }
}

// ------------------------------------------------------------------------------------------------
#include "TinQTSchedulesWinMoc.cpp"

// ------------------------------------------------------------------------------------------------
// eof
// ------------------------------------------------------------------------------------------------
