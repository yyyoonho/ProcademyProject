#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "RingBuffer.lib")
#pragma comment(lib, "CustomProfiler.lib")

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <iostream>
#include <thread>
#include <list>
#include <vector>
#include <map>
#include <unordered_map>
#include <conio.h>
#include "RingBuffer.h"
#include "CustomProfiler.h"

#include "protocol.h"

using namespace std;

#define PORT 6000
#define BUFSIZE 10

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
};

struct Session
{
    SOCKET sock;

    myOverlapped recvMyOverlapped;
    myOverlapped sendMyOverlapped;

    RingBuffer recvQ;
    RingBuffer sendQ;

    LONG IO_Count = 0;

    DWORD64 sessionID;

    // Send 1회로 제한
    // TRUE -> send 호출 가능
    // FALSE -> send 호출 불가능.
    LONG checkSend = TRUE;

    SRWLOCK sessionLock;

};

/**************************************************************/

// 네트워크 전역변수
DWORD wsaStartupRet;
DWORD bindRet;
DWORD listenRet;
DWORD setSockOptRet;

SOCKET listenSocket;

DWORD64 g_sessionID = 0;
unordered_map<DWORD64, Session*> sessionMap;

LONG sendCheck = TRUE;

// IOCP
HANDLE hIOCP;

// 쓰레드 핸들
HANDLE hAcceptThread;
vector<HANDLE> workerThreadHandles;

// SRWLock
SRWLOCK sessionMapLock;

// 함수 선언
void Init();
void AcceptThread();
void WorkerThread();

void RecvPost(Session* pSession);
void SendPost(Session* pSession);

void OnMessage(DWORD sessionID, stMessage msg);
void SendPacket(DWORD sessionID, stMessage msg);

void IncreaseIO_Count(Session* pSession);
void DecreaseIO_Count(Session* pSession);

int main()
{
    timeBeginPeriod(1);

    // sessionMap 락 생성
    InitializeSRWLock(&sessionMapLock);

    Init();
    hAcceptThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&AcceptThread, NULL, NULL, NULL);

    // IOCP 생성
    hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);
    if (hIOCP == NULL)
        return 0;

    // CPU 갯수 확인
    SYSTEM_INFO si;
    GetSystemInfo(&si);

    HANDLE hThread;
    //for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; i++)
    for (int i = 0; i < 2; i++)
    {
        hThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&WorkerThread, NULL, NULL, NULL);
        workerThreadHandles.push_back(hThread);
    }

    while (1)
    {
        char inputKey = _getch();
        if (inputKey == 'Q' || inputKey == 'q')
        {
            closesocket(listenSocket);
            PostQueuedCompletionStatus(hIOCP, NULL, NULL, NULL);

            break;
        }

        ProfilerInput();
    }

    WaitForMultipleObjects(workerThreadHandles.size(), workerThreadHandles.data(), TRUE, INFINITE);
    WaitForSingleObject(hAcceptThread, INFINITE);

    return 0;
}


void Init()
{
    WSADATA wsaData;
    wsaStartupRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaStartupRet != 0)
        return;

    listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == INVALID_SOCKET)
    {
        printf("Error: listen socket()\n");
        return;
    }

    int optVal = 0;
    setSockOptRet = setsockopt(listenSocket, SOL_SOCKET, SO_SNDBUF, (char*)&optVal, sizeof(optVal));
    if (setSockOptRet == SOCKET_ERROR)
    {
        printf("ERROR: setsockopt() %d\n", GetLastError());
        return;
    }

    SOCKADDR_IN serverAddr;
    memset(&serverAddr, 0, sizeof(SOCKADDR_IN));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

    bindRet = bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
    if (bindRet == SOCKET_ERROR)
    {
        printf("Error: bind()\n");
        return;
    }

    listenRet = listen(listenSocket, SOMAXCONN);
    if (listenRet == SOCKET_ERROR)
    {
        printf("Error: listen()\n");
        return;
    }
}

void AcceptThread()
{
    while (1)
    {
        SOCKADDR_IN clientAddr;
        memset(&clientAddr, 0, sizeof(SOCKADDR_IN));
        int clientAddrSize = sizeof(clientAddr);

        SOCKET clientSocket = accept(listenSocket, (SOCKADDR*)&clientAddr, &clientAddrSize);
        if (clientSocket == INVALID_SOCKET)
        {
            printf("Error: accept() %d\n", WSAGetLastError());
            return;
        }

        // 세션 생성 및 세팅
        Session* newSession = new Session;

        newSession->sock = clientSocket;
        newSession->sessionID = ++g_sessionID;

        memset(&(newSession->recvMyOverlapped.overlapped), 0, sizeof(WSAOVERLAPPED));
        newSession->recvMyOverlapped.pSession = newSession;
        newSession->recvMyOverlapped.type = RECV;

        newSession->recvQ.Resize(20000);

        memset(&(newSession->sendMyOverlapped.overlapped), 0, sizeof(WSAOVERLAPPED));
        newSession->sendMyOverlapped.pSession = newSession;
        newSession->sendMyOverlapped.type = SEND;

        newSession->sendQ.Resize(20000);

        InitializeSRWLock(&newSession->sessionLock);

        AcquireSRWLockExclusive(&sessionMapLock);
        sessionMap.insert({ newSession->sessionID, newSession });
        ReleaseSRWLockExclusive(&sessionMapLock);

        getpeername(clientSocket, (SOCKADDR*)&clientAddr, &clientAddrSize);
        WCHAR addrBuf[40];
        InetNtop(AF_INET, &clientAddr.sin_addr, addrBuf, 40);
        //printf("\n[TCP 서버] 클라이언트 접속: IP주소=%ls, 포트번호=%d\n", addrBuf, ntohs(clientAddr.sin_port));

        // 소켓 <-> IOCP 연결
        CreateIoCompletionPort((HANDLE)newSession->sock, hIOCP, (ULONG_PTR)newSession, NULL);

        // 비동기 IO 걸어두기
        RecvPost(newSession);
    }
}

void WorkerThread()
{
    while (1)
    {
        // 비동기 IO 완료통지 대기
        DWORD cbTransferred = NULL;
        Session* pSession = NULL;
        myOverlapped* pMyOverlapped = NULL;

        SOCKADDR_IN clientAddr;
        int addrLen = sizeof(clientAddr);
        WCHAR addrBuf[40];

        BOOL retVal = GetQueuedCompletionStatus(hIOCP, &cbTransferred, (PULONG_PTR)&pSession, (LPWSAOVERLAPPED*)&pMyOverlapped, INFINITE);
        if (pMyOverlapped == NULL && cbTransferred == NULL && pSession == NULL)
        {
            // 종료통지 or IOCP 결함
            // => 워커쓰레드 종료.
            PostQueuedCompletionStatus(hIOCP, NULL, NULL, NULL);

            return;
        }

        if (cbTransferred == 0)
        {
            // 클라에서 FIN or RST 를 던졌다.
            getpeername(pSession->sock, (SOCKADDR*)&clientAddr, &addrLen);
            InetNtop(AF_INET, &clientAddr.sin_addr, addrBuf, 40);

            //printf("\n[TCP 서버] 클라이언트 종료신호 (FIN or SRT): IP주소=%ls, 포트번호=%d\n", addrBuf, ntohs(clientAddr.sin_port));
        }

        else if (pMyOverlapped->type == RECV)
        {
            pSession->recvQ.MoveRear(cbTransferred);

            while (1)
            {
                stHeader header;
                stMessage msg;

                if (pSession->recvQ.GetUseSize() < sizeof(stHeader))
                {
                    // 비동기IO: Recv 요청 후 break
                    RecvPost(pSession);
                    break;
                }

                pSession->recvQ.Peek((char*)&header, sizeof(stHeader));
                short payLoadLen = header.len;

                if (pSession->recvQ.GetUseSize() < sizeof(stHeader) + payLoadLen)
                {
                    // 비동기IO: Recv 요청 후 break
                    RecvPost(pSession);
                    break;
                }

                pSession->recvQ.MoveFront(sizeof(stHeader));
                pSession->recvQ.Dequeue((char*)&msg, payLoadLen);

                // 비동기IO: Send 요청
                // 이제 컨텐츠 단에서 요청
                OnMessage(pSession->sessionID, msg);
            }
        }

        else if (pMyOverlapped->type == SEND)
        {
            pSession->sendQ.MoveFront(cbTransferred);

            AcquireSRWLockExclusive(&pSession->sessionLock);

            // 비동기IO: Send 요청
            // 락도 걸었다.
            SendPost(pSession);

            ReleaseSRWLockExclusive(&pSession->sessionLock);
        }

        DecreaseIO_Count(pSession);
    }
}

void RecvPost(Session* pSession)
{
    DWORD sendBytes;
    DWORD flags = 0;

    WSABUF wsaBuf;
    wsaBuf.buf = pSession->recvQ.GetRearBufferPtr();
    wsaBuf.len = pSession->recvQ.DirectEnqueueSize();

    IncreaseIO_Count(pSession);

    DWORD wsaRecvRet = WSARecv(pSession->sock, &wsaBuf, 1, &sendBytes, &flags, (LPWSAOVERLAPPED)&pSession->recvMyOverlapped, NULL);
    if (wsaRecvRet == SOCKET_ERROR)
    {
        if (WSAGetLastError() != ERROR_IO_PENDING)
        {
            if (WSAGetLastError() != 0)
            {
                DecreaseIO_Count(pSession);
            }

            printf("Error: WSARecv() %d\n", WSAGetLastError());
            return;
        }
    }
}

void SendPost(Session* pSession)
{
    if (pSession->sendQ.DirectDequeueSize() > 0)
    {
        DWORD sendBytes;

        WSABUF wsaBuf;
        wsaBuf.buf = pSession->sendQ.GetFrontBufferPtr();
        wsaBuf.len = pSession->sendQ.DirectDequeueSize();

        // TODO: 나중에 세션에 대한 락이 없어지면 이렇게 해결해야할듯 싶다.
        /*if (wsaBuf.len == 0)
        {
            InterlockedExchange(&pSession->checkSend, TRUE);
            return;
        }*/

        IncreaseIO_Count(pSession);

        DWORD wsaSendRet = WSASend(pSession->sock, &wsaBuf, 1, &sendBytes, 0, (LPWSAOVERLAPPED)&pSession->sendMyOverlapped, NULL);
        DWORD tmp2 = WSAGetLastError();
        if (wsaSendRet == SOCKET_ERROR)
        {
            DWORD tmp = WSAGetLastError();

            if (WSAGetLastError() != ERROR_IO_PENDING && WSAGetLastError() != 0)
            {
                DecreaseIO_Count(pSession);

                printf("Error: WSASend() %d\n", WSAGetLastError());
                return;
            }
        }
    }
    else
    {
        InterlockedExchange(&pSession->checkSend, TRUE);
    }
}

void OnMessage(DWORD sessionID, stMessage msg)
{
    // 컨텐츠 코드
    SendPacket(sessionID, msg);

    return;
}

void SendPacket(DWORD sessionID, stMessage msg)
{
    Session* pSession = NULL;

    AcquireSRWLockShared(&sessionMapLock);

    unordered_map<DWORD64, Session*>::iterator iter;
    iter = sessionMap.find(sessionID);

    if (iter == sessionMap.end())
    {
        printf("Error: SendPacket() Can not find a session by ID(Key) in sessionMap\n");

        return;
    }

    pSession = iter->second;

    AcquireSRWLockExclusive(&pSession->sessionLock);

    ReleaseSRWLockShared(&sessionMapLock);


    stHeader header;
    header.len = sizeof(stMessage);

    pSession->sendQ.Enqueue((char*)&header, sizeof(stHeader));
    pSession->sendQ.Enqueue((char*)&msg, sizeof(stMessage));

    if (InterlockedExchange(&pSession->checkSend, false) == true)
    {
        SendPost(pSession);
    }
    ReleaseSRWLockExclusive(&pSession->sessionLock);

}

void IncreaseIO_Count(Session* pSession)
{
    InterlockedIncrement(&(pSession->IO_Count));
}

void DecreaseIO_Count(Session* pSession)
{
    LONG ret = InterlockedDecrement(&(pSession->IO_Count));
    if (ret == 0)
    {
        // Session Release
        //printf("[Session Release] session id: %d\n", pSession->sessionID);

        AcquireSRWLockExclusive(&sessionMapLock);

        sessionMap.erase(pSession->sessionID);

        ReleaseSRWLockExclusive(&sessionMapLock);

        AcquireSRWLockExclusive(&pSession->sessionLock);
        ReleaseSRWLockExclusive(&pSession->sessionLock);

        closesocket(pSession->sock);

        delete pSession;
    }
}