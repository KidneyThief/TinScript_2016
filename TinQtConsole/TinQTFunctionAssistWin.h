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
// TinQTFunctionAssistWin.h
// ====================================================================================================================

#ifndef __TINQTFUNCTIONASSISTWIN_H
#define __TINQTFUNCTIONASSISTWIN_H

#include <qwidget.h>
#include <qgridlayout.h>
#include <qlist.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qlistwidget.h>
#include <qtreewidget.h>

// --------------------------------------------------------------------------------------------------------------------
// -- forward declarations
class CFunctionAssistInput;
class CFunctionAssistList;
class CFunctionParameterList;

// ====================================================================================================================
// class CDebugFunctionAssistWin:  Window class containing the function assist input, filtered list, and parameters.
// ====================================================================================================================
class CDebugFunctionAssistWin : public QWidget
{
    Q_OBJECT

    public:
        CDebugFunctionAssistWin(QWidget* parent);
        virtual ~CDebugFunctionAssistWin();

        virtual void paintEvent(QPaintEvent* e)
        {
            CConsoleWindow::ExpandToParentSize(this);
            QWidget::paintEvent(e);
        }

        void NotifyCodeblockLoaded(uint32 codeblock_hash);

        void ClearSearch();
        void NotifyFunctionAssistEntry(const TinScript::CDebuggerFunctionAssistEntry& assist_entry);
        void NotifyListObjectsComplete();

        // -- there are three reasons to update the search results - if we haven't changed the
        // -- search string, but we've received potentially acceptable new entries
        // -- or if we extend our search string, so the existing list needs to remove more
        // -- or if we've changed, or backtracked our search string, so we search from scratch
        void UpdateSearchNewEntry(uint32 function_hash);

        // -- string comparison functions for filtering
        bool StringContainsFilter(const char* string, bool& exact_match, bool& new_object_id);
        bool FunctionContainsFilter(const char* string);
        void UpdateFilter(const char* filter, bool8 force_refresh = false);

        // -- the Signature panel also doubles as the origin callstack
        void DisplayObjectOrigin(uint32 obj_id);
        void DisplayFunctionSignature();

        // -- methods to handle selecting and issuing a function
        void SetAssistObjectID(uint32 object_id);
        void NotifyFunctionClicked(TinScript::CDebuggerFunctionAssistEntry* list_entry);
        void NotifyFunctionDoubleClicked(TinScript::CDebuggerFunctionAssistEntry* list_entry);

    public slots:
        void OnButtonFilterRefreshPressed();
	    void OnButtonShowAPIPressed();
        void OnButtonShowOriginPressed();
        void OnButtonCopyToConsolePressed();

    private:
        CFunctionAssistInput* mFunctionInput;
        QLabel* mObjectIndentifier;
        QLabel* mFunctionDisplayLabel;
        CFunctionAssistList* mFunctionList;

		// -- for the selected function
		uint32 mSelectedFunctionHash;
        uint32 mSelectedObjectID;
        CFunctionParameterList* mParameterList;

        // -- helper function that performs a single pass
        // -- the main method tries every substring looking for containment
        bool StringContainsFilterImpl(const char* string, const char* filter, bool& exact_match, bool& new_object_id);

        // -- for filtering, we need to keep track of several items, that will change the context of the search
        uint32 mSearchObjectID;
        char mFilterString[TinScript::kMaxNameLength];
        QMap<uint32, TinScript::CDebuggerFunctionAssistEntry*> mFunctionEntryMap;
        QMap<uint32, TinScript::CDebuggerFunctionAssistEntry*> mObjectEntryMap;
};

// ====================================================================================================================
// class CFunctionAssistInput:  Provides text input, and history, to perform incremental searches of function names.
// ====================================================================================================================
class CFunctionAssistInput : public QLineEdit
{
    Q_OBJECT;

    public:
        explicit CFunctionAssistInput(CDebugFunctionAssistWin* owner, QWidget* parent);
        virtual ~CFunctionAssistInput() { }

    protected:
        virtual void keyPressEvent(QKeyEvent * event);

    private:
        // $$$TZA same history approach used by the CConsoleInput...  consolidate implementations!
        static const int32 kMaxHistory = 64;
        bool8 mHistoryFull;
        int32 mHistoryIndex;
        int32 mHistoryLastIndex;
        typedef struct { char text[TinScript::kMaxTokenLength]; } tHistoryEntry;
        tHistoryEntry mHistory[kMaxHistory];

        // -- cache the pointer, since the input will be talking to the owner often
        CDebugFunctionAssistWin* mOwner;
};

// ====================================================================================================================
// class CFunctionListEntry:  An entry in the list of filtered functions available.
// ====================================================================================================================
class CFunctionListEntry : public QTreeWidgetItem
{
    public:
        CFunctionListEntry(TinScript::CDebuggerFunctionAssistEntry* _entry, QTreeWidget* _owner);
        ~CFunctionListEntry();

        TinScript::CDebuggerFunctionAssistEntry* mFunctionAssistEntry;
};

// ====================================================================================================================
// class CFunctionAssistList:  Lists the filtered functions/methods available.
// ====================================================================================================================
class CFunctionAssistList : public QTreeWidget
{
    Q_OBJECT

    public:
        CFunctionAssistList(CDebugFunctionAssistWin* owner, QWidget* parent);
        virtual ~CFunctionAssistList();

        CFunctionListEntry* FindEntry(TinScript::CDebuggerFunctionAssistEntry* assist_entry);
        void DisplayEntry(TinScript::CDebuggerFunctionAssistEntry* assist_entry);
        void FilterEntry(TinScript::CDebuggerFunctionAssistEntry* assist_entry);
        void Clear();

    public slots:
        void OnClicked(QTreeWidgetItem*);
        void OnDoubleClicked(QTreeWidgetItem*);

    private:
        // -- cache the pointer, since the input will be talking to the owner often
        CDebugFunctionAssistWin* mOwner;

        QList<CFunctionListEntry*> mFunctionList;
};

// ====================================================================================================================
// class CFunctionParameterEntry:  A parameter specification for the selected function.
// ====================================================================================================================
class CFunctionParameterEntry : public QTreeWidgetItem
{
    public:
        CFunctionParameterEntry(TinScript::eVarType var_type, bool is_array, const char* _name, QTreeWidget* _owner);
        CFunctionParameterEntry(const char* _filename, uint32 _filehash, int32 _line_number, QTreeWidget* _owner);
        ~CFunctionParameterEntry();

        bool mIsOriginEntry;
        uint32 mFileHash;
        int32 mLineNumber;

        bool8 GetOriginFileLine(uint32& out_file_hash, int32& out_line_number);
};

// ====================================================================================================================
// class CFunctionParameterList:  Lists the parameters for a the selected function
// ====================================================================================================================
class CFunctionParameterList : public QTreeWidget
{
    Q_OBJECT

    public:
        CFunctionParameterList(QWidget* parent);
        virtual ~CFunctionParameterList();

        void Clear();
        void PopulateWithSignature(TinScript::CDebuggerFunctionAssistEntry* assist_entry);
        void PopulateWithOrigin(int32 stack_size, const uint32* file_hash_array, const int32* line_array);

    public slots:
        void OnDoubleClicked(QTreeWidgetItem* _item);

    private:
        QTreeWidgetItem* mHeader;
        QList<CFunctionParameterEntry*> mParameterList;
};

#endif // __TINQTFUNCTIONASSISTWIN_H

// ====================================================================================================================
// eof
// ====================================================================================================================
