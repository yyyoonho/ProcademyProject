#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <iostream>

using namespace std;

#define SERVERPORT 9000
#define BUFSIZE 512

struct SOCKETINFO
{
    WSAOVERLAPPED overlapped;
    SOCKET sock;
    char buf[BUFSIZE + 1];

    int recvBytes;
    int sendBytes;

    WSABUF wsaBuf;
};

SOCKET client_sock;
HANDLE hReadEvent, hWriteEvent;

DWORD WINAPI WorkerThread(LPVOID arg);
void CALLBACK CompletionRoutine(DWORD dwError, DWORD cbTransferred,
    LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags);

int bindRet;
int listenRet;
int wsaRecvRet;
int wsaSendRet;

int main()
{
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

    // 이벤트 객체 생성
    hReadEvent = CreateEvent(NULL, FALSE, TRUE, NULL); // 만들때부터 시그널
    if (hReadEvent == NULL)
        return 1;
    hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL); // 만들때부터 논시그널
    if (hWriteEvent == NULL)
        return 1;

    // 쓰레드 생성
    HANDLE hThread = CreateThread(NULL, 0, WorkerThread, NULL, 0, NULL);
    if (hThread == NULL)
        return 1;
    CloseHandle(hThread);

    while (1)
    {
        WaitForSingleObject(hReadEvent, INFINITE);
        // accept()
        SOCKADDR_IN clientAddr;
        memset(&clientAddr, 0, sizeof(clientAddr));
        int addrLen = sizeof(clientAddr);

        client_sock = accept(listen_socket, (SOCKADDR*)&clientAddr, &addrLen);
        if (client_sock == INVALID_SOCKET)
        {
            printf("ERROR: accept() %d\n", GetLastError());
            return 0;
        }

        SetEvent(hWriteEvent);
    }

    WSACleanup();
    return 0;
}

DWORD WINAPI WorkerThread(LPVOID arg)
{
    
    while (1)
    {
        while (1)
        {
            // Alertable wait
            DWORD result = WaitForSingleObjectEx(hWriteEvent, INFINITE, true);
            if (result == WAIT_OBJECT_0)
                break;
            if (result != WAIT_IO_COMPLETION)
                return 1;
        }

        SOCKADDR_IN clientAddr;
        int addrLen = sizeof(clientAddr);
        getpeername(client_sock, (SOCKADDR*)&clientAddr, &addrLen);
        //printf("\n[TCP 서버] 클라이언트 접속: IP주소=%s, 포트번호=%d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
        WCHAR addrBuf[40];
        InetNtop(AF_INET, &clientAddr.sin_addr, addrBuf, 40);
        printf("\n[TCP 서버] 클라이언트 접속: IP주소=%ls, 포트번호=%d\n", addrBuf, ntohs(clientAddr.sin_port));

        

        SOCKETINFO* ptr = new SOCKETINFO;
        memset(&ptr->overlapped, 0, sizeof(ptr->overlapped));
        ptr->sock = client_sock;
        SetEvent(hReadEvent);
        ptr->recvBytes = 0;
        ptr->sendBytes = 0;
        ptr->wsaBuf.buf = ptr->buf;
        ptr->wsaBuf.len = BUFSIZE;

        // 비동기IO 시작
        DWORD recvBytes;
        DWORD flags = 0;

        wsaRecvRet = WSARecv(ptr->sock, &ptr->wsaBuf, 1, &recvBytes, &flags, &ptr->overlapped, CompletionRoutine);
        if (wsaRecvRet == SOCKET_ERROR)
        {
            if (WSAGetLastError() != WSA_IO_PENDING)
            {
                printf("ERROR: WSARecv() %d\n", WSAGetLastError());
                return 0;
            }
        }
    }

    return 0;
}

void CompletionRoutine(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags)
{
    SOCKETINFO* ptr = (SOCKETINFO*)lpOverlapped;
    SOCKADDR_IN clientAddr;
    int addrLen = sizeof(clientAddr);
    getpeername(client_sock, (SOCKADDR*)&clientAddr, &addrLen);

    // 비동기 IO 결과 확인
    if (dwError != 0 || cbTransferred == 0)
    {
        closesocket(ptr->sock);
        WCHAR addrBuf[40];
        InetNtop(AF_INET, &clientAddr.sin_addr, addrBuf, 40);
        printf("\n[TCP 서버] 클라이언트 종료: IP주소=%ls, 포트번호=%d\n", addrBuf, ntohs(clientAddr.sin_port));
        
        return;
    }

    // 데이터 전송량 갱신
    if (ptr->recvBytes == 0)
    {
        ptr->recvBytes = cbTransferred;
        ptr->sendBytes = 0;

        // 받은 데이터 출력
        ptr->buf[ptr->recvBytes] = '\0';
        WCHAR addrBuf[40];
        InetNtop(AF_INET, &clientAddr.sin_addr, addrBuf, 40);
        printf("[TCP/%ls: %d] %s\n", addrBuf, ntohs(clientAddr.sin_port), ptr->buf);
    }
    else
    {
        ptr->sendBytes += cbTransferred;
    }

    if (ptr->recvBytes > ptr->sendBytes)
    {
        // 데이터보내기
        memset(&ptr->overlapped, 0, sizeof(ptr->overlapped));
        ptr->wsaBuf.buf = ptr->buf + ptr->sendBytes;
        ptr->wsaBuf.len = ptr->recvBytes - ptr->sendBytes;

        DWORD sendBytes;
        wsaSendRet = WSASend(ptr->sock, &ptr->wsaBuf, 1, &sendBytes, 0, &ptr->overlapped, CompletionRoutine);
        printf("wsaSendRet: %d, sendBytes: %d\n", wsaSendRet, sendBytes);
        if (wsaSendRet == SOCKET_ERROR)
        {
            if (WSAGetLastError() != WSA_IO_PENDING)
            {
                printf("ERROR: WSASend() %d\n", WSAGetLastError());

                return;
            }
            if (WSAGetLastError() == WSA_IO_PENDING)
            {
                printf("WSASend: WSA_IO_PENDING!\n\n");
                return;
            }
        }
    }
    else
    {
        ptr->recvBytes = 0;

        // 데이터 받기
        memset(&ptr->overlapped, 0, sizeof(ptr->overlapped));
        ptr->wsaBuf.buf = ptr->buf;
        ptr->wsaBuf.len = BUFSIZE;

        DWORD recvBytes;
        DWORD flags = 0;
        wsaRecvRet = WSARecv(ptr->sock, &ptr->wsaBuf, 1, &recvBytes, &flags, &ptr->overlapped, CompletionRoutine);
        printf("wsaRecvRet: %d, recvBytes: %d\n", wsaRecvRet, recvBytes);
        if (wsaRecvRet == SOCKET_ERROR)
        {
            if (WSAGetLastError() != WSA_IO_PENDING)
            {
                printf("ERROR: WSARecv() %d\n", WSAGetLastError());
                return;
            }
            if (WSAGetLastError() == WSA_IO_PENDING)
            {
                printf("WSARecv: WSA_IO_PENDING!\n\n");
                return;
            }
        }
    }
}
