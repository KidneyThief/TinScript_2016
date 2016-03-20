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
// TinScheduler.h
// ====================================================================================================================

#ifndef __TINSCHEDULER_H
#define __TINSCHEDULER_H

// == namespace TinScript =============================================================================================

namespace TinScript
{

// --------------------------------------------------------------------------------------------------------------------
// -- forward declarations
class CFunctionContext;

// ====================================================================================================================
// class CScheduler:  Manages the requests for deferred function and method calls.
// ====================================================================================================================
class CScheduler
{
    public:

        CScheduler(CScriptContext* script_context = NULL);
        virtual ~CScheduler();

        CScriptContext* GetScriptContext() { return (mContextOwner); }

        // -- the update is what really matters, as it's expected to be called accurately by the application.
        // -- the set/get timescale is for communicating with the debugger, so it's schedule reflection is accurate
        void Update(uint32 curtime);
        float GetSimTimeScale() const { return (mSimTimeScale); }
        void SetSimTimeScale(float sim_time_scale);

        // ============================================================================================================
        // class CCommand: Stores the details of a a deferred function/method call request.
        // ============================================================================================================
        class CCommand
        {
            public:
                CCommand(CScriptContext* script_context, int32 _reqid, uint32 _objectid = 0,
                         uint32 _dispatchtime = 0, uint32 _repeat_time = 0, const char* _command = NULL,
                         bool8 immediate = false);
                CCommand(CScriptContext* script_context, int32 _reqid, uint32 _objectid,
                         uint32 _dispatchtime, uint32 _repeat_time, uint32 _funchash, bool8 immediate = false);

                virtual ~CCommand();

                CCommand* mPrev;
                CCommand* mNext;

                CScriptContext* mContextOwner;

                int32 mReqID;
                uint32 mObjectID;
                uint32 mDispatchTime;
                uint32 mRepeatTime;
                bool8 mImmediateExec;
                char mCommandBuf[kMaxTokenLength];

                uint32 mFuncHash;
                CFunctionContext* mFuncContext;
        };

        int32 Schedule(uint32 objectid, int32 delay, bool8 repeat, const char* commandstring);
        void CancelObject(uint32 objectid);
        void CancelRequest(int32 reqid);
        void Cancel(uint32 objectid, int32 reqid);
        void Dump();

        // -- debugger hook
        void DebuggerListSchedules();
        void DebuggerAddSchedule(const CCommand& command);
        void DebuggerRemoveSchedule(int32 request_id);

        void InsertCommand(CCommand* curcommand);
        CCommand* ScheduleCreate(uint32 objectid, int32 delay, uint32 funchash, bool8 immediate, bool8 repeat);
        CScheduler::CCommand* mCurrentSchedule;

    private:
        CScriptContext* mContextOwner;

        CCommand* mHead;
        uint32 mCurrentSimTime;
        float mSimTimeScale;
};

} // TinScript

#endif

// ====================================================================================================================
// EOF
// ====================================================================================================================
