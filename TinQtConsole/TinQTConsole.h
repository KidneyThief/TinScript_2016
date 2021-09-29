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

#ifndef __TINQTCONSOLE_H
#define __TINQTCONSOLE_H

// -- includes

#include <qlineedit.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qlabel.h>
#include "qlistwidget.h"
#include <QDockWidget>
#include <QDialog>
#include <QStringList>

#include "socket.h"

// --------------------------------------------------------------------------------------------------------------------
// -- statics
static const char* kConsoleSendPrefix = ">> ";
static const char* kConsoleRecvPrefix = "";
static const char* kLocalSendPrefix = "> ";

// --------------------------------------------------------------------------------------------------------------------
// -- Forward declarations

class CConsoleInput;
class CConsoleOutput;
class CDebugSourceWin;
class CDebugToolBar;
class QGridLayout;
class CDebugBreakpointsWin;
class CDebugCallstackWin;
class CDebugWatchWin;
class CDebugToolsWin;
class CDebugObjectBrowserWin;
class CDebugObjectInspectWin;
class CDebugSchedulesWin;
class CDebugFunctionAssistWin;

// -- new "dock widget" framework
class MainWindow;

// ====================================================================================================================
// class CConsoleWindow:  The main application class, owning instance of all other components
// ====================================================================================================================
class CConsoleWindow
{
    public:
        CConsoleWindow();
        virtual ~CConsoleWindow();
        static CConsoleWindow* GetInstance() { return (gConsoleWindow); }

        int Exec();

        // -- scripted methods
        void AddText(char* msg);

        // -- Qt component accessors
        CConsoleOutput* GetOutput() { return (mConsoleOutput); }
        CConsoleInput* GetInput() { return (mConsoleInput); }
        QLineEdit* GetConnectIP() { return (mConnectIP); }
        QPushButton* GetConnectButton() { return (mButtonConnect); }
        QLineEdit* GetFileLineEdit() { return (mFileLineEdit); }
        QLineEdit* GetFindLineEdit() { return (mFindLineEdit); }
        QLabel* GetFindResult() { return (mFindResult); }
        QLineEdit* GetUnhashLineEdit() { return (mUnhashLineEdit); }
        QLabel* GetUnhashResult() { return (mUnhashResult); }
        CDebugSourceWin* GetDebugSourceWin() { return (mDebugSourceWin); }
        CDebugBreakpointsWin* GetDebugBreakpointsWin() { return (mBreakpointsWin); }
        CDebugCallstackWin* GetDebugCallstackWin() { return (mCallstackWin); }
        CDebugWatchWin* GetDebugAutosWin() { return (mAutosWin); }
        CDebugWatchWin* GetDebugWatchesWin() { return (mWatchesWin); }
        CDebugObjectBrowserWin* GetDebugObjectBrowserWin() { return (mObjectBrowserWin); }
        CDebugSchedulesWin* GetDebugSchedulesWin() { return (mSchedulesWin); }
        CDebugFunctionAssistWin* GetDebugFunctionAssistWin() { return (mFunctionAssistWin); }

        void InitFontMetrics() const;
        static int32 FontWidth() { return gPaddedFontWidth; }
        static int32 FontHeight() { return gPaddedFontHeight; }
        static int32 TitleHeight() { return gPaddedFontHeight + 8; }
        static int32 TextEditHeight() { return gPaddedFontHeight + 12; }
        static int32 StringWidth(const QString& in_string);

        static void ExpandToParentSize(QWidget* in_console_widget);

        QLabel* CreateTitleLabel(const QString& in_title_txt, QWidget* in_parent = nullptr) const;

        void SetStatusMessage(const char* message);
        void SetTargetInfoMessage(const char* message);

        // -- Qt components
        QApplication* mApp;
        MainWindow* mMainWindow;
        QGridLayout* mGridLayout;
        MainWindow* GetMainWindow() { return (mMainWindow); }
        QApplication* GetApplication() { return (mApp); }

        CConsoleOutput* mConsoleOutput;

        QLabel* mInputLabel;
        CConsoleInput* mConsoleInput;

        QLabel* mStatusLabel;
        QLabel* mTargetInfoLabel;
        QLabel* mIPLabel;
        QLineEdit* mConnectIP;
        QPushButton* mButtonConnect;

        QDockWidget* mSourceWinDockWidget;
        QDockWidget* mAutosWinDockWidget;
        CDebugSourceWin* mDebugSourceWin;
        CDebugBreakpointsWin* mBreakpointsWin;
        CDebugCallstackWin* mCallstackWin;
        CDebugWatchWin* mAutosWin;
        CDebugWatchWin* mWatchesWin;
        CDebugObjectBrowserWin* mObjectBrowserWin;
        CDebugSchedulesWin* mSchedulesWin;
        CDebugFunctionAssistWin* mFunctionAssistWin;

        QHBoxLayout* mToolbarLayout;
        QLineEdit* mFileLineEdit;
        QPushButton* mButtonExec;
        QPushButton* mButtonRun;
        QPushButton* mButtonStep;
        QPushButton* mButtonStepIn;
        QLineEdit* mFindLineEdit;
        QLabel* mFindResult;
        QLineEdit* mUnhashLineEdit;
        QLabel* mUnhashResult;

        // -- notifications
        bool8 IsConnected() const
        {
            return (mIsConnected);
        }
        void NotifyOnConnect();
        void NotifyOnDisconnect();

        // -- if we close the window, we destroy the console input (our main signal/slot hub)
        void NotifyOnClose();

        // -- debugger methods
        void ToggleBreakpoint(uint32 codeblock_hash, int32 line_number, bool add, bool enable);

        // -- notify breakpoint hit - allows the next update to execute the HandleBreakpointHit()
        // -- keeps the threads separate
        void NotifyBreakpointHit(int32 watch_request_id, uint32 codeblock_hash, int32 line_number);
        bool HasBreakpoint(int32& watch_request_id, uint32& codeblock_hash, int32& line_number);
        void HandleBreakpointHit(const char* breakpoint_msg);

        // -- not dissimilar to a breakpoint
        void NotifyAssertTriggered(const char* assert_msg, uint32 codeblock_hash, int32 line_number);
        bool HasAssert(const char*& assert_msg, uint32& codeblock_hash, int32& line_number);
        void ClearAssert(bool set_break);

        // -- interface supporting multiple tools windows
        CDebugToolsWin* FindOrCreateToolsWindow(const char* window_name);
        void ToolsWindowClear(const char* window_name);
        int32 ToolsWindowAddMessage(const char* window_name, const char* message);
        int32 ToolsWindowAddButton(const char* window_name, const char* name, const char* description,
                                   const char* value, const char* command);
        int32 ToolsWindowAddSlider(const char* window_name, const char* name, const char* description,
                                   int32 min_value, int32 max_value, int32 cur_value, const char* command);
        int32 ToolsWindowAddTextEdit(const char* window_name, const char* name, const char* description,
                                     const char* value, const char* command);
        int32 ToolsWindowAddCheckBox(const char* window_name, const char* name, const char* description,
                                     bool value, const char* command);

        // -- interface supporting multiple object inspect windows
        CDebugObjectInspectWin* FindOrCreateObjectInspectWin(uint32 object_id, const char* object_identifier);
        void NotifyDestroyObject(uint32 object_id);
        void NotifyWatchVarEntry(TinScript::CDebuggerWatchVarEntry* watch_var_entry);

        // -- breakpoint members
        bool mBreakpointHit;
        int32 mBreakpointWatchRequestID;
        uint32 mBreakpointCodeblockHash;
        int32 mBreakpointLinenumber;
        bool mBreakpointRun;
        bool mBreakpointStep;
        bool mBreakpointStepIn;
        bool mBreakpointStepOut;

        // -- assert members
        bool mAssertTriggered;
        char mAssertMessage[kMaxArgLength];

    private:
        static CConsoleWindow* gConsoleWindow;

        // -- map of all the tool palette windows, indexed by hash of the window name
        QMap<uint32, CDebugToolsWin*> mToolsWindowMap;

        // -- map of all the object inspect widows, indexed by the object ID
        QMap<uint32, CDebugObjectInspectWin*> mObjectInspectWindowMap;

        // -- store whether we're connected
        bool8 mIsConnected;

        // -- font metrics members
        static int32 gPaddedFontWidth;
        static int32 gPaddedFontHeight;
};

// ====================================================================================================================
// class CConsoleInput:  Provides text input, and history, to issue commands to the debug target.
// ====================================================================================================================
class CConsoleInput : public QLineEdit
{
    Q_OBJECT;

    public:
        explicit CConsoleInput(QWidget* parent);
        virtual ~CConsoleInput() { }

        void NotifyConnectionStatus(bool is_connected)
        {
	        QPalette myPalette = mInputLabel->palette();
	        myPalette.setColor(QPalette::WindowText, is_connected ? Qt::darkGreen : Qt::red);	
	        mInputLabel->setPalette(myPalette);
        }

        void ExpandToParentSize()
        {
            // -- make sure our input height is correct (allow padding of 2)
            setFixedHeight(CConsoleWindow::TextEditHeight() - 4);

            // -- leave room at the start for the input label: 3x characters '==>'
			int labelWidth = (CConsoleWindow::FontWidth() * 3);
            QSize parentSize = parentWidget()->size();
            int newWidth = parentSize.width() - labelWidth;
            if (newWidth < 0)
                newWidth = 0;  
			int newYOffset = parentSize.height() - CConsoleWindow::TextEditHeight() + 2;
            if (newYOffset < 0)
                newYOffset = 0;
			setGeometry(labelWidth, newYOffset, newWidth, CConsoleWindow::TextEditHeight());
            updateGeometry();

            // -- update the label as well
            int32 label_y_offset = (CConsoleWindow::TextEditHeight() - CConsoleWindow::FontHeight()) / 2;
			mInputLabel->setGeometry(0, newYOffset + label_y_offset, labelWidth, CConsoleWindow::FontHeight());
        }

        void SetText(const char* text, int cursor_pos);
        void RequestTabComplete();
        void NotifyTabComplete(int32 request_id, const char* tab_completed_string, int32 tab_complete_index);

        void NotifyStringUnhash(uint32 string_hash, const char* string_result);

        typedef struct { char text[TinScript::kMaxTokenLength]; } tHistoryEntry;
        void GetHistory(QStringList& history) const;

    public slots:
        void OnButtonConnectPressed();
        void OnConnectIPReturnPressed();
        void OnReturnPressed();
        void OnFileEditReturnPressed();
        void OnButtonStopPressed();
        void OnButtonExecPressed();
        void OnButtonRunPressed();
        void OnButtonStepPressed();
        void OnButtonStepInPressed();
        void OnButtonStepOutPressed();
        void OnFindEditFocus();
        void OnFindEditReturnPressed();
        void OnUnhashEditReturnPressed();
        void OnFunctionAssistPressed();

    protected:
        virtual void keyPressEvent(QKeyEvent * event);
        virtual bool focusNextPrevChild(bool next) { return (false); }
            
        virtual void paintEvent(QPaintEvent* e)
        {
            ExpandToParentSize();
            QLineEdit::paintEvent(e);
        }

    private:
        QLabel* mInputLabel;
        static const int32 kMaxHistory = 64;
        bool8 mHistoryFull;
        int32 mHistoryIndex;
        int32 mHistoryLastIndex;
        tHistoryEntry mHistory[kMaxHistory];

        // -- tab completion members
        int32 mTabCompleteRequestID;
        int32 mTabCompletionIndex;
        char mTabCompletionBuf[TinScript::kMaxTokenLength];
};

// ====================================================================================================================
// class CConsoleOutput:  An output window, receiving any form of output message from the debug target.
// ====================================================================================================================
class CConsoleOutput : public QListWidget
{
    Q_OBJECT;

    public:
        explicit CConsoleOutput(QWidget* parent);
        virtual ~CConsoleOutput();

        void NotifyConnectionStatus(bool is_connected)
        {
            QDockWidget* parent_widget = static_cast<QDockWidget*>(parent());
	        QPalette myPalette = parent_widget->palette();
	        myPalette.setColor(QPalette::WindowText, is_connected ? Qt::darkGreen : Qt::red);	
	        parent_widget->setPalette(myPalette);
        }

        virtual void paintEvent(QPaintEvent* e)
        {
            ExpandToParentSize();
            QListWidget::paintEvent(e);
        }

        void ExpandToParentSize()
        {
            // -- resize to be the parent widget's size, with room for the title,
            // -- leave room at the bottom for the console input
            QSize parentSize = parentWidget()->size();
            int newWidth = parentSize.width();
            int newHeight = parentSize.height() - CConsoleWindow::TitleHeight() -
                            CConsoleWindow::TextEditHeight();
            if (newHeight < CConsoleWindow::TitleHeight())
                newHeight = CConsoleWindow::TitleHeight();
            setGeometry(0, CConsoleWindow::TitleHeight(), newWidth, newHeight);
            updateGeometry();

            // -- reposition the console input
            CConsoleInput* console_input = CConsoleWindow::GetInstance()->GetInput();
            console_input->ExpandToParentSize();
        }

        static const unsigned int kUpdateTime = 33;
        int32 GetSimTime() { return (mCurrentTime); }

        // -- these methods queue and process data packets, received from the socket
        // -- both must be thread safe
        void ReceiveDataPacket(SocketManager::tDataPacket* packet);
        void ProcessDataPackets();

        // -- handlers for the data packet
        void HandlePacketCurrentWorkingDir(int32* dataPtr);
        void HandlePacketCodeblockLoaded(int32* dataPtr);
        void HandlePacketBreakpointConfirm(int32* dataPtr);
        void HandlePacketVarWatchConfirm(int32* dataPtr);
        void HandlePacketBreakpointHit(int32* dataPtr);
        void HandlePacketCallstack(int32* dataPtr);
        void HandlePacketWatchVarEntry(int32* dataPtr);
        void HandlePacketAssertMsg(int32* dataPtr);
        void HandlePacketPrintMsg(int32* dataPtr);
        void HandlePacketFunctionAssist(int32* dataPtr);
        void HandlePacketObjectCreated(int32* dataPtr);

        // -- called while handling a breakpoint, to ensure we still get to update our own context
        void DebuggerUpdate();

    public slots:
        void Update();

    private:
        // -- the console output handles the current time, and timer events to call Update()
        QTimer* mTimer;

        unsigned int mCurrentTime;

        // -- the console output also needs to receive and process data packets
        TinScript::CThreadMutex mThreadLock;
        std::vector<SocketManager::tDataPacket*> mReceivedPackets;
};

// ====================================================================================================================
// -- global interface
TinScript::CScriptContext* GetScriptContext();
int ConsolePrint(const char* fmt, ...);

#endif // __TINQTCONSOLE_H

// ====================================================================================================================
// eof
// ====================================================================================================================
