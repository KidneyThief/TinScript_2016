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
// TinScheduler.cpp
// ====================================================================================================================

// -- includes
#include "stdio.h"

#include "socket.h"

#include "TinScript.h"
#include "TinRegistration.h"
#include "TinInterface.h"
#include "TinExecute.h"
#include "TinScheduler.h"

// == namespace TinScript =============================================================================================

namespace TinScript
{

// == class CScheduler ================================================================================================

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CScheduler::CScheduler(CScriptContext* script_context)
{
    mContextOwner = script_context;
    mHead = NULL;
    mCurrentSimTime = 0;
    mCurrentSchedule = NULL;
    mSimTimeScale = 1.0f;
}

// ====================================================================================================================
// Destructor
// ====================================================================================================================
CScheduler::~CScheduler()
{
    // -- clean up all pending scheduled events
    while (mHead)
    {
        CCommand* next = mHead->mNext;
        TinFree(mHead);
        mHead = next;
    }
}

// ====================================================================================================================
// Update():  Iterates through the list of requests, executing those who's requested time has elapsed.
// ====================================================================================================================
void CScheduler::Update(uint32 curtime)
{
    // -- cache the current time
    mCurrentSimTime = curtime;

    // -- execute all commands scheduled for dispatch by this time
    while (mHead && mHead->mDispatchTime <= curtime)
    {
        // -- get the current command, and remove it from the list - now, before we execute,
        // -- since executing this command could 
        CCommand* curcommand = mHead;
        if (curcommand->mNext)
            curcommand->mNext->mPrev = NULL;
        mHead = curcommand->mNext;

        // -- notify the debugger
        DebuggerRemoveSchedule(curcommand->mReqID);

        // -- dispatch the command - see if it's a direct function call, or a command buf
        if (curcommand->mFuncHash != 0)
        {
            ExecuteScheduledFunction(GetScriptContext(), curcommand->mObjectID, 0, curcommand->mFuncHash,
                                        curcommand->mFuncContext);
        }
        else
        {
            if(curcommand->mObjectID > 0)
            {
                int32 dummy = 0;
                ObjExecF(curcommand->mObjectID, dummy, curcommand->mCommandBuf);
            }
            else
            {
                GetScriptContext()->ExecCommand(curcommand->mCommandBuf);
            }
        }

        // -- if the command is to be repeated, re-insert it back into the list
        if (curcommand->mRepeatTime > 0)
        {
            // -- first, update the dispatch time
            curcommand->mDispatchTime = mCurrentSimTime + curcommand->mRepeatTime;

            // -- insert the command back into the list
            InsertCommand(curcommand);

            // -- notify the debugger
            DebuggerAddSchedule(*curcommand);
        }
        else
        {
            // -- delete the command
            TinFree(curcommand);
        }
    }
}

// ====================================================================================================================
// SetSimTimeScale():  Allows the scheduler to communicate with the debugger for accurate reflection of schedules.
// ====================================================================================================================
void CScheduler::SetSimTimeScale(float time_scale)
{
    mSimTimeScale = time_scale;
    if (mSimTimeScale < 0.0f)
        mSimTimeScale = 0.0f;

    // -- if we're connected, notify the debugger
    int32 debugger_session = 0;
    if (GetScriptContext()->IsDebuggerConnected(debugger_session))
    {
        SocketManager::SendCommandf("DebuggerNotifyTimeScale(%f);", mSimTimeScale);
    }
}

// ====================================================================================================================
// CancelObject():  On destruction of an object, cancel all scheduled method calls.
// ====================================================================================================================
void CScheduler::CancelObject(uint32 objectid)
{
    if(objectid == 0)
        return;
    Cancel(objectid, 0);
}

// ====================================================================================================================
// CancelRequest():  Cancel a scheduled function/method call by ID
// ====================================================================================================================
void CScheduler::CancelRequest(int32 reqid)
{
    if(reqid <= 0)
        return;
    Cancel(0, reqid);
}

// ====================================================================================================================
// Cancel():  Cancel a scheduled method call by ID, but for a specific object
// ====================================================================================================================
void CScheduler::Cancel(uint32 objectid, int32 reqid)
{
    // -- loop through and delete any schedules pending for this object
    CCommand** prevcommand = &mHead;
    CCommand* curcommand = mHead;
    while (curcommand)
    {
        if (curcommand->mObjectID == objectid || curcommand->mReqID == reqid)
        {
            // -- notify the debugger
            DebuggerRemoveSchedule(curcommand->mReqID);

            *prevcommand = curcommand->mNext;
            TinFree(curcommand);
            curcommand = *prevcommand;
        }
        else
        {
            prevcommand = &curcommand->mNext;
            curcommand = curcommand->mNext;
        }
    }
}

// ====================================================================================================================
// Dump():  Display the list of scheduled requests through standard text.
// ====================================================================================================================
void CScheduler::Dump()
{
    // -- loop through and delete any schedules pending for this object
    CCommand* curcommand = mHead;
    while (curcommand)
    {
        if (curcommand->mFuncHash != 0)
        {
            TinPrint(GetScriptContext(), "ReqID: %d, ObjID: %d, Function: %s\n", curcommand->mReqID,
                     curcommand->mObjectID, UnHash(curcommand->mFuncHash));
        }
        else
        {
            TinPrint(GetScriptContext(), "ReqID: %d, ObjID: %d, Command: %s\n", curcommand->mReqID,
                     curcommand->mObjectID, curcommand->mCommandBuf);
        }
        curcommand = curcommand->mNext;
    }
}

// ====================================================================================================================
// DebuggerListSchedules():  Send the connected debugger a list of schedules
// ====================================================================================================================
void CScheduler::DebuggerListSchedules()
{
    // -- nothing to send if we're not connected
    int32 debugger_session = 0;
    if (!GetScriptContext()->IsDebuggerConnected(debugger_session))
        return;

    // -- this is a good time to notify the debugger of our current timescale, as it tends to be called "on connect"
    SocketManager::SendCommandf("DebuggerNotifyTimeScale(%f);", mSimTimeScale);

    // -- loop through and delete any schedules pending for this object
    CCommand* curcommand = mHead;
    while (curcommand)
    {
        DebuggerAddSchedule(*curcommand);
        curcommand = curcommand->mNext;
    }
}

// ====================================================================================================================
// DebuggerAddSchedule():  Send the connected debugger notification of a schedule.
// ====================================================================================================================
void CScheduler::DebuggerAddSchedule(const CCommand& command)
{
    // -- nothing to send if we're not connected
    int32 debugger_session = 0;
    if (!GetScriptContext()->IsDebuggerConnected(debugger_session))
        return;

    int32 time_remaining_ms = static_cast<int32>(command.mDispatchTime) - static_cast<int32>(mCurrentSimTime);
    if (time_remaining_ms < 0)
        time_remaining_ms = 0;

    char debug_msg[kMaxTokenLength];
    if (command.mFuncHash != 0)
    {
        sprintf_s(debug_msg, "DebuggerAddSchedule(%d, %s, %d, %d, `%s();`);", command.mReqID,
                  command.mRepeatTime > 0 ? "true" : "false", time_remaining_ms, command.mObjectID,
                  UnHash(command.mFuncHash));
    }
    else
    {
        sprintf_s(debug_msg, "DebuggerAddSchedule(%d, %s, %d, %d, `%s`);", command.mReqID,
                  command.mRepeatTime > 0 ? "true" : "false", time_remaining_ms, command.mObjectID,
                  command.mCommandBuf);
    }

    // -- send the command
    SocketManager::SendCommand(debug_msg);
}

// ====================================================================================================================
// DebuggerRemoveSchedule():  Send the connected debugger notification of a schedule.
// ====================================================================================================================
void CScheduler::DebuggerRemoveSchedule(int32 req_id)
{
    // -- nothing to send if we're not connected
    int32 debugger_session = 0;
    if (!GetScriptContext()->IsDebuggerConnected(debugger_session))
        return;

    // -- send the command
    SocketManager::SendCommandf("DebuggerRemoveSchedule(%d);", req_id);
}

// == class CScheduler::CCommand ======================================================================================

// ====================================================================================================================
// Constructor: Schedule a raw text statement, to be parsed and executed.
// ====================================================================================================================
CScheduler::CCommand::CCommand(CScriptContext* script_context, int32 _reqid, uint32 _objectid,
                               uint32 _dispatchtime, uint32 _repeat_time, const char* _command, bool8 immediate)
{
    // -- set the context
    mContextOwner = script_context;

    // --  members copy the command members
    mReqID = _reqid;
    mObjectID = _objectid;
    mDispatchTime = _dispatchtime;
    mRepeatTime = _repeat_time;
    mImmediateExec = immediate;
    SafeStrcpy(mCommandBuf, sizeof(mCommandBuf), _command, kMaxTokenLength);

    // -- command string, null out the direct function call members
    mFuncHash = 0;
    mFuncContext = NULL;
}

// ====================================================================================================================
// Constructor:  Schedule a specific function/method call - much more efficient than raw text.
// ====================================================================================================================
CScheduler::CCommand::CCommand(CScriptContext* script_context, int32 _reqid, uint32 _objectid,
                               uint32 _dispatchtime, uint32 _repeat_time, uint32 _funchash, bool8 immediate)
{
    // -- set the context
    mContextOwner = script_context;

    // --  members copy the command members
    mReqID = _reqid;
    mObjectID = _objectid;
    mDispatchTime = _dispatchtime;
    mRepeatTime = _repeat_time;
    mImmediateExec = immediate;
    mCommandBuf[0] = '\0';

    // -- command string, null out the direct function call members
    mFuncHash = _funchash;
    mFuncContext = TinAlloc(ALLOC_FuncContext, CFunctionContext);
}

// ====================================================================================================================
// Destructor
// ====================================================================================================================
CScheduler::CCommand::~CCommand()
{
    // clean up the function context, if it exists
    if (mFuncContext)
        TinFree(mFuncContext);
}

// ====================================================================================================================
// Schedule():  Schedule a raw text command.
// ====================================================================================================================
static int32 gScheduleID = 0;
int32 CScheduler::Schedule(uint32 objectid, int32 delay, bool8 repeat, const char* commandstring)
{
    ++gScheduleID;

    // -- ensure we have a valid command string
    if(!commandstring || !commandstring[0])
        return 0;

    // -- calculate the dispatch time - enforce a one-frame delay
    uint32 delay_time = (delay > 0 ? delay : 1);
    uint32 dispatchtime = mCurrentSimTime + delay_time;
    uint32 repeat_time = repeat ? delay_time : 0;

    // -- create the new commmand
    CCommand* newcommand = TinAlloc(ALLOC_SchedCmd, CCommand, GetScriptContext(), gScheduleID,
                                    objectid, dispatchtime, repeat_time, commandstring);

    // -- insert the command into the list
    InsertCommand(newcommand);

    // -- notify the debugger
    DebuggerAddSchedule(*newcommand);

    // -- return the request id, so we have a way to cancel
    return newcommand->mReqID;
}

// ====================================================================================================================
// InsertCommand():  Insert the command into the list, by dispatch time.
// ====================================================================================================================
void CScheduler::InsertCommand(CCommand* newcommand)
{
    // -- sanity check
    if (!newcommand)
        return;

    // -- see if it goes at the front of the list
    if (!mHead || newcommand->mDispatchTime < mHead->mDispatchTime)
    {
        newcommand->mNext = mHead;
        newcommand->mPrev = NULL;
        if (mHead)
            mHead->mPrev = newcommand;
        mHead = newcommand;
    }
    else
    {
        // -- insert it into the list, in after curschedule
        // note:  if the dispatch times are the same, it goes *after*, so we preserve the insertion order
        CCommand* curschedule = mHead;
        while (curschedule->mNext && curschedule->mNext->mDispatchTime <= newcommand->mDispatchTime)
            curschedule = curschedule->mNext;
        newcommand->mNext = curschedule->mNext;
        newcommand->mPrev = curschedule;
        if (curschedule->mNext)
            curschedule->mNext->mPrev = newcommand;
        curschedule->mNext = newcommand;
    }
}

// ====================================================================================================================
// ScheduleCreate():  Create a schedule request.
// ====================================================================================================================
CScheduler::CCommand* CScheduler::ScheduleCreate(uint32 objectid, int32 delay, uint32 funchash, bool8 immediate,
                                                 bool8 repeat)
{
    ++gScheduleID;

    // -- calculate the dispatch time - enforce a one-frame delay
    uint32 delay_time = (delay > 0 ? delay : 1);
    uint32 dispatchtime = mCurrentSimTime + delay_time;
    uint32 repeat_time = repeat ? delay_time : 0;

    // -- create the new commmand
    CCommand* newcommand = TinAlloc(ALLOC_SchedCmd, CCommand, GetScriptContext(), gScheduleID,
                                    objectid, dispatchtime, repeat_time, funchash, immediate);

    // -- add space to store a return value
    newcommand->mFuncContext->AddParameter("__return", Hash("__return"), TYPE__resolve, 1, 0);

    // -- insert the command into the list
    InsertCommand(newcommand);

    // -- notify the debugger
    DebuggerAddSchedule(*newcommand);

    // -- return the actual commmand object, since we'll be updating the parameter values
    return newcommand;
}

// ====================================================================================================================
// RemoteScheduleCreate():  Called from the socket thread, to queue up schedules until the main thread can process.
// ====================================================================================================================
CScheduler::CCommand* CScheduler::RemoteScheduleCreate(uint32 funchash)
{
    // -- create the new commmand
    CCommand* new_command = TinAlloc(ALLOC_SchedCmd, CCommand, GetScriptContext(), -1, 0, 0, 0, funchash, true);

    // -- add space to store a return value
    new_command->mFuncContext->AddParameter("__return", Hash("__return"), TYPE__resolve, 1, 0);

    // -- return the command
    return (new_command);
}

} // TinScript

// ====================================================================================================================
// EOF
// ====================================================================================================================
