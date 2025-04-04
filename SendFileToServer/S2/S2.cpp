#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")

#include <winsock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <iostream>

using namespace std;

#define SERVERPORT 10010
//#define SERVERPORT 9000
#define SERVERDOMAIN L"procademyserver.iptime.org"

struct st_PACKET_HEADER
{
    DWORD dwPacketCode;     // 0x11223344 우리의 패킷확인 고정값

    WCHAR szName[32];       // 본인이름, 유니코드 utf-16 NULL 문자 끝
    WCHAR szFileName[128];  // 파일이름, 유니코드 utf-16 NULL 문자 끝
    int iFileSize;
};

bool DomainToIP(const WCHAR* domain, IN_ADDR* pAddr)
{
    ADDRINFOW* pAddrInfo;
    SOCKADDR_IN* pSockAddr;

    if (GetAddrInfo(domain, NULL, NULL, &pAddrInfo) != 0)
    {
        printf("ERROR: GetAddrInfo()\n");
        return false;
    }

    pSockAddr = (SOCKADDR_IN*)pAddrInfo->ai_addr;
    *pAddr = pSockAddr->sin_addr;

    FreeAddrInfo(pAddrInfo);
    return true;
}

void CreatePakcetHeader(st_PACKET_HEADER* packetHeader, int fileSize)
{
    packetHeader->dwPacketCode = 0x11223344;
    
    const WCHAR* myName = L"ChoiYoonho";
    const WCHAR* fileName = L"yoonhoSample.jpg";

    wcscpy_s(packetHeader->szName, myName);
    wcscpy_s(packetHeader->szFileName, fileName);

    packetHeader->iFileSize = fileSize;

    return;
}

int main()
{
    timeBeginPeriod(1);

    int retVal = 0;
    
    WSADATA wsaData;
    retVal = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (retVal != 0)
    {
        printf("ERROR: WSAStartup() %d\n", WSAGetLastError());
        return 0;
    }

    SOCKADDR_IN serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVERPORT);
    DomainToIP(SERVERDOMAIN, &serverAddr.sin_addr);
    ///InetPton(AF_INET, L"192.168.219.105", &serverAddr.sin_addr);
    //InetPton(AF_INET, L"127.0.0.1", &serverAddr.sin_addr);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
    {
        printf("ERROR: socket() %d\n", WSAGetLastError());
        return 0;
    }

    // 소켓옵션 세팅
    LINGER optval;
    optval.l_onoff = 1;
    optval.l_linger = 0;
    retVal = setsockopt(sock, SOL_SOCKET, SO_LINGER, (const char*)&optval, sizeof(optval));
    if (retVal == SOCKET_ERROR)
    {
        printf("ERROR: setsockopt() %d\n", WSAGetLastError());
        return 0;
    }

    // 서버접속
    retVal = connect(sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
    if (retVal == SOCKET_ERROR)
    {
        printf("ERROR: connect() %d\n", WSAGetLastError());
        return 0;
    }
    

    // 파일오픈
    FILE* fpBmp;
    retVal = fopen_s(&fpBmp, "yoonhoSample.jpg", "rb");
    if (retVal != 0)
    {
        printf("ERROR: fopen_s()\n");
        return 0;
    }
    
    // 파일 사이즈 체크
    fseek(fpBmp, 0, SEEK_END);
    int fileSize = ftell(fpBmp);
    rewind(fpBmp);

    if (fileSize <= 0)
    {
        printf("ERROR: ftell()\n");
        return 0;
    }

    BYTE* fileBuf = (BYTE*)malloc(sizeof(BYTE) * fileSize);
    if (fileBuf == NULL)
    {
        printf("ERROR: malloc()\n");
        return 0;
    }

    retVal = fread_s(fileBuf, fileSize, fileSize, 1, fpBmp);
    if (retVal != 1)
    {
        printf("ERROR: fread_s()\n");
        return 0;
    }

    fclose(fpBmp);


    // 헤더 생성
    st_PACKET_HEADER packetHeader;
    CreatePakcetHeader(&packetHeader, fileSize);

    // 서버로 헤더 전송
    retVal = send(sock, (const char*)&packetHeader, sizeof(packetHeader), 0);
    if (retVal == SOCKET_ERROR)
    {
        printf("ERROR: send() packetHeader %d\n", WSAGetLastError());
        return 0;
    }

    // 서버로 파일 데이터 전송 (1000단위로 쪼개서 보내기)
    int leftByte = fileSize;
    int cnt = 0;
    while (leftByte > 0)
    {
        if (leftByte >= 1000)
        {
            retVal = send(sock, (char*)fileBuf + cnt * 1000, 1000, 0);
            if (retVal == SOCKET_ERROR)
            {
                printf("ERROR: send() file1 %d\n", WSAGetLastError());
                return 0;
            }
        }
        else
        {
            retVal = send(sock, (char*)fileBuf + cnt * 1000, leftByte, 0);
            if (retVal == SOCKET_ERROR)
            {
                printf("ERROR: send() file2 %d\n", WSAGetLastError());
                return 0;
            }
        }

        leftByte -= 1000;
        cnt++;
    }

    unsigned int answer = 0;
    retVal = recv(sock, (char*)&answer, sizeof(answer), 0);
    if (retVal == SOCKET_ERROR)
    {
        printf("ERROR: recv() %d\n", WSAGetLastError());
        return 0;
    }

    printf("%x\n", answer);

    closesocket(sock);
    
    timeEndPeriod(1);
}