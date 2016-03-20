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
// TinQTConsole.h

#ifndef __TINQTBREAKPOINTSWIN_H
#define __TINQTBREAKPOINTSWIN_H

#include <qgridlayout.h>
#include <qlist.h>
#include <qcheckbox.h>
#include <qlabel.h>

class CBreakpointEntry;
class QKeyEvent;

class CBreakpointEntry : public QListWidgetItem {
    public:
        CBreakpointEntry(uint32 codeblock_hash, int32 line_number, QListWidget* owner);
        CBreakpointEntry(int32 watch_request_id, uint32 var_object_id, uint32 var_name_hash, QListWidget* owner);
        virtual ~CBreakpointEntry();

        // -- manually manage the "enabled" check
        bool8 mChecked;
        void SetCheckedState(bool8 enabled, bool8 hasCondition);

        // -- breakpoint32 based on a file/line
        uint32 mCodeblockHash;
        int32 mLineNumber;

        // -- breakpoint32 based on a variable watch
        int32 mWatchRequestID;
        uint32 mWatchVarObjectID;
        uint32 mWatchVarNameHash;

        // -- update the label to match when line number is confirmed, or condition changes, etc...
        void UpdateLabel(uint32 codeblock_hash, int32 line_number);
        void UpdateLabel(int32 watch_request_id, uint32 var_object_id, uint32 var_name_hash);

        // -- conditional members
        char mCondition[TinScript::kMaxNameLength];
        bool8 mConditionEnabled;

        char mTracePoint[TinScript::kMaxNameLength];
        bool8 mTraceEnabled;
        bool8 mTraceOnCondition;
};

class CDebugBreakpointsWin : public QListWidget
{
    Q_OBJECT

    public:
        CDebugBreakpointsWin(QWidget* parent);
        virtual ~CDebugBreakpointsWin();

        virtual void paintEvent(QPaintEvent* e)
        {
            ExpandToParentSize();
            QListWidget::paintEvent(e);
        }

        void ExpandToParentSize()
        {
            // -- resize to be the parent widget's size, with room for the title
            QSize parentSize = parentWidget()->size();
            int32 newWidth = parentSize.width();
            int32 newHeight = parentSize.height();
            if (newHeight < 20)
                newHeight = 20;
            setGeometry(0, 20, newWidth, newHeight);
            updateGeometry();
        }

        // -- Toggle the breakpoint32 for a file/line
        void ToggleBreakpoint(uint32 codeblock_hash, int32 line_number, bool add);

        // -- set the current breakpoint, when a file/line break has been triggered
        void SetCurrentBreakpoint(uint32 codeblock_hash, int32 line_number);

        // -- set the current variable watch break, when a variable watch has been triggered
        void SetCurrentVarWatch(int32 watch_request_id);

        // -- set/modify/disable a condition on the the currently selected break
        void SetBreakCondition(const char* expression, bool8 cond_enabled);

        // -- set/modify/disable a tracepoint32 on the currently selected break
        void SetTraceExpression(const char* expression, bool8 trace_enabled, bool8 trace_on_condition);

        // -- get the break condition for the currently selected break
        const char* GetBreakCondition(bool8& enabled);

        // -- get the trace expression for the currently selected break
        const char* GetTraceExpression(bool8& trace_enabled, bool8& trace_on_condition);

        void NotifyCodeblockLoaded(uint32 codeblock_hash);

        // -- confirmation of a file/line breakpoint, to correct the actual breakable line
        void NotifyConfirmBreakpoint(uint32 codeblock_hash, int32 line_number, int32 actual_line);

        // -- confirmation of a variable watch, requested to break on write
        void NotifyConfirmVarWatch(int32 watch_request_id, uint32 watch_object_id, uint32 var_name_hash);

        void NotifySourceFile(uint32 filehash);
        void NotifyOnConnect();

    public slots:
        void OnClicked(QListWidgetItem*);
        void OnDoubleClicked(QListWidgetItem*);

    protected:
        virtual void keyPressEvent(QKeyEvent * event);

    private:
        QList<CBreakpointEntry*> mBreakpoints;
};

class CCallstackEntry : public QListWidgetItem {
    public:
        CCallstackEntry(uint32 codeblock_hash, int32 line_number, uint32 object_id,
                        uint32 namespace_hash, uint32 function_hash);
        uint32 mCodeblockHash;
        int32 mLineNumber;
        uint32 mObjectID;
        uint32 mNamespaceHash;
        uint32 mFunctionHash;
};

class CDebugCallstackWin : public QListWidget {
    Q_OBJECT

    public:
        CDebugCallstackWin(QWidget* parent);
        virtual ~CDebugCallstackWin();

        virtual void paintEvent(QPaintEvent* e)
        {
            ExpandToParentSize();
            QListWidget::paintEvent(e);
        }

        void ExpandToParentSize()
        {
            // -- resize to be the parent widget's size, with room for the title
            QSize parentSize = parentWidget()->size();
            int32 newWidth = parentSize.width();
            int32 newHeight = parentSize.height();
            if (newHeight < 20)
                newHeight = 20;
            setGeometry(0, 20, newWidth, newHeight);
            updateGeometry();
        }

        void NotifyCallstack(uint32* codeblock_array, uint32* objid_array, uint32* namespace_array,
                             uint32* func_array, uint32* linenumber_array, int32 array_size);
        void ClearCallstack();

        // -- returns the index of the currently selected stack entry, as well as the calling attributes
        int32 GetSelectedStackEntry(uint32& funcHash, uint32& objectID, uint32& nsHash);

        // -- returns the index of the currently selected stack entry, as well as the calling attributes
        int32 GetTopStackEntry(uint32& funcHash, uint32& objectID, uint32& nsHash);

        // -- returns the stack entry matching the given attributes, or -1 if it doesn't exist
        int32 ValidateStackEntry(uint32 func_ns_hash, uint32 func_hash, uint32 func_obj_id);

    public slots:
        void OnDoubleClicked(QListWidgetItem*);

    private:
        QList<CCallstackEntry*> mCallstack;
};

#endif

// ------------------------------------------------------------------------------------------------
// eof
// ------------------------------------------------------------------------------------------------
