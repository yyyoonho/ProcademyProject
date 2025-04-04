#pragma comment(lib,"winmm.lib")
#pragma comment(lib,"ws2_32.lib")

#include <WinSock2.h>
#include <Windows.h>
#include <WS2tcpip.h>
#include <iostream>
#include <list>
#include <queue>

#include "Data.h"
#include "MSGInfo.h"
#include "BufferRender.h"
#include "Console.h"

using namespace std;

// 네트워크 전역변수
SOCKET listenSocket;

// 게임 전역변수
list<Player*> pListPlayer;
queue<Player*> pQcleanUpPlayer;
int id = 0;
int totalPlayerCount = 0;

// 리턴 값 체크 전역변수
int wsaVal;
int bindVal;
int setSockOptVal;
int ioctlVal;
int listenVal;
int selectVal;
int recvVal;
int sendVal;

void InitNetwork();
void EndNetwork();
void CleanUp();
void Disconnect(Player* targetPlayer);
void SendUnicast(Player* targetPlayer, MSG_Base* msg);
void SendBroadcast(Player* exceptPlayer, MSG_Base* msg);
void AcceptProc();
void RecvProc(Player* recvPlayer);
void NetworkLogic();
void Rendering();
void RenderPlayer();
void RenderInfo();

int main()
{
    timeBeginPeriod(1);
    cs_Initial();

    InitNetwork();

    while (1)
    {
        // GameLogic();
        // 현재 이 서버는 GameLogic이 없음.

        NetworkLogic();

        CleanUp();

        Rendering();
    }
    
    EndNetwork();

    timeEndPeriod(1);
}


void InitNetwork()
{
    WSADATA wsaData;
    wsaVal = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaVal != 0)
    {
        printf("Error: WSAStartup() \n");
        return;
    }

    listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == INVALID_SOCKET)
    {
        printf("Error: socket() %d\n", WSAGetLastError());
        return;
    }

    SOCKADDR_IN serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVERPORT);
    serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

    bindVal = bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
    if (bindVal == SOCKET_ERROR)
    {
        printf("Error: bind() %d\n", WSAGetLastError());
        return;
    }

    LINGER optval;
    optval.l_onoff = 1;
    optval.l_linger = 0;
    setSockOptVal = setsockopt(listenSocket, SOL_SOCKET, SO_LINGER, (const char*)&optval, sizeof(optval));
    if (setSockOptVal == SOCKET_ERROR)
    {
        printf("ERROR: setsockopt() %d\n", WSAGetLastError());
        return;
    }

    u_long on = 1;
    ioctlVal = ioctlsocket(listenSocket, FIONBIO, &on);
    if (ioctlVal == SOCKET_ERROR)
    {
        printf("Error: ioctlsocket() %d\n", WSAGetLastError());
        return;
    }

    listenVal = listen(listenSocket, SOMAXCONN);
    if (listenVal == SOCKET_ERROR)
    {
        printf("Error: listen() %d\n", WSAGetLastError());
        return;
    }
}

void EndNetwork()
{
    closesocket(listenSocket);
    WSACleanup();
}

void CleanUp()
{
    while (!pQcleanUpPlayer.empty())
    {
        Player* nowPlayer = pQcleanUpPlayer.front();
        pQcleanUpPlayer.pop();

        pListPlayer.remove(nowPlayer);
        delete nowPlayer;

        totalPlayerCount--;
    }
}

void Disconnect(Player* targetPlayer)
{
    targetPlayer->_destory = true;
    pQcleanUpPlayer.push(targetPlayer);

    MSG_DestroyStar msg;
    msg._type = MSG_TYPE::DESTROYSTAR;
    msg._id = targetPlayer->_id;

    list<Player*>::iterator iter;
    for (iter = pListPlayer.begin(); iter != pListPlayer.end(); ++iter)
    {
        SendBroadcast(NULL, &msg);
    }
}

void SendUnicast(Player* targetPlayer, MSG_Base* msg)
{
    sendVal = send(targetPlayer->clientSocket, (char*)msg, MSGLEN, 0);
    if (sendVal == SOCKET_ERROR)
    {
        if (sendVal != WSAEWOULDBLOCK)
        {
            printf("Error: sendVal() %d\n", WSAGetLastError());
            Disconnect(targetPlayer);
        }
    }

}

void SendBroadcast(Player* exceptPlayer, MSG_Base* msg)
{
    list<Player*>::iterator iter;

    if (exceptPlayer == NULL)
    {
        for (iter = pListPlayer.begin(); iter != pListPlayer.end(); ++iter)
        {
            if ((*iter)->_destory == true)
                continue;

            sendVal = send((*iter)->clientSocket, (char*)msg, MSGLEN, 0);
            if (sendVal == SOCKET_ERROR)
            {
                if (sendVal != WSAEWOULDBLOCK)
                {
                    printf("Error: sendVal() %d\n", WSAGetLastError());
                    Disconnect((*iter));
                }
            }

        }
    }
    else
    {
        for (iter = pListPlayer.begin(); iter != pListPlayer.end(); ++iter)
        {
            if ((*iter) == exceptPlayer) continue;

            sendVal = send((*iter)->clientSocket, (char*)msg, MSGLEN, 0);
            if (sendVal == SOCKET_ERROR)
            {
                if (sendVal != WSAEWOULDBLOCK)
                {
                    printf("Error: sendVal() %d\n", WSAGetLastError());
                    Disconnect((*iter));
                }
            }

        }
    }

}

void AcceptProc()
{
    SOCKADDR_IN clientAddr;
    int addrSize = sizeof(clientAddr);

    SOCKET socket = accept(listenSocket, (SOCKADDR*)&clientAddr, &addrSize);
    if (socket == INVALID_SOCKET)
    {
        printf("Error: accept() %d\n", WSAGetLastError());
        return;
    }

    Player* newPlayer = new Player;
    newPlayer->clientSocket = socket;
    newPlayer->addr = clientAddr;
    newPlayer->_id = id++;
    newPlayer->_x = MAX_X / 2;
    newPlayer->_y = MAX_Y / 2;

    pListPlayer.push_back(newPlayer);
    totalPlayerCount++;

    // sendUnicast()
        // MSG_IDAlloc를 보내자.
    MSG_IDAlloc msgIdAlloc;
    msgIdAlloc._type = MSG_TYPE::IDALLOC;
    msgIdAlloc._id = newPlayer->_id;
    SendUnicast(newPlayer, (MSG_Base*)&msgIdAlloc);

    // MSG_CreateStar를 보내자.
    list<Player*>::iterator iter;
    for (iter = pListPlayer.begin(); iter != pListPlayer.end(); ++iter)
    {
        MSG_CreateStar msgCreateStar;
        msgCreateStar._type = MSG_TYPE::CREATESTAR;
        msgCreateStar._id = (*iter)->_id;
        msgCreateStar._x = (*iter)->_x;
        msgCreateStar._y = (*iter)->_y;

        SendUnicast(newPlayer, (MSG_Base*)&msgCreateStar);
    }


    // sendBroadcast()
        // newPlayer를 제외한 모든 플레이어에게 newPlayer MSG_CreateStar를 보내자.
    MSG_CreateStar msgCreateStar;
    msgCreateStar._type = MSG_TYPE::CREATESTAR;
    msgCreateStar._id = newPlayer->_id;
    msgCreateStar._x = newPlayer->_x;
    msgCreateStar._y = newPlayer->_y;

    SendBroadcast(newPlayer, (MSG_Base*)&msgCreateStar);
}

void RecvProc(Player* recvPlayer)
{
    char buf[16 * 20];
    recvVal = recv(recvPlayer->clientSocket, buf, 16 * 20, 0);
    if (recvVal == SOCKET_ERROR)
    {
        if (recvVal != WSAEWOULDBLOCK)
        {
            printf("Error: recv() %d\n", WSAGetLastError());
            Disconnect(recvPlayer);
            return;
        }
    }

    int msgCount = recvVal / MSGLEN;
    for (int i = 0; i < msgCount; i++)
    {
        int msgType = (int)(*(buf + i * MSGLEN));

        switch (msgType)
        {
        case IDALLOC:
            break;

        case CREATESTAR:
            break;

        case DESTROYSTAR:
            break;

        case MOVESTAR:
        {
            // 내 리스트에 반영.
            MSG_MoveStar* msg = (MSG_MoveStar*)(buf + i * MSGLEN);
            recvPlayer->_x = msg->_x;
            recvPlayer->_y = msg->_y;

            // broadcast. (recvPlayer를 제외하고)
            SendBroadcast(recvPlayer, (MSG_Base*)msg);
        }
        break;
        }
    }
}

void NetworkLogic()
{
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(listenSocket, &readSet);

    list<Player*>::iterator iter;
    for (iter = pListPlayer.begin(); iter != pListPlayer.end(); ++iter)
    {
        FD_SET((*iter)->clientSocket, &readSet);
    }

    selectVal = select(0, &readSet, NULL, NULL, NULL);
    if (selectVal == SOCKET_ERROR)
    {
        printf("Error: select() %d\n", WSAGetLastError());
        return;
    }

    if (selectVal > 0)
    {
        // listenSocket Check
        if (FD_ISSET(listenSocket, &readSet))
        {
            AcceptProc();
        }

        // clientSocket Check
        for (iter = pListPlayer.begin(); iter != pListPlayer.end(); ++iter)
        {
            if (FD_ISSET((*iter)->clientSocket, &readSet))
            {
                RecvProc((*iter));
            }
        }
    }

}

void Rendering()
{
    Buffer_Clear();

    RenderPlayer();
    RenderInfo();

    Buffer_Flip();
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
}
