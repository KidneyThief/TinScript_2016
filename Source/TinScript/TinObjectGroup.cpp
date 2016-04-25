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
// TinObjectGroup.cpp
// ====================================================================================================================

// -- includes
#include "assert.h"

#include "TinScript.h"
#include "TinRegistration.h"
#include "TinObjectGroup.h"

// == namespace TinScript =============================================================================================

namespace TinScript
{

// == class CMasterMembershipList =====================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CMasterMembershipList::CMasterMembershipList(CScriptContext* script_context, int32 _size)
{
    assert(script_context != NULL && _size > 0);
    mContextOwner = script_context;

    mMasterMembershipList = TinAlloc(ALLOC_ObjectGroup, CHashTable<tMembershipList>, _size);
}

// ====================================================================================================================
// Destructor
// ====================================================================================================================
CMasterMembershipList::~CMasterMembershipList()
{
    if (mMasterMembershipList)
    {
        mMasterMembershipList->DestroyAll();
        TinFree(mMasterMembershipList);
    }
}

// ====================================================================================================================
// AddMembership():  Notify the master list that an object has been added to an object set.
// ====================================================================================================================
void CMasterMembershipList::AddMembership(CObjectEntry* oe, CObjectSet* group)
{
    // -- sanity check
    if (!oe || !group)
        return;

    uint32 group_id = GetScriptContext()->FindIDByAddress(group);
    uint32 object_id = oe->GetID();

    // -- get the member list for the specific object, and add this group to it
    tMembershipList* member_list = mMasterMembershipList->FindItem(object_id);
    if (!member_list)
    {
        member_list = TinAlloc(ALLOC_ObjectGroup, tMembershipList, kObjectGroupTableSize);
        mMasterMembershipList->AddItem(*member_list, object_id);
    }

    // -- ensure we don't add this group twice
    if (!member_list->FindItem(group_id))
        member_list->AddItem(*group, group_id);

    // -- notify the debugger of the discontinued membership
    GetScriptContext()->DebuggerNotifySetAddObject(group_id, object_id, (oe->GetGroupID() == group_id));
}

// ====================================================================================================================
// RemoveMembership():  Notify the master list that an object has been removed from an object set.
// ====================================================================================================================
void CMasterMembershipList::RemoveMembership(CObjectEntry* oe, CObjectSet* group)
{
    // -- sanity check
    if (!oe || !group)
        return;

    uint32 group_id = GetScriptContext()->FindIDByAddress(group);
    uint32 object_id = oe->GetID();

    // -- get the member list for the specific object, and add this group to it
    tMembershipList* member_list = mMasterMembershipList->FindItem(object_id);
    if (!member_list)
    {
        ScriptAssert_(GetScriptContext(), 0, "<internal>", -1,
                      "Error - RemoveMembership() - no membership list for object %d\n", object_id);
        return;
    }

    // -- ensure the object is actually in the group
    if (member_list->FindItem(group_id))
    {
        member_list->RemoveItem(group_id);

        // -- if this is the last group the object is a member of, we can delete the group
        if (member_list->Used() == 0)
        {
            mMasterMembershipList->RemoveItem(object_id);
            TinFree(member_list);
        }
    }

    // -- notify the debugger of the discontinued membership
    GetScriptContext()->DebuggerNotifySetRemoveObject(group_id, object_id);
}

// ====================================================================================================================
// OnDelete():  Notify the master list that an object is being deleted - this will remove it from all object sets.
// ====================================================================================================================
void CMasterMembershipList::OnDelete(CObjectEntry* oe)
{
    // -- sanity check
    if (!oe)
        return;

    uint32 objectid = oe->GetID();
    // -- see if this object belongs to any groups
    tMembershipList* member_list = mMasterMembershipList->FindItem(oe->GetID());
    if (member_list)
    {
        int32 cur_count = member_list->Used();
        while (cur_count > 1)
        {
            CObjectSet* group = member_list->First();
            group->RemoveObject(objectid);

            // -- ensure our count actually went down
            int32 new_count = member_list->Used();
            if (new_count != (cur_count - 1))
            {
                //Assert_(false, "Error - CMasterMembershipList::OnDelete() failed to remove object");
                Assert_(false);
            }
            cur_count = new_count;
        }

        // -- delete the last membership entry separately since empty lists
        // -- auto-delete themselves
        CObjectSet* group = member_list->First();
        if (group)
            group->RemoveObject(objectid);
    }
}

// == class CObjectSet ================================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CObjectSet::CObjectSet()
{
    mContextOwner = TinScript::GetContext();
    mObjectList = TinAlloc(ALLOC_ObjectGroup, CHashTable<CObjectEntry>, kObjectGroupTableSize);
}

// ====================================================================================================================
// Destructor
// ====================================================================================================================
CObjectSet::~CObjectSet()
{
    // -- use RemoveAll(), as it will call OnRemove cleanly
    RemoveAll();
    TinFree(mObjectList);
}

// ====================================================================================================================
// Contains():  Returns true if the set contains the object.
// ====================================================================================================================
bool8 CObjectSet::Contains(uint32 objectid)
{
    bool8 result = mObjectList->FindItem(objectid) != NULL;
    return (result);
}

// ====================================================================================================================
// IsInHierarchy():  returns true, if the given object is within the hierarchy.
// ====================================================================================================================
bool8 CObjectSet::IsInHierarchy(uint32 objectid)
{
    // -- if the given objectid is ourself, it's "in the hierarchy"
    uint32 self_id = GetScriptContext()->FindObjectByAddress(this)->GetID();
    if (self_id == objectid)
    {
        return (true);
    }

    // -- if the object is a direct child, it's in the hierarchy
    if (Contains(objectid))
    {
        return (true);
    }

    // -- loop through the child list - if any of them are sets, see if the object is contained within their hierarchy
    CObjectEntry* child_oe = mObjectList->First();
    while (child_oe)
    {
        static uint32 object_set_hash = Hash("CObjectSet");
        if (child_oe->HasNamespace(object_set_hash))
        {
            CObjectSet* child_set = static_cast<CObjectSet*>(GetScriptContext()->FindObject(child_oe->GetID()));
            if (child_set->IsInHierarchy(objectid))
            {
                return (true);
            }
        }

        // -- next child
        child_oe = mObjectList->Next();
    }

    // -- the object is not in the hierarchy
    return (false);
}

// ====================================================================================================================
// AddObject():  Add an object to this object set.
// ====================================================================================================================
void CObjectSet::AddObject(uint32 objectid)
{
    // -- find the object entry
    uint32 self_id = GetScriptContext()->FindObjectByAddress(this)->GetID();
    CObjectEntry* oe = GetScriptContext()->FindObjectEntry(objectid);
    if (!oe)
    {
        ScriptAssert_(GetScriptContext(), 0, "<internal>", -1,
                      "Error - [%d] CObjectSet::AddObject(): unable to find object %d\n",
                      self_id, objectid);
        return;
    }

    // -- ensure we don't create a circular ownership
    static uint32 object_set_hash = Hash("CObjectSet");
    if (oe->HasNamespace(object_set_hash))
    {
        CObjectSet* object_set = static_cast<CObjectSet*>(GetScriptContext()->FindObject(objectid));
        if (object_set->IsInHierarchy(self_id))
        {
            ScriptAssert_(GetScriptContext(), 0, "<internal>", -1,
                "Error - [%d] CObjectSet::AddObject() - circular reference: object %d is parent of set %d\n",
                self_id, objectid, self_id);
            return;
        }
    }

    if (!mObjectList->FindItem(objectid))
    {
        mObjectList->AddItem(*oe, objectid);

        // -- notify the master membership list that an object has been added to a group
        GetScriptContext()->GetMasterMembershipList()->AddMembership(oe, this);

        // -- automatically call "OnAdd" for the group
        if (GetScriptContext()->HasMethod(this, "OnAdd"))
        {
            int32 dummy = 0;
            ObjExecF(this, dummy, "OnAdd(%d);", objectid);
        }
    }
}

// ====================================================================================================================
// InsertObject():  Add an object to this object set.
// ====================================================================================================================
void CObjectSet::InsertObject(uint32 objectid, int32 index)
{
    // -- find the object entry
    uint32 self_id = GetScriptContext()->FindObjectByAddress(this)->GetID();
    CObjectEntry* oe = GetScriptContext()->FindObjectEntry(objectid);
    if (!oe)
    {
        ScriptAssert_(GetScriptContext(), 0, "<internal>", -1,
                      "Error - [%d] CObjectSet::InsertObject(): unable to find object %d\n",
                      self_id, objectid);
        return;
    }

    // -- ensure we don't create a circular ownership
    static uint32 object_set_hash = Hash("CObjectSet");
    if (oe->HasNamespace(object_set_hash))
    {
        CObjectSet* object_set = static_cast<CObjectSet*>(GetScriptContext()->FindObject(objectid));
        if (object_set->IsInHierarchy(self_id))
        {
            ScriptAssert_(GetScriptContext(), 0, "<internal>", -1,
                "Error - [%d] CObjectSet::InsertObject() - circular reference: object %d is parent of set %d\n",
                self_id, objectid, self_id);
            return;
        }
    }

    if (!mObjectList->FindItem(objectid))
    {
        mObjectList->InsertItem(*oe, objectid, index);

        // -- notify the master membership list that an object has been added to a group
        GetScriptContext()->GetMasterMembershipList()->AddMembership(oe, this);

        // -- automatically call "OnAdd" for the group
        if (GetScriptContext()->HasMethod(this, "OnAdd"))
        {
            int32 dummy = 0;
            ObjExecF(this, dummy, "OnAdd(%d);", objectid);
        }
    }
}

// ====================================================================================================================
// RemoveObject():  Remove an object from this object set.
// ====================================================================================================================
void CObjectSet::RemoveObject(uint32 objectid)
{
    // -- find the object entry
    CObjectEntry* oe = GetScriptContext()->FindObjectEntry(objectid);
    if (!oe)
    {
        ScriptAssert_(GetScriptContext(), 0, "<internal>", -1,
                      "Error - [%d] CObjectSet::RemoveObject(): unable to find object %d\n",
                      GetScriptContext()->FindObjectByAddress(this)->GetID(), objectid);
        return;
    }

    if (mObjectList->FindItem(objectid))
    {
        mObjectList->RemoveItem(objectid);

        // -- notify the master membership list that an object has been added to a group
        GetScriptContext()->GetMasterMembershipList()->RemoveMembership(oe, this);

        // -- automatically call "OnAdd" for the group
        if (GetScriptContext()->HasMethod(this, "OnRemove"))
        {
            int32 dummy = 0;
            ObjExecF(this, dummy, "OnRemove(%d);", objectid);
        }
    }
}

// ====================================================================================================================
// ListObjects():  Debug method to dump the contents of this object set to standard out.
// ====================================================================================================================
void CObjectSet::ListObjects(int32 indent)
{
    if (indent == 0)
        TinPrint(GetScriptContext(), "\n");

    CObjectEntry* oe = mObjectList->First();
    while (oe)
    {
        GetScriptContext()->PrintObject(oe, indent);

        // -- if the object is an ObjectSet, list it's objects
        if (GetScriptContext()->HasMethod(oe->GetID(), "ListObjects"))
        {
            int32 dummy = 0;
            ObjExecF(oe->GetID(), dummy, "ListObjects(%d);", indent + 1);
        }

        // -- next object
        oe = mObjectList->Next();
    }
}

// ====================================================================================================================
// RemoveAll():  Remove all objects contained in this object set.
// ====================================================================================================================
void CObjectSet::RemoveAll()
{
    int32 count = mObjectList->Used();
    while (count > 0)
    {
        CObjectEntry* oe = mObjectList->FindItemByIndex(count - 1);
        RemoveObject(oe->GetID());
        count = mObjectList->Used();
    }
}

// ====================================================================================================================
// First():  Returns the first object in this set.  Note:  sets the internal iterator, in anticipation of Next().
// ====================================================================================================================
uint32 CObjectSet::First()
{
    CObjectEntry* oe = mObjectList->First();
    if (oe)
        return (oe->GetID());
    else
        return (0);
}

// ====================================================================================================================
// Next():  Returns the next object added to the set, updating the internal iterator.
// ====================================================================================================================
uint32 CObjectSet::Next()
{
    CObjectEntry* oe = mObjectList->Next();
    if (oe)
        return (oe->GetID());
    else
        return (0);
}

// ====================================================================================================================
// Prev():  Returns the prev object added to the set, updating the internal iterator.
// ====================================================================================================================
uint32 CObjectSet::Prev()
{
    CObjectEntry* oe = mObjectList->Prev();
    if (oe)
        return (oe->GetID());
    else
        return (0);
}

// ====================================================================================================================
// Last():  Returns the last object added to the set, updating the internal iterator.
// ====================================================================================================================
uint32 CObjectSet::Last()
{
    CObjectEntry* oe = mObjectList->Last();
    if (oe)
        return (oe->GetID());
    else
        return (0);
}

// ====================================================================================================================
// Current():  Returns the current object, the interal iterator is referring to.
// ====================================================================================================================
uint32 CObjectSet::Current()
{
    CObjectEntry* oe = mObjectList->Current();
    if (oe)
        return (oe->GetID());
    else
        return (0);
}

// ====================================================================================================================
// IsFirst():  Returns true if the given object is the first in the set
// ====================================================================================================================
bool8 CObjectSet::IsFirst(uint32 object_id)
{
    int32 count = mObjectList->Used();
    if (count <= 0)
        return (false);

    CObjectEntry* oe = mObjectList->FindItemByIndex(0);
    return (oe->GetID() == object_id);
}

// ====================================================================================================================
// IsLast():  Returns true if the given object is the last in the set
// ====================================================================================================================
bool8 CObjectSet::IsLast(uint32 object_id)
{
    int32 count = mObjectList->Used();
    if (count <= 0)
        return (false);

    CObjectEntry* oe = mObjectList->FindItemByIndex(count - 1);
    return (oe->GetID() == object_id);
}

// ====================================================================================================================
// CreateIterator():  Creates an independent iterator to loop through the set.
// ====================================================================================================================
uint32 CObjectSet::CreateIterator()
{
    // -- find the object entry
    CObjectEntry* oe = GetScriptContext()->FindObjectByAddress(this);
    CHashTable<CObjectEntry>::CHashTableIterator* newIterator = mObjectList->CreateIterator();

    // -- the iterator is a scriptable object
    uint32 iteratorID = TinScript::GetContext()->CreateObject(TinScript::Hash("CGroupIterator"), 0);
    if (iteratorID == 0)
        return (0);

    // -- get the actual iterator
    CGroupIterator* iteratorObject = (CGroupIterator*)(TinScript::GetContext()->FindObject(iteratorID));

    // -- initialize the iterator for the specific group
    iteratorObject->Initialize(oe->GetID(), newIterator, iteratorID);

    // -- return the object
    return (iteratorID);
}

// ====================================================================================================================
// Used():  Returns the number of objects contained in tis object set.
// ====================================================================================================================
int32 CObjectSet::Used()
{
    return mObjectList->Used();
}

// ====================================================================================================================
// GetObjectByIndex():  Returns the nth object added to this object set.  Also sets the internal iterator.
// ====================================================================================================================
uint32 CObjectSet::GetObjectByIndex(int32 index)
{
    // -- sanity check
    if (index < 0 || index >= Used())
        return 0;

    CObjectEntry* oe = mObjectList->FindItemByIndex(index);
    return (oe->GetID());
}

// == class CObjectGroup ==============================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CObjectGroup::CObjectGroup() : CObjectSet()
{
}

// ====================================================================================================================
// Destructor
// ====================================================================================================================
CObjectGroup::~CObjectGroup()
{
    // -- object groups actually delete their children
    if (mObjectList)
    {
        int32 count = mObjectList->Used();
        while (count > 0)
        {
            CObjectEntry* oe = mObjectList->FindItemByIndex(count - 1);
            GetScriptContext()->DestroyObject(oe->GetID());
            count = mObjectList->Used();
        }
    }
}

// ====================================================================================================================
// AddObject():  Adds an object to this object group - automatically removes it from it's previous object group.
// ====================================================================================================================
void CObjectGroup::AddObject(uint32 objectid)
{
    // -- find the object entry
    CObjectEntry* oe = GetScriptContext()->FindObjectEntry(objectid);
    if (!oe)
    {
        ScriptAssert_(GetScriptContext(), 0, "<internal>", -1,
                      "Error - [%d] CObjectGroup::AddObject(): unable to find object %d\n",
                      GetScriptContext()->FindObjectByAddress(this)->GetID(), objectid);
        return;
    }

    // -- if we have a current owner, different this group, remove us from it
    CObjectGroup* current_owner = oe->GetObjectGroup();
    if (current_owner == this)
        return;

    if (current_owner)
        current_owner->RemoveObject(objectid);

    // -- add the object to this group
    oe->SetObjectGroup(this);
    CObjectSet::AddObject(objectid);
}

// ====================================================================================================================
// RemoveObject():  Remove an object from this object group.
// ====================================================================================================================
void CObjectGroup::RemoveObject(uint32 objectid)
{
    // -- find the object entry
    CObjectEntry* oe = GetScriptContext()->FindObjectEntry(objectid);
    if (!oe)
    {
        ScriptAssert_(GetScriptContext(), 0, "<internal>", -1,
                      "Error - [%d] CObjectGroup::RemoveObject(): unable to find object %d\n",
                      GetScriptContext()->FindObjectByAddress(this)->GetID(), objectid);
        return;
    }

    // -- remove the object
    CObjectSet::RemoveObject(objectid);
    oe->SetObjectGroup(NULL);
}

// == class CGroupIterator =============================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CGroupIterator::CGroupIterator()
{
    m_groupID = 0;
    m_iterator = nullptr;
}

// ====================================================================================================================
// Initialize():  Cache the members for the group to iterate on, and the actua hash table iterator.
// ====================================================================================================================
void CGroupIterator::Initialize(uint32 groupID, CHashTable<CObjectEntry>::CHashTableIterator* iterator,
                                uint32 iter_object_id)
{
    // -- set the members to cache the group being iterated on, and the actual iterator
    m_groupID = groupID;
    m_iterator = iterator;
    iterator->m_objectID = iter_object_id;
}

// ====================================================================================================================
// Destructor
// ====================================================================================================================
CGroupIterator::~CGroupIterator()
{
    // -- remove ourself from the hashtable iterator list
    if (m_iterator != nullptr)
        TinFree(m_iterator);
}

// ====================================================================================================================
// First():  Initializes the iterator to returns the first object in the set.
// ====================================================================================================================
uint32 CGroupIterator::First()
{
    if (m_iterator == nullptr)
    {
        ScriptAssert_(TinScript::GetContext(), 0, "<internal>", -1,
                      "Error - [%d] CGroupIterator::First(): this iterator has not been initialized\n"
                      "Use CObjectSet::CreateIterator() to construct a properly initialized iterator.",
                      TinScript::GetContext()->FindObjectByAddress(this)->GetID());
        return (0);
    }

    CObjectEntry* oe = m_iterator->First();
    if (oe != nullptr)
        return (oe->GetID());
    else
        return (0);
}

// ====================================================================================================================
// Next():  Increments the iterator to return the next object in the set.
// ====================================================================================================================
uint32 CGroupIterator::Next()
{
    if (m_iterator == nullptr)
    {
        ScriptAssert_(TinScript::GetContext(), 0, "<internal>", -1,
                      "Error - [%d] CGroupIterator::Next(): this iterator has not been initialized\n"
                      "Use CObjectSet::CreateIterator() to construct a properly initialized iterator.",
                      TinScript::GetContext()->FindObjectByAddress(this)->GetID());
        return (0);
    }

    CObjectEntry* oe = m_iterator->Next();
    if (oe != nullptr)
        return (oe->GetID());
    else
        return (0);
}

// ====================================================================================================================
// Prev():  Decrements the iterator to return the prev object in the set.
// ====================================================================================================================
uint32 CGroupIterator::Prev()
{
    if (m_iterator == nullptr)
    {
        ScriptAssert_(TinScript::GetContext(), 0, "<internal>", -1,
                      "Error - [%d] CGroupIterator::Prev(): this iterator has not been initialized\n"
                      "Use CObjectSet::CreateIterator() to construct a properly initialized iterator.",
                      TinScript::GetContext()->FindObjectByAddress(this)->GetID());
        return (0);
    }

    CObjectEntry* oe = m_iterator->Prev();
    if (oe != nullptr)
        return (oe->GetID());
    else
        return (0);
}

// ====================================================================================================================
// Last():  Increments the iterator to return the last object in the set.
// ====================================================================================================================
uint32 CGroupIterator::Last()
{
    if (m_iterator == nullptr)
    {
        ScriptAssert_(TinScript::GetContext(), 0, "<internal>", -1,
                      "Error - [%d] CGroupIterator::Last(): this iterator has not been initialized\n"
                      "Use CObjectSet::CreateIterator() to construct a properly initialized iterator.",
                      TinScript::GetContext()->FindObjectByAddress(this)->GetID());
        return (0);
    }

    CObjectEntry* oe = m_iterator->Last();
    if (oe != nullptr)
        return (oe->GetID());
    else
        return (0);
}

// ====================================================================================================================
// Current():  Returns the object the iterator is currently referencing.
// ====================================================================================================================
uint32 CGroupIterator::Current()
{
    if (m_iterator == nullptr)
    {
        ScriptAssert_(TinScript::GetContext(), 0, "<internal>", -1,
                      "Error - [%d] CGroupIterator::Current(): this iterator has not been initialized\n"
                      "Use CObjectSet::CreateIterator() to construct a properly initialized iterator.",
                      TinScript::GetContext()->FindObjectByAddress(this)->GetID());
        return (0);
    }

    CObjectEntry* oe = m_iterator->Current();
    if (oe != nullptr)
        return (oe->GetID());
    else
        return (0);
}

// ====================================================================================================================
// GetGroup():  Increments the iterator to return the next object in the set.
// ====================================================================================================================
uint32 CGroupIterator::GetGroup()
{
    if (m_iterator == nullptr)
    {
        ScriptAssert_(TinScript::GetContext(), 0, "<internal>", -1,
                      "Error - [%d] CGroupIterator::First(): this iterator has not been initialized\n"
                      "Use CObjectSet::CreateIterator() to construct a properly initialized iterator.",
                      TinScript::GetContext()->FindObjectByAddress(this)->GetID());
        return (0);
    }

    return (m_groupID);
}

// == Registration =====================================================================================================

// =====================================================================================================================
// -- CObjectSet member/method registration
IMPLEMENT_SCRIPT_CLASS_BEGIN(CObjectSet, VOID)
IMPLEMENT_SCRIPT_CLASS_END()

REGISTER_METHOD_P1(CObjectSet, Contains, Contains, bool8, uint32);
REGISTER_METHOD_P1(CObjectSet, AddObject, AddObject, void, uint32);
REGISTER_METHOD_P2(CObjectSet, InsertObject, InsertObject, void, uint32, int32);
REGISTER_METHOD_P1(CObjectSet, RemoveObject, RemoveObject, void, uint32);
REGISTER_METHOD_P1(CObjectSet, ListObjects, ListObjects, void, int32);
REGISTER_METHOD_P0(CObjectSet, RemoveAll, RemoveAll, void);

REGISTER_METHOD_P0(CObjectSet, First, First, uint32);
REGISTER_METHOD_P0(CObjectSet, Next, Next, uint32);
REGISTER_METHOD_P0(CObjectSet, Prev, Prev, uint32);
REGISTER_METHOD_P0(CObjectSet, Last, Last, uint32);
REGISTER_METHOD_P0(CObjectSet, Current, Current, uint32);

REGISTER_METHOD_P1(CObjectSet, IsFirst, IsFirst, bool8, uint32);
REGISTER_METHOD_P1(CObjectSet, IsLast, IsLast, bool8, uint32);

REGISTER_METHOD_P0(CObjectSet, Used, Used, int32);

REGISTER_METHOD_P1(CObjectSet, GetObjectByIndex, GetObjectByIndex, uint32, int32);

REGISTER_METHOD_P0(CObjectSet, CreateIterator, CreateIterator, uint32);

// =====================================================================================================================
// -- CObjectGroup member/method registration
IMPLEMENT_SCRIPT_CLASS_BEGIN(CObjectGroup, CObjectSet)
IMPLEMENT_SCRIPT_CLASS_END()

REGISTER_METHOD_P1(CObjectGroup, AddObject, AddObject, void, uint32);
REGISTER_METHOD_P1(CObjectGroup, RemoveObject, RemoveObject, void, uint32);

// =====================================================================================================================
// -- CGroupIterator member/method registration
IMPLEMENT_SCRIPT_CLASS_BEGIN(CGroupIterator, VOID)
IMPLEMENT_SCRIPT_CLASS_END()

REGISTER_METHOD_P0(CGroupIterator, First, First, uint32);
REGISTER_METHOD_P0(CGroupIterator, Next, Next, uint32);
REGISTER_METHOD_P0(CGroupIterator, Prev, Prev, uint32);
REGISTER_METHOD_P0(CGroupIterator, Last, Last, uint32);
REGISTER_METHOD_P0(CGroupIterator, Current, Current, uint32);
REGISTER_METHOD_P0(CGroupIterator, GetGroup, GetGroup, uint32);

} // TinScript

// =====================================================================================================================
// EOF
// =====================================================================================================================
