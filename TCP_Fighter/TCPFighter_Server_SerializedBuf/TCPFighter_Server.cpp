#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "RingBuffer.lib")
#pragma comment(lib, "SerializeBuffer.lib")

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
#include "SerializeBuffer.h"
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
bool PacketProc(Session* pSession, PACKET_TYPE type, SerializePacket* sPacket);

bool netPacketProc_MoveStart(Session* pSession, SerializePacket* sPacket);
bool netPacketProc_MoveStop(Session* pSession, SerializePacket* sPacket);
bool netPacketProc_Attack1(Session* pSession, SerializePacket* sPacket);
bool netPacketProc_Attack2(Session* pSession, SerializePacket* sPacket);
bool netPacketProc_Attack3(Session* pSession, SerializePacket* sPacket);

void DamageToTarget(Session* targetSession);

// 게임 로직 함수
void Update();
void Move(DWORD deltaTime, Session* pSession, BYTE dir);
void DestroySession();

bool IsGround(int nextY, int nextX);

void SendBroadcast(Session* exceptPlayer, SerializePacket* packet);
void SendUnicast(Session* targetPlayer, SerializePacket* packet);

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
        SerializePacket sPacket;
        
        mpCreateMyCharacter(&sPacket, newSession->_id, newSession->_characterDirection, newSession->_x, newSession->_y);
        SendUnicast(newSession, &sPacket);
    }

    
    // new에게 기존 멤버들 생성 메시지 보내기.
    {
        list<Session*>::iterator iter;
        for (iter = sessionList.begin(); iter != sessionList.end(); ++iter)
        {
            Session* nowSession = *iter;
            if (nowSession == newSession)
                continue;

            SerializePacket sPacket;
            mpCreateOtherCharacter(&sPacket, nowSession->_id, nowSession->_characterDirection, nowSession->_x, nowSession->_y, nowSession->_hp);
            SendUnicast(newSession, &sPacket);
        }

    }

    // new에게 기존 멤버들 액션 메시지 보내기.
    {
        list<Session*>::iterator iter;
        for (iter = sessionList.begin(); iter != sessionList.end(); ++iter)
        {
            Session* nowSession = *iter;
            if (nowSession == newSession)
                continue;

            if (nowSession->_action == PACKET_STOP)
                continue;

            SerializePacket sPacket;
            mpMoveStart(&sPacket, nowSession->_id, nowSession->_action, nowSession->_x, nowSession->_y);
            SendUnicast(newSession, &sPacket);
        }

    }
    
    
    // 기존멤버들에게 new 생성 메시지 보내기.
    {
        SerializePacket sPacket;

        mpCreateOtherCharacter(&sPacket, newSession->_id, newSession->_characterDirection, newSession->_x, newSession->_y, newSession->_hp);
        SendBroadcast(newSession, &sPacket);
    }

    
}

void RecvProc(Session* pSession)
{
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

        SerializePacket sPacket;

        // 헤더 뽑기
        int headerPeekSize = pSession->recvQ.Peek(sPacket.GetBufferPtr(), sizeof(stPacketHeader));
        sPacket.MoveWritePos(headerPeekSize);

        stPacketHeader header;
        sPacket.GetData((char*)&header, sizeof(stPacketHeader));

        int payloadLen = header._size;

        // 페이로드 뽑기
        if (pSession->recvQ.GetUseSize() < sizeof(stPacketHeader) + payloadLen)
            break;

        pSession->recvQ.MoveFront(headerPeekSize);

        sPacket.Clear();

        int peekSize = pSession->recvQ.Peek(sPacket.GetBufferPtr(), payloadLen);
        sPacket.MoveWritePos(peekSize);

        pSession->recvQ.MoveFront(peekSize);

        PacketProc(pSession, (PACKET_TYPE)(header._type), &sPacket);
    }

}

void SendProc(Session* pSession)
{
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

bool PacketProc(Session* pSession, PACKET_TYPE type, SerializePacket* sPacket)
{
    switch (type)
    {
    case PACKET_CS_MOVE_START:
        return netPacketProc_MoveStart(pSession, sPacket);
        break;

    case PACKET_CS_MOVE_STOP:
        return netPacketProc_MoveStop(pSession, sPacket);
        break;

    case PACKET_CS_ATTACK1:
        return netPacketProc_Attack1(pSession, sPacket);
        break;

    case PACKET_CS_ATTACK2:
        return netPacketProc_Attack2(pSession, sPacket);
        break;

    case PACKET_CS_ATTACK3:
        return netPacketProc_Attack3(pSession, sPacket);
        break;
    }

    return true;
}

bool netPacketProc_MoveStart(Session* pSession, SerializePacket* sPacket)
{
    BYTE direction;
    short x;
    short y;

    *sPacket >> direction;
    *sPacket >> x;
    *sPacket >> y;
    
    printf("RECV: MoveStart [PACKET] _x:%d, _y:%d, dir:%d\n", x, y, direction);

    int diffX = abs(x - pSession->_x);
    int diffY = abs(y - pSession->_y);
    if (diffX > ERROR_RANGE || diffY > ERROR_RANGE)
    {
        // 끊어버리자
        destroyQ.push(pSession);

        // 로그 출력
        printf("CLOSE: MoveStart [PACKET] _x:%d, _y:%d [Player] _x:%d, _y:%d \n" 
            ,x, y, pSession->_x, pSession->_y);
    }
    pSession->_x = x;
    pSession->_y = y;

    // 동작변경
    pSession->_action = direction;

    // 시선변경
    switch (direction)
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

    SerializePacket moveStartPacket;

    mpMoveStart(&moveStartPacket, pSession->_id, pSession->_action, pSession->_x, pSession->_y);
    SendBroadcast(pSession, &moveStartPacket);

    return true;
}

bool netPacketProc_MoveStop(Session* pSession, SerializePacket* sPacket)
{
    BYTE direction;
    short x;
    short y;

    *sPacket >> direction;
    *sPacket >> x;
    *sPacket >> y;

    int diffX = abs(pSession->_x - x);
    int diffY = abs(pSession->_y - y);

    if (diffX > ERROR_RANGE || diffY > ERROR_RANGE)
    {
        destroyQ.push(pSession);
        return false;
    }

    pSession->_x = x;
    pSession->_y = y;

    // 시선변경
    switch (direction)
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

    SerializePacket moveStopPacket;

    mpMoveStop(&moveStopPacket, pSession->_id, pSession->_characterDirection, pSession->_x, pSession->_y);
    SendBroadcast(pSession, &moveStopPacket);

    return true;
}

bool netPacketProc_Attack1(Session* pSession, SerializePacket* sPacket)
{
    BYTE direction;
    short x;
    short y;

    *sPacket >> direction;
    *sPacket >> x;
    *sPacket >> y;

    pSession->_characterDirection = direction;
    pSession->_x = x;
    pSession->_y = y;

    // 캐릭터 공격 모션 패킷
    {
        SerializePacket attack1Packet;

        mpAttack1(&attack1Packet, pSession->_id, pSession->_characterDirection, pSession->_x, pSession->_y);
        SendBroadcast(pSession, &attack1Packet);
    }

    
    // 공격 판정 패킷
    int attackRangeMinX;
    int attackRangeMaxX;
    int attackRangeMinY = max(RANGE_MOVE_TOP, y - ATTACK1_RANGE_Y);
    int attackRangeMaxY = min(RANGE_MOVE_BOTTOM, y + ATTACK1_RANGE_Y);

    if (direction == PACKET_MOVE_DIR_LL)
    {
        attackRangeMinX = max(RANGE_MOVE_LEFT, x - ATTACK1_RANGE_X);
        attackRangeMaxX = x;
    }
    else
    {
        attackRangeMinX = x;
        attackRangeMaxX = min(RANGE_MOVE_RIGHT, x + ATTACK1_RANGE_X);
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
            SerializePacket damagePacket;

            mpDamage(&damagePacket, pSession->_id, tmpSession->_id, tmpSession->_hp);
            SendBroadcast(NULL, &damagePacket);
        }
    }

    return true;
}

bool netPacketProc_Attack2(Session* pSession, SerializePacket* sPacket)
{
    BYTE direction;
    short x;
    short y;

    *sPacket >> direction;
    *sPacket >> x;
    *sPacket >> y;


    pSession->_characterDirection = direction;
    pSession->_x = x;
    pSession->_y = y;

    // 캐릭터 공격 모션 패킷
    {
        SerializePacket attack2Packet;

        mpAttack2(&attack2Packet, pSession->_id, pSession->_characterDirection, pSession->_x, pSession->_y);
        SendBroadcast(pSession, &attack2Packet);
    }


    // 공격 판정 패킷
    int attackRangeMinX;
    int attackRangeMaxX;
    int attackRangeMinY = max(RANGE_MOVE_TOP, y - ATTACK2_RANGE_Y);
    int attackRangeMaxY = min(RANGE_MOVE_BOTTOM, y + ATTACK2_RANGE_Y);

    if (direction == PACKET_MOVE_DIR_LL)
    {
        attackRangeMinX = max(RANGE_MOVE_LEFT, x - ATTACK2_RANGE_X);
        attackRangeMaxX = x;
    }
    else
    {
        attackRangeMinX = x;
        attackRangeMaxX = min(RANGE_MOVE_RIGHT, x + ATTACK2_RANGE_X);
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
            SerializePacket damagePacket;

            mpDamage(&damagePacket, pSession->_id, tmpSession->_id, tmpSession->_hp);
            SendBroadcast(NULL, &damagePacket);
        }
    }

    return true;
}

bool netPacketProc_Attack3(Session* pSession, SerializePacket* sPacket)
{
    BYTE direction;
    short x;
    short y;

    *sPacket >> direction;
    *sPacket >> x;
    *sPacket >> y;

    pSession->_characterDirection = direction;
    pSession->_x = x;
    pSession->_y = y;

    // 캐릭터 공격 모션 패킷
    {
        SerializePacket attack3Packet;

        mpAttack3(&attack3Packet, pSession->_id, pSession->_characterDirection, pSession->_x, pSession->_y);
        SendBroadcast(pSession, &attack3Packet);
    }


    // 공격 판정 패킷
    int attackRangeMinX;
    int attackRangeMaxX;
    int attackRangeMinY = max(RANGE_MOVE_TOP, y - ATTACK3_RANGE_Y);
    int attackRangeMaxY = min(RANGE_MOVE_BOTTOM, y + ATTACK3_RANGE_Y);

    if (direction == PACKET_MOVE_DIR_LL)
    {
        attackRangeMinX = max(RANGE_MOVE_LEFT, x - ATTACK3_RANGE_X);
        attackRangeMaxX = x;
    }
    else
    {
        attackRangeMinX = x;
        attackRangeMaxX = min(RANGE_MOVE_RIGHT, x + ATTACK3_RANGE_X);
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
            SerializePacket damagePacket;

            mpDamage(&damagePacket, pSession->_id, tmpSession->_id, tmpSession->_hp);
            SendBroadcast(NULL, &damagePacket);
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

        //stPacketHeader header;
        //stPACKET_SC_DELETE_CHARACTER packet;
        SerializePacket sPacket;

        mpDeleteCharacter(&sPacket, tmpSession->_id);

        //SendBroadcast(NULL, (char*)&header, sizeof(header), (char*)&packet, sizeof(packet));
        SendBroadcast(NULL, &sPacket);

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

void SendBroadcast(Session* exceptPlayer, SerializePacket* packet)
{
    int packetSize = packet->GetDataSize();

    if (exceptPlayer == NULL)
    {
        list<Session*>::iterator iter;
        for (iter = sessionList.begin(); iter != sessionList.end(); ++iter)
        {
            (*iter)->sendQ.Enqueue(packet->GetBufferPtr(), packetSize);
        }
    }
    else
    {
        list<Session*>::iterator iter;
        for (iter = sessionList.begin(); iter != sessionList.end(); ++iter)
        {
            if (*iter == exceptPlayer)
                continue;

            (*iter)->sendQ.Enqueue(packet->GetBufferPtr(), packetSize);
        }
    }
}

void SendUnicast(Session* targetPlayer, SerializePacket* packet)
{
    int packetSize = packet->GetDataSize();

    targetPlayer->sendQ.Enqueue(packet->GetBufferPtr(), packetSize);
}
