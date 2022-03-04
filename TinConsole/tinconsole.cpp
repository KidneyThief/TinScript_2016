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

// ------------------------------------------------------------------------------------------------
// tinconsole.cpp - a simple shell used to demonstrate and develop the tinscript library
// ------------------------------------------------------------------------------------------------

//#include "stdafx.h"
#include <tchar.h>
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "windows.h"
#include "conio.h"

#include "TinScript.h"
#include "TinRegistration.h"

// -- external includes
#include "cmdshell.h"
#include "socket.h"

// ------------------------------------------------------------------------------------------------
// statics - mostly for the quick and dirty console implementation
static const uint32 gFramesPerSecond = 33;
static const uint32 gMSPerFrame = 1000 / gFramesPerSecond;
static uint32 gCurrentTime = 0;

// ------------------------------------------------------------------------------------------------
// quick and dirty console framework
// ------------------------------------------------------------------------------------------------
static CCmdShell* gCmdShell = NULL;
static bool8 gRunning = true;

void Quit() {
    gRunning = false;
}

static bool8 gPaused = false;
void Pause() {
    gPaused = true;
}
void UnPause() {
    gPaused = false;
}

REGISTER_FUNCTION(Quit, Quit);
REGISTER_FUNCTION(Pause, Pause);
REGISTER_FUNCTION(UnPause, UnPause);

uint32 GetCurrentSimTime() {
    return gCurrentTime;
}

float32 GetSimTime() {
    float32 curtime = (float32)(GetCurrentSimTime()) / 1000.0f;
    return curtime;
}
REGISTER_FUNCTION(GetSimTime, GetSimTime);

int32 _tmain(int32 argc, _TCHAR* argv[])
{
    // -- required to ensure registered functions from unittest.cpp are linked.
    REGISTER_FILE(unittest_cpp);
    REGISTER_FILE(mathutil_cpp);
    REGISTER_FILE(socket_cpp);
    REGISTER_FILE(tinhashtable_cpp);

    // -- initialize
    TinScript::CreateContext(CmdShellPrintf, CmdShellAssertHandler, true);

    // -- create a command shell
    gCmdShell = new CCmdShell();

    // -- create a socket, so we can allow a remote debugger to connect
    SocketManager::Initialize();

	// -- convert all the wide args into an array of const char*
	char argstring[kMaxArgs][kMaxArgLength];
	for (int32 i = 0; i < argc; ++i)
	{
		strcpy_s(argstring[i], argv[i]);
	}

	// -- info passed in via command line arguments
	const char* infilename = NULL;
	int32 argindex = 1;
	while (argindex < argc)
	{
		size_t arglength = 0;
		char currarg[kMaxArgLength] = { 0 };
		strcpy_s(currarg, argstring[argindex]);
		if (!_stricmp(currarg, "-f") || !_stricmp(currarg, "-file"))
        {
			if (argindex >= argc - 1)
            {
                TinPrint(TinScript::GetContext(), "Error - invalid arg '-f': no filename given\n");
				return 1;
			}
			else
            {
				infilename = argstring[argindex + 1];
				argindex += 2;
			}
		}
		else
        {
            TinPrint(TinScript::GetContext(), "Error - unknown arg: %s\n", argstring[argindex]);
			return 1;
		}
	}

	// -- parse the file
	if (infilename && infilename[0] && !TinScript::ExecScript(infilename))
    {
        TinPrint(TinScript::GetContext(), "Error - unable to parse file: %s\n", infilename);
		return 1;
	}

    while (gRunning)
    {
        // -- simulate a 33ms frametime
        // -- time needs to stand still while an assert is active
        Sleep(gMSPerFrame);
        if (!gPaused)
        {
            gCurrentTime += gMSPerFrame;
        }

        // -- update the cmd shell - see if we have a command to execute
        if (gCmdShell)
        {
            const char* command = gCmdShell->Update();
            if (command)
            {
                TinScript::ExecCommand(command);

                // -- once handled, refresh the prompt
                gCmdShell->RefreshConsoleInput(true, "");
            }
        }

        // -- keep the system running...
        TinScript::UpdateContext(gCurrentTime);
    }

    // -- cleanup
    SocketManager::Terminate();
    delete gCmdShell;
    gCmdShell = NULL;
    TinScript::DestroyContext();

	return 0;
}

// ------------------------------------------------------------------------------------------------
// eof
// ------------------------------------------------------------------------------------------------
