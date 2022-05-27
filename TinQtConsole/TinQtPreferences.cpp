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
#include "TinQtConsole.h"
#include "TinQTBreakpointsWin.h"

#include <QFile>

#include <TinScript.h>
#include <TinHash.h>
#include <TinRegBinding.h>

// -- statics  / forward declarations ---------------------------------------------------------------------------------

TinPreferences* TinPreferences::gInstance = nullptr;
static const char* kPreferencesScriptFileName = "TinQtPrefs.ts";
static const char* kBreakpointsScriptFileName = "TinQtBreakpoints.ts";

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
// SavePreferences():  We're going to save the preferences as an executable script
// ====================================================================================================================
bool TinPreferences::SavePreferences()
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
            char command[kMaxTokenLength];
            snprintf(command, sizeof(command), "SetTinQtPreference(`%s`, `%s`);\n", it, value);

            // -- write the command out to the file
            file.write(command);
        }
    }

    // -- close the preferences file and return
    file.close();
    return true;
}

// ====================================================================================================================
// SavePreferences():  We're going to save the breakpoints as an executable script
// ====================================================================================================================
bool TinPreferences::SaveBreakpoints()
{
    QFile breakpoints_file(kBreakpointsScriptFileName);
    if (!breakpoints_file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    // -- we also want to preserve breakpoints
    const QList<CBreakpointEntry*>& breakpoints =
        CConsoleWindow::GetInstance()->GetDebugBreakpointsWin()->GetBreakpointEntries();

    // -- we want to save all breakpoints (active or not) to the preferences file, to be reloaded on restart
    // note:  this is very similar to NotifyOnConnect()
    for (int32 i = 0; i < breakpoints.size(); ++i)
    {
        const CBreakpointEntry* breakpoint = breakpoints.at(i);

        // -- we're not preserving data breakpoints (obviously)
        if (breakpoint->mWatchRequestID != 0)
        {
            continue;
        }

        // -- create the command to restore the breakpoint
        char command[kMaxTokenLength];
        snprintf(command, sizeof(command),
                  "SetTinQtBreakpoint(`%s`, %d, %s, `%s`, %s, `%s`, %s, %s);\n",
                  TinScript::UnHash(breakpoint->mCodeblockHash), breakpoint->mLineNumber,
                  (breakpoint->mChecked ? "true" : "false"),
                  &breakpoint->mCondition[0], (breakpoint->mConditionEnabled ? "true" : "false"),
                  &breakpoint->mTracePoint[0], (breakpoint->mTraceEnabled ? "true" : "false"),
                  (breakpoint->mTraceOnCondition ? "true" : "false"));

        // -- write the command out to the file
        breakpoints_file.write(command);
    }

    // -- close the file
    breakpoints_file.close();

    // -- success
    return true;
}

// ====================================================================================================================
// LoadPreferences():  Simply execute the script
// ====================================================================================================================
bool TinPreferences::LoadPreferences()
{
    TinScript::ExecScript(kPreferencesScriptFileName, true);
    return true;
}

// ====================================================================================================================
// LoadBreakpoints():  Simply execute the script
// ====================================================================================================================
bool TinPreferences::LoadBreakpoints()
{
    TinScript::ExecScript(kBreakpointsScriptFileName, true);
    return true;
}

// -- registered globals ----------------------------------------------------------------------------------------------

void SetTinQtPreference(const char* key, const char* value)
{
    TinPreferences* preferences = TinPreferences::GetInstance();
    preferences->SetValue(key, value);
}

void SetTinQtBreakpoint(const char* filepath, int32 line_number, bool enabled, const char* condition,
                        bool condition_enabled, const char* trace, bool trace_enabled, bool trace_on_condition)
{
    if (filepath == nullptr || filepath[0] == '\0')
        return;

    // -- notify the Breakpoints Window - add the new breakpoint, but restore the "enabled" state
    CConsoleWindow::GetInstance()->GetDebugBreakpointsWin()->ToggleBreakpoint(TinScript::Hash(filepath), line_number,
                                                                              true, enabled);
    
    // -- set the condition
    if (condition != nullptr && condition[0] != '\0')
    {
        CConsoleWindow::GetInstance()->GetDebugBreakpointsWin()->SetBreakCondition(condition, condition_enabled);
    }

    // -- set the tracepoint
    if (trace != nullptr && trace[0] != '\0')
    {
        CConsoleWindow::GetInstance()->GetDebugBreakpointsWin()->SetTraceExpression(trace, trace_enabled,
                                                                                    trace_on_condition);
    }
}

REGISTER_FUNCTION(SetTinQtPreference, SetTinQtPreference);
REGISTER_FUNCTION(SetTinQtBreakpoint, SetTinQtBreakpoint);

// -- eof -------------------------------------------------------------------------------------------------------------
