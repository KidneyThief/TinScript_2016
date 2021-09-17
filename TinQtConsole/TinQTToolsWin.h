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
// TinQTToolsWin.h
// ====================================================================================================================

#ifndef __TINQTTOOLSWIN_H
#define __TINQTTOOLSWIN_H

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

// ====================================================================================================================
// class CDebugToolEntry:  The base class for gui elements to be added to a ToolPalette window.
// ====================================================================================================================
class CDebugToolEntry : public QWidget
{
    public:
        CDebugToolEntry(CDebugToolsWin* parent);
        virtual ~CDebugToolEntry();

        int32 Initialize(const char* name, const char* description, QWidget* element);
        int32 GetEntryID() const
        {
            return (mEntryID);
        }

        void SetName(const char* new_name);
        void SetDescription(const char* new_description);
        virtual void SetValue(const char* new_value) { }

    protected:
        static int32 gToolsWindowElementIndex;
        int32 mEntryID;
        uint32 mEntryNameHash;
        CDebugToolsWin* mParent;

        QLabel* mName;
        QLabel* mDescription;
};

// ====================================================================================================================
// CDebugToolMessage:  Gui element of type "message", to be added to a ToolPalette
// ====================================================================================================================
class CDebugToolMessage : public CDebugToolEntry
{
    public:
        CDebugToolMessage(const char* message, CDebugToolsWin* parent);
        virtual ~CDebugToolMessage();

        virtual void SetValue(const char* new_value) override;

    protected:
        QLabel* mMessage;
};

// ====================================================================================================================
// CDebugToolButton:  Gui element of type "button", to be added to a ToolPalette
// ====================================================================================================================
class CDebugToolButton : public CDebugToolEntry
{
    Q_OBJECT

    public:
        CDebugToolButton(const char* name, const char* description, const char* label, const char* command,
                         CDebugToolsWin* parent);
        virtual ~CDebugToolButton();

        virtual void SetValue(const char* new_value) override;

    public slots:
        void OnButtonPressed();

    protected:
        QPushButton* mButton;
        char mCommand[TinScript::kMaxTokenLength];
};

// ====================================================================================================================
// CDebugToolSlider:  Gui element of type "slider", to be added to a ToolPalette
// ====================================================================================================================
class CDebugToolSlider : public CDebugToolEntry
{
    Q_OBJECT

    public:
        CDebugToolSlider(const char* name, const char* description, int32 min_value, int32 max_value,
                         int32 cur_value, const char* command, CDebugToolsWin* parent);
        virtual ~CDebugToolSlider();

        virtual void SetValue(const char* new_value) override;

    public slots:
        void OnSliderReleased();

    protected:
        QSlider* mSlider;
        char mCommand[TinScript::kMaxTokenLength];
};

// ====================================================================================================================
// CDebugToolTextEdit:  Gui element of type "text edit", to be added to a ToolPalette
// ====================================================================================================================
class CDebugToolTextEdit : public CDebugToolEntry
{
    Q_OBJECT

    public:
        CDebugToolTextEdit(const char* name, const char* description, const char* cur_value, const char* command,
                           CDebugToolsWin* parent);
        virtual ~CDebugToolTextEdit();

        virtual void SetValue(const char* new_value) override;

    public slots:
        void OnReturnPressed();

    protected:
        SafeLineEdit* mLineEdit;
        char mCommand[TinScript::kMaxTokenLength];
};

// ====================================================================================================================
// CDebugToolCheckBox:  Gui element of type "check box", to be added to a ToolPalette
// ====================================================================================================================
class CDebugToolCheckBox : public CDebugToolEntry
{
    Q_OBJECT

    public:
        CDebugToolCheckBox(const char* name, const char* description, bool cur_value, const char* command,
                           CDebugToolsWin* parent);
        virtual ~CDebugToolCheckBox();

        virtual void SetValue(const char* new_value) override;

    public slots:
        void OnClicked();

    protected:
        QCheckBox* mCheckBox;
        char mCommand[TinScript::kMaxTokenLength];
};

// ====================================================================================================================
// class CDebugToolsWin:  The base class for ToolPalette windows
// ====================================================================================================================

class CDebugToolsWin : public QWidget
{
    Q_OBJECT

    public:
        CDebugToolsWin(const char* tools_name, QWidget* parent);
        virtual ~CDebugToolsWin();

        virtual void paintEvent(QPaintEvent* e)
        {
            ExpandToParentSize();
            //QListWidget::paintEvent(e);
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

			mScrollArea->setGeometry(0, CConsoleWindow::FontHeight(), newWidth, newHeight);
            mScrollArea->updateGeometry();
        }

        // -- interface to populate with GUI elements
        void ClearAll();
        int GetEntryCount() { return (mEntryList.size()); }
        QGridLayout* GetLayout() { return (mLayout); }
        QWidget* GetContent() { return (mScrollContent); }
        QScrollArea* GetScrollArea() { return (mScrollArea); }
        const char* GetWindowName() { return (mWindowName); }
        void AddEntry(CDebugToolEntry* entry) { mEntryList.push_back(entry); }

        int32 AddMessage(const char* message);
        int32 AddButton(const char* name, const char* description, const char* value, const char* command);
        int32 AddSlider(const char* name, const char* description, int32 min_value, int32 max_value, int32 cur_value,
                        const char* command);
        int32 AddTextEdit(const char* name, const char* description, const char* cur_value, const char* command);
        int32 AddCheckBox(const char* name, const char* description, bool cur_value, const char* command);

        // -- setting the value of a DebugEntry by ID, regardless of which window it belongs to
        static void SetEntryDescription(const char* entry_name, const char* new_description);
        static void SetEntryDescription(int32 entry_id, const char* new_description);
        static void SetEntryValue(const char* entry_name, const char* new_value);
        static void SetEntryValue(int32 entry_id, const char* new_value);

        // -- static map of all entries, regardless of which tool window it actually belongs to
        static QMap<int32, CDebugToolEntry*> gDebugToolEntryMap;
        static QMap<uint32, CDebugToolEntry*> gDebugToolEntryNamedMap;

    private:
        char mWindowName[TinScript::kMaxNameLength];
        QList<CDebugToolEntry*> mEntryList;
        QGridLayout* mLayout;
        QScrollArea* mScrollArea;
        QWidget* mScrollContent;
};

#endif

// ====================================================================================================================
// EOF
// ====================================================================================================================
