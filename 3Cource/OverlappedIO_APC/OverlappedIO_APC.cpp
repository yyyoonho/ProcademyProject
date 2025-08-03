#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32")
#pragma comment(lib, "RingBuffer.lib")

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <iostream>
#include <thread>
#include <list>
#include <conio.h>
#include "RingBuffer.h"

using namespace std;

#define PORT 9000

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

/********************************************************/

// 네트워크 전역변수
DWORD wsaStartupRet;
DWORD bindRet;
DWORD listenRet;

SOCKET listenSocket;

RingBuffer socketQ;
list<Session*> sessionList;

// 쓰레드 핸들
HANDLE hAcceptThread;
HANDLE hWorkerThread;

// 이벤트 핸들
HANDLE hEvent_SocketQ;
HANDLE hEvent_Quit;


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

        socketQ.Enqueue((char*)&clientSocket, sizeof(SOCKET));
        SetEvent(hEvent_SocketQ);
    }
}

void CompletionRoutine(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags)
{
    myOverlapped* myOverlappedPtr = (myOverlapped*)lpOverlapped;
    Session* pSession = myOverlappedPtr->pSession;

    SOCKADDR_IN clientAddr;
    int addrLen = sizeof(clientAddr);
    getpeername(pSession->sock, (SOCKADDR*)&clientAddr, &addrLen);

    if (dwError != 0 || cbTransferred == 0)
    {
        closesocket(pSession->sock);
        WCHAR addrBuf[40];
        InetNtop(AF_INET, &clientAddr.sin_addr, addrBuf, 40);
        printf("\n[TCP 서버] 클라이언트 종료: IP주소=%ls, 포트번호=%d\n", addrBuf, ntohs(clientAddr.sin_port));

        return;
    }

    if (myOverlappedPtr->type == RECV)
    {
        pSession->recvQ.MoveRear(cbTransferred);

        WCHAR addrBuf[40];
        InetNtop(AF_INET, &clientAddr.sin_addr, addrBuf, 40);

        char buf[1000 + 1];
        memset(buf, 0, 1000 + 1);

        int deQueueSize = pSession->recvQ.Dequeue(buf, 1000);

        printf("[TCP/%ls: %d] %s\n", addrBuf, ntohs(clientAddr.sin_port), buf);

        pSession->sendQ.Enqueue(buf, deQueueSize);

        // WSASend
        DWORD sendBytes;
        DWORD flags = 0;

        WSABUF wsaBuf;
        wsaBuf.buf = pSession->sendQ.GetFrontBufferPtr();
        wsaBuf.len = cbTransferred;

        DWORD wsaSendRet = WSASend(pSession->sock, &wsaBuf, 1, NULL, 0, (LPWSAOVERLAPPED)&pSession->sendMyOverlapped, CompletionRoutine);
        if (wsaSendRet == SOCKET_ERROR)
        {
            if (WSAGetLastError() != WSA_IO_PENDING)
            {
                printf("ERROR: WSARecv() %d\n", WSAGetLastError());
                return;
            }

            
        }
        printf("wsaSendRet : %d\n", wsaSendRet);

        // WSARecv
        DWORD recvBytes;

        WSABUF wsaBuf2;
        wsaBuf2.buf = pSession->recvQ.GetRearBufferPtr();
        wsaBuf2.len = pSession->recvQ.GetFreeSize();

        DWORD wsaRecvRet = WSARecv(pSession->sock, &wsaBuf2, 1, &recvBytes, &flags, (LPWSAOVERLAPPED)&pSession->recvMyOverlapped, CompletionRoutine);
        if (wsaRecvRet == SOCKET_ERROR)
        {
            if (WSAGetLastError() != WSA_IO_PENDING)
            {
                printf("ERROR: WSARecv() %d\n", WSAGetLastError());
                return;
            }

            
        }
        printf("wsaRecvRet : %d\n", wsaRecvRet);
    }

    if (myOverlappedPtr->type == SEND)
    {
        pSession->sendQ.MoveFront(cbTransferred);
    }
}

void WorkerThread()
{
    HANDLE handles[2] = { hEvent_Quit, hEvent_SocketQ };

    while (1)
    {
        while (1)
        {
            DWORD ret = WaitForMultipleObjectsEx(2, handles, FALSE, INFINITE, TRUE);
            if (ret == WAIT_OBJECT_0)
                return;
            if (ret == WAIT_OBJECT_0 + 1)
                break;
            if (ret == WAIT_IO_COMPLETION)
                continue;
        }

        // 세션 생성
        SOCKET clientSocket;
        while (socketQ.GetUseSize() >= sizeof(SOCKET))
        {
            socketQ.Dequeue((char*)&clientSocket, sizeof(SOCKET));

            Session* newSession = new Session;
            newSession->sock = clientSocket;

            memset(&(newSession->recvMyOverlapped.overlapped), 0, sizeof(WSAOVERLAPPED));
            newSession->recvMyOverlapped.pSession = newSession;
            newSession->recvMyOverlapped.type = RECV;

            memset(&(newSession->sendMyOverlapped.overlapped), 0, sizeof(WSAOVERLAPPED));
            newSession->sendMyOverlapped.pSession = newSession;
            newSession->sendMyOverlapped.type = SEND;

            sessionList.push_back(newSession);

            SOCKADDR_IN clientAddr;
            int addrLen = sizeof(clientAddr);
            getpeername(clientSocket, (SOCKADDR*)&clientAddr, &addrLen);
            WCHAR addrBuf[40];
            InetNtop(AF_INET, &clientAddr.sin_addr, addrBuf, 40);
            printf("\n[TCP 서버] 클라이언트 접속: IP주소=%ls, 포트번호=%d\n", addrBuf, ntohs(clientAddr.sin_port));

            // 비동기 IO시작
            WSABUF wsaBuf;
            wsaBuf.buf = newSession->recvQ.GetRearBufferPtr();
            wsaBuf.len = newSession->recvQ.GetFreeSize();

            DWORD flags = 0;

            DWORD ret = WSARecv(clientSocket, &wsaBuf, 1, NULL, &flags, (LPWSAOVERLAPPED) & (newSession->recvMyOverlapped), CompletionRoutine);
            if (ret == SOCKET_ERROR)
            {
                if (WSAGetLastError() != WSA_IO_PENDING)
                {
                    printf("Error: WSARecv()\n");
                    return;
                }
            }
        }
    }
}

int main()
{
    timeBeginPeriod(1);

    hEvent_SocketQ = CreateEvent(NULL, FALSE, FALSE, NULL);
    hEvent_Quit = CreateEvent(NULL, TRUE, FALSE, NULL);

    Init();
    hAcceptThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&AcceptThread, NULL, NULL, NULL);
    hWorkerThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&WorkerThread, NULL, NULL, NULL);

    HANDLE hThreads[2] = { hAcceptThread ,hWorkerThread };

    while (1)
    {
        char inputKey = _getch();
        if (inputKey == 'Q' || inputKey == 'q')
        {
            SetEvent(hEvent_Quit);
            break;
        }
    }

    WaitForMultipleObjects(2, hThreads, TRUE, INFINITE);

    return 0;
}