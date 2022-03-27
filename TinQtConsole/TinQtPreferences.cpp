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
// TinQtPreferences.cpp
// ====================================================================================================================

// -- includes --------------------------------------------------------------------------------------------------------

#include "TinQtPreferences.h"

#include <QFile>

#include "TinScript.h"
#include "TinHash.h"
#include "TinRegistration.h"

// -- statics  / forward declarations ---------------------------------------------------------------------------------

TinPreferences* TinPreferences::gInstance = nullptr;
static const char* kPreferencesScriptFileName = "TinQtPrefs.ts";

// -- class TinPreferences --------------------------------------------------------------------------------------------

// ====================================================================================================================
// GetInstance():  singleton accessor
// ====================================================================================================================
TinPreferences* TinPreferences::GetInstance()
{
    if (gInstance == nullptr)
    {
        gInstance = new TinPreferences();
    }
    return gInstance;
}

// ====================================================================================================================
// constructor
// ====================================================================================================================
TinPreferences::TinPreferences()
{
    // -- create the hashtable of all preference values
    mPreferencesMap = new TinScript::CHashtable;
}

// ====================================================================================================================
// destructor
// ====================================================================================================================
TinPreferences::~TinPreferences()
{
    delete mPreferencesMap;
}

// ====================================================================================================================
// IsValidKey(): returns true of the key is valid (no excess whitespace, no internal string delineators, non empty)
// ====================================================================================================================
bool TinPreferences::IsValidKey(const char* key)
{
    // -- check empty
    if (key == nullptr || key[0] == '\0')
    {
        assert(false && "Error - IsValidKey() - empty key");
        return false;
    }

    // -- check whitespace
    size_t key_length = strlen(key);
    if (key[0] <= 0x20 || key[key_length - 1] <= 0x20)
    {
        assert(false && "Error - IsValidKey() - key must not have leading or trailing whitespace");
        return false;
    }

    if (strchr(key, '\"') != nullptr || strchr(key, '\'') || strchr(key, '`'))
    {
        assert(false && "Error - IsValidKey() - key must not have internal string delineators");
        return false;
    }

    // -- seems ok!
    return true;
}

// ====================================================================================================================
// Save():  We're going to save the preferences as an executable script
// ====================================================================================================================
bool TinPreferences::Save()
{
    QFile file(kPreferencesScriptFileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    // -- iterate through the preferences hashtable
    std::list<const char*> keys;
    if (!mPreferencesMap->GetKeys(keys))
        return false;

    // -- for each key in the preference map, create the command, that when executed, would set the preference
    // to the current value (during Load())
    for (const auto& it : keys)
    {
        const char* value = "";
        if (mPreferencesMap->GetValue(it, value))
        {
            char command[TinScript::kMaxTokenLength];
            sprintf_s(command, TinScript::kMaxTokenLength, "SetTinQtPreference(`%s`, `%s`);\n", it, value);

            // -- write the command out to the file
            file.write(command);
        }
    }

    // -- close the file and return
    file.close();
    return true;
}

// ====================================================================================================================
// Load():  Simply execute the preferences script
// ====================================================================================================================
bool TinPreferences::Load()
{
    TinScript::ExecScript(kPreferencesScriptFileName, true);
    return true;
}

// -- registered globals ----------------------------------------------------------------------------------------------

void SetTinQtPreference(const char* key, const char* value)
{
    TinPreferences* preferences = TinPreferences::GetInstance();
    preferences->SetValue(key, value);
}

REGISTER_FUNCTION(SetTinQtPreference, SetTinQtPreference);

// -- eof -------------------------------------------------------------------------------------------------------------
