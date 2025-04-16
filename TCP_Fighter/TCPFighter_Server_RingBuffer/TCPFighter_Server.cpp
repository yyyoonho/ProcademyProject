#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "RingBuffer.lib")

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <iostream>
#include <list>
#include <Queue>

#include "RingBuffer.h"
#include "Session.h"
#include "Data.h"
#include "PacketData.h"
#include "MakePacket.h"
#include "TCPFighter_Server.h"

using namespace std;

#define SERVERPORT 5000

// 네트워크 통신 전역변수
SOCKET listenSocket;

// 게임 전역변수
bool bShutdown;
list<Session*> sessionList;
queue<Session*> destroyQ;
int g_id = 1;

// 리턴 확인용 전역변수
int bindRet;
int setSockOptRet;
int ioctlRet;
int listenRet;
int recvRet;
int sendRet;


bool NetWorkInit();
void NetworkEnd();
void NetworkLogic();

void AcceptProc();
void RecvProc(Session* pSession);
void SendProc(Session* pSession);

// recv하고 처리하는 함수
bool PacketProc(Session* pSession, PACKET_TYPE type, char* pPacket);
bool netPacketProc_MoveStart(Session* pSession, char* pPacket);
bool netPacketProc_MoveStop(Session* pSession, char* pPacket);
bool netPacketProc_Attack1(Session* pSession, char* pPacket);
bool netPacketProc_Attack2(Session* pSession, char* pPacket);
bool netPacketProc_Attack3(Session* pSession, char* pPacket);

void DamageToTarget(Session* targetSession);

// 게임 로직 함수
void Update();
void Move(DWORD deltaTime, Session* pSession, BYTE dir);
void DestroySession();

bool IsGround(int nextY, int nextX);

void SendBroadcast(Session* targetPlayer, char* header, int headerSize, char* packet, int packetSize);
void SendUnicast(Session* targetPlayer, char* header, int headerSize, char* packet, int packetSize);

int main()
{
    timeBeginPeriod(1);

    NetWorkInit();

    while (!bShutdown)
    {
        NetworkLogic(); // 네트워크 송수신 처리

        Update(); // 게임 로직 업데이트

        DestroySession(); // Destroy는 항상 최하단에서 호출
    }

    NetworkEnd();

    timeEndPeriod(1);
}

bool NetWorkInit()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("ERROR: WSAStartup() %d\n", WSAGetLastError());
        return false;
    }

    listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == INVALID_SOCKET)
    {
        printf("ERROR: socket() %d\n", WSAGetLastError());
        return false;
    }

    SOCKADDR_IN serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVERPORT);
    serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

    bindRet = bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
    if (bindRet == SOCKET_ERROR)
    {
        printf("ERROR: bind() %d\n", WSAGetLastError());
        return false;
    }

    LINGER optval;
    optval.l_onoff = 1;
    optval.l_linger = 0;
    setSockOptRet = setsockopt(listenSocket, SOL_SOCKET, SO_LINGER, (const char*)&optval, sizeof(optval));
    if (setSockOptRet == SOCKET_ERROR)
    {
        printf("ERROR: setsockopt() %d\n", WSAGetLastError());
        return false;
    }

    u_long on = 1;
    ioctlRet = ioctlsocket(listenSocket, FIONBIO, &on);
    if (ioctlRet == SOCKET_ERROR)
    {
        printf("Error: ioctlsocket() %d\n", WSAGetLastError());
        return false;
    }

    listenRet = listen(listenSocket, SOMAXCONN);
    if (listenRet == SOCKET_ERROR)
    {
        printf("ERROR: listen() %d\n", WSAGetLastError());
        return false;
    }

    return true;
}

void NetworkEnd()
{
    closesocket(listenSocket);
    WSACleanup();
}

void NetworkLogic()
{
    fd_set readSet;
    fd_set writeSet;

    FD_ZERO(&readSet);
    FD_ZERO(&writeSet);

    FD_SET(listenSocket, &readSet);

    list<Session*>::iterator iter;
    for (iter = sessionList.begin(); iter != sessionList.end(); ++iter)
    {
        Session* nowSession = *iter;

        FD_SET(nowSession->_socket, &readSet);

        if (nowSession->sendQ.GetUseSize() > 0)
        {
            FD_SET(nowSession->_socket, &writeSet);
        }
    }

    timeval time;
    time.tv_sec = 0;
    time.tv_usec = 0;

    int result = select(0, &readSet, &writeSet, 0, &time);

    if (result > 0)
    {
        if (FD_ISSET(listenSocket, &readSet))
        {
            AcceptProc();
        }

        list<Session*>::iterator iter;
        for (iter = sessionList.begin(); iter != sessionList.end(); ++iter)
        {
            Session* nowSession = *iter;

            if (FD_ISSET(nowSession->_socket, &readSet))
            {
                --result;
                RecvProc(nowSession);
            }

            if (FD_ISSET(nowSession->_socket, &writeSet))
            {
                --result;
                SendProc(nowSession);
            }
        }

    }
    else if (result == SOCKET_ERROR)
    {
        // 에러처리
        printf("ERROR: select %d\n", WSAGetLastError());
        return;
    }
}

void AcceptProc()
{
    SOCKADDR_IN clientAddr;
    memset(&clientAddr, 0, sizeof(clientAddr));
    int addrLen = sizeof(clientAddr);

    SOCKET newSocket = accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
    if (newSocket == INVALID_SOCKET)
    {
        printf("Error: accecpt() %d\n", WSAGetLastError());
        return;
    }

    Session* newSession = new Session;
    newSession->_socket = newSocket;
    newSession->_clientAddr = clientAddr;
    newSession->_id = g_id++;
    newSession->_x = RANGE_MOVE_RIGHT / 2;
    newSession->_y = RANGE_MOVE_BOTTOM / 2;
    newSession->_hp = 100;
    newSession->_dX = RANGE_MOVE_RIGHT / 2;
    newSession->_dY = RANGE_MOVE_BOTTOM / 2;
    newSession->_action = PACKET_STOP;
    newSession->_characterDirection = PACKET_MOVE_DIR_LL;

    newSession->recvQ.Resize(1000);
    newSession->sendQ.Resize(1000);

    sessionList.push_back(newSession);

    // new에게 자신의 생성 메시지 보내기.
    {
        stPacketHeader header;
        stPACKET_SC_CREATE_MY_CHARACTER packet;
        
        mpCreateMyCharacter(&header, &packet, newSession->_id, newSession->_characterDirection, newSession->_x, newSession->_y);
        SendUnicast(newSession, (char*)&header, sizeof(header), (char*)&packet, sizeof(packet));
        printf("SEND1: [HEADER] type: %d  [PACKET] _id:%d, _x:%d, _y:%d, dir:%d\n", header._type, packet._id, packet._x, packet._y, packet._direction);
    }

    
    // new에게 기존 멤버들 생성 메시지 보내기.
    {
        stPacketHeader header;
        stPACKET_SC_CREATE_OTHER_CHARACTER packet;

        list<Session*>::iterator iter;
        for (iter = sessionList.begin(); iter != sessionList.end(); ++iter)
        {
            Session* nowSession = *iter;
            if (nowSession == newSession)
                continue;

            mpCreateOtherCharacter(&header, &packet, nowSession->_id, nowSession->_characterDirection, nowSession->_x, nowSession->_y, nowSession->_hp);
            SendUnicast(newSession, (char*)&header, sizeof(header), (char*)&packet, sizeof(packet));
            printf("SEND2: [HEADER] type: %d  [PACKET] _id:%d, _x:%d, _y:%d, dir:%d\n", header._type, packet._id, packet._x, packet._y, packet._direction);
        }

    }

    // new에게 기존 멤버들 생성 메시지 보내기.
    {
        stPacketHeader header;
        stPACKET_SC_MOVE_START packet;

        list<Session*>::iterator iter;
        for (iter = sessionList.begin(); iter != sessionList.end(); ++iter)
        {
            Session* nowSession = *iter;
            if (nowSession == newSession)
                continue;

            if (nowSession->_action == PACKET_STOP)
                continue;

            mpMoveStart(&header, &packet, nowSession->_id, nowSession->_action, nowSession->_x, nowSession->_y);
            SendUnicast(newSession, (char*)&header, sizeof(header), (char*)&packet, sizeof(packet));
            printf("SEND4: [HEADER] type: %d  [PACKET] _id:%d, _x:%d, _y:%d, dir:%d\n", header._type, packet._id, packet._x, packet._y, packet._direction);
        }

    }
    
    
    // 기존멤버들에게 new 생성 메시지 보내기.
    {
        stPacketHeader header;
        stPACKET_SC_CREATE_OTHER_CHARACTER packet;

        mpCreateOtherCharacter(&header, &packet, newSession->_id, newSession->_characterDirection, newSession->_x, newSession->_y, newSession->_hp);
        SendBroadcast(newSession, (char*)&header, sizeof(header), (char*)&packet, sizeof(packet));
        printf("SEND3: [HEADER] type: %d  [PACKET] _id:%d, _x:%d, _y:%d, dir:%d\n", header._type, packet._id, packet._x, packet._y, packet._direction);
    }

    
}

void RecvProc(Session* pSession)
{
    /*
    char recvBuf[10000];
    recvRet = recv(pSession->_socket, recvBuf, 10000, 0);
    if (recvRet == SOCKET_ERROR)
    {
        if (WSAGetLastError() == 10054)
        {
            printf("[상대방의 비 정상 연결 종료] id : %d\n", pSession->_id);
            destroyQ.push(pSession);
            return;
        }

        if (WSAGetLastError() != WSAEWOULDBLOCK)
        {
            printf("ERROR: recv() %d\n", WSAGetLastError());
            return;
        }
    }
    if (recvRet == 0)
    {
        // 종료처리 (상대 FIN보낸거지)
        printf("[상대방의 정상 연결 종료] id : %d\n", pSession->_id);
        destroyQ.push(pSession);
        return;
    }

    pSession->recvQ.Enqueue(recvBuf, recvRet);
    */

    recvRet = recv(pSession->_socket, pSession->recvQ.GetRearBufferPtr(), pSession->recvQ.DirectEnqueueSize(), 0);
    if (recvRet == SOCKET_ERROR)
    {
        if (WSAGetLastError() == 10054)
        {
            printf("[상대방의 비 정상 연결 종료] id : %d\n", pSession->_id);
            destroyQ.push(pSession);
            return;
        }

        if (WSAGetLastError() != WSAEWOULDBLOCK)
        {
            printf("ERROR: recv() %d\n", WSAGetLastError());
            return;
        }
    }
    if (recvRet == 0)
    {
        // 종료처리 (상대 FIN보낸거지)
        printf("[상대방의 정상 연결 종료] id : %d\n", pSession->_id);
        destroyQ.push(pSession);
        return;
    }

    pSession->recvQ.MoveRear(recvRet);

    while (1)
    {
        if (pSession->recvQ.GetUseSize() <= sizeof(stPacketHeader))
            break;

        char buf[100];
        pSession->recvQ.Peek(buf, sizeof(stPacketHeader));

        stPacketHeader* pHeader = (stPacketHeader*)buf;
        int payloadLen = pHeader->_size;

        if (pSession->recvQ.GetUseSize() < sizeof(stPacketHeader) + payloadLen)
            break;

        int peekSize = pSession->recvQ.Peek(buf, sizeof(stPacketHeader) + payloadLen);
        pSession->recvQ.MoveFront(peekSize);


        PacketProc(pSession, (PACKET_TYPE)(pHeader->_type), buf + sizeof(stPacketHeader));
    }

}

void SendProc(Session* pSession)
{
    /*
    while (1)
    {
        char buf[10000];
        int peekSize = pSession->sendQ.Peek(buf, 10000);

        sendRet = send(pSession->_socket, buf, peekSize, 0);
        if (sendRet == SOCKET_ERROR)
        {
            if (WSAGetLastError() != WSAEWOULDBLOCK)
            {
                printf("ERROR: send() %d\n", WSAGetLastError());
                return;
            }
        }

        pSession->sendQ.MoveFront(sendRet);

        if (pSession->sendQ.GetUseSize() == 0)
            break;
    }
    */
    
    int a = pSession->sendQ.DirectDequeueSize();
    sendRet = send(pSession->_socket, pSession->sendQ.GetFrontBufferPtr(), pSession->sendQ.DirectDequeueSize(), 0);
    if (sendRet == SOCKET_ERROR)
    {
        if (WSAGetLastError() != WSAEWOULDBLOCK)
        {
            printf("ERROR: send() %d\n", WSAGetLastError());
            return;
        }
    }

    pSession->sendQ.MoveFront(sendRet);

    return;
}

bool PacketProc(Session* pSession, PACKET_TYPE type, char* pPacket)
{
    switch (type)
    {
    case PACKET_CS_MOVE_START:
        return netPacketProc_MoveStart(pSession, pPacket);
        break;

    case PACKET_CS_MOVE_STOP:
        return netPacketProc_MoveStop(pSession, pPacket);
        break;

    case PACKET_CS_ATTACK1:
        return netPacketProc_Attack1(pSession, pPacket);
        break;

    case PACKET_CS_ATTACK2:
        return netPacketProc_Attack2(pSession, pPacket);
        break;

    case PACKET_CS_ATTACK3:
        return netPacketProc_Attack3(pSession, pPacket);
        break;
    }

    return true;
}

bool netPacketProc_MoveStart(Session* pSession, char* pPacket)
{
    stPACKET_CS_MOVE_START* pMoveStart = (stPACKET_CS_MOVE_START*)pPacket;
    
    printf("RECV: MoveStart [PACKET] _x:%d, _y:%d, dir:%d\n", pMoveStart->_x, pMoveStart->_y, pMoveStart->_direction);

    int diffX = abs(pMoveStart->_x - pSession->_x);
    int diffY = abs(pMoveStart->_y - pSession->_y);
    if (diffX > ERROR_RANGE || diffY > ERROR_RANGE)
    {
        // 끊어버리자
        destroyQ.push(pSession);

        // 로그 출력
        printf("CLOSE: MoveStart [PACKET] _x:%d, _y:%d [Player] _x:%d, _y:%d \n" 
            ,pMoveStart->_x, pMoveStart->_y, pSession->_x, pSession->_y);
    }
    pSession->_x = pMoveStart->_x;
    pSession->_y = pMoveStart->_y;

    // 동작변경
    pSession->_action = pMoveStart->_direction;

    // 시선변경
    switch (pMoveStart->_direction)
    {
    case PACKET_MOVE_DIR_RR:
    case PACKET_MOVE_DIR_RU:
    case PACKET_MOVE_DIR_RD:
        pSession->_characterDirection = PACKET_MOVE_DIR_RR;
        break;
    case PACKET_MOVE_DIR_LL:
    case PACKET_MOVE_DIR_LU:
    case PACKET_MOVE_DIR_LD:
        pSession->_characterDirection = PACKET_MOVE_DIR_LL;
        break;
    }

    stPacketHeader header;
    stPACKET_SC_MOVE_START packet;
    mpMoveStart(&header, &packet, pSession->_id, pSession->_action, pSession->_x, pSession->_y);
    SendBroadcast(pSession, (char*)&header, sizeof(header), (char*)&packet, sizeof(packet));
    printf("SEND 00 : MoveStart [PACKET] _x:%d, _y:%d, dir:%d\n", pSession->_x, pSession->_y, pSession->_action);

    return true;
}

bool netPacketProc_MoveStop(Session* pSession, char* pPacket)
{
    stPACKET_CS_MOVE_STOP* pMoveStop = (stPACKET_CS_MOVE_STOP*)pPacket;

    int diffX = abs(pSession->_x - pMoveStop->_x);
    int diffY = abs(pSession->_y - pMoveStop->_y);

    if (diffX > ERROR_RANGE || diffY > ERROR_RANGE)
    {
        destroyQ.push(pSession);
        return false;
    }

    pSession->_x = pMoveStop->_x;
    pSession->_y = pMoveStop->_y;

    // 시선변경
    switch (pMoveStop->_direction)
    {
    case PACKET_MOVE_DIR_RR:
    case PACKET_MOVE_DIR_RU:
    case PACKET_MOVE_DIR_RD:
        pSession->_characterDirection = PACKET_MOVE_DIR_RR;
        pSession->_action = PACKET_STOP;
        break;
    case PACKET_MOVE_DIR_LL:
    case PACKET_MOVE_DIR_LU:
    case PACKET_MOVE_DIR_LD:
        pSession->_characterDirection = PACKET_MOVE_DIR_LL;
        pSession->_action = PACKET_STOP;
        break;
    }

    stPacketHeader header;
    stPACKET_SC_MOVE_STOP packet;
    mpMoveStop(&header, &packet, pSession->_id, pSession->_characterDirection, pSession->_x, pSession->_y);
    SendBroadcast(pSession, (char*)&header, sizeof(stPacketHeader), (char*)&packet, sizeof(stPACKET_SC_MOVE_STOP));

    return true;
}

bool netPacketProc_Attack1(Session* pSession, char* pPacket)
{
    stPACKET_CS_ATTACK1* pAttack1 = (stPACKET_CS_ATTACK1*)pPacket;

    pSession->_characterDirection = pAttack1->_direction;
    pSession->_x = pAttack1->_x;
    pSession->_y = pAttack1->_y;

    // 캐릭터 공격 모션 패킷
    {
        stPacketHeader header;
        stPACKET_SC_ATTACK1 packet;
        mpAttack1(&header, &packet, pSession->_id, pSession->_characterDirection, pSession->_x, pSession->_y);
        SendBroadcast(pSession, (char*)&header, sizeof(header), (char*)&packet, sizeof(packet));
    }

    
    // 공격 판정 패킷
    int attackRangeMinX;
    int attackRangeMaxX;
    int attackRangeMinY = max(RANGE_MOVE_TOP, pAttack1->_y - ATTACK1_RANGE_Y);
    int attackRangeMaxY = min(RANGE_MOVE_BOTTOM, pAttack1->_y + ATTACK1_RANGE_Y);

    if (pAttack1->_direction == PACKET_MOVE_DIR_LL)
    {
        attackRangeMinX = max(RANGE_MOVE_LEFT, pAttack1->_x - ATTACK1_RANGE_X);
        attackRangeMaxX = pAttack1->_x;
    }
    else
    {
        attackRangeMinX = pAttack1->_x;
        attackRangeMaxX = min(RANGE_MOVE_RIGHT, pAttack1->_x + ATTACK1_RANGE_X);
    }

    list<Session*>::iterator iter;
    for (iter = sessionList.begin(); iter != sessionList.end(); ++iter)
    {
        Session* tmpSession = *iter;
        if (tmpSession == pSession)
            continue;

        if (tmpSession->_x < attackRangeMinX || tmpSession->_x > attackRangeMaxX)
            continue;
        if (tmpSession->_y < attackRangeMinY || tmpSession->_y > attackRangeMaxY)
            continue;

        DamageToTarget(tmpSession);

        {
            stPacketHeader header;
            stPACKET_SC_DAMAGE packet;
            mpDamage(&header, &packet, pSession->_id, tmpSession->_id, tmpSession->_hp);
            SendBroadcast(NULL, (char*)&header, sizeof(header), (char*)&packet, sizeof(packet));
        }
    }

    return true;
}

bool netPacketProc_Attack2(Session* pSession, char* pPacket)
{
    stPACKET_CS_ATTACK2* pAttack2 = (stPACKET_CS_ATTACK2*)pPacket;

    pSession->_characterDirection = pAttack2->_direction;
    pSession->_x = pAttack2->_x;
    pSession->_y = pAttack2->_y;

    // 캐릭터 공격 모션 패킷
    {
        stPacketHeader header;
        stPACKET_SC_ATTACK2 packet;
        mpAttack2(&header, &packet, pSession->_id, pSession->_characterDirection, pSession->_x, pSession->_y);
        SendBroadcast(pSession, (char*)&header, sizeof(header), (char*)&packet, sizeof(packet));
    }


    // 공격 판정 패킷
    int attackRangeMinX;
    int attackRangeMaxX;
    int attackRangeMinY = max(RANGE_MOVE_TOP, pAttack2->_y - ATTACK2_RANGE_Y);
    int attackRangeMaxY = min(RANGE_MOVE_BOTTOM, pAttack2->_y + ATTACK2_RANGE_Y);

    if (pAttack2->_direction == PACKET_MOVE_DIR_LL)
    {
        attackRangeMinX = max(RANGE_MOVE_LEFT, pAttack2->_x - ATTACK2_RANGE_X);
        attackRangeMaxX = pAttack2->_x;
    }
    else
    {
        attackRangeMinX = pAttack2->_x;
        attackRangeMaxX = min(RANGE_MOVE_RIGHT, pAttack2->_x + ATTACK2_RANGE_X);
    }

    list<Session*>::iterator iter;
    for (iter = sessionList.begin(); iter != sessionList.end(); ++iter)
    {
        Session* tmpSession = *iter;
        if (tmpSession == pSession)
            continue;

        if (tmpSession->_x < attackRangeMinX || tmpSession->_x > attackRangeMaxX)
            continue;
        if (tmpSession->_y < attackRangeMinY || tmpSession->_y > attackRangeMaxY)
            continue;

        DamageToTarget(tmpSession);

        {
            stPacketHeader header;
            stPACKET_SC_DAMAGE packet;
            mpDamage(&header, &packet, pSession->_id, tmpSession->_id, tmpSession->_hp);
            SendBroadcast(NULL, (char*)&header, sizeof(header), (char*)&packet, sizeof(packet));
        }
    }

    return true;
}

bool netPacketProc_Attack3(Session* pSession, char* pPacket)
{
    stPACKET_CS_ATTACK3* pAttack3 = (stPACKET_CS_ATTACK3*)pPacket;

    pSession->_characterDirection = pAttack3->_direction;
    pSession->_x = pAttack3->_x;
    pSession->_y = pAttack3->_y;

    // 캐릭터 공격 모션 패킷
    {
        stPacketHeader header;
        stPACKET_SC_ATTACK3 packet;
        mpAttack3(&header, &packet, pSession->_id, pSession->_characterDirection, pSession->_x, pSession->_y);
        SendBroadcast(pSession, (char*)&header, sizeof(header), (char*)&packet, sizeof(packet));
    }


    // 공격 판정 패킷
    int attackRangeMinX;
    int attackRangeMaxX;
    int attackRangeMinY = max(RANGE_MOVE_TOP, pAttack3->_y - ATTACK3_RANGE_Y);
    int attackRangeMaxY = min(RANGE_MOVE_BOTTOM, pAttack3->_y + ATTACK3_RANGE_Y);

    if (pAttack3->_direction == PACKET_MOVE_DIR_LL)
    {
        attackRangeMinX = max(RANGE_MOVE_LEFT, pAttack3->_x - ATTACK3_RANGE_X);
        attackRangeMaxX = pAttack3->_x;
    }
    else
    {
        attackRangeMinX = pAttack3->_x;
        attackRangeMaxX = min(RANGE_MOVE_RIGHT, pAttack3->_x + ATTACK3_RANGE_X);
    }

    list<Session*>::iterator iter;
    for (iter = sessionList.begin(); iter != sessionList.end(); ++iter)
    {
        Session* tmpSession = *iter;
        if (tmpSession == pSession)
            continue;

        if (tmpSession->_x < attackRangeMinX || tmpSession->_x > attackRangeMaxX)
            continue;
        if (tmpSession->_y < attackRangeMinY || tmpSession->_y > attackRangeMaxY)
            continue;

        DamageToTarget(tmpSession);

        {
            stPacketHeader header;
            stPACKET_SC_DAMAGE packet;
            mpDamage(&header, &packet, pSession->_id, tmpSession->_id, tmpSession->_hp);
            SendBroadcast(NULL, (char*)&header, sizeof(header), (char*)&packet, sizeof(packet));
        }
    }

    return true;
}

void DamageToTarget(Session* targetSession)
{
    targetSession->_hp -= 2;
}

void Update()
{
    static DWORD oldTime = GetTickCount();

    // 너무 자주 들어오면 연산량이 많아지니까 적당히 들어오도록 조절. (최소 20ms 텀)
    DWORD deltaTime = GetTickCount() - oldTime;
    if (deltaTime< 20)
        return;

    list<Session*>::iterator iter;
    for (iter = sessionList.begin(); iter != sessionList.end(); ++iter)
    {
        Session* pSession = *iter;
        if (pSession->_hp <= 0)
        {
            destroyQ.push(pSession);
        }
        else
        {
            switch (pSession->_action)
            {
            case PACKET_MOVE_DIR_LL:
                Move(deltaTime, pSession, pSession->_action);
                break;
            case PACKET_MOVE_DIR_LU:
                Move(deltaTime, pSession, pSession->_action);
                break;
            case PACKET_MOVE_DIR_UU:
                Move(deltaTime, pSession, pSession->_action);
                break;
            case PACKET_MOVE_DIR_RU:
                Move(deltaTime, pSession, pSession->_action);
                break;
            case PACKET_MOVE_DIR_RR:
                Move(deltaTime, pSession, pSession->_action);
                break;
            case PACKET_MOVE_DIR_RD:
                Move(deltaTime, pSession, pSession->_action);
                break;
            case PACKET_MOVE_DIR_DD:
                Move(deltaTime, pSession, pSession->_action);
                break;
            case PACKET_MOVE_DIR_LD:
                Move(deltaTime, pSession, pSession->_action);
                break;
            }
        }
    }

    oldTime = GetTickCount();
}

void Move(DWORD deltaTime, Session* pSession, BYTE dir)
{
    double deltaTimeSec = (double)deltaTime / 1000;

    double tmpX = (double(XMOVE * FRAME) * deltaTimeSec) * dx[dir];
    double tmpY = (double(YMOVE * FRAME) * deltaTimeSec) * dy[dir];

    short nextX = pSession->_dX + tmpX;
    short nextY = pSession->_dY + tmpY;

    if (nextX >= RANGE_MOVE_RIGHT || nextX <= RANGE_MOVE_LEFT)
        return;

    if (nextY >= RANGE_MOVE_BOTTOM || nextY <= RANGE_MOVE_TOP)
        return;

    pSession->_dX = pSession->_dX + tmpX;
    pSession->_dY = pSession->_dY + tmpY;

    pSession->_x = nextX;
    pSession->_y = nextY;

    printf("[MOVE UPDATE] ID:%d _x:%d, _y%d\n", pSession->_id, pSession->_x, pSession->_y);

    return;
}

void DestroySession()
{
    while (!destroyQ.empty())
    {
        Session* tmpSession = destroyQ.front();
        destroyQ.pop();

        stPacketHeader header;
        stPACKET_SC_DELETE_CHARACTER packet;
        mpDeleteCharacter(&header, &packet, tmpSession->_id);

        SendBroadcast(NULL, (char*)&header, sizeof(header), (char*)&packet, sizeof(packet));

        closesocket(tmpSession->_socket);
        sessionList.remove(tmpSession);
        delete tmpSession;
    }
}

bool IsGround(int nextY, int nextX)
{
    if (nextY <= RANGE_MOVE_TOP || nextY >= RANGE_MOVE_BOTTOM || nextX <= RANGE_MOVE_LEFT || nextX >= RANGE_MOVE_RIGHT)
        return false;
    else
        return true;
}

void SendBroadcast(Session* exceptPlayer, char* header, int headerSize, char* packet, int packetSize)
{
    if (exceptPlayer == NULL)
    {
        list<Session*>::iterator iter;
        for (iter = sessionList.begin(); iter != sessionList.end(); ++iter)
        {
            (*iter)->sendQ.Enqueue(header, headerSize);
            (*iter)->sendQ.Enqueue(packet, packetSize);
        }
    }
    else
    {
        list<Session*>::iterator iter;
        for (iter = sessionList.begin(); iter != sessionList.end(); ++iter)
        {
            if (*iter == exceptPlayer)
                continue;

            (*iter)->sendQ.Enqueue(header, headerSize);
            (*iter)->sendQ.Enqueue(packet, packetSize);
        }
    }
}

void SendUnicast(Session* targetPlayer, char* header, int headerSize, char* packet, int packetSize)
{
    targetPlayer->sendQ.Enqueue(header, headerSize);
    targetPlayer->sendQ.Enqueue(packet, packetSize);
}
