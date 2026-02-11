#pragma once

class Player;
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
    // =================================================
    // 🔥 [CACHE LINE 0] : Echo / TPS Hot Path
    // =================================================

    Player* pPlayer;        // 8
    INT64   accountNo;      // 8

    // padding → 정확히 64B 맞춤
    char    _pad00[64 - (
        sizeof(Player*) +
        sizeof(INT64)
        )];

    // =================================================
    // [CACHE LINE 1] : Connection / State (IO, Send)
    // =================================================

    DWORD64 sessionID;      // 8

    LONG    checkSend;      // 4
    LONG    disconnectNotified; // 4
    LONG    cancelIOCheck;  // 4

    IOReleasePair IOCountNReleaseCheck; // game

    char    _pad0[64 - (
        sizeof(DWORD64) +
        sizeof(LONG) * 3 +
        sizeof(IOReleasePair)
        )];

    // =================================================
    // [CACHE LINE 2] : Socket / Overlapped (IOCP)
    // =================================================

    SOCKET sock;

    myOverlapped recvMyOverlapped;
    myOverlapped sendMyOverlapped;

    char    _pad1[(64 - (
        sizeof(SOCKET) +
        sizeof(myOverlapped) * 2
        )) & 63];

    // =================================================
    // [CACHE LINE 3+] : Buffers / Queues (COLD)
    // =================================================

    RingBuffer recvQ;
    LockFreeQueue<RawPtr> LockFreeSendQ;

    char _pad2[(64 - (
        sizeof(RingBuffer) +
        sizeof(LockFreeQueue<RawPtr>)
        )) & 63];

    // =================================================
    // [CACHE LINE N] : Flags / Content (VERY COLD)
    // =================================================

    bool loginCheck;
    bool releaseWait;

    char _pad3[64 - 2];

    // =================================================
    // game (coldest)
    // =================================================

    RingBuffer contentMsgQ;
};