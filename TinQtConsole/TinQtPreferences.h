// ------------------------------------------------------------------------------------------------
//  The MIT License
//  
//  Copyright (c) 2022 Tim Andersen
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
// TinQtPreferences.h
// ====================================================================================================================

#pragma once

// -- includes --------------------------------------------------------------------------------------------------------

#include "TinQTConsole.h"

#include "TinHashtable.h"
#include "TinScript.h"

// -- class TinPreferences --------------------------------------------------------------------------------------------

class TinPreferences
{
    public:
        TinPreferences();
        ~TinPreferences();

        static TinPreferences* GetInstance();

        // save preferences
        bool SavePreferences();
        bool LoadPreferences();

        bool SaveBreakpoints();
        bool LoadBreakpoints();

        // -- GetValue() is templated, so *if* there's no preference for the given key yet,
        // we can create a new one, and both the type and default value are already specified
        template <typename T>
        T GetValue(const char* key, const T& default_value);

        template <typename T>
        bool SetValue(const char* key, const T& new_value);

        const TinScript::CHashtable* GetPreferencesMap() const { return mPreferencesMap; }

    private:
        bool IsValidKey(const char* key);

        static TinPreferences* gInstance;
        TinScript::CHashtable* mPreferencesMap;
};

template <typename T>
inline T TinPreferences::GetValue(const char* key, const T& default_value)
{
    // -- ensure the key is valid
    if (!IsValidKey(key))
    {
        assert(false && "TinPreferences::GetValue() invalid key");
        return {};
    }

    // -- if we already have a key, try to return it as a 'T'
    if (mPreferencesMap->HasKey(key))
    {
        T value;
        if (!mPreferencesMap->GetValue<T>(key, value))
        {
            assert(false && "TinPreferences::GetValue() key already exists, invalid type");
            return default_value;
        }
        else
        {
            return value;
        }
    }
    else
    {
        if (!mPreferencesMap->AddEntry(key, default_value))
        {
            assert(false && "TinPreferences::GetValue() failed to add default value");
        }

        // -- either way, the default value is all we have available
        return default_value;
    }
}

template <typename T>
inline bool TinPreferences::SetValue(const char* key, const T& new_value)
{
    // -- ensure the key is valid
    if (!IsValidKey(key))
    {
        assert(false && "TinPreferences::SetValue() invalid key");
        return {};
    }

    // -- whether the key exists or not, the hashtable uses AddEntry() as its api
    bool exists = mPreferencesMap->HasKey(key);
    if (!mPreferencesMap->AddEntry(key, new_value))
    {
        if (exists)
        {
            assert(false && "TinPreferences::SetValue() - key exists, unable to set new value");
        }
        else
        {
            assert(false && "TinPreferences::SetValue() - unable to set new value");
        }
        return false;
    }

    // -- save preferences on any change
    SavePreferences();

    // -- success
    return true;
};

// -- eof -------------------------------------------------------------------------------------------------------------
