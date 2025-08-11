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
#include <conio.h>
#include "RingBuffer.h"
#include "CustomProfiler.h"

using namespace std;

#define PORT 9000
#define BUFSIZE 500

class Session;

enum
{
    RECV=0,
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

    RingBuffer recvQ;
    RingBuffer sendQ;

    myOverlapped recvMyOverlapped;
    myOverlapped sendMyOverlapped;
};

/**************************************************************/

// 네트워크 전역변수
DWORD wsaStartupRet;
DWORD bindRet;
DWORD listenRet;
DWORD setSockOptRet;

SOCKET listenSocket;

list<Session*> sessionList;

LONG sendCheck = TRUE;

// IOCP
HANDLE hIOCP;

// 쓰레드 핸들
HANDLE hAcceptThread;
vector<HANDLE> workerThreadHandles;

// SRWLock
SRWLOCK sessionListLock;

// Send 1회로 제한
// TRUE -> send 호출 가능
// FALSE -> send 호출 불가능.
LONG checkSend = TRUE;

// 함수 선언
void Init();
void AcceptThread();
void WorkerThread();

int main()
{
    timeBeginPeriod(1);
    
    // 세션리스트 락 생성
    InitializeSRWLock(&sessionListLock);

    // IOCP 생성
    hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);
    if (hIOCP == NULL)
        return 0;

    // CPU 갯수 확인
    SYSTEM_INFO si;
    GetSystemInfo(&si);

    HANDLE hThread;
    for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; i++)
    {
        hThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&WorkerThread, NULL, NULL, NULL);
        workerThreadHandles.push_back(hThread);
    }

    Init();
    hAcceptThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&AcceptThread, NULL, NULL, NULL);

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

    SOCKADDR_IN serverAddr;
    memset(&serverAddr, 0, sizeof(SOCKADDR_IN));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

    int optVal = 0;
    setSockOptRet = setsockopt(listenSocket, SOL_SOCKET, SO_SNDBUF, (char*)&optVal, sizeof(optVal));
    if (setSockOptRet == SOCKET_ERROR)
    {
        printf("ERROR: setsockopt() %d\n", GetLastError());
        return;
    }

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

        // 세션 생성
        Session* newSession = new Session;

        newSession->sock = clientSocket;

        memset(&(newSession->recvMyOverlapped.overlapped), 0, sizeof(WSAOVERLAPPED));
        newSession->recvMyOverlapped.pSession = newSession;
        newSession->recvMyOverlapped.type = RECV;
        
        newSession->recvQ.Resize(20000);

        memset(&(newSession->sendMyOverlapped.overlapped), 0, sizeof(WSAOVERLAPPED));
        newSession->sendMyOverlapped.pSession = newSession;
        newSession->sendMyOverlapped.type = SEND;

        newSession->sendQ.Resize(20000);

        AcquireSRWLockExclusive(&sessionListLock);
        sessionList.push_back(newSession);
        ReleaseSRWLockExclusive(&sessionListLock);

        getpeername(clientSocket, (SOCKADDR*)&clientAddr, &clientAddrSize);
        WCHAR addrBuf[40];
        InetNtop(AF_INET, &clientAddr.sin_addr, addrBuf, 40);
        printf("\n[TCP 서버] 클라이언트 접속: IP주소=%ls, 포트번호=%d\n", addrBuf, ntohs(clientAddr.sin_port));

        // 소켓 <-> IOCP 연결
        CreateIoCompletionPort((HANDLE)newSession->sock, hIOCP, (ULONG_PTR)newSession, NULL);

        // 비동기 IO 걸어두기
        DWORD recvBytes;
        DWORD flags = 0;

        WSABUF wsaBuf;
        wsaBuf.buf = newSession->recvQ.GetRearBufferPtr();
        wsaBuf.len = newSession->recvQ.DirectEnqueueSize();

        DWORD wsaRecvRet = WSARecv(newSession->sock, &wsaBuf, 1, &recvBytes, &flags, (LPWSAOVERLAPPED) & (newSession->recvMyOverlapped), NULL);
        if (wsaRecvRet == SOCKET_ERROR)
        {
            if (WSAGetLastError() != ERROR_IO_PENDING)
            {
                printf("Error: WSARecv() %d\n", WSAGetLastError());
                return;
            }
        }
    }
}

void WorkerThread()
{
    while (1)
    {
        // 비동기 IO 완료통지 대기
        DWORD cbTransferred = NULL;
        Session* pSession = NULL;
        myOverlapped* pMyOverlapped;

        SOCKADDR_IN clientAddr;
        int addrLen = sizeof(clientAddr);
        WCHAR addrBuf[40];

        BOOL retVal = GetQueuedCompletionStatus(hIOCP, &cbTransferred, (PULONG_PTR)&pSession, (LPWSAOVERLAPPED*)&pMyOverlapped, INFINITE);
        if (pMyOverlapped == NULL)
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

            printf("\n[TCP 서버] 클라이언트 종료: IP주소=%ls, 포트번호=%d\n", addrBuf, ntohs(clientAddr.sin_port));

            closesocket(pSession->sock);

            AcquireSRWLockExclusive(&sessionListLock);
            sessionList.erase(find(sessionList.begin(), sessionList.end(), pSession));
            ReleaseSRWLockExclusive(&sessionListLock);

            continue;
        }

        if (pMyOverlapped->type == RECV)
        {
            pSession->recvQ.MoveRear(cbTransferred);

            char strBuf[BUFSIZE + 1];
            int ret = pSession->recvQ.Dequeue(strBuf, BUFSIZE);
            strBuf[ret] = NULL;

            getpeername(pSession->sock, (SOCKADDR*)&clientAddr, &addrLen);
            InetNtop(AF_INET, &clientAddr.sin_addr, addrBuf, 40);
            printf("[TCP/%ls: %d] %s\n", addrBuf, ntohs(clientAddr.sin_port), strBuf);

            // WSASend
            pSession->sendQ.Enqueue(strBuf, ret);

            if (InterlockedExchange(&checkSend, FALSE) == TRUE)
            {
                DWORD sendBytes;

                WSABUF wsaBuf;
                wsaBuf.buf = pSession->sendQ.GetFrontBufferPtr();
                wsaBuf.len = pSession->sendQ.DirectDequeueSize();

                DWORD wsaSendRet = WSASend(pSession->sock, &wsaBuf, 1, &sendBytes, 0, (LPWSAOVERLAPPED)&pSession->sendMyOverlapped, NULL);

                if (wsaSendRet == SOCKET_ERROR)
                {
                    if (WSAGetLastError() != ERROR_IO_PENDING)
                    {
                        printf("Error: WSASend() %d\n", WSAGetLastError());
                        return;
                    }
                }
            }

            // WSARecv
            DWORD sendBytes2;
            DWORD flags = 0;

            WSABUF wsaBuf2;
            wsaBuf2.buf = pSession->recvQ.GetRearBufferPtr();
            wsaBuf2.len = pSession->recvQ.DirectEnqueueSize();

            DWORD wsaRecvRet = WSARecv(pSession->sock, &wsaBuf2, 1, &sendBytes2, &flags, (LPWSAOVERLAPPED) &pSession->recvMyOverlapped, NULL);
            if (wsaRecvRet == SOCKET_ERROR)
            {
                if (WSAGetLastError() != ERROR_IO_PENDING)
                {
                    printf("Error: WSARecv() %d\n", WSAGetLastError());
                    return;
                }
            }
        }

        if (pMyOverlapped->type == SEND)
        {
            pSession->sendQ.MoveFront(cbTransferred);
            
            if (pSession->sendQ.GetUseSize() > 0)
            {
                DWORD sendBytes;

                WSABUF wsaBuf;
                wsaBuf.buf = pSession->sendQ.GetFrontBufferPtr();
                wsaBuf.len = pSession->sendQ.DirectDequeueSize();

                DWORD wsaSendRet = WSASend(pSession->sock, &wsaBuf, 1, &sendBytes, 0, (LPWSAOVERLAPPED)&pSession->sendMyOverlapped, NULL);

                if (wsaSendRet == SOCKET_ERROR)
                {
                    if (WSAGetLastError() != ERROR_IO_PENDING)
                    {
                        printf("Error: WSASend() %d\n", WSAGetLastError());

                        return;
                    }
                }
            }
            else
            {
                InterlockedExchange(&checkSend, TRUE);
            }

        }
    }
}
