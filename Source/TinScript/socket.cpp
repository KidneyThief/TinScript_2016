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

#include "integration.h"

#if PLATFORM_UE4
	#undef TEXT
	#define WIN32_LEAN_AND_MEAN
#endif

// -- system includes
#include <map>

// -- sockets are only implemented in WIN32
#include <winsock2.h>
#include <ws2tcpip.h>

#include <vector>

// -- includes
#include "mathutil.h"
#include "socket.h"

// -- includes required by any system wanting access to TinScript
#include "TinTypes.h"
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
static bool mInitialized = false;
static DWORD mThreadID = 0;
static HANDLE mThreadHandle = NULL;
static CSocket* mThreadSocket = NULL;
static int mThreadSocketID = 1;

// -- WSA variables
bool mWSAInitialized = false;
static WSADATA mWSAdata;

// --------------------------------------------------------------------------------------------------------------------
// class cSocketFunctionSignature
// An optimization to cache the signatures of functions executed remotely - allows for proper argument conversion
// locally, more efficient than sending strings, and saves the conversion on the remote end
// --------------------------------------------------------------------------------------------------------------------
class cSocketFunctionSignature
{
    public:
        cSocketFunctionSignature(uint32 function_hash)
        {
            // -- note:  remotely executing functions uses the first argument as the function hash, so
            // -- so we only allow 7x actual arguments
            m_functionHash = function_hash;

            // -- initialize all arg types to NULL
            for (int32 i = 0; i < kMaxRegisteredParameterCount; ++i)
                m_paramTypes[i] = TinScript::eVarType::TYPE_NULL;
        }

        uint32 m_functionHash;
        TinScript::eVarType m_paramTypes[kMaxRegisteredParameterCount];
};

// -- because this dictionary is forever increasing in size, we can we only have one socket thread,
// -- we can simply use a static global
static std::map<uint32, cSocketFunctionSignature> m_socketExecFunctionList;
typedef std::map<uint32, cSocketFunctionSignature>::iterator tSignatureIterator;

// -- a custom data handler
static ProcessRecvDataCallback mRecvDataCallback = NULL;

DWORD WINAPI ThreadUpdate(void* script_context);

// ====================================================================================================================
// Initialize():  Initialize the SocketManager
// ====================================================================================================================
void Initialize()
{
    // -- create the thread
    mThreadHandle = CreateThread(NULL,                      // default security attributes
                                    0,                         // use default stack size  
                                    ThreadUpdate,              // thread function name
                                    TinScript::GetContext(),   // argument to thread function 
                                    0,                         // use default creation flags 
                                    &mThreadID);               // returns the thread identifier 
}

// ====================================================================================================================
// ThreadUpdate():  Update loop for the SocketManager, run inside the thread
// ====================================================================================================================
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
		// if our thread socket is deleted (hard shutdown??), we're done
		if (mThreadSocket == nullptr)
		{
			break;
		}

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

// ====================================================================================================================
// Termintate():  Perform all shutdown and cleanup of the SocketManager
// ====================================================================================================================
void Terminate()
{
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
}

// ====================================================================================================================
// IsLIstening(): return if we're not yet connected, but listening
// ====================================================================================================================
bool IsListening()
{
    // -- see if we can enable listening
    if (mThreadSocket)
    {
        return (mThreadSocket->IsListening());
    }

    // -- not listening for new connections
    return (false);
}

// ====================================================================================================================
// Listen(): Set the connection to listen for incoming connect requests
// ====================================================================================================================
bool Listen()
{
    // -- see if we can enable listening
    if (mThreadSocket && !mThreadSocket->IsConnected())
    {
        mThreadSocket->SetListen(true);
        return (true);
    }

    // -- unable to listen for new connections
    return (false);
}

// ====================================================================================================================
// Connect():  send a winsock connection request
// ====================================================================================================================
bool Connect(const char* ipAddress)
{
    bool result = false;

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

    return (result);
}

// ====================================================================================================================
// IsConnected():  returns if we have a valid winsock connected
// ====================================================================================================================
bool IsConnected()
{
    bool result = false;

    // -- sanity check
    if (!mThreadSocket)
        return (false);

    result = mThreadSocket->IsConnected();

    // -- return the result
    return (result);
}

// ====================================================================================================================
// Disconnect():  disconnect a winsock connection
// ====================================================================================================================
void Disconnect()
{
    // -- sanity check
    if (!mThreadSocket)
        return;

    // -- call disconnect
    mThreadSocket->RequestDisconnect();
    mThreadSocket->SetListen(false);
}

// ====================================================================================================================
// SendCommand():  send a script command to a connected socket
// ====================================================================================================================
bool SendCommand(const char* command)
{
    bool result = false;

    if (!mThreadSocket)
    {
        return (false);
    }

    result = mThreadSocket->SendScriptCommand(command);

    return (result);
}

// ====================================================================================================================
// SendCommandf():  send a formatted script command to a connected socket
// ====================================================================================================================
bool SendCommandf(const char* fmt, ...)
{
    bool result = false;

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

    return (result);
}

// ====================================================================================================================
// SendExec():  send a function and args to a connected socket, to be executed directly
// ====================================================================================================================
bool SendExec(int32 func_hash, const char* arg1, const char* arg2, const char* arg3, const char* arg4,
              const char* arg5, const char* arg6, const char* arg7)
{
    // -- create the packet data - convert each arg to a valid type, and serialize the values
    char packet_buffer[k_MaxPacketSize];
    int32 max_size = k_MaxPacketSize;
    uint32* dataptr = (uint32*)packet_buffer;

    // -- reserve space for the arg count
    uint32* arg_count = dataptr;
    *dataptr++ = 0;
    max_size -= sizeof(uint32);

    // -- the first arg is the function hash
    *dataptr++ = (int32)TinScript::TYPE_int;
    *dataptr++ = func_hash;
    *arg_count += 1;
    max_size -= sizeof(uint32);

    // -- see if we have a remote signature for this function
    cSocketFunctionSignature* remote_signature = nullptr;
    tSignatureIterator found = m_socketExecFunctionList.find(func_hash);
    if (found != m_socketExecFunctionList.end())
        remote_signature = &found->second;

    // -- note:  the first argument of a remotely executed function is the function hash,
    // -- therefore, we only have room for 7 actual arguments 
    // -- add the args
    const char* args[kMaxRegisteredParameterCount] = { "", arg1, arg2, arg3, arg4, arg5, arg6, arg7 };
    for (int32 i = 1; i < kMaxRegisteredParameterCount; ++i)
    {
        // -- if the arg is null or empty, we're done
        char* arg_string = const_cast<char*>(args[i]);
        if (arg_string == nullptr || arg_string[0] == '\0')
            break;

        // -- if we have a remote signature, and the argument type is convertable, send a properly typed argument
        bool arg_added = false;
        if (remote_signature != nullptr)
        {
            // -- get the type to which the arg is to be converted
            TinScript::eVarType arg_type = remote_signature->m_paramTypes[i];

            // -- if the remote signature indicates a NULL parameter, we're also done
            if (arg_type == TinScript::eVarType::TYPE_NULL)
                break;

            // -- if the type is anything other than a string, try to convert and encode it
            if (arg_type != TinScript::eVarType::TYPE_string)
            {
                char value_buf[MAX_TYPE_SIZE * sizeof(uint32)];
                arg_added = TinScript::gRegisteredStringToType[arg_type](TinScript::GetContext(), (void*)value_buf, arg_string);

                // -- if we were successful, write the type and the value
                if (arg_added)
                {
                    // -- note:  bools are only 1 byte, but we want to ensure the dataptr is advance by a whole uint32
                    *dataptr++ = (uint32)arg_type;
                    max_size -= sizeof(uint32);
                    int32 type_size = ((TinScript::gRegisteredTypeSize[arg_type] + 3) / 4) * 4;
                    memcpy((void*)dataptr, (void*)value_buf, type_size);
                    max_size -= type_size;
                    dataptr += type_size / 4;
                }
            }
        }

        // -- if we were not able to encode the arg (or the arg is supposed to remain a string), send it directly
        if (!arg_added)
        {
            int32 arg_length = (int32)strlen(arg_string);
            *dataptr++ = (uint32)TinScript::TYPE_string;
            TinScript::SafeStrcpy((char*)dataptr, max_size, arg_string, max_size);

            // -- ensure we keep the data ptr aligned (include the string terminator)
            int32 data_count = (arg_length / 4) + 1;
            dataptr += data_count;
            max_size -= data_count * sizeof(uint32);
        }

        // -- increment the arg count
        *arg_count += 1;
    }

    // -- at this point, we have the packet data ready to send
    void* data_start = &packet_buffer[0];
    int32 total_size = kPointerDiffUInt32(dataptr, data_start);

    // -- create the packet header
    SocketManager::tPacketHeader header(k_PacketVersion, SocketManager::tPacketHeader::SCRIPT_FUNCTION_EXEC, total_size);

    // -- now create the packet to send
    SocketManager::tDataPacket* newPacket = SocketManager::CreateDataPacket(&header, data_start);
    if (!newPacket)
    {
        ScriptAssert_(TinScript::GetContext(), false, "<internal>", -1,
                      "Error - SocketManager::SendExec(): not connected - don't forget to SocketListen()\n");
        return (false);
    }

    // -- send the packet
    SocketManager::SendDataPacket(newPacket);

    // -- success
    return (true);
}

// ====================================================================================================================
// DebuggerBreak():  Called to set the debugger force break in the script context.
// ====================================================================================================================
void SendDebuggerBreak()
{
    // -- construct and send a debugger break packet
    tPacketHeader header(k_PacketVersion, tPacketHeader::DEBUGGER_BREAK, 0);
    tDataPacket* newPacket = new tDataPacket(&header, NULL);
    SendDataPacket(newPacket);
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

    // -- ensure we've initialized and connected the socket
    if (!mThreadSocket || !mThreadSocket->IsConnected())
    {
        return (NULL);
    }

    // -- create the packet, given a header, and possibly data
    newPacket = new tDataPacket(header, data);

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

    // -- ensure we've initialized and connected the socket
    if (!mThreadSocket || !mThreadSocket->IsConnected())
    {
        return (NULL);
    }

    result = mThreadSocket->SendDataPacket(dataPacket);

    // -- return the result
    return (result);
}

// ====================================================================================================================
// SendPrintDataPacket():  Send a pre-constructed data packet through the socket, for print data packets
// ====================================================================================================================
bool SendPrintDataPacket(tDataPacket* dataPacket)
{
    // -- initialize the result
    bool result = false;

    // -- ensure we've initialized and connected the socket
    if (!mThreadSocket || !mThreadSocket->IsConnected())
    {
        return (NULL);
    }

    result = mThreadSocket->SendPrintDataPacket(dataPacket);

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

// ====================================================================================================================
// Constructor
// ====================================================================================================================
CSocket::CSocket(TinScript::CScriptContext* script_context)
    : mListen(false)
    , mConnected(false)
    , mListenSocket(nullptr)
    , mConnectSocket(nullptr)
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
    if (mListenSocket != nullptr)
        closesocket((SOCKET)mConnectSocket);

    // -- if we have an open connection socket, we also need to cleanup WSA
    if (mConnectSocket != nullptr)
        closesocket((SOCKET)mConnectSocket);
}

// ====================================================================================================================
// IsListening():  see if we're not connected, but listening for a connection
// ====================================================================================================================
bool CSocket::IsListening()
{
    return (!mConnected && mListen);
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
    if (mListenSocket == nullptr)
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
        mListenSocket = (uint32*)socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (mListenSocket == nullptr)
        {
            // -- failed to create a socket
            return (false); 
        }

        // -- bind the socket to this address
        if (bind((SOCKET)mListenSocket, (LPSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR)
        {
            // -- failed to bind
            closesocket((SOCKET)mListenSocket);
            mListenSocket = nullptr;
            return (false);
        }

        // -- only one connection - use SOMAXCONN if you want multiple
        if (listen((SOCKET)mListenSocket, 1) != 0)
        {
            // -- failed to listen
            ScriptCommand("Print('Error - CSocket: listen() failed with error %d\n');", WSAGetLastError());
            closesocket((SOCKET)mListenSocket);
            mListenSocket = nullptr;
            return (false);
        }

        // -- accept a socket request
        mConnectSocket = (uint32*)accept((SOCKET)mListenSocket, NULL, NULL);
        if (mConnectSocket == nullptr)
        {
            // -- failed to listen - note, if we chose to disconnect, this will also fail
            // -- so we still return true, to allow the thread to continue
            ScriptCommand("Print('Error - CSocket: accept() failed with error %d\n');", WSAGetLastError());
            closesocket((SOCKET)mListenSocket);
            mListenSocket = nullptr;
            return (true);
        }

        // -- ensure the connect socket is non-blocking
        u_long iMode=1;
        ioctlsocket((SOCKET)mConnectSocket, FIONBIO, &iMode);

        // -- we're connected
        ScriptCommand("Print('CSocket: Connected.');");
        closesocket((SOCKET)mListenSocket);
        mListenSocket = nullptr;
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
    mConnectSocket = (uint32*)socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (mConnectSocket == nullptr)
    {
        // -- failed to create a socket
        return (false); 
    }

    // -- see if we can request a connection
    int32 connectResult = (int32)connect((SOCKET)mConnectSocket, addressResult->ai_addr, (int32)addressResult->ai_addrlen);
    if (connectResult == SOCKET_ERROR)
    {
        ScriptCommand("Print('Error CSocket: connect() failed.\n');");
        closesocket((SOCKET)mConnectSocket);
        mConnectSocket = nullptr;
        return (false);
    }

    // -- cleanup the addr info
    freeaddrinfo(addressResult);

    // -- see if we actually created a socket
    if (mConnectSocket == nullptr)
    {
        ScriptCommand("Print('Error CSocket: connect() failed.\n');");
        return (false);
    }

    // -- ensure the socket is non-blocking
    u_long iMode=1;
    ioctlsocket((SOCKET)mConnectSocket, FIONBIO, &iMode);

    // -- success
    ScriptCommand("Print('CSocket: Connected.\n');");

    // -- connection was successful - close down the listening connection
    if (mListenSocket != nullptr)
    {
        closesocket((SOCKET)mListenSocket);
        mListenSocket = nullptr;
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
    if (mListenSocket != nullptr)
    {
        // -- close the socket
        shutdown((SOCKET)mListenSocket, 2);
        closesocket((SOCKET)mListenSocket);
        mListenSocket = nullptr;
    }

    // -- close the socket
    closesocket((SOCKET)mConnectSocket);
    mConnectSocket = nullptr;
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
    int packets_sent_count = kSocketPacketProcessMax;
    bool errorDisconnect = false;
    int error = 0;
    tDataPacket* send_packet = nullptr;
    while (--packets_sent_count >= 0 && mSendQueue.Dequeue(send_packet, true))
    {
        // -- as long as we're attempting to send, reset the heartbeat timer
        mSendHeartbeatTimer = k_HeartbeatTimeMS;

        // -- see if we have to send the header
        bool sendingHeader = !send_packet->mHeader.mHeaderSent;
        int bytesToSend = 0;
        if (sendingHeader)
        {
            // -- ensure we've initialized the send ptr
            if (send_packet->mHeader.mSendPtr == NULL)
            {
                send_packet->mHeader.mSendPtr = (const char*)&send_packet->mHeader;
            }

            void* header_ptr = &send_packet->mHeader;
            bytesToSend = tPacketHeader::HeaderSize - kPointerDiffUInt32(send_packet->mHeader.mSendPtr, header_ptr);
        }
        else
        {
            bytesToSend = send_packet->mHeader.mSize - kPointerDiffUInt32(send_packet->mHeader.mSendPtr, send_packet->mData);
        }

        // -- send (skip this, if we the packet has no data
        int bytesSent = 0;
        if (bytesToSend > 0)
        {
            bytesSent = send((SOCKET)mConnectSocket, send_packet->mHeader.mSendPtr, bytesToSend, 0);
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
            send_packet->mHeader.mSendPtr += bytesSent;

            // -- see if we're done
            if (bytesToSend <= 0)
            {
                if (sendingHeader)
                {
                    send_packet->mHeader.mHeaderSent = true;
                    send_packet->mHeader.mSendPtr = (const char*)send_packet->mData;
                }
                else
                {
                    mSendQueue.Dequeue(send_packet);
                    delete send_packet;

                    // -- clear pointers for the next loop
                    send_packet = nullptr;

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
        int bytesRecv = recv((SOCKET)mConnectSocket, recvbuf, recvbuflen, 0);

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
    int packet_receive_count = 0;
    bool receivedDisconnect = false;
    bool deletePacket = false;
    tDataPacket* recvPacket = NULL;
    //while (++packet_receive_count < kSocketPacketProcessMax && mRecvQueue.Dequeue(recvPacket))
    while (mRecvQueue.Dequeue(recvPacket))
    {
        // -- set the bool to delete the packet
        deletePacket = true;

        // -- if the packet contains a TinScript string to execute
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

        // -- if the packet contains an immediate execution command
        else if (recvPacket->mHeader.mType == tPacketHeader::SCRIPT_FUNCTION_EXEC)
        {
            // -- queue the immediate execution command
            // -- success or fail, the packet is de-queued
            if (!ReceiveScriptExec(recvPacket->mData))
            {
                // note:  asserts during threaded execution cause crashes - find out why!
                TinPrint(mScriptContext, "Error - ProcessRecvPackets() - SCRIPT_FUNCTION_EXEC:  unable to process the packet\n\n");
            }
        }

        // -- if the packet contains an immediate execution command
        else if (recvPacket->mHeader.mType == tPacketHeader::SCRIPT_FUNCTION_SIGNATURE)
        {
            // -- queue the immediate execution command
            // -- success or fail, the packet is de-queued
            if (!ReceiveScriptSignature(recvPacket->mData))
            {
                TinPrint(mScriptContext,"Error - ProcessRecvPackets() - SCRIPT_FUNCTION_SIGNATURE:  unable to process the packet\n\n");
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
// SendDataPacket():  Send a pre-constructed packet through the socket
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
    // -- so the requester must either try again, or delete the memory
    return (result);
}

// ====================================================================================================================
// SendPrintDataPacket():  Send a pre-constructed packet through the socket, for print messages
// ====================================================================================================================
bool CSocket::SendPrintDataPacket(tDataPacket* dataPacket)
{
    // -- ensure we're connected, and have something to send
    if (!dataPacket)
    {
        return (false);
    }

    // -- lock the thread before modifying the queues
    mThreadLock.Lock();

    // -- enqueue the packet - 
    // for now, queue everything, and let the debugger decide to ignore prints if it receives more than it can handle
    bool result = mSendQueue.Enqueue(dataPacket);

    // -- unlock the thread
    mThreadLock.Unlock();

    // -- return the result
    // -- note:  if this returns false, the packet will not have been enqueued,
    // -- so the requester must either try again, or delete the memory
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
// ReceiveScriptExec():  Unpack the packet data into an immediate execution command
// ====================================================================================================================
bool CSocket::ReceiveScriptExec(void* data)
{
    // -- get the arg count
    uint32* dataptr = (uint32*)data;
    int32 arg_count = (int32)*dataptr++;
    if (arg_count < 0 || arg_count > TinScript::CFunctionContext::eMaxParameterCount)
        return (false);

    // -- the first arg is the function hash
    if (*dataptr++ != (int32)TinScript::TYPE_int)
        return (false);
    uint32 func_hash = (uint32)*dataptr++;

    // -- create the schedule
    if (!mScriptContext->BeginThreadExec(func_hash))
        return (false);

    // -- if we receive parameters that are not correctly typed,  send the function signature
    // -- back to the sender
    bool paramTypesOptimal = true;

    // -- add the parameters
    for (int32 i = 1; i < arg_count; ++i)
    {
        // -- get the type
        TinScript::eVarType arg_type = (TinScript::eVarType)*dataptr++;

        // -- if the type is a string, add the argument
        if (arg_type == TinScript::TYPE_string)
        {
            const char* string_ptr = (const char*)dataptr;
            if (!mScriptContext->AddThreadExecParam(arg_type, (void*)string_ptr))
                paramTypesOptimal = false;

            // -- update the string ptr
            int32 length = (int32)strlen(string_ptr);
            int32 data_length = (length / 4) + 1;
            dataptr += data_length;
        }

        // -- else add the argument by type
        else
        {
            if (!mScriptContext->AddThreadExecParam(arg_type, (void*)dataptr))
                paramTypesOptimal = false;

            int32 type_size = ((TinScript::gRegisteredTypeSize[arg_type] + 3) / 4) * 4;
            dataptr += type_size / 4;
        }
    }

    // -- after all the arguments have been added, queue the command
    mScriptContext->QueueThreadExec();

    // -- if the parameter types were not optimal, send back the actual function signature
    if (!paramTypesOptimal)
    {
        TinScript::CFunctionEntry* fe = mScriptContext->GetGlobalNamespace()->GetFuncTable()->FindItem(func_hash);
        if (fe != nullptr &&  fe->GetContext() != nullptr)
        {
            SendScriptSignature(func_hash, fe->GetContext());
        }
    }

    // -- success
    return (true);
}

// ====================================================================================================================
// SendScriptSignature():  Send a data packet containing the function signature
// ====================================================================================================================
bool CSocket::SendScriptSignature(uint32 func_hash, TinScript::CFunctionContext* func_context)
{
    // -- sanity check
    if (func_hash == 0 || func_context == nullptr)
        return (false);

    // -- we pack the function hash, then the arg count, and then the type of each arg
    uint32 packet_buffer[kMaxRegisteredParameterCount + 2];
    uint32* data_ptr = (uint32*)packet_buffer;

    // -- function hash
    *data_ptr++ = func_hash;

    // -- arg count
    int32 arg_count = func_context->GetParameterCount();
    if (arg_count > kMaxRegisteredParameterCount)
        arg_count = kMaxRegisteredParameterCount;
    *data_ptr++ = arg_count;

    // -- loop through the args, add the type of each
    for (int i = 0; i < arg_count; ++i)
    {
        *data_ptr++ = (uint32)func_context->GetParameter(i)->GetType();
    }

    // -- create the header
    // -- at this point, we have the packet data ready to send
    void* data_start = &packet_buffer[0];
    int32 total_size = kPointerDiffUInt32(data_ptr, data_start);

    // -- create the packet header
    SocketManager::tPacketHeader header(k_PacketVersion, SocketManager::tPacketHeader::SCRIPT_FUNCTION_SIGNATURE, total_size);

    // -- now create the packet to send
    SocketManager::tDataPacket* newPacket = SocketManager::CreateDataPacket(&header, data_start);
    if (!newPacket)
    {
        ScriptAssert_(mScriptContext, false, "<internal>", -1,
                      "Error - SendScriptSignature():  not connected - don't forget to SocketListen()\n");
        return (false);
    }

    // -- send the packet
    SocketManager::SendDataPacket(newPacket);

    // -- success
    return (true);
}

// ====================================================================================================================
// ReceiveScriptSignature():  Unpack the packet data and store the argument list for the function signature
// ====================================================================================================================
bool CSocket::ReceiveScriptSignature(void* data)
{
    // -- get the function hash, then the arg count
    uint32* dataptr = (uint32*)data;
    uint32 func_hash = (uint32)*dataptr++;
    int32 arg_count = (int32)*dataptr++;

    // -- using SocketExec(), the first arg is the function hash, so we can only support signatures with one less arg
    if (arg_count < 0 || arg_count > TinScript::CFunctionContext::eMaxParameterCount)
        return (false);

    tSignatureIterator found = m_socketExecFunctionList.find(func_hash);
    if (found == m_socketExecFunctionList.end())
        m_socketExecFunctionList.insert(std::make_pair(func_hash, cSocketFunctionSignature(func_hash)));

    // -- clear the existing function args
    found = m_socketExecFunctionList.find(func_hash);
    for (int i = 0; i < kMaxRegisteredParameterCount; ++i)
        found->second.m_paramTypes[i] = TinScript::eVarType::TYPE_NULL;

    // -- set the arg types we received
    for (int i = 0; i < arg_count; ++i)
    {
        int32 arg_type_int = (int32)*dataptr++;
        if (arg_type_int < 0 || arg_type_int >= (int32)TinScript::eVarType::TYPE_COUNT)
            return (false);

        // -- set the type
        found->second.m_paramTypes[i] = (TinScript::eVarType)arg_type_int;
    }

    // -- success
    return (true);
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
	    // $$$TZA TEST ME!
        // -- set up the pointers to fill in either the header, or the data of a packet
        bool processHeader = (mRecvPacket == NULL);
        char* basePtr = (processHeader) ? mRecvHeader : mRecvPacket->mData;
        int bufferSize = (processHeader) ? tPacketHeader::HeaderSize : mRecvPacket->mHeader.mSize;
        int bytesRequired = bufferSize - kPointerDiffUInt32(mRecvPtr, basePtr);

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

} // namespace SocketManager

// ====================================================================================================================
// -- TinScript Registration
REGISTER_FUNCTION(SocketListen, SocketManager::Listen);
REGISTER_FUNCTION(SocketIsListening, SocketManager::IsListening);
REGISTER_FUNCTION(SocketConnect, SocketManager::Connect);
REGISTER_FUNCTION(SocketDisconnect, SocketManager::Disconnect);
REGISTER_FUNCTION(SocketIsConnected, SocketManager::IsConnected);
REGISTER_FUNCTION(SocketSend, SocketManager::SendCommand);
REGISTER_FUNCTION(SocketExec, SocketManager::SendExec);

// ====================================================================================================================
// EOF
// ====================================================================================================================
