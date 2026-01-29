#pragma once

class Session;

enum
{
    RECV = 0,
    SEND,
};

struct myOverlapped
{
    WSAOVERLAPPED overlapped;
    DWORD type;
    Session* pSession;

    RawPtr sendSerializePacketPtrArr[100];
    int sPacketCount;
};

struct alignas(8) IOReleasePair
{
    LONG IO_Count = 0;
    LONG releaseCheck;
};

struct SendPacketJob
{
    DWORD64 sessionID;
    SerializePacketPtr packet;
};

enum class FieldName
{
    None = 0,
    Auth,
    Echo,

    COUNT,
};

struct FieldMovePack
{
    FieldName fieldName;
    DWORD64 sessionID;
    void* pPlayer;
};

struct joinQContext
{
    Session* pSession;
    void* pPlayer;
};

struct Session
{
    SOCKET sock;

    myOverlapped recvMyOverlapped;
    myOverlapped sendMyOverlapped;

    RingBuffer recvQ;
    LockFreeQueue<RawPtr> LockFreeSendQ; // Now This sendQ is Q for SerializeBuffer Pointer.

    DWORD64 sessionID;

    // Send 1회로 제한
    // TRUE -> send 호출 가능
    // FALSE -> send 호출 불가능.
    LONG checkSend = TRUE;

    bool loginCheck;
    bool cancelIOCheck;

    IOReleasePair IOCountNReleaseCheck;


    // Game
    //LockFreeQueue<RawPtr> contentMsgQ;
    RingBuffer      contentMsgQ;
    bool            releaseWait;
    LONG            disconnectNotified;
};
