#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "RingBuffer.lib")

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <iostream>
#include <thread>
#include <conio.h>
#include <vector>
#include <list>
#include <unordered_map>
#include <algorithm>

#include "RingBuffer.h"

using namespace std;

#define SERVERPORT 6000
#define BUFSIZE 512
#define HEADERSIZE 2

struct Session;

struct stHeader
{
    short len;
};

struct stPayload
{
    long long data;
};

enum
{
    RECV,
    SEND,
};

struct MyOverlapped
{
    OVERLAPPED overlapped;
    int type;
    Session* pSession;
};

struct Session
{
    SOCKET sock;
    DWORD64 sessionId;

    RingBuffer recvQ;
    RingBuffer sendQ;

    MyOverlapped recvOverlapped;
    MyOverlapped sendOverlapped;

    LONG sendFlag;  // true -> 획득가능, false -> 획득불가능

    LONG IOCount = 0;
};

// 리턴 체크용 전역변수
int bindRet;
int listenRet;
int ioctlRet;
int setSockOptRet;
int wsaRecvRet;
int wsaSendRet;

int acceptTotal;

// 함수 전방선언
void NetInit();
void WorkerThread();
void AcceptThread();
bool RequestWSARecv(Session* pSession);
bool RequestWSASend(Session* pSession);
void DestroySession(Session* pSession);

// 핸들
HANDLE hIOCP;
vector<HANDLE> hWorkerThreads;
SOCKET listenSocket;
SRWLOCK srwLock;
HANDLE hAcceptThread;

// 세션MAP
unordered_map<DWORD64, Session*> sessionMap;
DWORD64 g_SessionId;

int main()
{
    timeBeginPeriod(1);

    InitializeSRWLock(&srwLock);

    NetInit();

    // Accept 쓰레드 생성
    hAcceptThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&AcceptThread, NULL, NULL, NULL);
    if (hAcceptThread == NULL)
    {
        printf("ERROR: hAcceptThread %d\n", WSAGetLastError());
        return 0;
    }

    // IOCP 생성
    hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 6);
    if (hIOCP == NULL)
    {
        printf("ERROR: CreateIoCompletionPort() %d\n", WSAGetLastError());
        return 0;
    }

    // CPU 갯수 확인
    SYSTEM_INFO si;
    GetSystemInfo(&si);

    // 워커쓰레드 생성
    HANDLE hThread;
    for (int i = 0; i < (int)si.dwNumberOfProcessors; i++)
    //for (int i = 0; i < 1; i++)
    {
        hThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&WorkerThread, NULL, NULL, NULL);
        hWorkerThreads.push_back(hThread);
    }

    while (1)
    {
        if (_kbhit())
        {
            char inputKey = _getch();
            if (inputKey == 'Q' || inputKey == 'q')
            {
                closesocket(listenSocket);
                
                PostQueuedCompletionStatus(hIOCP, NULL, NULL, NULL);

                break;
            }
        }
    }

    WaitForSingleObject(hAcceptThread, INFINITE);
    WaitForMultipleObjects(hWorkerThreads.size(), hWorkerThreads.data(), TRUE, INFINITE);
    WSACleanup();
}

void NetInit()
{
    WSADATA wsaData;
    int wsaDataRet = WSAStartup(MAKEWORD(2, 2), &wsaData);

    // listen socket 생성
    listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == INVALID_SOCKET)
    {
        printf("ERROR: socket() %d\n", WSAGetLastError());
        return;
    }

    // 소켓 옵션 설정
    int optVal = 0;
    setsockopt(listenSocket, SOL_SOCKET, SO_SNDBUF, (char*)&optVal, sizeof(optVal));
    if (setSockOptRet == SOCKET_ERROR)
    {
        printf("ERROR: setsockopt() %d\n", GetLastError());
        return ;
    }

    // bind
    SOCKADDR_IN serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(SERVERPORT);

    bindRet = bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
    if (bindRet == SOCKET_ERROR)
    {
        printf("ERROR: bind() %d\n", GetLastError());
        return;
    }

    // listen
    listenRet = listen(listenSocket, SOMAXCONN);
    if (listenRet == SOCKET_ERROR)
    {
        printf("ERROR: listen() %d\n", GetLastError());
        return ;
    }
}

void WorkerThread()
{
    while (1)
    {
        // GQCS() OUT파라메터 초기화
        DWORD cbTransferred = 0;
        Session* pSession = NULL;
        OVERLAPPED* pOverlapped;

        // GQCS()
        int retVal = GetQueuedCompletionStatus(hIOCP, &cbTransferred, (PULONG_PTR)&pSession, &pOverlapped, INFINITE);
        if (cbTransferred == NULL && pSession == NULL && pOverlapped == NULL)
        {
            printf("\n[TCP 서버] IOCP 워커쓰레드 종료...\n");
            PostQueuedCompletionStatus(hIOCP, NULL, NULL, NULL);
            return;
        }
        if (cbTransferred == 0)
        {
            // TODO: 연결종료를 위한 something

        }

        else if (((MyOverlapped*)pOverlapped)->type == RECV)
        {
            bool flag = true;
            pSession->recvQ.MoveRear(cbTransferred);

            while (1)
            {
                if (pSession->recvQ.GetUseSize() < HEADERSIZE)
                {
                    // WSARecv
                    RequestWSARecv(pSession);
                    flag = false;
                    break;
                }

                char headerBuf[100 + 1];
                char payloadBuf[100 + 1];

                pSession->recvQ.Peek(headerBuf, HEADERSIZE);
                short payLoadLen = ((stHeader*)headerBuf)->len;

                if (pSession->recvQ.GetUseSize() < HEADERSIZE + payLoadLen)
                {
                    // WSARecv
                    RequestWSARecv(pSession);
                        
                    flag = false;
                    break;
                }

                pSession->recvQ.Dequeue(headerBuf, HEADERSIZE);
                pSession->recvQ.Dequeue(payloadBuf, payLoadLen);

                // WSASend
                pSession->sendQ.Enqueue(headerBuf, HEADERSIZE);
                pSession->sendQ.Enqueue(payloadBuf, payLoadLen);
                if (InterlockedExchange(&pSession->sendFlag, false) == true)
                {
                    RequestWSASend(pSession);
                }

            }

            // WSARecv
            if (flag == true)
            {
                RequestWSARecv(pSession);
            }

            // ********
            if (InterlockedDecrement(&pSession->IOCount) == 0)
            {
                DestroySession(pSession);
            }

        }
        else if (((MyOverlapped*)pOverlapped)->type == SEND)
        {
            pSession->sendQ.MoveFront(cbTransferred);

            if (pSession->sendQ.GetUseSize() > 0)
            {
                RequestWSASend(pSession);

                // ********
                if (InterlockedDecrement(&pSession->IOCount) == 0)
                {
                    DestroySession(pSession);
                }
            }
            else
            {
                InterlockedExchange(&pSession->sendFlag, true);
            }
        }

    }

    return;
}

void AcceptThread()
{
    while (1)
    {
        // accept
        SOCKADDR_IN clientAddr;
        memset(&clientAddr, 0, sizeof(clientAddr));
        int addrLen = sizeof(clientAddr);

        SOCKET clientSocket = accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
        if (clientSocket == INVALID_SOCKET)
        {
            if (WSAGetLastError() == WSAEINTR)
            {
                printf("Accept Thread 종료...\n");
                return;
            }
            else
            {
                printf("Error: accept() %d\n", WSAGetLastError());
                return;
            }
        }

        acceptTotal++;
        //printf("AcceptTotal: %d\n", acceptTotal);

        getpeername(clientSocket, (SOCKADDR*)&clientAddr, &addrLen);
        WCHAR addrBuf[40];
        InetNtop(AF_INET, &clientAddr.sin_addr, addrBuf, 40);
        //printf("\n[TCP 서버] 클라이언트 접속: IP주소=%ls, 포트번호=%d\n", addrBuf, ntohs(clientAddr.sin_port));

        // 세션 생성
        Session* newSession = new Session;

        newSession->sock = clientSocket;
        newSession->sessionId = g_SessionId++;

        newSession->recvQ.Resize(10000);
        newSession->sendQ.Resize(10000);

        memset(&newSession->recvOverlapped.overlapped, 0, sizeof(OVERLAPPED));
        newSession->recvOverlapped.pSession = newSession;
        newSession->recvOverlapped.type = RECV;

        memset(&newSession->sendOverlapped.overlapped, 0, sizeof(OVERLAPPED));
        newSession->sendOverlapped.pSession = newSession;
        newSession->sendOverlapped.type = SEND;

        newSession->sendFlag = true;

        AcquireSRWLockExclusive(&srwLock);
        sessionMap.insert({ newSession->sessionId, newSession });
        ReleaseSRWLockExclusive(&srwLock);

        // 소켓 - IOCP 연결
        CreateIoCompletionPort((HANDLE)clientSocket, hIOCP, (ULONG_PTR)newSession, 0);

        // 비동기 Recv 걸어버리기
        bool ret = RequestWSARecv(newSession);
        if (ret == false)
            return;
    }
}

bool RequestWSARecv(Session* pSession)
{
    DWORD recvBytes;
    DWORD flags = 0;

    WSABUF wsaBuf;
    wsaBuf.buf = pSession->recvQ.GetRearBufferPtr();
    wsaBuf.len = pSession->recvQ.DirectEnqueueSize();

    wsaRecvRet = WSARecv(pSession->sock, &wsaBuf, 1, &recvBytes, &flags, (LPWSAOVERLAPPED)&pSession->recvOverlapped, NULL);
    if (wsaRecvRet == SOCKET_ERROR)
    {
        if (WSAGetLastError() != WSA_IO_PENDING)
        {
            printf("ERROR: WSARecv() %d\n", WSAGetLastError());
            // TODO: 연결종료를 위한 something

            return false;
        }
    }

    InterlockedIncrement(&pSession->IOCount);
    return true;
}

bool RequestWSASend(Session* pSession)
{
    DWORD sendBytes;

    WSABUF wsaBuf;
    wsaBuf.buf = pSession->sendQ.GetFrontBufferPtr();
    wsaBuf.len = pSession->sendQ.DirectDequeueSize();

    wsaSendRet = WSASend(pSession->sock, &wsaBuf, 1, &sendBytes, 0, (LPWSAOVERLAPPED)&pSession->sendOverlapped, NULL);
    if (wsaSendRet == SOCKET_ERROR)
    {
        if (WSAGetLastError() != WSA_IO_PENDING)
        {
            printf("ERROR: WSASend() %d\n", WSAGetLastError());
            // TODO: 연결종료를 위한 something

            return false;
        }
    }

    InterlockedIncrement(&pSession->IOCount);
    return true;
}

void DestroySession(Session* pSession)
{
    AcquireSRWLockExclusive(&srwLock);
    sessionMap.erase(pSession->sessionId);
    ReleaseSRWLockExclusive(&srwLock);

    closesocket(pSession->sock);

    delete pSession;

    return;
}
