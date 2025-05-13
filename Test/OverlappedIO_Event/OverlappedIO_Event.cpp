#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")

#include <WS2tcpip.h>
#include <Windows.h>

#include <WinSock2.h>
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

int nTotalSockets = 0;
SOCKETINFO* socketInfoArray[WSA_MAXIMUM_WAIT_EVENTS];
WSAEVENT EventArray[WSA_MAXIMUM_WAIT_EVENTS];
CRITICAL_SECTION cs;

// 비동기IO 처리 함수
DWORD WINAPI WorkerThread(LPVOID arg);

// 소켓 관리 함수
BOOL AddSocketInfo(SOCKET sock);
void RemoveSocketInfo(int index);

// 리턴변수
int bindRet;
int listenRet;
int wsaRecvRet;
int wsaSendRet;

int overlappedRet;

int main()
{
    timeBeginPeriod(1);

    InitializeCriticalSection(&cs);

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == INVALID_SOCKET)
    {
        printf("ERROR: socket() %d", GetLastError());
        return 0;
    }

    SOCKADDR_IN serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVERPORT);
    serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

    bindRet = bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
    if (bindRet == SOCKET_ERROR)
    {
        printf("ERROR: bind() %d", GetLastError());
        return 0;
    }

    listenRet = listen(listenSocket, SOMAXCONN);
    if (listenRet == SOCKET_ERROR)
    {
        printf("ERROR: listen() %d", GetLastError());
        return 0;
    }

    // 더미 이벤트 객체 생성
    WSAEVENT hEvent = WSACreateEvent();
    if (hEvent == WSA_INVALID_EVENT)
    {
        printf("ERROR: WSACreateEvent() %d", WSAGetLastError());
        return 0;
    }
    EventArray[nTotalSockets++] = hEvent;

    // 스레드 생성
    HANDLE hThread = CreateThread(NULL, 0, WorkerThread, NULL, 0, NULL);
    if (hThread == NULL)
        return 1;
    CloseHandle(hThread);

    // 데이터 통신에 사용할 변수
    SOCKET client_sock;
    SOCKADDR_IN clientAddr;
    int addrLen;
    DWORD recvBytes, flags;

    while (1)
    {
        // accept()
        addrLen = sizeof(clientAddr);
        client_sock = accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
        if (client_sock == INVALID_SOCKET)
        {
            printf("ERROR: accept() %d", GetLastError());
            return 0;
        }
        WCHAR tmpBuf[40];
        InetNtop(AF_INET, &clientAddr.sin_addr, tmpBuf, 40);
        printf("\n[TCP 서버] 클라이언트 접속: IP주소=%ls, 포트번호=%d\n", tmpBuf, ntohs(clientAddr.sin_port));

        // 소켓 정보 추가
        if (AddSocketInfo(client_sock) == FALSE)
        {
            closesocket(client_sock);
            WCHAR tmpBuf[40];
            InetNtop(AF_INET, &clientAddr.sin_addr, tmpBuf, 40);
            printf("\n[TCP 서버] 클라이언트 종료: IP주소=%ls, 포트번호=%d\n", tmpBuf, ntohs(clientAddr.sin_port));
            continue;
        }

        // 비동기 입출력 시작
        SOCKETINFO* ptr = socketInfoArray[nTotalSockets - 1];
        flags = 0;
        wsaRecvRet = WSARecv(ptr->sock, &ptr->wsaBuf, 1, &recvBytes, &flags, &ptr->overlapped, NULL);
        printf("wsaRecvRet: %d, recvBytes: %d\n", wsaSendRet, recvBytes);
        if (wsaRecvRet == SOCKET_ERROR)
        {
            if (WSAGetLastError() != WSA_IO_PENDING)
            {
                printf("ERROR: WSARecv() %d", WSAGetLastError());
                RemoveSocketInfo(nTotalSockets - 1);
                continue;
            }
            if (WSAGetLastError() == WSA_IO_PENDING)
            {
                printf("[맨 처음]WSARecv: WSA_IO_PENDING\n\n");
            }
        }
        

        WSASetEvent(EventArray[0]);
    }

    WSACleanup();
    DeleteCriticalSection(&cs);
    return 0;
}

DWORD WINAPI WorkerThread(LPVOID arg)
{
    while (1)
    {
        // 이벤트 객체 관찰
        DWORD index = WSAWaitForMultipleEvents(nTotalSockets, EventArray, FALSE, WSA_INFINITE, FALSE);
        if (index == WSA_WAIT_FAILED)
        {
            continue;
        }

        index -= WSA_WAIT_EVENT_0;
        WSAResetEvent(EventArray[index]);
        if (index == 0)
            continue;

        // 클라이언트 정보 얻기
        SOCKETINFO* ptr = socketInfoArray[index];
        SOCKADDR_IN clientAddr;
        int addrLen = sizeof(clientAddr);
        getpeername(ptr->sock, (SOCKADDR*)&clientAddr, &addrLen);

        // 비동기IO 결과 확인
        DWORD cbTransferred, flags;
        overlappedRet = WSAGetOverlappedResult(ptr->sock, &ptr->overlapped, &cbTransferred, FALSE, &flags);
        if (overlappedRet == false || cbTransferred == 0)
        {
            RemoveSocketInfo(index);
            WCHAR tmpBuf[40];
            InetNtop(AF_INET, &clientAddr.sin_addr, tmpBuf, 40);
            printf("\n[TCP 서버] 클라이언트 종료: IP주소=%ls, 포트번호=%d\n", tmpBuf, ntohs(clientAddr.sin_port));
            continue;
        }

        // 데이터 전송량 갱신
        if (ptr->recvBytes == 0)
        {
            ptr->recvBytes = cbTransferred;
            ptr->sendBytes = 0;
            // 받은 데이터 출력
            ptr->buf[ptr->recvBytes] = '\0';
            WCHAR tmpBuf[40];
            InetNtop(AF_INET, &clientAddr.sin_addr, tmpBuf, 40);
            printf("[TCP/%ls:%d] %s\n", tmpBuf, ntohs(clientAddr.sin_port), ptr->buf);
        }
        else
        {
            ptr->sendBytes += cbTransferred;
        }

        if (ptr->recvBytes > ptr->sendBytes)
        {
            // 데이터보내기
            memset(&ptr->overlapped, 0, sizeof(ptr->overlapped));
            ptr->overlapped.hEvent = EventArray[index];
            ptr->wsaBuf.buf = ptr->buf + ptr->sendBytes;
            ptr->wsaBuf.len = ptr->recvBytes - ptr->sendBytes;

            DWORD sendBytes;
            wsaSendRet = WSASend(ptr->sock, &ptr->wsaBuf, 1, &sendBytes, 0, &ptr->overlapped, NULL);
            printf("wsaSendRet: %d, sendBytes: %d\n", wsaSendRet, sendBytes);
            if (wsaSendRet == SOCKET_ERROR)
            {
                if (WSAGetLastError() != WSA_IO_PENDING)
                {
                    printf("ERROR: WSASend %d\n", WSAGetLastError());
                }
                if (WSAGetLastError() == WSA_IO_PENDING)
                {
                    printf("WSASend: WSA_IO_PENDING!\n\n");
                }
                continue;
            }
            

        }
        else
        {
            ptr->recvBytes = 0;

            // 데이터 받기
            memset(&ptr->overlapped, 0, sizeof(ptr->overlapped));
            ptr->overlapped.hEvent = EventArray[index];
            ptr->wsaBuf.buf = ptr->buf;
            ptr->wsaBuf.len = BUFSIZE;

            DWORD recvBytes;
            flags = 0;
            wsaRecvRet = WSARecv(ptr->sock, &ptr->wsaBuf, 1, &recvBytes, &flags, &ptr->overlapped, NULL);
            printf("wsaRecvRet: %d, recvBytes: %d\n", wsaRecvRet, recvBytes);
            if (wsaRecvRet == SOCKET_ERROR)
            {
                if (WSAGetLastError() != WSA_IO_PENDING)
                {
                    printf("ERROR: WSARecv %d\n", WSAGetLastError());
                }
                if (WSAGetLastError() == WSA_IO_PENDING)
                {
                    printf("WSARecv: WSA_IO_PENDING!\n\n");
                }

                continue;
            }
            
        }

    }
}

BOOL AddSocketInfo(SOCKET sock)
{
    EnterCriticalSection(&cs);
    if (nTotalSockets >= WSA_MAXIMUM_WAIT_EVENTS)
        return false;

    SOCKETINFO* ptr = new SOCKETINFO;
    
    WSAEVENT hEvent = WSACreateEvent();
    if (hEvent == WSA_INVALID_EVENT)
    {
        printf("ERROR: WSACreateEvent() %d", WSAGetLastError());
        return false;
    }

    memset(&ptr->overlapped, 0, sizeof(ptr->overlapped));
    ptr->overlapped.hEvent = hEvent;
    ptr->sock = sock;
    ptr->recvBytes = 0;
    ptr->sendBytes = 0;
    ptr->wsaBuf.buf = ptr->buf;
    ptr->wsaBuf.len = BUFSIZE;

    socketInfoArray[nTotalSockets] = ptr;
    EventArray[nTotalSockets] = hEvent;
    
    nTotalSockets++;

    LeaveCriticalSection(&cs);
    return true;
}

void RemoveSocketInfo(int index)
{
    EnterCriticalSection(&cs);

    SOCKETINFO* ptr = socketInfoArray[index];
    closesocket(ptr->sock);
    delete ptr;

    WSACloseEvent(EventArray[index]);

    if (index != (nTotalSockets - 1))
    {
        socketInfoArray[index] = socketInfoArray[nTotalSockets - 1];
        EventArray[index] = EventArray[nTotalSockets - 1];
    }

    nTotalSockets--;

    LeaveCriticalSection(&cs);
}
