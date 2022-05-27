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
// TinQTObjectInspectWin.h
// ====================================================================================================================

#ifndef __TINQTOBJECTINSPECTWIN_H
#define __TINQTOBJECTINSPECTWIN_H

#include <qpushbutton.h>
#include <qgridlayout.h>
#include <qlineedit.h>
#include <qbytearray.h>
#include <qscrollarea.h>

#include "mainwindow.h"

// --------------------------------------------------------------------------------------------------------------------
// -- forward declarations
class QLabel;
class QScroller;
class QScrollArea;
class QButton;
class QCheckBox;
class SafeLineEdit;
class CDebugObjectInspectWin;

// ====================================================================================================================
// class CObjectInspectEntry:  The base class for gui elements to be added to a ObjectInspect window.
// ====================================================================================================================
class CObjectInspectEntry : public QWidget
{
    Q_OBJECT

    public:
        CObjectInspectEntry(CDebugObjectInspectWin* parent);
        virtual ~CObjectInspectEntry();

        void Initialize(const TinScript::CDebuggerWatchVarEntry& debugger_entry);
        void SetValue(const char* new_value);

        uint32 GetHash() const { return (mNameHash); }

    public slots:
        void OnReturnPressed();

    protected:
        CDebugObjectInspectWin* mParent;

        QLabel* mNameLabel;
        char mName[kMaxNameLength];
        uint32 mNameHash;

        QLabel* mTypeLabel;
        TinScript::eVarType mType;
        SafeLineEdit* mValue;
};

// ====================================================================================================================
// class CDebugObjectInspectWin:  The base class for ObjectInspector windows
// ====================================================================================================================
class CDebugObjectInspectWin : public QWidget
{
    Q_OBJECT

    public:
        CDebugObjectInspectWin(uint32 object_id, const char* object_identifier, QWidget* parent);
        virtual ~CDebugObjectInspectWin();

        virtual void paintEvent(QPaintEvent* e)
        {
            ExpandToParentSize();
        }

        void ExpandToParentSize()
        {
            // -- resize to be the parent widget's size, with room for the title
            QSize parentSize = parentWidget()->size();
            int newWidth = parentSize.width();
            int newHeight = parentSize.height() - CConsoleWindow::FontHeight();
            if (newHeight < CConsoleWindow::FontHeight())
                newHeight = CConsoleWindow::FontHeight();
            setGeometry(0, CConsoleWindow::FontHeight(), newWidth, newHeight);
            updateGeometry();

            mScrollArea->setGeometry(0, CConsoleWindow::FontHeight(), newWidth, newHeight - CConsoleWindow::FontHeight());
            mScrollArea->updateGeometry();
        }

        // -- interface to populate with GUI elements
        int GetEntryCount() { return (mEntryMap.size()); }
        QGridLayout* GetLayout() { return (mLayout); }
        QWidget* GetContent() { return (mScrollContent); }
        QScrollArea* GetScrollArea() { return (mScrollArea); }

        uint32 GetObjectID() const { return (mObjectID); }
        void AddEntry(CObjectInspectEntry* entry);
        void SetEntryValue(const TinScript::CDebuggerWatchVarEntry& debugger_entry);

    public slots:
        void OnButtonRefreshPressed();

    private:
        uint32 mObjectID;
        char mWindowName[kMaxNameLength];
        QMap<int32, CObjectInspectEntry*> mEntryMap;
        QPushButton* mRefreshButton;
        QGridLayout* mLayout;
        QScrollArea* mScrollArea;
        QWidget* mScrollContent;
};

#endif // __TINQTOBJECTINSPECTWIN_H

// ====================================================================================================================
// EOF
// ====================================================================================================================
