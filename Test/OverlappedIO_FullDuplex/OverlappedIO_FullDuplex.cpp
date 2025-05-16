#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <iostream>
#include <queue>
#include <list>
#include <conio.h>

using namespace std;

#define SERVERPORT 9000
#define BUFSIZE 512

struct MyOverlapped;

enum
{
    RECV,
    SEND,
};

struct Session
{
    SOCKET sock;
    SOCKADDR_IN clientAddr;

    char recvBuf[BUFSIZE + 1];
    char sendBuf[BUFSIZE + 1];

    WSABUF wsaBuf;

    MyOverlapped* myOverlapped[2];
};

struct MyOverlapped
{
    WSAOVERLAPPED overlapped;
    DWORD type;
    Session* pSession;
};

list<Session*> sessionList;
queue<SOCKET> socketQ;

DWORD WINAPI WorkerThread(LPVOID arg);
void CALLBACK CompletionRoutine(DWORD dwError, DWORD cbTransferred,
    LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags);

HANDLE hQuitEvent;

// 리턴 체크용 전역변수
int bindRet;
int listenRet;
int ioctlRet;
int setSockOptRet;
int wsaRecvRet;
int wsaSendRet;

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

    u_long optOn = 1;
    ioctlRet = ioctlsocket(listen_socket, FIONBIO, &optOn);
    if (ioctlRet == SOCKET_ERROR)
    {
        printf("ERROR: ioctlsocket() %d\n", GetLastError());
        return 0;
    }

    int optVal = 0;
    setSockOptRet = setsockopt(listen_socket, SOL_SOCKET, SO_SNDBUF, (char*) & optVal, sizeof(optVal));
    if (setSockOptRet == SOCKET_ERROR)
    {
        printf("ERROR: setsockopt() %d\n", GetLastError());
        return 0;
    }

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

    // 이벤트 생성
    hQuitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (hQuitEvent == NULL)
        return 1;

    // 쓰레드 생성
    HANDLE hThread = CreateThread(NULL, 0, WorkerThread, NULL, 0, NULL);
    if (hThread == NULL)
        return 1;

    while (1)
    {
        if (_kbhit())
        {
            char inputKey = _getch();
            if (inputKey == 'Q' || inputKey == 'q')
                SetEvent(hQuitEvent);

            break;
        }

        // accept()
        SOCKADDR_IN clientAddr;
        memset(&clientAddr, 0, sizeof(clientAddr));
        int addrLen = sizeof(clientAddr);

        SOCKET client_sock = accept(listen_socket, (SOCKADDR*)&clientAddr, &addrLen);
        if (client_sock == INVALID_SOCKET)
        {
            printf("ERROR: accept() %d\n", GetLastError());
            return 0;
        }

        

        socketQ.push(client_sock);
        sessionList.push_front(newSession);
    }
    
    WaitForSingleObject(hThread, INFINITE);

    return 0;
}

DWORD WINAPI WorkerThread(LPVOID arg)
{
    while (1)
    {
        //
        Session* newSession = new Session;
        newSession->sock = client_sock;
        newSession->clientAddr = clientAddr;

        newSession->myOverlapped[0] = new MyOverlapped;
        memset(&newSession->myOverlapped[0]->overlapped, 0, sizeof(OVERLAPPED));
        newSession->myOverlapped[0]->pSession = newSession;
        newSession->myOverlapped[0]->type = RECV;

        newSession->myOverlapped[1] = new MyOverlapped;
        memset(&newSession->myOverlapped[1]->overlapped, 0, sizeof(OVERLAPPED));
        newSession->myOverlapped[1]->pSession = newSession;
        newSession->myOverlapped[1]->type = SEND;


    }

    return 0;
}

void CompletionRoutine(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags)
{
}
