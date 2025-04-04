#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")

#include <WinSock2.h>
#include <Windows.h>
#include <WS2tcpip.h>
#include <iostream>
#include <list>
#include "Star_Client.h"
#include "MSGInfo.h"
#include "Data.h"
#include "Console.h"
#include "BufferRender.h"

using namespace std;

// 함수 리턴 및 오류 확인용 변수
int connectVal;
int ioctlVal;
int recvVal;
int sendVal;
int selectVal;

// 전역 관리 변수
SOCKET sock;
list<Player*> pListPlayer;
Player* myPlayer;
int myId = -1;
int totalPlayerCount;
int packetPer1Cycle;

void AllocId(MSG_IDAlloc* msg)
{
    myId = msg->_id;

    list<Player*>::iterator iter;
    for (iter = pListPlayer.begin(); iter != pListPlayer.end(); ++iter)
    {
        if ((*iter)->_id == myId)
        {
            myPlayer = *iter;
            break;
        }
    }

    return;
}

void CreateStar(MSG_CreateStar* msg)
{
    Player* newPlayer = new Player;
    newPlayer->_id = msg->_id;
    newPlayer->_x = msg->_x;
    newPlayer->_y = msg->_y;

    pListPlayer.push_back(newPlayer);
    totalPlayerCount++;

    if (myId == newPlayer->_id)
    {
        myPlayer = newPlayer;
    }

    return;
}

void DestroyStar(MSG_DestroyStar* msg)
{
    list<Player*>::iterator iter;
    for (iter = pListPlayer.begin(); iter != pListPlayer.end(); ++iter)
    {
        if ((*iter)->_id == msg->_id)
        {
            pListPlayer.erase(iter);
            delete (*iter);
            totalPlayerCount--;

            return;
        }
    }
}

void MoveStar(MSG_MoveStar* msg)
{
    list<Player*>::iterator iter;
    for (iter = pListPlayer.begin(); iter != pListPlayer.end(); ++iter)
    {
        if ((*iter)->_id == msg->_id)
        {
            (*iter)->_x = msg->_x;
            (*iter)->_y = msg->_y;
            return;
        }
    }
}

void CreateMoveStarMSG(OUT MSG_MoveStar* msg)
{
    msg->_type = MSG_TYPE::MOVESTAR;
    msg->_id = myPlayer->_id;
    msg->_x = myPlayer->_x;
    msg->_y = myPlayer->_y;

    return;
}

void InputLogic()
{
    if (myPlayer == NULL)
        return;

    bool isMove = false;

    if (GetAsyncKeyState(VK_LEFT))
    {
        if (myPlayer->_x - 1 < 0)
            return;

        myPlayer->_x = myPlayer->_x - 1;
        isMove = true;
    }
    if (GetAsyncKeyState(VK_RIGHT))
    {
        if (myPlayer->_x + 1 > MAX_X)
            return;

        myPlayer->_x = myPlayer->_x + 1;
        isMove = true;
    }
    if (GetAsyncKeyState(VK_UP))
    {
        if (myPlayer->_y - 1 < 0)
            return;

        myPlayer->_y = myPlayer->_y - 1;
        isMove = true;
    }
    if (GetAsyncKeyState(VK_DOWN))
    {
        if (myPlayer->_y + 1 > MAX_Y)
            return;

        myPlayer->_y = myPlayer->_y + 1;
        isMove = true;
    }

    if (isMove == true)
    {
        MSG_MoveStar msg;
        CreateMoveStarMSG(&msg);

        sendVal = send(sock, (char*)&msg, sizeof(msg), 0);
        if (sendVal == SOCKET_ERROR)
        {
            if (WSAGetLastError() != WSAEWOULDBLOCK)
            {
                printf("Error: send() %d\n", WSAGetLastError());
                return;
            }
        }
    }
}

void NetworkLogic()
{
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(sock, &readSet);

    TIMEVAL timeVal;
    timeVal.tv_sec = 0;
    timeVal.tv_usec = 0;

    packetPer1Cycle = 0;

    selectVal = select(0, &readSet, NULL, NULL, &timeVal);
    if (selectVal == SOCKET_ERROR)
    {
        printf("Error: select() %d\n", WSAGetLastError());
        return;
    }
    if (selectVal > 0)
    {
        if (FD_ISSET(sock, &readSet))
        {
            BYTE buf[16 * 20];
            recvVal = recv(sock, (char*)buf, 16 * 20, 0);
            if (recvVal == SOCKET_ERROR)
            {
                if (WSAGetLastError() != WSAEWOULDBLOCK)
                {
                    printf("Error: recv() %d\n", WSAGetLastError());
                    return;
                }
            }

            if (recvVal < MSGLEN)
            {
                printf("16바이트 미만의 데이터를 받았습니다\n");
                return;
            }

            int msgCount = recvVal / MSGLEN;
            packetPer1Cycle = msgCount;

            int a = 3;

            for (int i = 0; i < msgCount; i++)
            {
                int msgType = (int)*(buf + i * MSGLEN);

                switch (msgType)
                {
                case IDALLOC:
                {
                    MSG_IDAlloc* msgIdAlloc = (MSG_IDAlloc*)(buf + i * MSGLEN);
                    AllocId(msgIdAlloc);
                }
                    break;

                case CREATESTAR:
                {
                    MSG_CreateStar* msgCreateStar = (MSG_CreateStar*)(buf + i * MSGLEN);
                    CreateStar(msgCreateStar);
                }
                    break;

                case DESTROYSTAR:
                {
                    MSG_DestroyStar* msgDestroyStar = (MSG_DestroyStar*)(buf + i * MSGLEN);
                    DestroyStar(msgDestroyStar);
                }
                    break;

                case MOVESTAR:
                {
                    MSG_MoveStar* msgMoveStar = (MSG_MoveStar*)(buf + i * MSGLEN);
                    MoveStar(msgMoveStar);
                }
                    break;
                }
            }
        }
    }

}

void RenderPlayer()
{
    list<Player*>::iterator iter;
    for (iter = pListPlayer.begin(); iter != pListPlayer.end(); ++iter)
    {
        Player* nowPlayer = *iter;
        Sprite_Draw(nowPlayer->_x, nowPlayer->_y, '*');
    }
}

void RenderInfo()
{
    const char* str1 = "Connect client: ";
    const char* str2 = "/ recvPacket: ";

    char intToCharBuf[10];
    int cursorPoint_X = 0;
    int cursorPoint_Y = 0;

    for (int i = 0; i < strlen(str1); i++)
    {
        Sprite_Draw(cursorPoint_X++, cursorPoint_Y, str1[i]);
    }

    sprintf_s(intToCharBuf, "%d", totalPlayerCount);
    for (int i = 0; i < strlen(intToCharBuf); i++)
    {
        Sprite_Draw(cursorPoint_X++, cursorPoint_Y, intToCharBuf[i]);
    }
    

    for (int i = 0; i < strlen(str2); i++)
    {
        Sprite_Draw(cursorPoint_X++, cursorPoint_Y, str2[i]);
    }

    sprintf_s(intToCharBuf, "%d", packetPer1Cycle);
    for (int i = 0; i < strlen(intToCharBuf); i++)
    {
        Sprite_Draw(cursorPoint_X++, cursorPoint_Y, intToCharBuf[i]);
    }
}

void Rendering()
{
    Buffer_Clear();

    RenderPlayer();
    RenderInfo();

    Buffer_Flip();
}

bool RenderSkip()
{
    static int oldTime = timeGetTime();

    int diffTime = timeGetTime() - oldTime;

    if (diffTime < MSPERFRAME)
    {
        Sleep(MSPERFRAME - diffTime);
        oldTime += MSPERFRAME;
        return false;
    }
    else if (diffTime > MSPERFRAME * 2)
    {
        oldTime += MSPERFRAME;
        return true;
    }
    else
    {
        oldTime += MSPERFRAME;
        return false;
    }
}

int main()
{
    timeBeginPeriod(1);

    cs_Initial();

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
    {
        printf("Error: socket() %d\n", WSAGetLastError());
        return 0;
    }

    WCHAR serverIP[20] = { 0 };
    printf("접속할 IP를 입력하세요: ");
    wscanf_s(L"%ls", serverIP, (unsigned)_countof(serverIP));
    if (serverIP[19] != 0)
        return 0;

    SOCKADDR_IN serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVERPORT);
    //InetPton(AF_INET, SERVERIP, &serverAddr.sin_addr);
    //InetPton(AF_INET, L"127.0.0.1", &serverAddr.sin_addr);
    InetPton(AF_INET, serverIP, &serverAddr.sin_addr);

    connectVal = connect(sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
    if (connectVal == SOCKET_ERROR)
    {
        printf("Error: connect() %d\n", WSAGetLastError());
        return 0;
    }

    u_long on = 1;
    ioctlVal = ioctlsocket(sock, FIONBIO, &on);
    if (ioctlVal == SOCKET_ERROR)
    {
        printf("Error: ioctlsocket() %d\n", WSAGetLastError());
        return 0;
    }

    while (1)
    {
        // 키 입력
        InputLogic();

        // 네트워크
        NetworkLogic();

        if(RenderSkip())
            continue;

        // 렌더링
        Rendering();
    }

    closesocket(sock);
    WSACleanup();
    timeEndPeriod(1);
}