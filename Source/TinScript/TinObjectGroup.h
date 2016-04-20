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
// TinObjectGroup.h
// ====================================================================================================================

#ifndef __TINOBJECTGROUP_H
#define __TINOBJECTGROUP_H

// -- includes --------------------------------------------------------------------------------------------------------

#include "TinHash.h"

// == namespace TinScript =============================================================================================

namespace TinScript
{

// --------------------------------------------------------------------------------------------------------------------
// -- forward declarations
class CObjectSet;

// ====================================================================================================================
// class CMasterMembershipList: This class is a "reverse lookup", where the has is the objectID, it returns
// a list of all CObjectSet's that the object is a member of. 
// ====================================================================================================================
class CMasterMembershipList
{
    public:
        typedef CHashTable<CObjectSet> tMembershipList;

        CMasterMembershipList(CScriptContext* script_context = NULL, int32 _size = 0);
        virtual ~CMasterMembershipList();

        CScriptContext* GetScriptContext() { return (mContextOwner); }

        void AddMembership(CObjectEntry* oe, CObjectSet* group);
        void RemoveMembership(CObjectEntry* oe, CObjectSet* group);

        void OnDelete(CObjectEntry* oe);

    private:
        CScriptContext* mContextOwner;
        CHashTable<tMembershipList>* mMasterMembershipList;
};

// ====================================================================================================================
// class CObjectSet: A basic storage class for sets of registered objects
// ====================================================================================================================
class CObjectSet
{
    public:
        DECLARE_SCRIPT_CLASS(CObjectSet, VOID);

        CObjectSet();
        virtual ~CObjectSet();

        CScriptContext* GetScriptContext() { return (mContextOwner); }

        bool8 IsInHierarchy(uint32 objectid);

        virtual bool8 Contains(uint32 objectid);
        virtual void AddObject(uint32 objectid);
        virtual void InsertObject(uint32 objectid, int32 index);
        virtual void RemoveObject(uint32 objectid);
        virtual void ListObjects(int32 indent = 0);
        virtual void RemoveAll();

        uint32 First();
        uint32 Next();
        uint32 Prev();
        uint32 Last();
        uint32 Current();

        bool8 IsFirst(uint32 object_id);
        bool8 IsLast(uint32 object_id);

        // -- creates an independent iterator
		uint32 CreateIterator();

        int32 Used();
        uint32 GetObjectByIndex(int32 index);

    protected:
        CScriptContext* mContextOwner;
        CHashTable<CObjectEntry>* mObjectList;
};

// ====================================================================================================================
// class CObjectGroup:  A derived class implies exclusive ownership - delete the group, it deletes its children.
// ====================================================================================================================
class CObjectGroup : public CObjectSet
{
    public:
        DECLARE_SCRIPT_CLASS(CObjectGroup, CObjectSet);

        CObjectGroup();
        virtual ~CObjectGroup();

        virtual void AddObject(uint32 objectid);
        virtual void RemoveObject(uint32 objectid);
};

// ====================================================================================================================
// class CGroupIterator: An iterator for looping through the elements of a group, tolerant of objects being
// added or removed from the group in the middle of the iteration loop.
// ====================================================================================================================
class CGroupIterator
{
	public:
		DECLARE_SCRIPT_CLASS(CGroupIterator, VOID);

        CGroupIterator();
        void Initialize(uint32 groupID, CHashTable<CObjectEntry>::CHashTableIterator* iterator, uint32 iter_object_id);
		virtual ~CGroupIterator();

		uint32 First();
		uint32 Next();
        uint32 Prev();
        uint32 Last();
        uint32 Current();

        uint32 GetGroup();

    private:
        uint32 m_groupID;
        CHashTable<CObjectEntry>::CHashTableIterator* m_iterator;
};

} // TinScript

#endif

// ====================================================================================================================
// eof
// ====================================================================================================================
