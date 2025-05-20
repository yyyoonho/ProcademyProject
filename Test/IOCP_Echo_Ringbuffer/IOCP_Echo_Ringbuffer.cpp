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

#include "RingBuffer.h"

using namespace std;

#define SERVERPORT 9000
#define BUFSIZE 512

struct Session;

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

    RingBuffer recvQ;
    RingBuffer sendQ;

    MyOverlapped recvOverlapped;
    MyOverlapped sendOverlapped;

    bool sendFlag;
};

// 리턴 체크용 전역변수
int bindRet;
int listenRet;
int ioctlRet;
int setSockOptRet;
int wsaRecvRet;
int wsaSendRet;

// 함수 전방선언
void NetInit();
void WorkerThread();

// 핸들
HANDLE hIOCP;
vector<HANDLE> hWorkerThreads;
SOCKET listenSocket;

list<Session*> sessionList;

int main()
{
    timeBeginPeriod(1);

    NetInit();

    // IOCP 생성
    hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
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
    for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; i++)
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
                // TODO: 종료메시지 던지기
                break;
            }
        }

        // accept
        SOCKADDR_IN clientAddr;
        memset(&clientAddr, 0, sizeof(clientAddr));
        int addrLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
        if (clientSocket == INVALID_SOCKET)
        {
            if (GetLastError() == WSAEWOULDBLOCK)
            {
                continue;
            }

            printf("Error: accept() %d\n", WSAGetLastError());
            return 0;
        }

        getpeername(clientSocket, (SOCKADDR*)&clientAddr, &addrLen);
        WCHAR addrBuf[40];
        InetNtop(AF_INET, &clientAddr.sin_addr, addrBuf, 40);
        printf("\n[TCP 서버] 클라이언트 접속: IP주소=%ls, 포트번호=%d\n", addrBuf, ntohs(clientAddr.sin_port));
        
        // 세션 생성
        Session* newSession = new Session;

        newSession->sock = clientSocket;

        newSession->recvQ.Resize(20000);
        newSession->sendQ.Resize(20000);

        memset(&newSession->recvOverlapped.overlapped, 0, sizeof(OVERLAPPED));
        newSession->recvOverlapped.pSession = newSession;
        newSession->recvOverlapped.type = RECV;

        memset(&newSession->sendOverlapped.overlapped, 0, sizeof(OVERLAPPED));
        newSession->sendOverlapped.pSession = newSession;
        newSession->sendOverlapped.type = SEND;

        newSession->sendFlag = true;

        sessionList.push_front(newSession);

        // 소켓 - IOCP 연결
        CreateIoCompletionPort((HANDLE)clientSocket, hIOCP, (ULONG_PTR)newSession, 0);

        // 비동기 RECV 걸어버리기
        DWORD recvBytes;
        DWORD flags = 0;

        WSABUF wsaBuf;
        wsaBuf.buf = newSession->recvQ.GetRearBufferPtr();
        wsaBuf.len = newSession->recvQ.DirectEnqueueSize();

        wsaRecvRet = WSARecv(clientSocket, &wsaBuf, 1, &recvBytes, &flags, (LPWSAOVERLAPPED)&newSession->recvOverlapped, NULL);
        if (wsaRecvRet == SOCKET_ERROR)
        {
            if (WSAGetLastError() != ERROR_IO_PENDING)
            {
                printf("Error: WSARecv() %d\n", WSAGetLastError());
                return 0;
            }
        }
    }

    WaitForMultipleObjects(hWorkerThreads.size(), hWorkerThreads.data(), TRUE, INFINITE);
    WSACleanup();
}

void NetInit()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

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

    u_long optVal2 = 1;
    ioctlsocket(listenSocket, FIONBIO, &optVal2);

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
        // 비동기IO 완료통지 기다리기
        DWORD cbTransferred;
        Session* pSession;
        OVERLAPPED* pOverlapped;

        SOCKADDR_IN clientAddr;
        int addrLen = sizeof(clientAddr);
        WCHAR addrBuf[40];

        // GQCS()
        int retVal = GetQueuedCompletionStatus(hIOCP, &cbTransferred, (PULONG_PTR)&pSession, &pOverlapped, INFINITE);
        getpeername(pSession->sock, (SOCKADDR*)&clientAddr, &addrLen);
        InetNtop(AF_INET, &clientAddr.sin_addr, addrBuf, 40);
        if (retVal == FALSE || cbTransferred == 0)
        {
            printf("\n[TCP 서버] 클라이언트 종료: IP주소=%ls, 포트번호=%d\n", addrBuf, ntohs(clientAddr.sin_port));

            closesocket(pSession->sock);

            sessionList.erase(find(sessionList.begin(), sessionList.end(), pSession));

            continue;
        }
        
        if (((MyOverlapped*)pOverlapped)->type == RECV)
        {
            pSession->recvQ.MoveRear(cbTransferred);

            char buf[5000];
            memset(buf, 0, 5000);
            pSession->recvQ.Dequeue(buf, cbTransferred);
            buf[cbTransferred] = '\0';

            printf("#RECV [TCP/%ls: %d] %s\n", addrBuf, ntohs(clientAddr.sin_port), buf);

            // WSASend
            pSession->sendQ.Enqueue(buf, cbTransferred);
            if (pSession->sendFlag == true)
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
                        return;
                    }
                }

                pSession->sendFlag = false;
            }

            // WSARecv
            DWORD recvBytes;
            DWORD flags = 0;

            WSABUF wsaBuf2;
            wsaBuf2.buf = pSession->recvQ.GetRearBufferPtr();
            wsaBuf2.len = pSession->recvQ.DirectEnqueueSize();

            wsaRecvRet = WSARecv(pSession->sock, &wsaBuf2, 1, &recvBytes, &flags, (LPWSAOVERLAPPED)&pSession->recvOverlapped, NULL);
            if (wsaRecvRet == SOCKET_ERROR)
            {
                if (WSAGetLastError() != WSA_IO_PENDING)
                {
                    printf("ERROR: WSARecv() %d\n", WSAGetLastError());
                    return;
                }
            }
        }
        else if (((MyOverlapped*)pOverlapped)->type == SEND)
        {
            //TODO: 보낸메시지 출력

            pSession->sendQ.MoveFront(cbTransferred);

            pSession->sendFlag = true;

            if (pSession->recvQ.GetUseSize() > 0)
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
                        return;
                    }
                }

                pSession->sendFlag = false;
            }
        }
    }

}
