#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <iostream>
#include <queue>
#include <list>
#include <conio.h>
#include <thread>
#include <algorithm>

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
    WSAOVERLAPPED overlapped;
    DWORD type;
    Session* pSession;
};

struct Session
{
    SOCKET sock;

    char recvBuf[BUFSIZE + 1];
    char sendBuf[BUFSIZE + 1];

    MyOverlapped myOverlapped[2];
};

// 리턴 체크용 전역변수
int bindRet;
int listenRet;
int ioctlRet;
int setSockOptRet;
int wsaRecvRet;
int wsaSendRet;

// 이벤트
HANDLE hQuitEvent;

// 핸들
HANDLE hIOCP;
vector<HANDLE> workerThreadHandles;

list<Session*> sessionList;

DWORD WINAPI WorkerThread(LPVOID arg);

int main()
{
    timeBeginPeriod(1);

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket == INVALID_SOCKET)
    {
        printf("ERROR: socket() %d\n", GetLastError());
        return 0;
    }

    SOCKADDR_IN serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(SERVERPORT);

    int optVal = 0;
    setSockOptRet = setsockopt(listen_socket, SOL_SOCKET, SO_SNDBUF, (char*)&optVal, sizeof(optVal));
    if (setSockOptRet == SOCKET_ERROR)
    {
        printf("ERROR: setsockopt() %d\n", GetLastError());
        return 0;
    }

    u_long optVal2 = 1;
    ioctlsocket(listen_socket, FIONBIO, &optVal2);

    bindRet = bind(listen_socket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
    if (bindRet == SOCKET_ERROR)
    {
        printf("ERROR: bind() %d\n", GetLastError());
        return 0;
    }

    listenRet = listen(listen_socket, SOMAXCONN);
    if (listenRet == SOCKET_ERROR)
    {
        printf("ERROR: listen() %d\n", GetLastError());
        return 0;
    }

    // 이벤트생성
    hQuitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (hQuitEvent == NULL)
        return 0;

    // IOCP 생성
    hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (hIOCP == NULL)
        return 0;

    // CPU 갯수 확인
    SYSTEM_INFO si;
    GetSystemInfo(&si);

    // 워커쓰레드 생성
    HANDLE hThread;
    for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; i++)
    {
        hThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&WorkerThread, NULL, NULL, NULL);
        workerThreadHandles.push_back(hThread);
    }

    while (1)
    {
        if (_kbhit())
        {
            char inputKey = _getch();
            if (inputKey == 'Q' || inputKey == 'q')
            {
                SetEvent(hQuitEvent);
                break;
            }
        }

        // accept()
        SOCKADDR_IN clientAddr;
        memset(&clientAddr, 0, sizeof(clientAddr));
        int addrLen = sizeof(clientAddr);
        SOCKET client_sock = accept(listen_socket, (SOCKADDR*)&clientAddr, &addrLen);
        if (client_sock == INVALID_SOCKET)
        {
            if (GetLastError() == WSAEWOULDBLOCK)
            {
                continue;
            }

            printf("Error: accept() %d\n", GetLastError());
            return 0;
        }

        getpeername(client_sock, (SOCKADDR*)&clientAddr, &addrLen);
        WCHAR addrBuf[40];
        InetNtop(AF_INET, &clientAddr.sin_addr, addrBuf, 40);
        printf("\n[TCP 서버] 클라이언트 접속: IP주소=%ls, 포트번호=%d\n", addrBuf, ntohs(clientAddr.sin_port));

        // 세션 생성
        Session* newSession = new Session;
        newSession->sock = client_sock;

        memset(&newSession->myOverlapped[0].overlapped, 0, sizeof(OVERLAPPED));
        newSession->myOverlapped[0].pSession = newSession;
        newSession->myOverlapped[0].type = RECV;

        memset(&newSession->myOverlapped[1].overlapped, 0, sizeof(OVERLAPPED));
        newSession->myOverlapped[1].pSession = newSession;
        newSession->myOverlapped[1].type = SEND;

        sessionList.push_back(newSession);

        // 소켓 - IOCP 연결
        CreateIoCompletionPort((HANDLE)client_sock, hIOCP, (ULONG_PTR)newSession, 0);

        // 비동기 IO 걸어버리기
        DWORD recvBytes;
        DWORD flags = 0;

        WSABUF wsaBuf;
        wsaBuf.buf = newSession->recvBuf;
        wsaBuf.len = BUFSIZE;

        wsaRecvRet = WSARecv(newSession->sock, &wsaBuf, 1, &recvBytes, &flags, (LPWSAOVERLAPPED)&newSession->myOverlapped[0], NULL);
        if (wsaRecvRet == SOCKET_ERROR)
        {
            if (WSAGetLastError() != ERROR_IO_PENDING)
            {
                printf("Error: WSARecv() %d\n", WSAGetLastError());
                return 0;
            }
        }
    }

    WaitForMultipleObjects(workerThreadHandles.size(), workerThreadHandles.data(), TRUE, INFINITE);

    WSACleanup();

    return 0;
}

DWORD WINAPI WorkerThread(LPVOID arg)
{
    while (1)
    {
        // 비동기 IO 완료통지 기다리기
        DWORD cbTransferred;
        Session* pSession;
        MyOverlapped* pMyOverlapped;

        SOCKADDR_IN clientAddr;
        int addrLen = sizeof(clientAddr);
        WCHAR addrBuf[40];

        int retVal = GetQueuedCompletionStatus(hIOCP, &cbTransferred, (PULONG_PTR)&pSession, (LPWSAOVERLAPPED*)&pMyOverlapped, INFINITE);
        if (retVal == FALSE || cbTransferred == 0)
        {
            getpeername(pSession->sock, (SOCKADDR*)&clientAddr, &addrLen);
            InetNtop(AF_INET, &clientAddr.sin_addr, addrBuf, 40);

            printf("\n[TCP 서버] 클라이언트 종료: IP주소=%ls, 포트번호=%d\n", addrBuf, ntohs(clientAddr.sin_port));

            closesocket(pSession->sock);

            sessionList.erase(find(sessionList.begin(), sessionList.end(), pSession));

            continue;
        }

        if (pMyOverlapped->type == RECV)
        {
            pSession->recvBuf[cbTransferred] = '\0';

            getpeername(pSession->sock, (SOCKADDR*)&clientAddr, &addrLen);
            InetNtop(AF_INET, &clientAddr.sin_addr, addrBuf, 40);
            printf("[TCP/%ls: %d] %s\n", addrBuf, ntohs(clientAddr.sin_port), pSession->recvBuf);

            // WSASend
            memcpy_s(pSession->sendBuf, BUFSIZE, pSession->recvBuf, cbTransferred);

            DWORD sendBytes;
            DWORD flags = 0;

            WSABUF wsaBuf;
            wsaBuf.buf = pSession->sendBuf;
            wsaBuf.len = cbTransferred;

            wsaSendRet = WSASend(pSession->sock, &wsaBuf, 1, &sendBytes, 0, (LPWSAOVERLAPPED)&pSession->myOverlapped[1], NULL);
            if (wsaSendRet == SOCKET_ERROR)
            {
                if (WSAGetLastError() != WSA_IO_PENDING)
                {
                    printf("ERROR: WSARecv() %d\n", WSAGetLastError());
                    return 0;
                }
            }

            // WSARecv
            DWORD recvBytes;

            WSABUF wsaBuf2;
            wsaBuf2.buf = pSession->recvBuf;
            wsaBuf2.len = BUFSIZE;

            wsaRecvRet = WSARecv(pSession->sock, &wsaBuf2, 1, &recvBytes, &flags, (LPWSAOVERLAPPED)&pSession->myOverlapped[0], NULL);
            if (wsaRecvRet == SOCKET_ERROR)
            {
                if (WSAGetLastError() != WSA_IO_PENDING)
                {
                    printf("ERROR: WSASend() %d\n", WSAGetLastError());
                    return 0;
                }
            }


        }

        if (pMyOverlapped->type == SEND)
        {

        }
    }


    return 0;
}
