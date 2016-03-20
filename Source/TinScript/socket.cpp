// ------------------------------------------------------------------------------------------------
//  The MIT License
//  
//  Copyright (c) 2014 Tim Andersen
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

// -- system includes
// -- sockets are only implemented in WIN32
#ifdef WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#endif

#include <vector>

// -- includes
#include "socket.h"

// -- includes required by any system wanting access to TinScript
#include "TinScript.h"
#include "TinRegistration.h"

// -- use the DECLARE_FILE/REGISTER_FILE macros to prevent deadstripping
DECLARE_FILE(socket_cpp);

// --------------------------------------------------------------------------------------------------------------------
// -- statics
const uint32 SCK_VERSION2 = 0x0202;

// == namespace SocketManager =========================================================================================

namespace SocketManager
{

// --------------------------------------------------------------------------------------------------------------------
// -- statics
#ifdef WIN32
    static bool mInitialized = false;
    static DWORD mThreadID = 0;
    static HANDLE mThreadHandle = NULL;
    static CSocket* mThreadSocket = NULL;
    static int mThreadSocketID = 1;

    // -- WSA variables
    bool mWSAInitialized = false;
    static WSADATA mWSAdata;
#endif // WIN32

// -- a custom data handler
static ProcessRecvDataCallback mRecvDataCallback = NULL;

// ====================================================================================================================
// Initialize():  Initialize the SocketManager
// ====================================================================================================================
void Initialize()
{
    #ifdef WIN32
        // -- create the thread
        mThreadHandle = CreateThread(NULL,                      // default security attributes
                                     0,                         // use default stack size  
                                     ThreadUpdate,              // thread function name
                                     TinScript::GetContext(),   // argument to thread function 
                                     0,                         // use default creation flags 
                                     &mThreadID);               // returns the thread identifier 
    #endif // WIN32
}

// ====================================================================================================================
// ThreadUpdate():  Update loop for the SocketManager, run inside the thread
// ====================================================================================================================
#ifdef WIN32
DWORD WINAPI ThreadUpdate(void* script_context)
{
    // -- see if we need to create the socket
    if (mThreadSocket == NULL)
    {
        mThreadSocket = new CSocket((TinScript::CScriptContext*)(script_context));

        // -- initialize the WSAData
        int error = WSAStartup(SCK_VERSION2, &mWSAdata);
        if (error)
        {
            // -- fail to start Winsock
            return (false);
        }

        // -- wrong Winsock version?
        if (mWSAdata.wVersion != SCK_VERSION2)
        {
            WSACleanup();
            return (false);
        }

        // -- we were successfully able to initialize WSA
        mWSAInitialized = true;
    }

    // -- update the socket until the thread is terminated
    while (true)
    {
        // -- listen for connections (internally, will actually listen, or simply ignore)
        if (!mThreadSocket->Listen())
        {
            return (0);
        }

        // -- update the socket (send/recv)
        if (!mThreadSocket->Update())
        {
            return (0);
        }

        Sleep(k_ThreadUpdateTimeMS);
    }

    // -- no errors
    return (0);
}
#endif // WIN32

// ====================================================================================================================
// Termintate():  Perform all shutdown and cleanup of the SocketManager
// ====================================================================================================================
void Terminate()
{
    #ifdef WIN32
        // -- first disconnect the socket
        Disconnect();

        // -- kill the thread
        TerminateThread(mThreadHandle, 0);

        // -- kill the socket
        if (mThreadSocket)
        {
            delete mThreadSocket;
            mThreadSocket = NULL;
        }

        // -- see if we need to shutdown WSA
        if (mWSAInitialized)
        {
            WSACleanup();
            mWSAInitialized = false;
        }
    #endif // WIN32
}

// ====================================================================================================================
// Listen(): Set the connection to listen for incoming connect requests
// ====================================================================================================================
bool Listen()
{
    #ifdef WIN32
        // -- see if we can enable listening
        if (mThreadSocket && !mThreadSocket->IsConnected())
        {
            mThreadSocket->SetListen(true);
            return (true);
        }
    #endif // WIN32

    // -- unable to listen for new connections
    return (false);
}

// ====================================================================================================================
// Connect():  send a winsock connection request
// ====================================================================================================================
bool Connect(const char* ipAddress)
{
    bool result = false;

    #ifdef WIN32
        if (!mThreadSocket)
        {
            TinPrint(TinScript::GetContext(), "Error - Connect(): SocketManager has not been initialized.\n");
        }
        else if (mThreadSocket->GetListen())
        {
            TinPrint(TinScript::GetContext(), "Error - Connect(): SocketManager is set to listen.\n");
            return (false);
        }

        // -- default address is loopback
        if (!ipAddress || !ipAddress[0])
            ipAddress = "127.0.0.1";

        result = mThreadSocket->Connect(ipAddress);
        if (!result)
        {
            TinPrint(TinScript::GetContext(),
                     "Error - Connect(): unable to connect - execute SocketListen() on target IP.\n");
        }
    #endif // WIN32

    return (result);
}

// ====================================================================================================================
// IsConnected():  returns if we have a valid winsock connected
// ====================================================================================================================
bool IsConnected()
{
    bool result = false;

    #ifdef WIN32
        // -- sanity check
        if (!mThreadSocket)
            return (false);

        result = mThreadSocket->IsConnected();
    #endif // WIN32

    // -- return the result
    return (result);
}

// ====================================================================================================================
// Disconnect():  disconnect a winsock connection
// ====================================================================================================================
void Disconnect()
{
    #ifdef WIN32
        // -- sanity check
        if (!mThreadSocket)
            return;

        // -- call disconnect
       mThreadSocket->RequestDisconnect();
       mThreadSocket->SetListen(false);

    #endif // WIN32
}

// ====================================================================================================================
// SendCommand():  send a script command to a connected socket
// ====================================================================================================================
bool SendCommand(const char* command)
{
    bool result = false;

    #ifdef WIN32
        if (!mThreadSocket)
        {
            return (false);
        }

        result = mThreadSocket->SendScriptCommand(command);
    #endif

    return (result);
}

// ====================================================================================================================
// SendCommandf():  send a formatted script command to a connected socket
// ====================================================================================================================
bool SendCommandf(const char* fmt, ...)
{
    bool result = false;

    #ifdef WIN32
        // -- ensure we're innitialized, and have a command to send
        if (!mThreadSocket || !fmt || !fmt[0])
            return (false);

        // -- create the script command
        va_list args;
        va_start(args, fmt);
        char cmdBuf[2048];
        vsprintf_s(cmdBuf, 2048, fmt, args);
        va_end(args);

        result = mThreadSocket->SendScriptCommand(cmdBuf);
    #endif // WIN32

    return (result);
}

// ====================================================================================================================
// DebuggerBreak():  Called to set the debugger force break in the script context.
// ====================================================================================================================
void SendDebuggerBreak()
{
    #ifdef WIN32
        // -- construct and send a debugger break packet
        tPacketHeader header(k_PacketVersion, tPacketHeader::DEBUGGER_BREAK, 0);
        tDataPacket* newPacket = new tDataPacket(&header, NULL);
        SendDataPacket(newPacket);
    #endif // WIN32
}

// ====================================================================================================================
// RegisterProcessRecvDataCallback():  Register a function to call, if a packt of type Socket::DATA is received
// ====================================================================================================================
void RegisterProcessRecvDataCallback(ProcessRecvDataCallback recvCallback)
{
    mRecvDataCallback = recvCallback;
}

// ====================================================================================================================
// CreateDataPacket():  Copies the header, and allocates the data buffer
// ====================================================================================================================
tDataPacket* CreateDataPacket(tPacketHeader* header, void* data)
{
    tDataPacket* newPacket = NULL;
    #ifdef WIN32
        // -- ensure we've initialized and connected the socket
        if (!mThreadSocket || !mThreadSocket->IsConnected())
        {
            return (NULL);
        }

        // -- create the packet, given a header, and possibly data
        newPacket = new tDataPacket(header, data);

    #endif // WIN32

    // -- return the result
    return (newPacket);
}

// ====================================================================================================================
// SendDataPacket():  Send a pre-constructed data packet through the socket
// ====================================================================================================================
bool SendDataPacket(tDataPacket* dataPacket)
{
    // -- initialize the result
    bool result = false;

    #ifdef WIN32
        // -- ensure we've initialized and connected the socket
        if (!mThreadSocket || !mThreadSocket->IsConnected())
        {
            return (NULL);
        }

        result = mThreadSocket->SendDataPacket(dataPacket);
    #endif // WIN32

    // -- return the result
    return (result);
}

// == class DataQueue =================================================================================================

// ====================================================================================================================
// Constructure
// ====================================================================================================================
DataQueue::DataQueue()
{
}

// ====================================================================================================================
// Enqueue():  Add data to the queue
// ====================================================================================================================
bool DataQueue::Enqueue(tDataPacket* packet, bool at_front)
{
    // -- sanity check - validate the packet
    if (!packet || (packet->mHeader.mSize > 0 && !packet->mData))
    {
        return (false);
    }
    if (packet->mHeader.mType <= tPacketHeader::NONE || packet->mHeader.mType >= tPacketHeader::COUNT)
    {
        return (false);
    }
    if (packet->mHeader.mVersion != k_PacketVersion)
    {
        return (false);
    }

    // -- add the packet to the queue
    if (at_front)
        mQueue.insert(mQueue.begin(), packet);
    else
        mQueue.push_back(packet);

    // -- success
    return (true);
}

// ====================================================================================================================
// Dequeue():  return the next data block and size
// ====================================================================================================================
bool DataQueue::Dequeue(tDataPacket*& packet, bool peekOnly)
{
    if (mQueue.size() == 0)
        return (false);

    // -- if we didn't request a specific packet ID, return the first element
    packet = mQueue[0];

    // -- if we're only peeking, then don't remove
    if (!peekOnly)
        mQueue.erase(mQueue.begin());

    return (true);
}

// ====================================================================================================================
// Clear();
// ====================================================================================================================
void DataQueue::Clear()
{
    // -- delete all queued packets
    while (mQueue.size() > 0)
    {
        tDataPacket* packet = mQueue[0];
        mQueue.erase(mQueue.begin());
        delete packet;
    }
}

// == CSocket =========================================================================================================

// -- only implmeented in WIN32
#ifdef WIN32

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CSocket::CSocket(TinScript::CScriptContext* script_context)
    : mListen(false)
    , mConnected(false)
    , mListenSocket(INVALID_SOCKET)
    , mConnectSocket(INVALID_SOCKET)
    , mScriptContext(script_context)
{
    // -- initialize the incoming packet members
    mRecvHeader[0] = '\0';
    mRecvPtr = mRecvHeader;
    mRecvPacket = NULL;
}

// ====================================================================================================================
// Destructor
// ====================================================================================================================
CSocket::~CSocket()
{
    // -- clear the queues
    mSendQueue.Clear();
    mRecvQueue.Clear();
    if (mRecvPacket != NULL)
    {
        delete mRecvPacket;
    }

    // -- close the socket if it exists
    if (mListenSocket != INVALID_SOCKET)
        closesocket(mConnectSocket);

    // -- if we have an open connection socket, we also need to cleanup WSA
    if (mConnectSocket != INVALID_SOCKET)
        closesocket(mConnectSocket);
}

// ====================================================================================================================
// ListenForConnection():  See if anyone is trying to connect
// ====================================================================================================================
bool CSocket::Listen()
{
    // -- if we're already connected, we're done
    if (mConnected || !mListen)
    {
        return (true);
    }

    // -- if we don't have a listen socket, create one
    if (mListenSocket == INVALID_SOCKET)
    {
        // -- this is executed within a thread - enqueue a thread command to print the message
        ScriptCommand("Print('CSocket::Listen(): listening for connection.\n');");

        // The address structure for a TCP socket
        SOCKADDR_IN addr;

        // -- address family
        addr.sin_family = AF_INET;      
        // -- assign port to this socket
        addr.sin_port = htons(k_DefaultPort);   

        //Accept a connection from any IP using INADDR_ANY
        //You could pass inet_addr("0.0.0.0") instead to accomplish the 
        //same thing. If you want only to watch for a connection from a 
        //specific IP, specify that instead.
        addr.sin_addr.s_addr = htonl(INADDR_ANY);

        // Create socket
        mListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (mListenSocket == INVALID_SOCKET)
        {
            // -- failed to create a socket
            return (false); 
        }

        // -- bind the socket to this address
        if (bind(mListenSocket, (LPSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR)
        {
            // -- failed to bind
            closesocket(mListenSocket);
            mListenSocket = INVALID_SOCKET;
            return (false);
        }

        // -- only one connection - use SOMAXCONN if you want multiple
        if (listen(mListenSocket, 1) != 0)
        {
            // -- failed to listen
            ScriptCommand("Print('Error - CSocket: listen() failed with error %d\n');", WSAGetLastError());
            closesocket(mListenSocket);
            mListenSocket = INVALID_SOCKET;
            return (false);
        }

        // -- accept a socket request
        mConnectSocket = accept(mListenSocket, NULL, NULL);
        if (mConnectSocket == INVALID_SOCKET)
        {
            // -- failed to listen - note, if we chose to disconnect, this will also fail
            // -- so we still return true, to allow the thread to continue
            ScriptCommand("Print('Error - CSocket: accept() failed with error %d\n');", WSAGetLastError());
            closesocket(mListenSocket);
            mListenSocket = INVALID_SOCKET;
            return (true);
        }

        // -- ensure the connect socket is non-blocking
        u_long iMode=1;
        ioctlsocket(mConnectSocket, FIONBIO, &iMode);

        // -- we're connected
        ScriptCommand("Print('CSocket: Connected.');");
        closesocket(mListenSocket);
        mListenSocket = INVALID_SOCKET;
        mConnected = true;

        // -- initialize the heartbeat
        mSendHeartbeatTimer = k_HeartbeatTimeMS;
        mRecvHeartbeatTimer = k_HeartbeatTimeoutMS;
    }

    // -- done
    return (true);
}

// ====================================================================================================================
// RequestConnection():  See if we can establish a connection
// ====================================================================================================================
bool CSocket::Connect(const char* ipAddress)
{
    // -- this only works if we're not already connected, and we have a listening socket, and a valid address
    if (mConnected || !ipAddress || !ipAddress[0])
    {
        return (false);
    }

    struct addrinfo* addressResult = NULL;
    struct addrinfo addressHints;
    struct addrinfo *addresses = NULL;

    // -- get the connection info
    char defaultPortStr[8];
    sprintf_s(defaultPortStr, "%d", k_DefaultPort);
    
    memset(&addressHints, 0, sizeof(addrinfo));
    int addrResult = getaddrinfo(ipAddress, defaultPortStr, &addressHints, &addressResult);
    if (addrResult != 0)
    {
        ScriptCommand("Print('Error - CSocket: getaddrinfo failed with error: %d\n');", addrResult);
        return (false);
    }

    // -- create the connect socket
    mConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (mConnectSocket == INVALID_SOCKET)
    {
        // -- failed to create a socket
        return (false); 
    }

    // -- see if we can request a conenction
    int32 connectResult = (int32)connect(mConnectSocket, addressResult->ai_addr, (int32)addressResult->ai_addrlen);
    if (connectResult == SOCKET_ERROR)
    {
        ScriptCommand("Print('Error CSocket: connect() failed.\n');");
        closesocket(mConnectSocket);
        mConnectSocket = INVALID_SOCKET;
        return (false);
    }

    // -- cleanup the addr info
    freeaddrinfo(addressResult);

    // -- see if we actually created a socket
    if (mConnectSocket == INVALID_SOCKET)
    {
        ScriptCommand("Print('Error CSocket: connect() failed.\n');");
        return (false);
    }

    // -- ensure the socket is non-blocking
    u_long iMode=1;
    ioctlsocket(mConnectSocket, FIONBIO, &iMode);

    // -- success
    ScriptCommand("Print('CSocket: Connected.\n');");

    // -- connection was successful - close down the listening connection
    if (mListenSocket != INVALID_SOCKET)
    {
        closesocket(mListenSocket);
        mListenSocket = INVALID_SOCKET;
    }

    // -- set the bool, return the result
    mConnected = true;
    mListen = false;

    // -- initialize the heartbeat
    mSendHeartbeatTimer = k_HeartbeatTimeMS;
    mRecvHeartbeatTimer = k_HeartbeatTimeoutMS;

    return (true);
}

// ====================================================================================================================
// RequestDisconnect():  Disconnect the socket from our end
// ====================================================================================================================
void CSocket::RequestDisconnect()
{
    // -- this must be threadsafe, so as not to stomp the current Update thread
    mThreadLock.Lock();

    // -- enqueue a disconnect packet to send to our partner
    tPacketHeader header(k_PacketVersion, tPacketHeader::DISCONNECT, 0);
    tDataPacket* newPacket = new tDataPacket(&header, NULL);
    mSendQueue.Enqueue(newPacket);

    // -- unlock the thread
    mThreadLock.Unlock();

    // -- perform an update, to flush the queues
    Update();

    // -- and finally disconnect
    Disconnect();
}

// ====================================================================================================================
// Disconnect():  Disconnect the socket, either from our connected partner, or from an error
// ====================================================================================================================
void CSocket::Disconnect()
{
    // -- kill the connection (thread safe)
    mThreadLock.Lock();

    // -- close the listening socket as well
    if (mListenSocket != INVALID_SOCKET)
    {
        // -- close the socket
        shutdown(mListenSocket, 2);
        closesocket(mListenSocket);
        mListenSocket = INVALID_SOCKET;
    }

    // -- close the socket
    closesocket(mConnectSocket);
    mConnectSocket = INVALID_SOCKET;
    mConnected = false;

    // -- notify TinScript, in case this connection was from a debugger
    ScriptCommand("Print('CSocket: Disconnected.');");
    ScriptCommand("DebuggerSetConnected(false);");

    // -- clear the queues
    mSendQueue.Clear();
    mRecvQueue.Clear();

    // -- unlock
    mThreadLock.Unlock();
}

// ====================================================================================================================
// ProcessSendPackets():  Send all queued packets
// ====================================================================================================================
bool CSocket::ProcessSendPackets()
{
    // -- this must be threadsafe
    mThreadLock.Lock();

    // -- if we're not connected, return (successfully)
    if (!mConnected)
    {
        mThreadLock.Unlock();
        return (true);
    }

    // -- check the heartbeat timer
    if (mSendHeartbeatTimer <= 0)
    {
        tPacketHeader header(k_PacketVersion, tPacketHeader::HEARTBEAT, 0);
        tDataPacket* newPacket = new tDataPacket(&header, NULL);
        mSendQueue.Enqueue(newPacket);
    }

    // -- send the messages we've queued, and check for an error
    bool errorDisconnect = false;
    int error = 0;
    tDataPacket* packetToSend = NULL;
    while (mSendQueue.Dequeue(packetToSend, true))
    {
        // -- as long as we're attempting to send, reset the heartbeat timer
        mSendHeartbeatTimer = k_HeartbeatTimeMS;

        // -- see if we have to send the header
        bool sendingHeader = !packetToSend->mHeader.mHeaderSent;
        int bytesToSend = 0;
        if (sendingHeader)
        {
            // -- ensure we've initialized the send ptr
            if (packetToSend->mHeader.mSendPtr == NULL)
            {
                packetToSend->mHeader.mSendPtr = (const char*)&packetToSend->mHeader;
            }

            bytesToSend = sizeof(tPacketHeader) -
                          ((int)packetToSend->mHeader.mSendPtr - (int)((const char*)&packetToSend->mHeader));
        }
        else
        {
            bytesToSend = packetToSend->mHeader.mSize -
                          ((int)packetToSend->mHeader.mSendPtr - (int)packetToSend->mData);
        }

        // -- send (skip this, if we the packet has no data
        int bytesSent = 0;
        if (bytesToSend > 0)
        {
            bytesSent = send(mConnectSocket, packetToSend->mHeader.mSendPtr, bytesToSend, 0);
        }

        // -- if we received an error, we'll have to disconnect
        if (bytesSent == SOCKET_ERROR)
        {
            error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK)
                errorDisconnect = true;
            break;
        }

        // -- otherwise, see if we're finished with this packet
        else
        {
            // -- update our pointers, etc... based on what we were able to send
            bytesToSend -= bytesSent;
            packetToSend->mHeader.mSendPtr += bytesSent;

            // -- see if we're done
            if (bytesToSend <= 0)
            {
                if (sendingHeader)
                {
                    packetToSend->mHeader.mHeaderSent = true;
                    packetToSend->mHeader.mSendPtr = (const char*)packetToSend->mData;
                }
                else
                {
                    mSendQueue.Dequeue(packetToSend);
                    delete packetToSend;

                }
            }

            // -- not done, but not able to send any more this frame
            else
                break;
        }
    }

    // -- if we encountered a socket error sending, disconnect
    if (errorDisconnect)
    {
        // -- notify the script context
        ScriptCommand("Print('Error - CSocket::Send(): failed with error: %d\n');", error);
        mThreadLock.Unlock();
        Disconnect();
        return (true);
    }

    // -- this must be threadsafe
    mThreadLock.Unlock();

    return (true);
}

// ====================================================================================================================
// ReceivePackets():  Receive the raw data from the socket
// ====================================================================================================================
bool CSocket::ReceivePackets()
{
    // -- in an update, we want to lock any access to the send and recv queues, from outside this thread
    mThreadLock.Lock();

    // -- if we're not connected, we're done
    if (!mConnected)
    {
        mThreadLock.Unlock();
        return (true);
    }

    // -- decrement the heartbeat timers
    mSendHeartbeatTimer -= k_ThreadUpdateTimeMS;
    mRecvHeartbeatTimer -= k_ThreadUpdateTimeMS;

    // -- see if we have something to recv
    char recvbuf[k_MaxBufferSize];
    int recvbuflen = k_MaxBufferSize;

    // -- recv data from the socket
    while (true)
    {
        int bytesRecv = recv(mConnectSocket, recvbuf, recvbuflen, 0);

        // -- check for a disconnecct
        int error = WSAGetLastError();
        if (error != WSAEWOULDBLOCK && error != 0)
        {
            // -- notify the script context
            ScriptCommand("Print('CSocket: Recv error %d\n');", error);
            mThreadLock.Unlock();
            Disconnect();
            return (true);
        }

        else if (bytesRecv > 0)
        {
            // -- we received data - reset the heartbeat timer
            mRecvHeartbeatTimer = k_HeartbeatTimeoutMS;

            // -- if we fail to process the data, something very bad has happened
            if (!ProcessRecvData(recvbuf, bytesRecv))
            {
                // -- notify the script context
                ScriptCommand("Print('CSocket: Unable to ProcessRecvData()\n');");
                mThreadLock.Unlock();
                Disconnect();
                return (true);
            }
        }

        // -- else we're done receiving
        else
        {
            // -- check the heartbeat
            if (mRecvHeartbeatTimer <= 0)
            {
                ScriptCommand("Print('CSocket: Heartbeat timeout\n');");
                mThreadLock.Unlock();
                Disconnect();
            }

            break;
        }
    }

    // -- unlock the thread, allowing access to the queues
    mThreadLock.Unlock();

    return (true);
}

// ====================================================================================================================
// ProcessSendPackets():  Send all queued packets
// ====================================================================================================================
bool CSocket::ProcessRecvPackets()
{
    // -- this must be threadsafe
    mThreadLock.Lock();

    // -- if we're not connected, return (successfully)
    if (!mConnected)
    {
        mThreadLock.Unlock();
        return (true);
    }

    // -- process our recv queue
    bool receivedDisconnect = false;
    bool deletePacket = false;
    tDataPacket* recvPacket = NULL;
    while (mRecvQueue.Dequeue(recvPacket))
    {
        // -- set the bool to delete the packet
        deletePacket = true;

        // -- if the packet contains a TinScript command
        if (recvPacket->mHeader.mType == tPacketHeader::SCRIPT)
        {
            // -- execute whatever command we received
            bool success = ScriptCommand((const char*)recvPacket->mData);

            // -- if the thread buffer was full, re-enqueue, and try again later
            if (!success)
            {
                mRecvQueue.Enqueue(recvPacket, true);
                break;
            }
        }

        // -- if the packet is raw data, send it through the registered callback
        // -- note:  the registered callback better be threadsafe
        // -- note:  the callback is responsible for deallocating the packet
        else if (recvPacket->mHeader.mType == tPacketHeader::DATA)
        {
            if (mRecvDataCallback != NULL)
            {
                // -- set the bool to prevent the packet from being deleted here
                deletePacket = false;

                // -- send the packet to the client
                mRecvDataCallback(recvPacket);
            }
        }

        // -- if we received a debugger break, notify the script context immediately
        // -- this is technically not thread safe, but it's the only way to break an infinite loop
        else if (recvPacket->mHeader.mType == tPacketHeader::DEBUGGER_BREAK)
        {
            mScriptContext->SetForceBreak(0);
        }

        // -- else if the packet contains a disconnect command
        else if (recvPacket->mHeader.mType == tPacketHeader::DISCONNECT)
        {
            receivedDisconnect = true;
            break;
        }

        // -- if we're supposed to delete the packet...
        if (deletePacket)
        {
            delete recvPacket;
        }
    }

    // -- Unlock and return success
    mThreadLock.Unlock();

    // -- if we received a disconnect...
    if (receivedDisconnect)
        Disconnect();

    return (true);
}

// ====================================================================================================================
// SendScriptCommand():  Send a script command through a connected socket
// ====================================================================================================================
bool CSocket::SendScriptCommand(const char* command)
{
    // -- ensure we're connected, and have something to send
    if (!mConnected || !command || !command[0])
    {
        return (false);
    }

    // -- lock the thread before modifying the queues
    mThreadLock.Lock();

    // -- fill in the packet header
    int32 length = (int32)strlen(command) + 1;

    // -- ensure we don't exceed the max length
    if (length > k_MaxPacketSize)
    {
        return (false);
    }

    tPacketHeader header(k_PacketVersion, tPacketHeader::SCRIPT, length);
    tDataPacket* newPacket = new tDataPacket(&header, (void*)command);

    // -- enqueue the packet
    bool result = mSendQueue.Enqueue(newPacket);

    // -- unlock the thread
    mThreadLock.Unlock();

    // -- return the result
    return (result);
}

// ====================================================================================================================
// SendData():  Send raw data through the socket
// ====================================================================================================================
bool CSocket::SendData(void* data, int dataSize)
{
    // -- ensure we're connected, and have something to send
    if (!data || dataSize <= 0)
    {
        return (false);
    }

    // -- lock the thread before modifying the queues
    mThreadLock.Lock();

    tPacketHeader header(k_PacketVersion, tPacketHeader::DATA, dataSize);
    tDataPacket* newPacket = new tDataPacket(&header, data);

    // -- enqueue the packet
    bool result = mSendQueue.Enqueue(newPacket);

    // -- unlock the thread
    mThreadLock.Unlock();

    // -- return the result
    return (result);
}

// ====================================================================================================================
// SendDataPacket():  Send a pre=constructed packet through the socket
// ====================================================================================================================
bool CSocket::SendDataPacket(tDataPacket* dataPacket)
{
    // -- ensure we're connected, and have something to send
    if (!dataPacket)
    {
        return (false);
    }

    // -- lock the thread before modifying the queues
    mThreadLock.Lock();

    // -- enqueue the packet
    bool result = mSendQueue.Enqueue(dataPacket);

    // -- unlock the thread
    mThreadLock.Unlock();

    // -- return the result
    // -- note:  if this returns false, the packet will not have been enqueued,
    // -- so the requestor must either try again, or delete the memory
    return (result);
}

// ====================================================================================================================
// Update():  Update the socket
// ====================================================================================================================
bool CSocket::Update()
{
    // -- first, receive socket data
    bool result = ReceivePackets();
    if (!result)
    {
        return (false);
    }

    // -- then send queued packets
    result = ProcessSendPackets();
    if (!result)
    {
        return (false);
    }

    // -- finally, process the received packets
    result = ProcessRecvPackets();
    if (!result)
    {
        return (false);
    }

    // -- success
    return (true);
}

// -- Because sockets run in their own thread, they have to enqueue commands and statements through a mutex
// ====================================================================================================================
// ScriptCommand():  Create a text command, to be enqueued in the script context thread
// ====================================================================================================================
bool8 CSocket::ScriptCommand(const char* fmt, ...)
{
    // -- sanity check
    if (!mScriptContext || !fmt || !fmt[0])
        return (true);

    // -- create the script command
    va_list args;
    va_start(args, fmt);
    char cmdBuf[2048];
    vsprintf_s(cmdBuf, 2048, fmt, args);
    va_end(args);

    // -- add the command to the thread buffer
    return (mScriptContext->AddThreadCommand(cmdBuf));
}

// ====================================================================================================================
// ProcessRecvData():  Reconstitute a data stream back into packets
// ====================================================================================================================
bool CSocket::ProcessRecvData(void* data, int dataSize)
{
    // -- keep track of the data left to process
    char* dataPtr = (char*)data;
    int bytesToProcess = dataSize;

    // -- keep processing, as long as we have data
    while (bytesToProcess)
    {
        // -- set up the pointers to fill in either the header, of the data of a packet
        bool processHeader = (mRecvPacket == NULL);
        char* basePtr = (processHeader) ? mRecvHeader : mRecvPacket->mData;
        int bufferSize = (processHeader) ? sizeof(tPacketHeader) : mRecvPacket->mHeader.mSize;
        int bytesRequired = bufferSize - ((int)mRecvPtr - (int)basePtr);

        // -- find out how many bytes we've received, and copy as many as we're able
        int bytesToCopy = (bytesToProcess >= bytesRequired) ? bytesRequired : bytesToProcess;
        if (bytesToCopy > 0)
        {
            // -- copy the data, and modify the counts and pointers
            memcpy(mRecvPtr, dataPtr, bytesToCopy);
            dataPtr += bytesToCopy;
            mRecvPtr += bytesToCopy;
            bytesRequired -= bytesToCopy;
            bytesToProcess -= bytesToCopy;
        }

        // -- see if we have filled our required bytes
        if (bytesRequired == 0)
        {
            // -- if we don't have a recv packet, then we've completed a header, and can construct one
            if (processHeader)
            {
                // -- verify the new packet is valid
                tPacketHeader* newPacketHeader = (tPacketHeader*)mRecvHeader;
                if (newPacketHeader->mVersion != k_PacketVersion)
                {
                    // -- invalid version
                    return (false);
                }
                if (newPacketHeader->mType <= tPacketHeader::NONE || newPacketHeader->mType >= tPacketHeader::COUNT)
                {
                    // -- invalid type
                    return (false);
                }

                if (newPacketHeader->mSize < 0 || newPacketHeader->mSize > 1024)
                {
                    // -- invalid size
                    return (false);
                }

                // -- create the new packet
                mRecvPacket = new tDataPacket((tPacketHeader*)mRecvHeader, NULL);

                // -- reset the recv pointer to the data of the new packet
                mRecvPtr = mRecvPacket->mData;

                // -- if we have data, we need to keep reading
                // -- otherwise, fall through, and process a complete packet
                if (mRecvPacket->mHeader.mSize == 0)
                {
                    processHeader = false;
                }
            }

            // -- if we're not processing the header, we've got complete packet
            // -- might have fallen through for a header with no data
            if (!processHeader)
            {
                // -- enqueue, and prepare for the next
                if (!mRecvQueue.Enqueue(mRecvPacket))
                {
                    // -- unable to enqueue - no more space
                    return (false);
                }

                // -- reset the recv members
                mRecvPacket = NULL;
                mRecvPtr = mRecvHeader;
            }
        }
    }

    // -- success
    return (true);
}

#endif // WIN32

} // namespace SocketManager

// ====================================================================================================================
// -- TinScript Registration
REGISTER_FUNCTION_P0(SocketListen, SocketManager::Listen, bool);
REGISTER_FUNCTION_P1(SocketConnect, SocketManager::Connect, bool, const char*);
REGISTER_FUNCTION_P0(SocketDisconnect, SocketManager::Disconnect, void);
REGISTER_FUNCTION_P1(SocketSend, SocketManager::SendCommand, bool, const char*);

// ====================================================================================================================
// EOF
// ====================================================================================================================
