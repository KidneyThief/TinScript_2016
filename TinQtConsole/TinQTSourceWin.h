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

#ifndef __TINQTSOURCEWIN_H
#define __TINQTSOURCEWIN_H

#include <qpushbutton.h>
#include <qgridlayout.h>
#include <qlineedit.h>
#include <qlist.h>
#include <qbytearray.h>
#include <qtablewidget.h>
#include <qtoolbar.h>
#include <qtoolbutton.h>

class CSourceLine : public QListWidgetItem
{
    public:
        enum class eBreakpointStatus { None, Disabled, Enabled };

        CSourceLine(const char* source_text, int line_number);
        int mLineNumber = -1;
        eBreakpointStatus mBreakpointSet = eBreakpointStatus::None;
        bool mIsPC = false;
        void UpdateIcon();
};

class CDebugSourceWin : public QListWidget
{
    Q_OBJECT

    public:
        CDebugSourceWin(QWidget* parent);
        virtual ~CDebugSourceWin();

        virtual void paintEvent(QPaintEvent* e)
        {
            CConsoleWindow::ExpandToParentSize(this);
            QListWidget::paintEvent(e);
        }

        void NotifyCurrentDir(const char* cwd, const char* exe_dir);
        bool OpenSourceFile(const char* filename, bool reload = false);
        bool OpenFullPathFile(const char* fullPath, bool reload = false);
        bool SetSourceView(uint32 codeblock_hash, int32 line_number, bool update_history = true);
        void SetCurrentPC(uint32 codeblock_hash, int32 line_number);
        void GoToLineNumber(int32 line_number);
        void FindInFile(const char* search_string);
        void ToggleBreakpoint(uint32 codeblock_hash, int32 line_number, bool add, bool enable);
        void NotifyCodeblockLoaded(uint32 codeblock_hash);
        void NotifyCodeblockLoaded(const char* filename);
        void NotifySourceStatus(const char* source_full_path, bool has_error);

        // -- get the full path, given a file name, and vice versa
        // note:  tolerate when the full path or file name only is already given
        static const char* GetFullPath(const char* file_name, char* out_full_path, int max_length);
        static const char* GetFileName(const char* full_path);

    public slots:
        void OpenHistoryPrevious();
        void OpenHistoryNext();

        void OnDoubleClicked(QListWidgetItem*);

        // -- exceedingly unsafe!!
        void OnForceExecuteLineNumber();

    private:
        QList<CSourceLine*> mSourceText;
        uint32 mCurrentCodeblockHash;
        int32 mCurrentLineNumber;

        // -- we need to store the current working directory of our debug target
        static char mDebuggerDir[kMaxArgLength];

        // -- cache the current visible line (different from the current PC line)
        int32 mViewLineNumber;

        // -- history stack of files open
        void UpdateHistory(uint32 codeblock_hash, int32 line_number);
        int32 mHistoryIndex = -1;
        std::vector<uint32> mHistoryCodeBlock;
        std::vector<int32> mHistoryLineNumber;
};

#endif

// ------------------------------------------------------------------------------------------------
// eof
// ------------------------------------------------------------------------------------------------
