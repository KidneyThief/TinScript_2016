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
// TinHash.h
// an implementation of the DJB hashing algoritm
// ref:  https://blogs.oracle.com/ali/entry/gnu_hash_elf_sections
// ====================================================================================================================

#ifndef __TINHASH_H
#define __TINHASH_H

// -- includes

#include "assert.h"
#include "stdio.h"

#include "integration.h"

// == namespace Tinscript =============================================================================================

namespace TinScript
{

class CMemoryTracker;
class CObjectEntry;

// -- forward declarations
CScriptContext* GetContext();

// ====================================================================================================================
// -- implemented in TinScript.cpp
uint32 Hash(const char *s, int32 length = -1, bool add_to_table = true);
uint32 HashAppend(uint32 h, const char *string, int32 length = -1);
const char* UnHash(uint32 hash);

// ====================================================================================================================
// class CHashTable:  This class is used for *all* TinScript hash tables, of any type.
// Regardless of the content type being stored, this hash table only allows pointers (to that type).
// This will allow hash table entries to be a fixed size, and can be pooled amongst all tables.
// ====================================================================================================================
template <class T>
class CHashTable
{
	public:

    // ====================================================================================================================
    // class CHashTableEntry:  As mentioned, all hash tables store pointers only, using this common entry class.
    // ====================================================================================================================
	class CHashTableEntry
    {
		public:
			CHashTableEntry(T& _item, uint32 _hash)
            {
				item = &_item;
				hash = _hash;
				nextbucket = NULL;

                index = -1;
                index_next = NULL;
			}

			T* item;
			uint32 hash;
			CHashTableEntry* nextbucket;

            int32 index;
			CHashTableEntry* index_next;
	};

	// ====================================================================================================================
	// class CHashTableIterator:  Hash tables maintain a list of current iterators, so if we're in the middle of a loop
	// iterating through, and an entry is inserted/deleted, the iterators are automatically updated, allowing the loop
	// to continue cleanly.
	// ====================================================================================================================
	class CHashTableIterator
	{
		public:
            CHashTableIterator(CHashTable* hash_table, CHashTableIterator** head)
			{
                m_hashTable = hash_table;
				m_objectID = 0;
				m_currentEntry = nullptr;
				m_entryWasRemoved = false;

				// -- link to the iterator list'
				m_head = head;
				m_next = *m_head;
                if (m_next != nullptr)
                    m_next->m_prev = this;
				m_prev = nullptr;
				*m_head = this;
			}

			~CHashTableIterator()
			{
				// -- unlink from the iterator list
				if (m_next != nullptr)
					m_next->m_prev = m_prev;
				if (m_prev != nullptr)
					m_prev->m_next = m_next;
				else
					*m_head = m_next;
			}

            // -- interface to access the hashtable, using this specific iterator
            T* First()
            {
                return (m_hashTable->First(*this, nullptr));
            }

            T* Next()
            {
                return (m_hashTable->Next(*this, nullptr));
            }

            T* Prev()
            {
                return (m_hashTable->Prev(*this, nullptr));
            }

            T* Last()
            {
                return (m_hashTable->Last(*this, nullptr));
            }

            T* Current()
            {
                return (m_hashTable->Current(*this, nullptr));
            }

            // -- members for the iterator content
            CHashTable* m_hashTable;
            uint32 m_objectID;
            CHashTableEntry* m_currentEntry;
            bool8 m_entryWasRemoved;

            // -- members for maintaining the double linked list
			CHashTableIterator** m_head;
			CHashTableIterator* m_next;
			CHashTableIterator* m_prev;

        private:
            // -- default constructor invalid
            CHashTableIterator() { }
	};

	// -- constructor / destructor
	CHashTable(int32 _size = 1)
    {
		size = _size;
		table = TinAllocArray(ALLOC_HashTable, CHashTableEntry*, size);
		index_table = TinAllocArray(ALLOC_HashTable, CHashTableEntry*, size);
		for (int32 i = 0; i < size; ++i)
        {
			table[i] = NULL;
			index_table[i] = NULL;
        }

        used = 0;

		// -- we always have one default iterator, create it, which will automatically set the iterator list 
		m_iteratorList = nullptr;
		m_defaultIterator = TinAlloc(ALLOC_HashTable, CHashTableIterator, this, &m_iteratorList);
	}

	virtual ~CHashTable()
    {
		// -- destroy all iterators
		while (m_iteratorList != nullptr)
		{
            // -- see if the iterator belongs to a CGroupIterator object
            // note:  the iterator->m_objectID is not the group ID, but the CGroupIterator object owning the iterator
			CHashTableIterator* iterator = m_iteratorList;
			CObjectEntry* object_entry = TinScript::GetContext() != nullptr
                                         ? TinScript::GetContext()->FindObjectEntry(iterator->m_objectID)
                                         : nullptr;

            // -- if the iterator has an objectID that's valid, we destroy the object
            if (object_entry != nullptr)
            {
                TinScript::GetContext()->DestroyObject(object_entry->GetID());
            }

            // -- otherwise, destroy the iterator directly
            else
            {
                TinFree(m_iteratorList);
            }
		}

        // -- delete the hash table entries
		for (int32 i = 0; i < size; ++i)
        {
			CHashTableEntry* entry = table[i];
			while (entry)
            {
				CHashTableEntry* nextentry = entry->nextbucket;
				TinFree(entry);
				entry = nextentry;
			}
		}

        // -- free both the storage table, and the table that tracked insertion order
        TinFreeArray(table);
        TinFreeArray(index_table);
	}

    CHashTableIterator* CreateIterator()
    {
        // -- create the iterator
        CHashTableIterator* newIterator = TinAlloc(ALLOC_HashTable, CHashTableIterator, this, &m_iteratorList);
        return (newIterator);
    }

    void DestroyIterator(uint32 objectID)
    {
        // -- destroy the iterator owned by the given objectID
        CHashTableIterator* found = m_iteratorList;
        while (found != nullptr && found->m_objectID != objectID)
            found = found->m_next;

        // -- if we found the iterator, delete it
        if (found != nullptr)
            TinFree(found);
    }

    void ResetIterators()
    {
        CHashTableIterator* iter = m_iteratorList;
        while (iter != nullptr)
        {
            iter->m_currentEntry = nullptr;
            iter->m_entryWasRemoved = false;

            // -- next iterator in the list
            iter = iter->m_next;
        }
    }

    void UpdateIteratorsDeletedEntry(CHashTableEntry* deleted_entry)
    {
        CHashTableIterator* iter = m_iteratorList;
        while (iter != nullptr)
        {
            if (deleted_entry == iter->m_currentEntry)
            {
                CHashTableEntry* prev_hte = NULL;
                iter->m_currentEntry = FindRawEntryByIndex(deleted_entry->index + 1, prev_hte);
                iter->m_entryWasRemoved = true;
            }

            // -- next iterator in the list
            iter = iter->m_next;
        }
    }

	void AddItem(T& _item, uint32 _hash)
	{
		// -- if the object is already contained within, we're done
		if (Contains(_item, _hash))
		{
			return;
		}

		CHashTableEntry* hte = TinAlloc(ALLOC_HashTable, CHashTableEntry, _item, _hash);
		int32 bucket = _hash % size;
		hte->nextbucket = table[bucket];
		table[bucket] = hte;

        // -- do we need to modify the iterators in any way?
        //ResetIterators();

        ++used;

        // -- add to the index table
        if (used == 1)
        {
            // -- add to the index table
            hte->index = 0;
            index_table[0] = hte;
        }
        else
        {
            // -- add to the index table (note:  used has already been incremented)
            hte->index = used - 1;
            hte->index_next = index_table[hte->index % size];
            index_table[hte->index % size] = hte;
        }
	}

	void InsertItem(T& _item, uint32 _hash, int32 _index)
	{
        // -- if the index is anywhere past the end, simply add
        if (_index >= used)
        {
            AddItem(_item, _hash);
            return;
        }

        // -- ensure the index is valid
        if (_index < 0)
            _index = 0;

		// -- ensure the item isn't already at the index
		if (FindItemByIndex(_index) == &_item)
			return;

		// -- remove the item, to ensure no duplicates
		// note:  this can probably screw up iterators, if a person were to write a loop
		// that keeps re-inserting the same item, while iterating through the list...
		RemoveItem(&_item, _hash);

        // -- create the entry, add it to the table as per the hash, and clear the iterators
		CHashTableEntry* hte = TinAlloc(ALLOC_HashTable, CHashTableEntry, _item, _hash);
		int32 bucket = _hash % size;
		hte->nextbucket = table[bucket];
		table[bucket] = hte;

        // -- loop through all iterators, and update their current entry and flag members
        ResetIterators();

        // -- we need to insert it into the double-linked list before the entry currently at the given index
        CHashTableEntry* prev_hte = NULL;
        CHashTableEntry* cur_entry = FindRawEntryByIndex(_index, prev_hte);
        assert(cur_entry);

        // -- insert it into the index table - note, we need to bump up the indices of all entries after this
        // -- also note used has not yet been incremented
        // -- update all entries after cur_entry, by decrementing and updating the index table
        for (int32 bump_index = used - 1; bump_index >= _index; --bump_index)
        {
            prev_hte = NULL;
            CHashTableEntry* bump_hte = FindRawEntryByIndex(bump_index, prev_hte);

            // -- remove the hte from the linked list in the index_table bucket
            assert(bump_hte != 0);
            if (prev_hte)
                prev_hte->index_next = bump_hte->index_next;
            else
                index_table[bump_index % size] = bump_hte->index_next;

            // -- increment the index and add it to the previous index bucket
            ++bump_hte->index;
            bump_hte->index_next = index_table[bump_hte->index % size];
            index_table[bump_hte->index % size] = bump_hte;
        }

        // -- now add ourself into the index table
        hte->index = _index;
        hte->index_next = index_table[_index % size];
        index_table[_index % size] = hte;

        // -- finally, increment the used count
        ++used;
	}

	bool Contains(T& _item, uint32 _hash)
	{
		int32 bucket = _hash % size;
		CHashTableEntry* hte = table[bucket];
		while (hte)
		{
			if (hte->hash == _hash && hte->item == &_item)
				return (true);

			hte = hte->nextbucket;
		}

		// -- not found
		return false;
	}

	T* FindItem(uint32 _hash) const
    {
		int32 bucket = _hash % size;
		CHashTableEntry* hte = table[bucket];
		while (hte)
        {
			if (hte->hash == _hash)
				return hte->item;

			hte = hte->nextbucket;
		}

		// -- not found
		return (NULL);
	}

    // -- if we have multiple items with the same hash, and we're looking for a specific item
    // (identifiable internally, beyond the hash), we want a way to iterate through everything in the bucket
    T* FindNextItem(const T* _current, uint32 _hash) const
    {
        // -- no current item specified - simply return the normal result
        if (_current == nullptr)
            return FindItem(_hash);

        bool found = false;
        int32 bucket = _hash % size;
        CHashTableEntry* hte = table[bucket];
        while (hte)
        {
            if (hte->hash == _hash)
            {
                if (!found)
                    found = (hte->item == _current);
                else
                    return hte->item;
            }

            hte = hte->nextbucket;
        }
        
        // -- not found
        return (nullptr);
    }

    T* FindItemByIndex(int32 _index) const
    {
        if (_index < 0 || _index >= used)
            return (NULL);

        CHashTableEntry* hte = index_table[_index % size];
        while (hte && hte->index != _index)
            hte = hte->index_next;

        return (hte ? hte->item : NULL);
    }

    void RemoveItemByIndex(int32 _index)
    {
        CHashTableEntry* hte = FindRawEntryByIndex(_index);
        if (hte)
        {
            RemoveItem(hte->item, hte->hash);
        }
    }

    CHashTableEntry* FindRawEntryByIndex(int32 _index, CHashTableEntry*& prev_entry) const
    {
        prev_entry = NULL;
        if (_index < 0 || _index >= used)
            return (NULL);

        CHashTableEntry* hte = index_table[_index % size];
        while (hte && hte->index != _index)
        {
            prev_entry = hte;
            hte = hte->index_next;
        }

        return (hte);
    }

    void RemoveRawEntryFromIndexTable(CHashTableEntry* cur_entry)
    {
        // -- remove the current entry from the index table (first)
        CHashTableEntry* prev_hte = NULL;
        CHashTableEntry* hte = FindRawEntryByIndex(cur_entry->index, prev_hte);

        assert(hte == cur_entry);
        if (prev_hte)
            prev_hte->index_next = hte->index_next;
        else
            index_table[cur_entry->index % size] = hte->index_next;

        // -- update all entries after cur_entry, by decrimenting and updating the index table
        for (int _index = cur_entry->index + 1; _index < used; ++_index)
        {
            prev_hte = NULL;
            hte = FindRawEntryByIndex(_index, prev_hte);

            // -- remove the hte from the linked list in the index_table bucket
            assert(hte != 0);
            if (prev_hte)
                prev_hte->index_next = hte->index_next;
            else
                index_table[_index % size] = hte->index_next;

            // -- decrement the index add it to the previous index bucket
            --hte->index;
            hte->index_next = index_table[hte->index % size];
            index_table[hte->index % size] = hte;
        }
    }

	void RemoveItem(uint32 _hash)
    {
		uint32 bucket = _hash % size;
		CHashTableEntry** prevptr = &table[bucket];
		CHashTableEntry* curentry = table[bucket];
		while (curentry)
        {
			if (curentry->hash == _hash)
            {
				*prevptr = curentry->nextbucket;

                // -- update the iterators
                UpdateIteratorsDeletedEntry(curentry);

                // -- remove the entry from the index table
                RemoveRawEntryFromIndexTable(curentry);

                // -- delete the entry, and decrement the count
				TinFree(curentry);
                --used;
				return;
			}
			else
            {
				prevptr = &curentry->nextbucket;
				curentry = curentry->nextbucket;
			}
		}
	}

	void RemoveItem(T* _item, uint32 _hash)
    {
        if (!_item)
            return;

		int32 bucket = _hash % size;
		CHashTableEntry** prevptr = &table[bucket];
		CHashTableEntry* curentry = table[bucket];
		while (curentry)
        {
			if (curentry->hash == _hash && curentry->item == _item)
            {
				*prevptr = curentry->nextbucket;

                // -- update the iterators
                UpdateIteratorsDeletedEntry(curentry);

                // -- remove the entry from the index table
                RemoveRawEntryFromIndexTable(curentry);

				TinFree(curentry);
                --used;
				return;
			}
			else
            {
				prevptr = &curentry->nextbucket;
				curentry = curentry->nextbucket;
			}
		}
	}

    T* First(CHashTableIterator& iterator, uint32* out_hash = nullptr) const
    {
        CHashTableEntry* prev_hte = NULL;
        iterator.m_currentEntry = FindRawEntryByIndex(0, prev_hte);
        iterator.m_entryWasRemoved = false;
        if (iterator.m_currentEntry)
        {
            // -- return the hash value, if requested
            if (out_hash)
                *out_hash = iterator.m_currentEntry->hash;
            return (iterator.m_currentEntry->item);
        }
        else
        {
            if (out_hash)
                *out_hash = 0;
            return (NULL);
        }
    }

    T* First(uint32* out_hash = NULL) const
    {
        return (First(*m_defaultIterator, out_hash));
    }

    T* Next(CHashTableIterator& iterator, uint32* out_hash = NULL) const
    {
		if (iterator.m_currentEntry && !iterator.m_entryWasRemoved)
        {
            CHashTableEntry* prev_hte = NULL;
			iterator.m_currentEntry =
				FindRawEntryByIndex(iterator.m_currentEntry->index + 1, prev_hte);
        }

		iterator.m_entryWasRemoved = false;
		if (iterator.m_currentEntry)
        {
            // -- return the hash value, if requested
            if (out_hash)
				*out_hash = iterator.m_currentEntry->hash;
			return (iterator.m_currentEntry->item);
        }
        else
        {
            if (out_hash)
                *out_hash = 0;
            return (NULL);
        }
    }

    T* Next(uint32* out_hash = NULL) const
    {
        return (Next(*m_defaultIterator, out_hash));
    }

    T* Prev(CHashTableIterator& iterator, uint32* out_hash = NULL) const
    {
        if (iterator.m_currentEntry && !iterator.m_entryWasRemoved)
        {
            CHashTableEntry* prev_hte = NULL;
            iterator.m_currentEntry =
                FindRawEntryByIndex(iterator.m_currentEntry->index - 1, prev_hte);
        }

        iterator.m_entryWasRemoved = false;
        if (iterator.m_currentEntry)
        {
            // -- return the hash value, if requested
            if (out_hash)
                *out_hash = iterator.m_currentEntry->hash;
            return (iterator.m_currentEntry->item);
        }
        else
        {
            if (out_hash)
                *out_hash = 0;
            return (NULL);
        }
    }

    T* Prev(uint32* out_hash = NULL) const
    {
        return (Prev(*m_defaultIterator, out_hash));
    }

    T* Last(CHashTableIterator& iterator, uint32* out_hash = NULL) const
    {
        if (used > 0)
        {
            CHashTableEntry* prev_hte = NULL;
			iterator.m_currentEntry = FindRawEntryByIndex(used - 1, prev_hte);
		}
        else
			iterator.m_currentEntry = NULL;

		iterator.m_entryWasRemoved = false;
		if (iterator.m_currentEntry)
        {
            // -- return the hash value, if requested
            if (out_hash)
				*out_hash = iterator.m_currentEntry->hash;
			return (iterator.m_currentEntry->item);
        }
        else
        {
            if (out_hash)
                *out_hash = 0;
            return (NULL);
        }
    }

    T* Last(uint32* out_hash = NULL) const
    {
        return (Last(*m_defaultIterator, out_hash));
    }

    T* Current(CHashTableIterator& iterator, uint32* out_hash = NULL) const
    {
        iterator.m_entryWasRemoved = false;
        if (iterator.m_currentEntry)
        {
            // -- return the hash value, if requested
            if (out_hash)
                *out_hash = iterator.m_currentEntry->hash;
            return (iterator.m_currentEntry->item);
        }
        else
        {
            if (out_hash)
                *out_hash = 0;
            return (NULL);
        }
    }

    T* Current(uint32* out_hash = NULL) const
    {
        return (Current(*m_defaultIterator, out_hash));
    }

    int32 Size() const
    {
		return size;
	}

    int32 Used() const
    {
        return used;
    }

    bool8 IsEmpty() const
    {
        return (used == 0);
    }

    void RemoveAll()
    {
        // -- reset any iterators
        ResetIterators();

		// -- delete all the entries, but do not delete the actual items
        while (used > 0)
        {
            CHashTableEntry* prev_entry = NULL;
            CHashTableEntry* entry = FindRawEntryByIndex(used - 1, prev_entry);
            RemoveItem(entry->item, entry->hash);
        }
    }

    // -- This method doesn't just remove all entries from the
    // -- hash table, but it deletes the actual items stored
    void DestroyAll()
    {
        // -- reset any iterators
        ResetIterators();
        while (used > 0)
        {
            CHashTableEntry* prev_entry = NULL;
            CHashTableEntry* entry = FindRawEntryByIndex(used - 1, prev_entry);
            T* object = entry->item;
            RemoveItem(object, entry->hash);
            TinFree(object);
        }
    }

	private:
		CHashTableEntry** table;
		CHashTableEntry** index_table;
		int32 size;
        int32 used;

		mutable CHashTableIterator* m_defaultIterator;
		mutable CHashTableIterator* m_iteratorList;
};

}  // TinScript

#endif // __TINHASH

// ====================================================================================================================
// EOF
// ====================================================================================================================
