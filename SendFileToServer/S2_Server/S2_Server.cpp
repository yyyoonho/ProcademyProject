#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")

#include <winsock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <iostream>

using namespace std;

#define SERVERPORT 9000

struct st_PACKET_HEADER
{
    DWORD dwPacketCode;     // 0x11223344 우리의 패킷확인 고정값

    WCHAR szName[32];       // 본인이름, 유니코드 utf-16 NULL 문자 끝
    WCHAR szFileName[128];  // 파일이름, 유니코드 utf-16 NULL 문자 끝
    int iFileSize;
};

int main()
{
    timeBeginPeriod(1);

    int retVal = 0;

    WSADATA wsaData;
    retVal = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (retVal != 0)
    {
        printf("ERROR: WSAStartup()\n");
        return 0;
    }

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSock == INVALID_SOCKET)
    {
        printf("ERROR: socket() %d\n", WSAGetLastError());
        return 0;
    }

    LINGER optval;
    optval.l_onoff = 1;
    optval.l_linger = 0;
    retVal = setsockopt(listenSock, SOL_SOCKET, SO_LINGER, (char*)&optval, sizeof(optval));
    if (retVal == SOCKET_ERROR)
    {
        printf("ERROR: setsockopt() %d\n", WSAGetLastError());
        return 0;
    }

    SOCKADDR_IN serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVERPORT);
    serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

    retVal = bind(listenSock, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
    if (retVal == SOCKET_ERROR)
    {
        printf("ERROR: bind() %d\n", WSAGetLastError());
        return 0;
    }

    retVal = listen(listenSock, SOMAXCONN);
    if (retVal == SOCKET_ERROR)
    {
        printf("ERROR: listen() %d\n", WSAGetLastError());
        return 0;
    }

    SOCKADDR_IN clientAddr;
    memset(&clientAddr, 0, sizeof(clientAddr));
    int clientAddrSize = sizeof(clientAddr);
    SOCKET sock = accept(listenSock, (SOCKADDR*)&clientAddr, &clientAddrSize);
    if (sock == INVALID_SOCKET)
    {
        printf("ERROR: accept() %d\n", WSAGetLastError());
        return 0;
    }

    st_PACKET_HEADER headerBuf;

    retVal = recv(sock, (char*)&headerBuf, sizeof(headerBuf), 0);
    if (retVal == SOCKET_ERROR)
    {
        printf("ERROR: recv()1 %d\n", WSAGetLastError());
        return 0;
    }

    BYTE* fileBuf = (BYTE*)malloc(headerBuf.iFileSize);
    if (fileBuf == NULL)
    {
        printf("ERROR: malloc() \n");
        return 0;
    }

    // recvn
    int leftByte = headerBuf.iFileSize;
    int recvByte = 0;
    while (leftByte > 0)
    {
        retVal = recv(sock, (char*)fileBuf + recvByte, leftByte, 0);
        if (retVal == SOCKET_ERROR)
        {
            printf("ERROR: recv()2 %d\n", WSAGetLastError());
            return 0;
        }

        recvByte += retVal;
        leftByte -= retVal;
    }

    FILE* fp;
    retVal = _wfopen_s(&fp, headerBuf.szFileName, L"wb");
    if (retVal != 0)
    {
        printf("ERROR: fopen_s()\n");
        return 0;
    }
    if (fp == NULL)
    {
        return 0;
    }

    retVal = fwrite(fileBuf, headerBuf.iFileSize, 1, fp);
    if (retVal != 1)
    {
        printf("ERROR: fwrite()\n");
        return 0;
    }

    fclose(fp);


   int answer = 0xdddddddd;
    retVal = send(sock, (char*)&answer, 4, 0);
    if (retVal == SOCKET_ERROR)
    {
        printf("ERROR: send()3 %d\n", WSAGetLastError());
        return 0;
    }

    closesocket(sock);
    closesocket(listenSock);

    timeEndPeriod(1);
}