#include "stdafx.h"

#include "CommonProtocol.h"
#include "FieldStruct.h"
#include "Field.h"
#include "NetServer.h"
#include "GameManager.h"
#include "MyField_Echo.h"

bool MyField_Echo::PacketProc_Echo(DWORD64 sessionID, SerializePacketPtr sPacket)
{
	WORD type;
	INT64 accountNo;
	LONGLONG sendTick;
	
	sPacket >> accountNo;
	sPacket >> sendTick;

		
	INT64 tmpAccountNo = pGameManager->GetAccountNoFromSID(sessionID);
	if (tmpAccountNo != accountNo || tmpAccountNo == -1)
	{
		Disconnect(sessionID);
		_LOG(dfLOG_LEVEL_SYSTEM, L"%ls\n", L"attack #10 Disconnect");
		return false;
	}


	sPacket.Clear();

	type = en_PACKET_CS_GAME_RES_ECHO;
	sPacket << type;
	sPacket << accountNo;
	sPacket << sendTick;
	
	SendPacket(sessionID, sPacket);

	return true;
}

bool MyField_Echo::PacketProc_HB(DWORD64 sessionID)
{
	auto iter = SIDToPlayer.find(sessionID);
	if (iter == SIDToPlayer.end())
	{
		Disconnect(sessionID);
		_LOG(dfLOG_LEVEL_SYSTEM, L"%ls\n", L"attack #3 Disconnect");
		return false;
	}

	Player* pPlayer = iter->second;
	pPlayer->heartbeat = GetTickCount64();

	return true;
}

bool MyField_Echo::CheckMessageRateLimit(DWORD64 sessionID)
{
	Player* player = pGameManager->GetPlayerFromSID(sessionID);
	if (player == nullptr)
	{
		return false;
	}

	DWORD now = timeGetTime();
	bool ret = true;

	// 1초 갱신
	if (now - player->rateLimitTick >= 1000)
	{
		player->rateLimitTick = now;
		player->rateLimitMsgCount = 0;
	}

	// 메시지 갯수 증가
	player->rateLimitMsgCount++;

	// 아웃카운트 검사
	if (player->rateLimitMsgCount > 900)
	{
		player->rateLimitOutCount++;

		if (player->rateLimitOutCount >= 2)
		{
			_LOG(dfLOG_LEVEL_SYSTEM, L"%ls\n", L"rateLimitOut");
			ret = false;
		}
		else
		{
			player->rateLimitTick = now;
			player->rateLimitMsgCount = 0;
		}
	}

	return ret;
}