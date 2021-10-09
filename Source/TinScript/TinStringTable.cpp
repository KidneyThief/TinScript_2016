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
// TinStringTable.cpp
// ====================================================================================================================

// -- includes
#include "string.h"

// -- TinScript includes
#include "TinScript.h"
#include "TinRegistration.h"
#include "TinStringTable.h"

// == namespace TinScript =============================================================================================

namespace TinScript
{

// == CStringTable ====================================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CStringTable::CStringTable(CScriptContext* owner, uint32 _size)
{
    mContextOwner = owner;

    assert(_size > 0);
    mSize = _size;
    mBuffer = TinAllocArray(ALLOC_StringTable, char, mSize);
    mBufptr = mBuffer;

    mStringDictionary = TinAlloc(ALLOC_StringTable, CHashTable<tStringEntry>, kStringTableDictionarySize);
}

// ====================================================================================================================
// Destructor
// ====================================================================================================================
CStringTable::~CStringTable()
{
    // -- destroy the hash table entries
    mStringDictionary->DestroyAll();

    // -- destroy the table and buffer
    TinFree(mStringDictionary);
    TinFree(mBuffer);
}

// ====================================================================================================================
// AddString():  Add a string to the dictionary, potentially with an initial ref count
// ====================================================================================================================
const char* CStringTable::AddString(const char* s, int length, uint32 hash, bool inc_refcount)
{
    // -- sanity check
    if (!s)
        return "";

    if (hash == 0)
        hash = Hash(s, length);

    // -- see if the string is already in the dictionary
    const char* exists = FindString(hash);
    if (!exists)
    {
        if (length < 0)
            length = (int32)strlen(s);

        // -- space left
        int32 remaining = int32(mSize - (kPointerToUInt32(mBufptr) -
                                            kPointerToUInt32(mBuffer)));
        if (remaining < length + 1)
        {
            ScriptAssert_(mContextOwner, 0,
                            "<internal>", -1, "Error - StringTable of size %d is full!\n",
                            mSize);
            return NULL;
        }
        const char* stringptr = mBufptr;
        SafeStrcpy(mBufptr, remaining, s, length + 1);
        mBufptr += length + 1;

        // -- create the string table entry
        tStringEntry* new_entry = TinAlloc(ALLOC_StringTable, tStringEntry, stringptr);

        // -- add the entry to the dictionary
        mStringDictionary->AddItem(*new_entry, hash);

        // -- if this item is meant to persist, increment the ref count
        if (inc_refcount)
            new_entry->mRefCount++;

        return (stringptr);
    }

    // -- else check for a collision
    else
    {
        if (length < 0)
            length = (int32)strlen(s);

        if (strncmp(exists, s, length) != 0)
        {
            ScriptAssert_(mContextOwner, 0, "<internal>", -1,
                          "Error - Hash collision [0x%x]: '%s', '%s'\n", hash, exists, s);
        }

        // -- if this item is meant to persist, increment the ref count
        if (inc_refcount)
            RefCountIncrement(hash);

        return exists;
    }
}

// ====================================================================================================================
// FindString():  Finds a string entry in the dictionary - returns the actual const char*
// ====================================================================================================================
const char* CStringTable::FindString(uint32 hash)
{
    // -- sanity check
    if (hash == 0)
        return ("");

    tStringEntry* ste = mStringDictionary->FindItem(hash);
    return (ste ? ste->mString : NULL);
}

// ====================================================================================================================
// RefCountIncrement(): called when the string is assigned, and (obviously) being referenced
// ====================================================================================================================
void CStringTable::RefCountIncrement(uint32 hash)
{
    // -- sanity check
    if (hash == 0)
        return;

    tStringEntry* ste = mStringDictionary->FindItem(hash);
    if (ste)
        ste->mRefCount++;
}

// ====================================================================================================================
// RefCountDecrement():  called when a variable is no longer refering to this string
// ====================================================================================================================
void CStringTable::RefCountDecrement(uint32 hash)
{
    // -- sanity check
    if (hash == 0)
        return;

    tStringEntry* ste = mStringDictionary->FindItem(hash);
    if (ste)
        ste->mRefCount--;
}

// ====================================================================================================================
// RemoveUnreferencedStrings():  All strings at the *end* of the dictionary, to which there are no variables
// assigned are removed.
// $$$TZA Convert this to an actual string handle system, allowing all unreferenced strings to be removed
// and the buffer fragmentation to be reduced
// ====================================================================================================================
void CStringTable::RemoveUnreferencedStrings()
{
    // -- loop back from the end of the linked list, removing all entries
    // -- with a zero refcount
    uint32 last_hash = 0;
    tStringEntry* last_entry = mStringDictionary->Last(&last_hash);
    while (last_entry)
    {
        // -- if we hit an entry that is still being used (e.g. assigned to
        // -- a variable) we're done
        if (last_entry->mRefCount > 0)
            break;

        // -- remove the entry from the dictionary
        tStringEntry* delete_me = last_entry;
        mStringDictionary->RemoveItem(last_hash);

        // -- back up the buf ptr, to store the next string
        mBufptr = const_cast<char*>(delete_me->mString);

        // -- delete the entry
        TinFree(delete_me);

        // -- next entry
        last_entry = mStringDictionary->Last(&last_hash);
    }
}

} // TinScript

// == Script Registration =============================================================================================

// ====================================================================================================================
// StringLen():  registered version of strlen()
// ====================================================================================================================
int32 StringLen(const char* string)
{
    // -- sanity check
    if (!string)
        return (0);

    int32 len = (int32)strlen(string);
    return (len);
}

REGISTER_FUNCTION(StringLen, StringLen);

// ====================================================================================================================
// StringCat():  multi-arg implementation of strcat
// ====================================================================================================================
const char* StringCat(const char* str0, const char* str1, const char* str2, const char* str3, const char* str4,
                      const char* str5, const char* str6, const char* str7)
{
    // -- ensure we have at least two strings
    if (!str0 || !str1 || !str1[0])
        return (str0);

    // -- concatenate the arguments
    // -- stop as soon as we have an empty string
    char buf[2048];
    if (!str2 || !str2[0])
        sprintf_s(buf, "%s%s", str0, str1);
    else if (!str3 || !str3[0])
        sprintf_s(buf, "%s%s%s", str0, str1, str2);
    else if (!str4 || !str4[0])
        sprintf_s(buf, "%s%s%s%s", str0, str1, str2, str3);
    else if (!str5 || !str5[0])
        sprintf_s(buf, "%s%s%s%s%s", str0, str1, str2, str3, str4);
    else if (!str6 || !str6[0])
        sprintf_s(buf, "%s%s%s%s%s%s", str0, str1, str2, str3, str4, str5);
    else if (!str7 || !str7[0])
        sprintf_s(buf, "%s%s%s%s%s%s%s", str0, str1, str2, str3, str4, str5, str6);
    else
        sprintf_s(buf, "%s%s%s%s%s%s%s%s", str0, str1, str2, str3, str4, str5, str6, str7);

    // -- add the result to the string table
    TinScript::CScriptContext* script_context = ::TinScript::GetContext();
    const char* result = script_context->GetStringTable()->AddString(buf);

    // -- return the result
    return (result);
}

REGISTER_FUNCTION(StringCat, StringCat);

// ====================================================================================================================
// StringCmp():  Same as strcmp, case sensitive
// ====================================================================================================================
int32 StringCmp(const char* str0, const char* str1)
{
    // -- be design, a NULL string is the same as an ""
    if (!str0)
        str0 = "";
    if (!str1)
        str1 = "";

    return (strcmp(str0, str1));
}

REGISTER_FUNCTION(StringCmp, StringCmp);

// ====================================================================================================================
// IntToChar():  Converts from an int to a single character string
// ====================================================================================================================
const char* IntToChar(int32 ascii_value)
{
    // -- skip hidden characters
    if (ascii_value < 0x20 || ascii_value > 255)
        return ("");

    char* buffer = TinScript::GetContext()->GetScratchBuffer();
    buffer[0] = (char)ascii_value;
    buffer[1] = '\0';
    return (buffer);
}

// -- UE4 needs the param specific registration
// $$$TZA related to using a pointer as an arg?  (const char* is 64-bit)??
#if PLATFORM_UE4
    REGISTER_FUNCTION_P1(IntToChar, IntToChar, const char*, int32);
#else	
	REGISTER_FUNCTION(IntToChar, IntToChar);
#endif

// ====================================================================================================================
// CharToInt():  Converts from char to the ascii int value
// ====================================================================================================================
int32 CharToInt(const char* input_string)
{
    if (! input_string)
        return (0);
    else
        return ((int32)(input_string[0]));
}

REGISTER_FUNCTION(CharToInt, CharToInt);

// ====================================================================================================================
// Print():  script method to call the main thread print handler
// Concatenates the strings as a side effect
// ====================================================================================================================
const char* Print(const char* str0, const char* str1, const char* str2, const char* str3, const char* str4,
                  const char* str5, const char* str6, const char* str7)
{
    const char* str_concat = StringCat(str0, str1, str2, str3, str4, str5, str6, str7);
    if (!str_concat)
        return ("");

    TinScript::CScriptContext* script_context = ::TinScript::GetContext();

    // -- automatically add a '\n'
    TinPrint(script_context, "%s\n", str_concat);

    // -- return the concatenated string
    return (str_concat);
}

REGISTER_FUNCTION(Print, Print);

// ====================================================================================================================
// EOF
// ====================================================================================================================
