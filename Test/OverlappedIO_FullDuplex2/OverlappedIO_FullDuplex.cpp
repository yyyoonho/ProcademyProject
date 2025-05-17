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

list<Session*> sessionList;
queue<SOCKET> socketQ;

DWORD WINAPI WorkerThread(LPVOID arg);
void CALLBACK CompletionRoutine(DWORD dwError, DWORD cbTransferred,
    LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags);

HANDLE hQuitEvent;
HANDLE hNewSessionEvent;

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

    u_long opt = 1;
    ioctlsocket(listen_socket, FIONBIO, &opt);

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

    hNewSessionEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (hNewSessionEvent == NULL)
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
            if (GetLastError() == WSAEWOULDBLOCK)
            {
                continue;
            } 

            printf("ERROR: accept() %d\n", GetLastError());
            return 0;
        }

        socketQ.push(client_sock);

        SetEvent(hNewSessionEvent);
    }
    
    WaitForSingleObject(hThread, INFINITE);

    return 0;
}

DWORD WINAPI WorkerThread(LPVOID arg)
{
    HANDLE handles[2] = { hQuitEvent, hNewSessionEvent };

    while (1)
    {
        while (1)
        {
            // Alertable wait
            DWORD result = WaitForMultipleObjectsEx(2, handles, FALSE, INFINITE, TRUE);
            if (result == WAIT_OBJECT_0)
                return 0;
            if (result == WAIT_OBJECT_0 + 1)
                break;
            if (result == WAIT_IO_COMPLETION)
                continue;
        }

        // 워커쓰레드가 많아지면, socketQ에 락을 걸어야 할듯. 잡큐에 걸듯이
        if (!socketQ.empty())
        {
            SOCKET client_sock = socketQ.front();
            socketQ.pop();

            Session* newSession = new Session;
            newSession->sock = client_sock;

            memset(&newSession->myOverlapped[0].overlapped, 0, sizeof(OVERLAPPED));
            newSession->myOverlapped[0].pSession = newSession;
            newSession->myOverlapped[0].type = RECV;

 
            memset(&newSession->myOverlapped[1].overlapped, 0, sizeof(OVERLAPPED));
            newSession->myOverlapped[1].pSession = newSession;
            newSession->myOverlapped[1].type = SEND;

            sessionList.push_front(newSession);

            SOCKADDR_IN clientAddr;
            int addrLen = sizeof(clientAddr);
            getpeername(client_sock, (SOCKADDR*)&clientAddr, &addrLen);
            WCHAR addrBuf[40];
            InetNtop(AF_INET, &clientAddr.sin_addr, addrBuf, 40);
            printf("\n[TCP 서버] 클라이언트 접속: IP주소=%ls, 포트번호=%d\n", addrBuf, ntohs(clientAddr.sin_port));
        
            WSABUF wsaBuf;
            wsaBuf.buf = newSession->recvBuf;
            wsaBuf.len = BUFSIZE;

            // 비동기IO 시작
            DWORD recvBytes;
            DWORD flags = 0;

            wsaRecvRet = WSARecv(newSession->sock, &wsaBuf, 1, &recvBytes, &flags, (LPWSAOVERLAPPED)&newSession->myOverlapped[0], CompletionRoutine);
            if (wsaRecvRet == SOCKET_ERROR)
            {
                if (WSAGetLastError() != WSA_IO_PENDING)
                {
                    printf("ERROR: WSARecv() %d\n", WSAGetLastError());
                    return 0;
                }
            }
        }
    }

    return 0;
}

void CompletionRoutine(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags)
{
    MyOverlapped* myOverlappedPtr = (MyOverlapped*)lpOverlapped;
    Session* pSession = myOverlappedPtr->pSession;

    SOCKADDR_IN clientAddr;
    int addrLen = sizeof(clientAddr);
    getpeername(pSession->sock, (SOCKADDR*)&clientAddr, &addrLen);

    // 비동기 IO 결과 확인
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
        pSession->recvBuf[cbTransferred] = '\0';
        WCHAR addrBuf[40];
        InetNtop(AF_INET, &clientAddr.sin_addr, addrBuf, 40);
        printf("[TCP/%ls: %d] %s\n", addrBuf, ntohs(clientAddr.sin_port), pSession->recvBuf);

        // WSASend
        memcpy_s(pSession->sendBuf, BUFSIZE, pSession->recvBuf, cbTransferred);

        DWORD sendBytes;
        DWORD flags = 0;

        WSABUF wsaBuf;
        wsaBuf.buf = pSession->sendBuf;
        wsaBuf.len = cbTransferred;
        
        wsaSendRet = WSASend(pSession->sock, &wsaBuf, 1, &sendBytes, 0, (LPWSAOVERLAPPED)&pSession->myOverlapped[1], CompletionRoutine);
        if (wsaSendRet == SOCKET_ERROR)
        {
            if (WSAGetLastError() != WSA_IO_PENDING)
            {
                printf("ERROR: WSARecv() %d\n", WSAGetLastError());
                return;
            }
        }

        // WSARecv
        DWORD recvBytes;

        WSABUF wsaBuf2;
        wsaBuf2.buf = pSession->recvBuf;
        wsaBuf2.len = BUFSIZE;

        wsaRecvRet = WSARecv(pSession->sock, &wsaBuf2, 1, &recvBytes, &flags, (LPWSAOVERLAPPED)&pSession->myOverlapped[0], CompletionRoutine);
        if (wsaRecvRet == SOCKET_ERROR)
        {
            if (WSAGetLastError() != WSA_IO_PENDING)
            {
                printf("ERROR: WSARecv() %d\n", WSAGetLastError());
                return;
            }
        }

    }

    if (myOverlappedPtr->type == SEND)
    {

    }

}
