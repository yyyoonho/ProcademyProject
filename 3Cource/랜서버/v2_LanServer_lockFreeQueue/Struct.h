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

struct Session
{
    SOCKET sock;

    myOverlapped recvMyOverlapped;
    myOverlapped sendMyOverlapped;

    RingBuffer recvQ;
    //LockFreeQueue<SerializePacketPtr> LockFreeSendQ; // Now This sendQ is Q for SerializeBuffer Pointer.
    LockFreeQueue<RawPtr> LockFreeSendQ; // Now This sendQ is Q for SerializeBuffer Pointer.

    LONG IO_Count = 0;

    DWORD64 sessionID;

    // Send 1회로 제한
    // TRUE -> send 호출 가능
    // FALSE -> send 호출 불가능.
    LONG checkSend = TRUE;

    bool loginCheck;

    //SRWLOCK sendQLock;
};
