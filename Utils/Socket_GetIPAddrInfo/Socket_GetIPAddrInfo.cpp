#include <winsock2.h>
#include <ws2tcpip.h>

#include <Windows.h>
#include <iostream>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")

using namespace std;

bool DomainToIP_Random(const WCHAR* szDomain, IN_ADDR* pAddr)
{
    ADDRINFOW* pAddrInfo;
    SOCKADDR_IN* pSockAddr;

    if (GetAddrInfo(szDomain, NULL, NULL, &pAddrInfo) != 0)
    {
        return false;
    }

    int count = 0;
    ADDRINFOW* now = pAddrInfo;
    while (1)
    {
        if (now == NULL) break;
        count++;

        now = now->ai_next;
    }

    int randIdx = rand() % count;
    for (int i = 0; i < randIdx; i++)
    {
        pAddrInfo = pAddrInfo->ai_next;
    }

    pSockAddr = (SOCKADDR_IN*)pAddrInfo->ai_addr;
    *pAddr = pSockAddr->sin_addr;

    FreeAddrInfo(pAddrInfo);
    return true;
}

bool DomainToIP_All(const WCHAR* szDomain, OUT IN_ADDR** ppAddr, OUT int* IPCount)
{
    /*
     이 함수안에서 *ppAddr에 동적할당을 하고 있습니다.
     사용 후 꼭 free 해야합니다.
    */

    ADDRINFOW* pAddrInfo;
    SOCKADDR_IN* pSockAddr;

    if (GetAddrInfo(szDomain, NULL, NULL, &pAddrInfo) != 0)
    {
        return false;
    }

    int count = 0;
    ADDRINFOW* now = pAddrInfo;
    while (1)
    {
        if (now == NULL) break;
        count++;

        now = now->ai_next;
    }

    *IPCount = count;
    *ppAddr = new IN_ADDR[count];

    now = pAddrInfo;
    for (int i = 0; i < count; i++)
    {
        pSockAddr = (SOCKADDR_IN*)now->ai_addr;
        (*ppAddr)[i] = pSockAddr->sin_addr;

        now = now->ai_next;
        // 안전장치
        if (now == NULL)
            break;
    }

    FreeAddrInfo(pAddrInfo);
    return true;
}

bool DomainToIP(const WCHAR* szDomain, IN_ADDR* pAddr)
{
    ADDRINFOW* pAddrInfo;
    SOCKADDR_IN* pSockAddr;

    if (GetAddrInfo(szDomain, NULL, NULL, &pAddrInfo) != 0)
    {
        return false;
    }

    pSockAddr = (SOCKADDR_IN*)pAddrInfo->ai_addr;
    *pAddr = pSockAddr->sin_addr;

    FreeAddrInfo(pAddrInfo);
    return true;
}

int main()
{
    timeBeginPeriod(1);

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    srand(time(nullptr));
    
    // DomainToIP_Random( )
    {
        SOCKADDR_IN sockAddrIn;
        memset(&sockAddrIn, 0, sizeof(sockAddrIn));
        IN_ADDR Addr;

        DomainToIP_Random(L"naver.com", &Addr);

        sockAddrIn.sin_family = AF_INET;
        sockAddrIn.sin_addr = Addr;
        sockAddrIn.sin_port = htons(80);

        WCHAR tmpStr[40] = { 0 };
        InetNtop(AF_INET, &sockAddrIn.sin_addr, tmpStr, sizeof(tmpStr));
        printf("%ls\n", tmpStr);
    }

    // DomainToIP_All( )
    {
        SOCKADDR_IN sockAddrIn;
        memset(&sockAddrIn, 0, sizeof(sockAddrIn));
        IN_ADDR* Addr;
        int ipCount;

        DomainToIP_All(L"naver.com", &Addr, &ipCount);

        sockAddrIn.sin_family = AF_INET;
        sockAddrIn.sin_addr = Addr[0]; // ipCount개 까지 중에 맘대로 사용가능.
        sockAddrIn.sin_port = htons(80);

        free(Addr); // DomainToIP_All( ) 함수 안에서 동적할당되어 데이터를 받아왔기 때문에 사용 후 해제해야한다.

        WCHAR tmpStr[40] = { 0 };
        InetNtop(AF_INET, &sockAddrIn.sin_addr, tmpStr, sizeof(tmpStr));
        printf("%ls\n", tmpStr);
    }

    timeEndPeriod(1);
}