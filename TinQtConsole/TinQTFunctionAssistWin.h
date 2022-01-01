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
        TinScript::CDebuggerFunctionAssistEntry*
            FindFunctionEntry(const TinScript::CDebuggerFunctionAssistEntry& assist_entry);
        TinScript::CDebuggerFunctionAssistEntry*
            FindFunctionEntry(TinScript::eFunctionEntryType entry_type, uint32 obj_id, uint32 ns_hash,
                              uint32 func_hash);

        void NotifyFunctionAssistEntry(const TinScript::CDebuggerFunctionAssistEntry& assist_entry);
        void NotifyListObjectsComplete();
        void NotifyFunctionAssistComplete();

        // -- there are three reasons to update the search results - if we haven't changed the
        // -- search string, but we've received potentially acceptable new entries
        // -- or if we extend our search string, so the existing list needs to remove more
        // -- or if we've changed, or backtracked our search string, so we search from scratch
        void UpdateSearchNewEntry(TinScript::CDebuggerFunctionAssistEntry* assist_entry);

        // -- string comparison functions for filtering
        bool StringContainsFilter(const char* string, bool& exact_match, bool& new_object_id);
        bool FilterStringCompare(const char* string);
        void UpdateFilter(const char* filter, bool8 force_refresh = false);

        // -- the Signature panel also doubles as the origin callstack
        void DisplayObjectOrigin(uint32 obj_id);
        void DisplayFunctionSignature();

        // -- methods to handle selecting and issuing a function
        void SetAssistObjectID(uint32 object_id);
        void NotifyAssistEntryClicked(TinScript::CDebuggerFunctionAssistEntry* list_entry);
        void NotifyAssistEntryDoubleClicked(TinScript::CDebuggerFunctionAssistEntry* list_entry);

        // -- if this window is selected when we try to create a new watch expression, use
        // the assist object (from the browser), or the selected object from the search
        uint32 GetAssistObjectID() const;
        bool GetSelectedWatchExpression(int32& out_use_watch_id, char* out_watch_string, int32 max_expr_length,
                                        char* out_value, int32 max_value_length);

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
        uint32 mSelectedNamespaceHash;
		uint32 mSelectedFunctionHash;
        uint32 mSelectedObjectID;
        CFunctionParameterList* mParameterList;

        // -- helper function that performs a single pass
        // -- the main method tries every substring looking for containment
        bool StringContainsFilterImpl(const char* string, const char* filter, bool& exact_match, bool& new_object_id);

        // -- for filtering, we need to keep track of several items, that will change the context of the search
        uint32 mSearchObjectID;
        uint32 mSearchNamespaceHash;
        char mFilterString[TinScript::kMaxNameLength];
        QList<TinScript::CDebuggerFunctionAssistEntry*> mFunctionEntryList; // includes objects and namespaces
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

        // -- note:  the "owner" of the CDebuggerFunctionAssistEntry is
        // CDebugFunctionAssistWin::mFunctionEntryList
        // -- this entry is always pointing at the instance within that list
        TinScript::CDebuggerFunctionAssistEntry* mFunctionAssistEntry;

        // -- sorting the display is *complicated*...!
        virtual bool operator < (const QTreeWidgetItem& other) const override;
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

        CFunctionListEntry* FindEntry(TinScript::CDebuggerFunctionAssistEntry* in_entry);
        bool DisplayEntry(TinScript::CDebuggerFunctionAssistEntry* in_entry);
        bool FilterEntry(TinScript::CDebuggerFunctionAssistEntry* in_entry);
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
        CFunctionParameterEntry(TinScript::eVarType var_type, bool is_array, const char* _name,
                                uint32* default_value, QTreeWidget* _owner);
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
