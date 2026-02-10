#include "stdafx.h"

#include "CommonProtocol.h"
#include "Monitoring.h"
#include "FieldStruct.h"
#include "Field.h"
#include "NetServer.h"
#include "GameManager.h"
#include "MyField_Echo.h"

MyField_Echo::MyField_Echo()
{
}

MyField_Echo::~MyField_Echo()
{
}

void MyField_Echo::OnEnter(DWORD64 sessionID)
{
	// TODO(콘텐츠):
	// 1. 넘어온 sessionID를 Key로 Player생성하기.
	// 2. newPlayer에게 줄 데이터들 sendPacket();

	// 근데 사실 여기에 Enter가 들어올 일이 없다.
}

void MyField_Echo::OnEnterWithPlayer(DWORD64 sessionID, void* pPlayer)
{
	// TODO(콘텐츠):
	// 1. 넘어온 sessionID를 Key로 넘어온 Player 그대로 이양받기
	// 2. newPlayer에게 줄 데이터들 sendPacket();

	Player* movePlayer = (Player*)pPlayer;


	// 중복로그인 검사 + 등록
	INT64 tmpAccountNo = movePlayer->accountNo;
	DWORD64 oldSID = 0;
	auto iter = accountNoToSID.find(tmpAccountNo);
	if (iter != accountNoToSID.end())
	{
		oldSID = iter->second;
	}
	accountNoToSID[tmpAccountNo] = sessionID;

	// oldSID가 있고, 그게 지금 세션이 아리나면 중복로그인 -> old 끊기
	if (oldSID != 0 && oldSID != sessionID)
	{
		Disconnect(oldSID);
		_LOG(dfLOG_LEVEL_SYSTEM, L"%ls\n", L"중복로그인");
	}

	
	movePlayer->heartbeat = GetTickCount64();

	PlayerArr.push_back(movePlayer);
	SIDToPlayer.insert({ sessionID, movePlayer });

	// 여기서 en_PACKET_CS_GAME_RES_LOGIN 응답을 해주자.
	SerializePacketPtr newPacket = SerializePacketPtr::MakeSerializePacket();
	newPacket.Clear();

	WORD type = en_PACKET_CS_GAME_RES_LOGIN;
	BYTE status = dfGAME_LOGIN_OK;
	INT64 accountNo = movePlayer->accountNo;

	newPacket << type;
	newPacket << status;
	newPacket << accountNo;

	SendPacket(sessionID, newPacket);

	Monitoring::GetInstance()->Increase(MonitorType::GamePlayerCount);
}

void MyField_Echo::OnRecv(DWORD64 sessionID, SerializePacketPtr sPacket)
{
	// TODO(콘텐츠):
	// 1. OnRecv로 메시지가 들어왔다?
	// 2. 바로 메시지 처리하면된다.

	WORD msgType;
	sPacket >> msgType;

	bool flag = true;

	switch (msgType)
	{
	case en_PACKET_CS_GAME_REQ_ECHO:
		flag = PacketProc_Echo(sessionID, sPacket);
		Monitoring::GetInstance()->Increase(MonitorType::RecvEchoTPS);
		break;
	case en_PACKET_CS_GAME_REQ_HEARTBEAT:
		flag = PacketProc_HB(sessionID);
		Monitoring::GetInstance()->Increase(MonitorType::RecvHeartbeatTPS);
		break;
	dafault:
		Disconnect(sessionID);
		_LOG(dfLOG_LEVEL_SYSTEM, L"%ls\n", L"attack #2 Disconnect");
		return;
	}
}

void MyField_Echo::OnUpdate()
{
	// TODO(콘텐츠):
	// 1. 하트비트 정도만 체크하자.

	static DWORD64 oldTime = GetTickCount64();

	DWORD64 nowTime = GetTickCount64();
	DWORD64 diff = nowTime - oldTime;
	if (diff < 1000 * 60)
		return;

	for (int i = 0; i < PlayerArr.size(); i++)
	{
		DWORD64 sid = PlayerArr[i]->sessionID;
		DWORD64 hb = PlayerArr[i]->heartbeat;

		if (nowTime - hb < 1000 * 60 * 2)
			continue;

		Disconnect(sid);
		_LOG(dfLOG_LEVEL_SYSTEM, L"%ls\n", L"하트비트 Disconnect");
	}

	oldTime = nowTime;
}

void MyField_Echo::OnLeave(DWORD64 sessionID)
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

	INT64 accountNo = pPlayer->accountNo;
	accountNoToSID.erase(accountNo);


	for (int i = 0; i < PlayerArr.size(); i++)
	{
		if (PlayerArr[i] != pPlayer)
			continue;

		PlayerArr[i] = PlayerArr.back();
		PlayerArr.pop_back();
		break;
	}

	Field::playerPool.Free(pPlayer);

	Monitoring::GetInstance()->Decrease(MonitorType::GamePlayerCount);
}

void MyField_Echo::OnFieldLeave(DWORD64 sessionID)
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

	INT64 accountNo = pPlayer->accountNo;
	accountNoToSID.erase(accountNo);

	for (int i = 0; i < PlayerArr.size(); i++)
	{
		if (PlayerArr[i] != pPlayer)
			continue;

		PlayerArr[i] = PlayerArr.back();
		PlayerArr.pop_back();
		break;
	}

	Monitoring::GetInstance()->Decrease(MonitorType::GamePlayerCount);
}
