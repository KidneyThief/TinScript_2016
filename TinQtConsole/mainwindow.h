/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QLineEdit>
#include <QAction>
#include <QListWidget>
#include <QDialog>

#include "TinScript.h"

QT_FORWARD_DECLARE_CLASS(QWidget)
QT_FORWARD_DECLARE_CLASS(QFile)
QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QSignalMapper)

class CScriptOpenWidget;
class CScriptOpenAction;

// ====================================================================================================================
// class SafeLineEdit:  Implementation of a QLineEdit that caches the QString value into a buffer.
// ====================================================================================================================
class SafeLineEdit : public QLineEdit
{
    public:
        SafeLineEdit(QWidget* parent = NULL) : QLineEdit(parent)
        {
            mStringValue[0] = '\0';
        }

        void SetStringValue(const char* value)
        {
            const char* new_value = value ? value : "";
            TinScript::SafeStrcpy(mStringValue, new_value, TinScript::kMaxNameLength);
            setText(new_value);
        }

        const char* GetStringValue() const
        {
            return (mStringValue);
        }

    protected:
        virtual void keyPressEvent(QKeyEvent *e)
        {
            // -- pass the event along
            QLineEdit::keyPressEvent(e);

            // -- store the current value of the string
            TinScript::SafeStrcpy(mStringValue, text().toUtf8(), TinScript::kMaxNameLength);
        }

    private:
        char mStringValue[TinScript::kMaxNameLength];
};

// ====================================================================================================================
// class CommandHistoryDialog:  Creates a dialog box containing a listwidget of this history of input commands.
// ====================================================================================================================
class CommandHistoryDialog : public QDialog
{
    public:
        CommandHistoryDialog(QWidget *parent = 0);
};

class CommandHistoryList : public QListWidget
{
    Q_OBJECT

    public:
        CommandHistoryList(CommandHistoryDialog* owner, QWidget* parent) : QListWidget(parent), mOwner(owner) { }

    public slots:
        void OnDoubleClicked(QListWidgetItem*);

    public:
        CommandHistoryDialog* mOwner;
};

// ====================================================================================================================
// class MainWindow
// ====================================================================================================================
class MainWindow : public QMainWindow
{
    Q_OBJECT

    QTextEdit *center;
    QMenu *dockWidgetMenu;
    QMenu *mainWindowMenu;
    QSignalMapper *mapper;
    QList<QDockWidget*> extraDockWidgets;

public:
    // -- constructor
    MainWindow(const QMap<QString, QSize> &customSizeHints,
                QWidget *parent = 0, Qt::WindowFlags flags = 0);

    // -- destructor
    ~MainWindow();

    void AddScriptOpenAction(const char* fullPath);
    void CreateVariableWatch(int32 request_id, const char* watch_string, const char* cur_value);

protected:
    virtual void showEvent(QShowEvent *event);
    virtual void closeEvent(QCloseEvent *event);

public slots:
    void menuSaveLayout();
    void menuLoadLayout();
    void autoSaveLayout();
    void autoLoadLayout();
    void defaultLoadLayout();
    void setCorner(int id);
    void setDockOptions();

    void menuDebugStop();
    void menuDebugRun();
    void menuDebugStepOver();
    void menuDebugStepIn();
    void menuDebugStepOut();

    void menuOpenScript();
    void menuOpenScriptAction(QAction*);

	void menuAddVariableWatch();
	void menuCreateVariableWatch();
	void menuUpdateVarWatchValue();
    void menuCreateObjectInspector();
    void menuRefreshObjectBrowser();
    void menuSetBreakCondition();
    void menuGoToLine();
    void menuSearch();
    void menuSearchAgain();
    void menuFunctionAssist();
    void menuCommandHistory();

private:
    void readLayout(QFile& file);
    void writeLayout(QFile& file);
    void setupMenuBar();
    void setupDockWidgets(const QMap<QString, QSize> &customSizeHints);

    QMenu* mScriptsMenu;
    QList<CScriptOpenAction*> mScriptOpenActionList;
};

// ====================================================================================================================
// class CScriptOpenWidget:  Used to receive an action's "triggered" signal, so we know which action is the source.
// ====================================================================================================================
class CScriptOpenWidget : public QWidget
{
    Q_OBJECT

    public:
        CScriptOpenWidget(QAction* action, MainWindow* owner)
            : QWidget()
            , mAction(action)
            , mOwner(owner)
        {
        }

    public slots:
        void menuOpenScriptAction()
        {
            mOwner->menuOpenScriptAction(mAction);
        }

    public:
        QAction* mAction;
        MainWindow* mOwner;
};

// ====================================================================================================================
// class CScriptOpenAction:  Binds the action, the file name, an the receiving widget for a given menu entry.
// ====================================================================================================================
class CScriptOpenAction
{
public:
    CScriptOpenAction(const char* fullPath, uint32 fileHash, CScriptOpenWidget* action)
    {
        if (!fullPath)
            fullPath = "";
        TinScript::SafeStrcpy(mFullPath, fullPath, TinScript::kMaxNameLength);
        mFileHash = fileHash;
        mActionWidget = action;
    }
    ~CScriptOpenAction()
    {
        delete mActionWidget;
    }

    char mFullPath[TinScript::kMaxNameLength];
    uint32 mFileHash;
    CScriptOpenWidget* mActionWidget;
};

#endif

// ====================================================================================================================
// EOF
// ====================================================================================================================
