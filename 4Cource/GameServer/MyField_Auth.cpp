#include "stdafx.h"

#include "CommonProtocol.h"
#include "Monitoring.h"
#include "FieldStruct.h"
#include "Field.h"
#include "NetServer.h"
#include "GameManager.h"
#include "MyField_Auth.h"

MyField_Auth::MyField_Auth()
{
}

MyField_Auth::~MyField_Auth()
{
}

void MyField_Auth::OnEnter(DWORD64 sessionID)
{
	// TODO(콘텐츠):
	// 1. 넘어온 sessionID를 Key로 Player생성하기.
	// 2. newPlayer에게 줄 데이터들 sendPacket();

	Player* newPlayer = playerPool.Alloc();

	newPlayer->sessionID;
	newPlayer->accountNo = INT64_MAX;
	newPlayer->heartbeat = GetTickCount64();
	newPlayer->state = PLAYER_STATE::ACCEPT;

	loginWaitPlayerArr.push_back(newPlayer);
	SIDToPlayer.insert({ sessionID, newPlayer });

	Monitoring::GetInstance()->Increase(MonitorType::AuthPlayerCount);
}

void MyField_Auth::OnEnterWithPlayer(DWORD64 sessionID, void* pPlayer)
{
	// TODO(콘텐츠):
	// 1. 넘어온 sessionID를 Key로 넘어온 Player 그대로 이양받기
	// 2. newPlayer에게 줄 데이터들 sendPacket();

	Player* movePlayer = (Player*)pPlayer;
	movePlayer->heartbeat = GetTickCount64();

	loginWaitPlayerArr.push_back(movePlayer);
	SIDToPlayer.insert({ sessionID, movePlayer });

	Monitoring::GetInstance()->Increase(MonitorType::AuthPlayerCount);
}

void MyField_Auth::OnRecv(DWORD64 sessionID, SerializePacketPtr sPacket)
{
	// TODO(콘텐츠):
	// 1. OnRecv로 메시지가 들어왔다?
	// 2. 바로 메시지 처리하면된다.

	WORD msgType;
	sPacket >> msgType;

	switch (msgType)
	{
	case en_PACKET_CS_GAME_REQ_LOGIN:
		PacketProc_Login(sessionID, sPacket);
		Monitoring::GetInstance()->Increase(MonitorType::RecvLoginTPS);
		break;
	case en_PACKET_CS_GAME_REQ_HEARTBEAT:
		PacketProc_HB(sessionID);
		Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::RecvHeartbeatTPS);
		break;
	}
}

void MyField_Auth::OnUpdate()
{
	// TODO(콘텐츠):
	// 1. 하트비트 정도만 체크하자.

	static DWORD64 oldTime = GetTickCount64();

	DWORD64 nowTime = GetTickCount64();
	DWORD64 diff = nowTime - oldTime;
	if (diff < 1000 * 2)
		return;

	for (int i = 0; i < loginWaitPlayerArr.size(); i++)
	{
		DWORD64 sid = loginWaitPlayerArr[i]->sessionID;
		DWORD64 hb = loginWaitPlayerArr[i]->heartbeat;

		if (nowTime - hb < 1000 * 10)
			continue;

		Disconnect(sid);
	}
}

void MyField_Auth::OnLeave(DWORD64 sessionID)
{
	// TODO(콘텐츠):
	// 1. OnLeave로 넘어온 sessionID를 확인하자.
	// 2. sessionID와 매칭되는 Player를 없애자. (free)

	auto iter = SIDToPlayer.find(sessionID);
	if (iter == SIDToPlayer.end())
	{
		DebugBreak();
	}

	Player* pPlayer = iter->second;
	SIDToPlayer.erase(iter);

	for (int i = 0; i < loginWaitPlayerArr.size(); i++)
	{
		if (loginWaitPlayerArr[i] != pPlayer)
			continue;

		loginWaitPlayerArr[i] = loginWaitPlayerArr.back();
		loginWaitPlayerArr.pop_back();
		break;
	}
	
	playerPool.Free(pPlayer);

	Monitoring::GetInstance()->Decrease(MonitorType::AuthPlayerCount);
}

void MyField_Auth::OnFieldLeave(DWORD64 sessionID)
{
	// TODO(콘텐츠):
	// 1. OnLeave로 넘어온 sessionID를 확인하자.
	// 2. sessionID와 매칭되는 Player를 없애자. (just 현재 필드 자료구조에서만 제거.)

	auto iter = SIDToPlayer.find(sessionID);
	if (iter == SIDToPlayer.end())
	{
		DebugBreak();
	}

	Player* pPlayer = iter->second;
	SIDToPlayer.erase(iter);

	for (int i = 0; i < loginWaitPlayerArr.size(); i++)
	{
		if (loginWaitPlayerArr[i] != pPlayer)
			continue;

		loginWaitPlayerArr[i] = loginWaitPlayerArr.back();
		loginWaitPlayerArr.pop_back();
		break;
	}

	Monitoring::GetInstance()->Decrease(MonitorType::AuthPlayerCount);
}

