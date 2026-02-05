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
    DWORD64 sid;
    RawPtr r;
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

struct alignas(64) Session
{
    //=================================================

    DWORD64 sessionID;

    LONG checkSend = TRUE;
    LONG            disconnectNotified;
    LONG cancelIOCheck;

    IOReleasePair IOCountNReleaseCheck; // game

    char    _pad0[64 - (
        sizeof(DWORD64) +
        sizeof(LONG) * 3 +
        sizeof(IOReleasePair)
        )];

    //=================================================

    SOCKET sock;

    myOverlapped recvMyOverlapped;
    myOverlapped sendMyOverlapped;

    char    _pad1[64 - (
        sizeof(SOCKET) +
        sizeof(myOverlapped) * 2)
        % 64];

    //=================================================

    RingBuffer recvQ;
    LockFreeQueue<RawPtr> LockFreeSendQ; // Now This sendQ is Q for SerializeBuffer Pointer.

    char _pad2[64 - (
        sizeof(RingBuffer) +
        sizeof(LockFreeQueue<RawPtr>)
        ) % 64];

    //=================================================

    bool loginCheck;
    bool releaseWait;

    char        _pad3[62];

    //=================================================
    // game
    RingBuffer      contentMsgQ;
    
};
