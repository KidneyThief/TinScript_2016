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

#ifndef __TINQTWATCHWIN_H
#define __TINQTWATCHWIN_H

#include <qgridlayout.h>
#include <qtreewidget.h>
#include <qlabel.h>
#include <QLineEdit>

// ------------------------------------------------------------------------------------------------
// forward declares
namespace TinScript {
    class CDebuggerWatchVarEntry;
}

class CConsoleWindow;

// ------------------------------------------------------------------------------------------------
class CWatchEntry : public QTreeWidgetItem {
    public:
        CWatchEntry(const TinScript::CDebuggerWatchVarEntry& debugger_entry, bool breakOnWrite = false);

        virtual ~CWatchEntry();

        void UpdateType(TinScript::eVarType type, int32 array_size);
        void UpdateArrayVar(uint32 var_array_id, int32 array_size);
        void UpdateValue(const char* newValue);
        void UpdateDisplay();

        TinScript::CDebuggerWatchVarEntry mDebuggerEntry;
        bool mBreakOnWrite;
        bool mRequestSent;
};

// ------------------------------------------------------------------------------------------------
class CDebugWatchWin : public QTreeWidget {
    Q_OBJECT

    public:
        CDebugWatchWin(QWidget* parent);
        virtual ~CDebugWatchWin();

        virtual void paintEvent(QPaintEvent* e)
        {
            CConsoleWindow::ExpandToParentSize(this);
            QTreeWidget::paintEvent(e);
        }

        virtual void resizeEvent(QResizeEvent* e)
        {
            CConsoleWindow::ExpandToParentSize(this);
            QTreeWidget::resizeEvent(e);
        }

        CWatchEntry* FindWatchEntry(uint32 funcHash, uint32 objectID, uint32 nsHash, bool& foundNamespaceLabel);
        CWatchEntry* FindWatchEntry(uint32 funcHash, uint32 objectID, uint32 nsHash, uint32 memberHash, bool isMember);

        void UpdateReturnValueEntry(const TinScript::CDebuggerWatchVarEntry& watch_var_entry);
        bool IsTopLevelEntry(const CWatchEntry& watch_var_entry);
        void AddTopLevelEntry(const TinScript::CDebuggerWatchVarEntry& watch_var_entry, bool update_only);
        void AddObjectMemberEntry(const TinScript::CDebuggerWatchVarEntry& watch_var_entry, bool update_only);

        CWatchEntry* FindTopLevelWatchEntry(const char* variableWatch);
		void AddVariableWatch(const char* variableWatch, bool breakOnWrite = false);
        void ResendVariableWatch(CWatchEntry* watch_entry, bool allow_break_on_write = false);
        void ResendAllUserWatches();
        bool GetSelectedWatchExpression(int32& out_use_watch_id, char* out_watch_string, int32 max_expr_length, char* out_value,
                                        int32 max_value_length);

        // -- this is used for creating an ObjectInspector, if the currently selected entry is a variable of type object
        uint32 GetSelectedObjectID();

        void NotifyOnConnect();
        void ClearWatchWin(bool clear_user_watches = false);

        void RemoveWatchVarChildren(int32 object_entry_index);
        void NotifyWatchVarEntry(TinScript::CDebuggerWatchVarEntry* watch_var_entry, bool update_only);
        void NotifyVarWatchResponse(TinScript::CDebuggerWatchVarEntry* watch_var_entry);
        void NotifyVarWatchMember(int32 parent_entry_index, TinScript::CDebuggerWatchVarEntry* watch_var_entry);
        void NotifyArrayEntry(int32 watch_request_id, int32 stack_offset_bottom,uint32 array_var_id,
                              int32 array_index, const char* value_str);

        void NotifyBreakpointHit();
        void NotifyEndOfBreakpoint() { }
        void NotifyUpdateCallstack(bool breakpointHit);

		static int gVariableWatchRequestID;

    protected:
        virtual void keyPressEvent(QKeyEvent * event);

    private:
        QTreeWidgetItem* mHeaderItem;
        QList<CWatchEntry*> mWatchList;
};

#endif //__TINQTWATCHWIN_H

// ------------------------------------------------------------------------------------------------
// eof
// ------------------------------------------------------------------------------------------------
