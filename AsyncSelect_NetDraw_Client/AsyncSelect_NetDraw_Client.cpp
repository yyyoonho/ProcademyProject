#define _WINSOCK_DEPRECATED_NO_WARNINGS

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "RingBuffer.lib")
#pragma comment(lib, "winmm.lib")

#include <winsock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <iostream>
#include <windowsx.h>

#include "AsyncSelect_NetDraw_Client.h"
#include "MsgProtocol.h"
#include "RingBuffer.h"

using namespace std;

#define SERVERPORT 25000
#define UM_SOCKET (WM_USER+1)
#define BUFSIZE 10000

// 네트워크 변수
SOCKET sock;
RingBuffer recvQ(5000);
RingBuffer sendQ(5000);
bool bConnect;

// 윈도우 메시지 처리함수
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void ReadProc(HWND hWnd, WPARAM wParam, LPARAM lParam);
void WriteProc(HWND hWnd, WPARAM wParam, LPARAM lParam);
void MoveProc(HWND hWnd, WPARAM wParam, LPARAM lParam);
void SendPacket(char* headerMsg, int headerMsgSize, char* packetMsg, int packetMsgSize);
void DrawLine(HWND hWnd, int oldX, int oldY, int curX, int curY);

// 윈도우 메시지 처리변수
HPEN hPen;
bool bDrag;
int oldX;
int oldY;

// 리턴값 체크용 변수
int WSAAsyncSelectVal;
int connectVal;
int recvVal;
int sendVal;

int main()
{
    timeBeginPeriod(1);

    /**************** 윈도우 생성 *****************/

    // 윈도우 클래스 등록
    WNDCLASS wndclass;
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = WndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = NULL;
    wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = L"MyWndClass";

    if (!RegisterClass(&wndclass))
        return 1;

    // 윈도우 생성
    HWND hWnd = CreateWindow(L"MyWndClass", L"TCP서버", WS_OVERLAPPEDWINDOW, 0, 0, 1000, 1000, NULL, NULL, NULL, NULL);
    if (hWnd == NULL)
        return 1;
    ShowWindow(hWnd, SW_SHOWNORMAL);
    UpdateWindow(hWnd);


    /**************** 소켓 생성 *****************/

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("Error: WSAStartup() %d\n", WSAGetLastError());
        return 0;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
    {
        printf("Error: socket() %d\n", WSAGetLastError());
        return 0;
    }

    WCHAR serverIP[20];
    wmemset(serverIP, 0, (unsigned)_countof(serverIP));
    printf("접속할 IP를 입력하세요: ");
    wscanf_s(L"%ls", serverIP, (unsigned)_countof(serverIP));


    SOCKADDR_IN serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(25000);
    InetPton(AF_INET, serverIP, &serverAddr.sin_addr);

    WSAAsyncSelectVal = WSAAsyncSelect(sock, hWnd, UM_SOCKET, FD_CONNECT | FD_READ | FD_WRITE | FD_CLOSE);
    if (WSAAsyncSelectVal == SOCKET_ERROR)
    {
        printf("Error: WSAAsyncSelect() %d\n", WSAGetLastError());
        return 0;
    }

    connectVal = connect(sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
    if (connectVal == SOCKET_ERROR)
    {
        if (WSAGetLastError() != WSAEWOULDBLOCK)
        {
            printf("Error: connect() %d\n", WSAGetLastError());
            return 0;
        }
    }

    /**************** 윈도우 메시지 루프 *****************/
    MSG msg;
    while (GetMessage(&msg, 0, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    timeEndPeriod(1);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
        hPen = CreatePen(PS_SOLID, 3, RGB(1, 1, 1));
        break;

    case UM_SOCKET:
    {
        if (WSAGETSELECTERROR(lParam))
        {
            printf("Error: WSAGETSELECTERROR %d\n", WSAGetLastError());
            return 0;
        }
        else
        {
            switch (WSAGETSELECTEVENT(lParam))
            {
            case FD_CONNECT:
                bConnect = true;
                break;
            case FD_READ:
                ReadProc(hWnd, wParam, lParam);
                break;
            case FD_WRITE:
                WriteProc(hWnd, wParam, lParam);
                break;
            case FD_CLOSE:
                
                break;
            }
        }
    }
        break;

    case WM_LBUTTONUP:
        bDrag = false;
        break;

    case WM_LBUTTONDOWN:
        bDrag = true;
        break;

    case WM_MOUSEMOVE:
        if (bConnect == true)
        {
            MoveProc(hWnd, wParam, lParam);
        }
        break;

    case WM_DESTROY:
        closesocket(sock);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);

    }

    return 0;
}

void ReadProc(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    char recvBuf[BUFSIZE];
    recvVal = recv(wParam, recvBuf, BUFSIZE, 0);
    if (recvVal == SOCKET_ERROR)
    {
        if (WSAGetLastError() != WSAEWOULDBLOCK)
        {
            printf("Error: recv %d\n", WSAGetLastError());
            return;
        }
    }

    recvQ.Enqueue(recvBuf, recvVal);

    while (1)
    {
        if (recvQ.GetUseSize() <= sizeof(stHeader))
            break;

        char buf[20];
        recvQ.Peek(buf, sizeof(stHeader));
        stHeader* header = (stHeader*)(buf);

        if ((unsigned)recvQ.GetUseSize() < sizeof(stHeader) + header->len)
            break;

        int peekSize = recvQ.Peek(buf, sizeof(stHeader) + header->len);
        recvQ.MoveFront(peekSize);

        // TODO : 그리기.
        stDrawPacket* drawPacket = (stDrawPacket*)(buf + sizeof(stHeader));
        DrawLine(hWnd, drawPacket->startX, drawPacket->startY, drawPacket->endX, drawPacket->endY);
    }

}

void WriteProc(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    while (1)
    {
        char buf[BUFSIZE];
        int peekVal = sendQ.Peek(buf, BUFSIZE);

        sendVal = send(wParam, buf, peekVal, 0);
        if (sendVal == SOCKET_ERROR)
        {
            if (WSAGetLastError() != WSAEWOULDBLOCK)
            {
                printf("Error: send() %d\n", WSAGetLastError());
                break;
            }
        }

        sendQ.MoveFront(sendVal);

        if (sendQ.GetUseSize() == 0)
            break;
    }
}

void MoveProc(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    int curX = GET_X_LPARAM(lParam);
    int curY = GET_Y_LPARAM(lParam);

    if (bDrag == true)
    {
        stHeader headerMsg;
        headerMsg.len = sizeof(stDrawPacket);

        stDrawPacket drawPacketMsg;
        drawPacketMsg.startX = oldX;
        drawPacketMsg.startY = oldY;
        drawPacketMsg.endX = curX;
        drawPacketMsg.endY = curY;

        SendPacket((char*)&headerMsg, sizeof(stHeader), (char*)&drawPacketMsg, sizeof(stDrawPacket));
    }

    oldX = curX;
    oldY = curY;
}

void SendPacket(char* headerMsg, int headerMsgSize, char* packetMsg, int packetMsgSize)
{
    sendQ.Enqueue(headerMsg, headerMsgSize);
    sendQ.Enqueue(packetMsg, packetMsgSize);
   
    while (1)
    {
        char buf[BUFSIZE];
        int peekVal = sendQ.Peek(buf, BUFSIZE);

        sendVal = send(sock, buf, peekVal, 0);
        if (sendVal == SOCKET_ERROR)
        {
            if (WSAGetLastError() != WSAEWOULDBLOCK)
            {
                printf("Error: send() %d\n", WSAGetLastError());
                break;
            }
        }
        
        sendQ.MoveFront(sendVal);

        if (sendQ.GetUseSize() == 0)
            break;
    }
}

void DrawLine(HWND hWnd, int oldX, int oldY, int curX, int curY)
{
    HDC hdc = GetDC(hWnd);
    HPEN hPenOld = (HPEN)SelectObject(hdc, hPen);

    MoveToEx(hdc, oldX, oldY, NULL);
    LineTo(hdc, curX, curY);

    SelectObject(hdc, hPenOld);
    ReleaseDC(hWnd, hdc);
}
