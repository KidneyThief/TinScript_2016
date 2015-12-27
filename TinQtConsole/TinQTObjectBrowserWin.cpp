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
// TinQTObjectBrowserWin.cpp
// ====================================================================================================================

#include <QLineEdit>
#include <QKeyEvent>
#include <QMessageBox>

#include "TinScript.h"
#include "TinRegistration.h"

#include "TinQTObjectBrowserWin.h"
#include "TinQTConsole.h"
#include "mainwindow.h"

// == class CBrowserEntry =============================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CBrowserEntry::CBrowserEntry(uint32 parent_id, uint32 object_id, bool8 owned, const char* object_name,
                             const char* derivation)
    : QTreeWidgetItem()
{
    mObjectID = object_id;
    mParentID = parent_id;
    mOwned = owned;
    TinScript::SafeStrcpy(mName, object_name, TinScript::kMaxNameLength);
    TinScript::SafeStrcpy(mDerivation, derivation, TinScript::kMaxNameLength);

    // -- create and store the formatted name string
    sprintf_s(mFormattedName, TinScript::kMaxNameLength, "[%d] %s", object_id, mName);

    // -- set the QT elements
    setText(0, mFormattedName);
    setText(1, mDerivation);
}

// ====================================================================================================================
// Destructor
// ====================================================================================================================
CBrowserEntry::~CBrowserEntry()
{
}

// == class CDebugObjectBrowserWin ====================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CDebugObjectBrowserWin::CDebugObjectBrowserWin(QWidget* parent) : QTreeWidget(parent)
{
    setColumnCount(2);
    setItemsExpandable(true);
    setExpandsOnDoubleClick(true);

    // -- set the headers
    QStringList headers;
    headers.append(tr("Object Hierarchy"));
    headers.append(tr("Derivation"));
    setHeaderLabels(headers);
}

// ====================================================================================================================
// Destructor
// ====================================================================================================================
CDebugObjectBrowserWin::~CDebugObjectBrowserWin()
{
    RemoveAll();
}

// ====================================================================================================================
// NotifyOnConnect():  Called when the debugger's connection to the target is initially confirmed.
// ====================================================================================================================
void CDebugObjectBrowserWin::NotifyOnConnect()
{
    // -- request a fresh population of the existing objects
    SocketManager::SendCommand("DebuggerListObjects();");
}

// ====================================================================================================================
// NotifyCreateObject():  Notify a new object has been created.
// ====================================================================================================================
void CDebugObjectBrowserWin::NotifyCreateObject(uint32 object_id, const char* object_name, const char* derivation)
{
    // -- if we already have an entry for this object, we're done
    if (mObjectDictionary.contains(object_id))
        return;
    
    // -- create the list, add it to the object dictionary
    QList<CBrowserEntry*>* entry_list = new QList<CBrowserEntry*>();
    mObjectDictionary.insert(object_id, entry_list);

    // -- now create the actual entry, and add it to the list
    CBrowserEntry* new_entry = new CBrowserEntry(0, object_id, false, object_name, derivation);
    entry_list->append(new_entry);

    // -- until we're parented, we want to display the entry
    addTopLevelItem(new_entry);
}

// ====================================================================================================================
// NotifyDestroyObject():  Notify an object has been destroyed.
// ====================================================================================================================
void CDebugObjectBrowserWin::NotifyDestroyObject(uint32 object_id)
{
    // -- remove all entries from the dictionary
    if (!mObjectDictionary.contains(object_id))
        return;

    // -- only the first entry ever needs to be deleted, as all others are parented
    // -- and therefore deleted when the parent entry is deleted
    QList<CBrowserEntry*>* object_entry_list = mObjectDictionary[object_id];
    CBrowserEntry* object_entry = (*object_entry_list)[0];
    delete object_entry;
    object_entry_list->clear();

    // -- remove the list from the dictionary
    mObjectDictionary.remove(object_id);
    delete object_entry_list;
}

// ====================================================================================================================
// RecursiveSetAddObject(): Add the entire hierarchy of an object to a new parent entry.
// ====================================================================================================================
void CDebugObjectBrowserWin::RecursiveSetAddObject(CBrowserEntry* parent_entry, uint32 child_id, bool8 owned)
{
    // -- get the list of entries refering to this object, and the first entry
    QList<CBrowserEntry*>* object_entry_list = mObjectDictionary[child_id];
    CBrowserEntry* object_entry = (*object_entry_list)[0];

    // -- we need to duplicate the object entry, add add it as a child to the new parent entry
    CBrowserEntry* new_entry = new CBrowserEntry(parent_entry->mObjectID, child_id, owned, object_entry->mName,
                                                 object_entry->mDerivation);
    // -- add the new entry as a child
    parent_entry->addChild(new_entry);

    // -- add the new entry to our entry list
    object_entry_list->append(new_entry);

    // -- now duplicate each child owned by the object_entry in the new branch
    int child_count = object_entry->childCount();
    for (int i = 0; i < child_count; ++i)
    {
        CBrowserEntry* child_entry = static_cast<CBrowserEntry*>(object_entry->child(i));
        if (child_entry)
        {
            // -- add this child's hierarchy to the new parent's hierarchy
            RecursiveSetAddObject(new_entry, child_entry->mObjectID, child_entry->mOwned);
        }
    }
}

// ====================================================================================================================
// NotifySetAddObject():  Notify an object has been destroyed.
// ====================================================================================================================
void CDebugObjectBrowserWin::NotifySetAddObject(uint32 set_id, uint32 object_id, bool8 owned)
{
    // -- ensure both objects exist
    if (!mObjectDictionary.contains(set_id) || !mObjectDictionary.contains(object_id))
        return;

    // -- get the original object entry (so we can clone the name and hierarchy)
    QList<CBrowserEntry*>* object_entry_list = mObjectDictionary[object_id];
    QList<CBrowserEntry*>* set_entry_list = mObjectDictionary[set_id];
    CBrowserEntry* object_entry = (*object_entry_list)[0];
    CBrowserEntry* set_entry = (*set_entry_list)[0];

    // -- if we've already received notification that object_id is a child of set_id, we're done
    int child_count = set_entry->childCount();
    for (int i = 0; i < child_count; ++i)
    {
        CBrowserEntry* child_entry = static_cast<CBrowserEntry*>(set_entry->child(i));
        if (child_entry && child_entry->mObjectID == object_id)
            return;
    }

    // -- for each entry in the set_entry_list, add a new object_entry
    for (int i = 0; i < set_entry_list->size(); ++i)
    {
        CBrowserEntry* set_entry = (*set_entry_list)[i];
        RecursiveSetAddObject(set_entry, object_id, owned);
    }

    // -- if we've found an owner for the object, the original "root level" entry is now hidden
    if (owned)
        object_entry->setHidden(true);
}

// ====================================================================================================================
// NotifySetRemoveObject():  Notify that an object is no longer a member of a set.
// ====================================================================================================================
void CDebugObjectBrowserWin::NotifySetRemoveObject(uint32 set_id, uint32 object_id)
{
    // -- ensure the object exists (and we have a valid set_id)
    if (!mObjectDictionary.contains(object_id) || set_id == 0)
        return;

    // -- get the list of all instances of the object
    QList<CBrowserEntry*>* object_entry_list = mObjectDictionary[object_id];

    // -- find the instance belonging to the set
    bool remove_owned = false;
    for (int i = 1; i < object_entry_list->size(); ++i)
    {
        // -- get the entry, see if it's the one matching the set
        CBrowserEntry* object_entry = (*object_entry_list)[i];
        if (object_entry->mParentID == set_id)
        {
            // -- remove, delete and break
            remove_owned = object_entry->mOwned;
            object_entry_list->removeAt(i);
            delete object_entry;
            break;
        }
    }

    // -- if the size of the object_entry_list is now just the original, ensure it is no longer hidden
    if (remove_owned)
    {
        CBrowserEntry* object_entry = (*object_entry_list)[0];
        object_entry->setHidden(false);
    }
}

// ====================================================================================================================
// RemoveAll():  Remove all browser entries.
// ====================================================================================================================
void CDebugObjectBrowserWin::RemoveAll()
{
    // -- clear the map of all object entries
    while (mObjectDictionary.size() > 0)
    {
        uint32 object_id = mObjectDictionary.begin().key();
        NotifyDestroyObject(object_id);
    }
}

// ====================================================================================================================
// GetSelectedObjectID():  Return the objectID of the currently selected entry.
// ====================================================================================================================
uint32 CDebugObjectBrowserWin::GetSelectedObjectID()
{
    CBrowserEntry* cur_item = static_cast<CBrowserEntry*>(currentItem());
    if (cur_item)
    {
        return (cur_item->mObjectID);
    }

    // -- no current objects selected
    return (0);
}

// ====================================================================================================================
// FindObjectByName():  Return the object ID for the given name.
// ====================================================================================================================
uint32 CDebugObjectBrowserWin::FindObjectByName(const char* name)
{
    // -- sanity check
    if (!name || !name[0])
        return (0);

    QList<uint32>& key_list = mObjectDictionary.keys();
    for (int i = 0; i < key_list.size(); ++i)
    {
        QList<CBrowserEntry*>* entry_list = mObjectDictionary[key_list[i]];
        if (entry_list->size() == 0)
            continue;
        CBrowserEntry* entry = (*entry_list)[0];
        if (!_stricmp(entry->mName, name))
        {
            return (entry->mObjectID);
        }
    }

    // -- not found
    return (0);
}

// ====================================================================================================================
// GetObjectIdentifier():  Returns the identifier (formated ID and object name) for the requested entry.
// ====================================================================================================================
const char* CDebugObjectBrowserWin::GetObjectName(uint32 object_id)
{
    if (mObjectDictionary.contains(object_id))
    {
        // -- dereference to get the List, and then again to get the first item in the list
        CBrowserEntry* entry = (*(mObjectDictionary[object_id]))[0];
        return (entry->mName);
    }

    // -- not found
    return ("");
}

// ====================================================================================================================
// GetObjectIdentifier():  Returns the identifier (formated ID and object name) for the requested entry.
// ====================================================================================================================
const char* CDebugObjectBrowserWin::GetObjectIdentifier(uint32 object_id)
{
    if (mObjectDictionary.contains(object_id))
    {
        // -- dereference to get the List, and then again to get the first item in the list
        CBrowserEntry* entry = (*(mObjectDictionary[object_id]))[0];
        return (entry->mFormattedName);
    }

    // -- not found
    return ("");
}

// ====================================================================================================================
// GetObjectDerivation():  Returns the derivation for the requested entry.
// ====================================================================================================================
const char* CDebugObjectBrowserWin::GetObjectDerivation(uint32 object_id)
{
    if (mObjectDictionary.contains(object_id))
    {
        // -- dereference to get the List, and then again to get the first item in the list
        CBrowserEntry* entry = (*(mObjectDictionary[object_id]))[0];
        return (entry->mDerivation);
    }

    // -- not found
    return ("");
}

// ====================================================================================================================
// SetSelectedObject():  Find the object in the browser window, and set it as the selected item.
// ====================================================================================================================
void CDebugObjectBrowserWin::SetSelectedObject(uint32 object_id)
{
    if (mObjectDictionary.contains(object_id))
    {
        // -- dereference to get the List, and then again to get the first item in the list
        CBrowserEntry* entry = NULL;
        QList<CBrowserEntry*>* entry_list = mObjectDictionary[object_id];
        for (int i = 1; i < entry_list->size(); ++i)
        {
            CBrowserEntry* child_entry = (*entry_list)[i];
            if (child_entry && child_entry->mOwned)
            {
                // -- see if this is the hierarchy that leads to a root group object
                bool is_ownership_tree = true;
                CBrowserEntry* parent = static_cast<CBrowserEntry*>(child_entry->parent());
                while (parent)
                {
                    if (!parent->mOwned)
                    {
                        // -- if we reached parent node, that is not an owned child, and either
                        // -- the do have a designated parent ID, or the parent is hidden, then
                        // -- this is not a root group entry.
                        if (parent->mParentID != 0 || parent->isHidden())
                        {
                            is_ownership_tree = false;
                        }

                        // -- at this point, we're either reached the root group entry, or a non-ownership parent
                        break;
                    }

                    // -- get the next parent
                    parent = static_cast<CBrowserEntry*>(parent->parent());
                }

                // -- if we verified this child entry as being part of the ownership hierarchy, we're done
                if (is_ownership_tree)
                {
                    entry = child_entry;
                    break;
                }
            }
        }

        // -- if we didn't find an "owned" child entry, presumably the item is unowned, select the first entry
        if (!entry)
            entry = (*entry_list)[0];

        if (entry != NULL)
        {
            // -- ensure the hierarchy is expanded
            QTreeWidgetItem* parent = entry->parent();
            while (parent)
            {
                parent->setExpanded(true);
                parent = parent->parent();
            }

            setCurrentItem(entry, QAbstractItemView::EnsureVisible);
            scrollToItem(entry, QAbstractItemView::PositionAtCenter);
        }
    }
}

// ====================================================================================================================
// PopulateObjectIDList():  Retrieve a list of all object IDs.
// ====================================================================================================================
void CDebugObjectBrowserWin::PopulateObjectIDList(QList<uint32>& object_id_list)
{
    QList<uint32>& key_list = mObjectDictionary.keys();
    for (int i = 0; i < key_list.size(); ++i)
    {
        object_id_list.push_back(key_list[i]);
    }
}

// --------------------------------------------------------------------------------------------------------------------
#include "TinQTObjectBrowserWinMoc.cpp"

// ====================================================================================================================
// eof
// ====================================================================================================================
