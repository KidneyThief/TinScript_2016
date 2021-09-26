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
// TinQTSchedulesWin.h
// ====================================================================================================================

#ifndef __TINQTSCHEDULESWIN_H
#define __TINQTSCHEDULESWIN_H

#include <qpushbutton.h>
#include <qgridlayout.h>
#include <qlineedit.h>
#include <qbytearray.h>
#include <qscrollarea.h>
#include <qcheckbox.h>

#include "mainwindow.h"

// --------------------------------------------------------------------------------------------------------------------
// -- forward declarations
class QLabel;
class QScroller;
class QScrollArea;
class CDebugSchedulesWin;

// ====================================================================================================================
// class CScheduleEntry:  The base class for displaying a pending schedule.
// ====================================================================================================================
class CScheduleEntry : public QWidget
{
    Q_OBJECT

    public:
        CScheduleEntry(uint32 sched_id, bool repeat, int32 time_remaining_ms, uint32 object_id, const char* command,
                       CDebugSchedulesWin* parent);
        virtual ~CScheduleEntry();

        uint32 GetScheduleID() const { return (mScheduleID); }
        void Update(int32 delta_time_ms);
        void SetTimeRemaining(int32 time_remaining_ms);
        int32 GetTimeRemaining() const { return (mTimeRemaining); }
        void SetLayoutRow(int32 row);

    public slots:
        void OnButtonKillPressed();

    protected:
        uint32 mScheduleID;
        int32 mTimeRemaining;

        // -- GUI elements that we'll need to shuffle around the different rows
        CDebugSchedulesWin* mParent;
        QCheckBox* mKillButton;
        QLabel* mTimeRemainingLabel;
        QLabel* mScheduleIDLabel;
        QLabel* mObjectIDLabel;
        QLabel* mCommandLabel;
};

// ====================================================================================================================
// class CDebugSchedulesWin:  The class to display all the schedule entries.
// ====================================================================================================================
class CDebugSchedulesWin : public QWidget
{
    Q_OBJECT

    public:
        CDebugSchedulesWin(QWidget* parent);
        virtual ~CDebugSchedulesWin();

        virtual void paintEvent(QPaintEvent* e)
        {
            ExpandToParentSize();
        }

        void ExpandToParentSize()
        {
            // -- resize to be the parent widget's size, with room for the title
            QSize parentSize = parentWidget()->size();
            int newWidth = parentSize.width();
            int newHeight = parentSize.height();
            if (newHeight < CConsoleWindow::TitleHeight())
                newHeight = CConsoleWindow::TitleHeight();
            setGeometry(0, CConsoleWindow::TitleHeight(), newWidth, newHeight);
            updateGeometry();

            mScrollArea->setGeometry(0, 20, newWidth, newHeight - CConsoleWindow::FontHeight() * 2);
            mScrollArea->updateGeometry();
        }

        // -- interface to populate with GUI elements
        int GetEntryCount() { return (mEntryMap.size()); }
        QGridLayout* GetLayout() { return (mLayout); }
        QWidget* GetContent() { return (mScrollContent); }
        QScrollArea* GetScrollArea() { return (mScrollArea); }

        void AddSchedule(uint32 sched_id, bool repeat, int32 time_remaining_ms, uint32 object_id, const char* command);
        void RemoveSchedule(uint32 sched_id);
        void RemoveAll();
        void SortSchedules();

        void AddEntry(CScheduleEntry* schedule_entry);
        void Update(int32 delta_ms);

        void NotifyOnConnect();
        void NotifyTargetTimeScale(float target_time_scale);

    public slots:
        void OnButtonRefreshPressed();

    private:
        QMap<uint32, CScheduleEntry*> mEntryMap;
        QPushButton* mRefreshButton;
        QGridLayout* mLayout;
        QScrollArea* mScrollArea;
        QWidget* mScrollContent;

        // -- we need to track what the target's simulation time is,
        // -- so if it's paused or time scaled, we can adjust
        float mTargetTimeScale;
};

#endif // __TINQTSCHEDULESWIN_H

// ====================================================================================================================
// EOF
// ====================================================================================================================
