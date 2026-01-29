#include "stdafx.h"

#include "CommonProtocol.h"
#include "FieldStruct.h"
#include "Field.h"
#include "NetServer.h"
#include "GameManager.h"
#include "MyField_Auth.h"

void MyField_Auth::PacketProc_Login(DWORD64 sessionID, SerializePacketPtr sPacket)
{
	INT64 accountNo;
	char sessionKey[64];
	int version;

	sPacket >> accountNo;
	sPacket.GetData(sessionKey, sizeof(char) * 64);
	sPacket >> version;

	auto iter = SIDToPlayer.find(sessionID);
	if (iter == SIDToPlayer.end())
	{
		DebugBreak();
	}

	Player* pPlayer = iter->second;

	pPlayer->accountNo = accountNo;
	//memcpy(pPlayer->sessionKey, sessionKey, 64);
	pPlayer->heartbeat = GetTickCount64();

	MoveToOtherField(FieldName::Echo, sessionID, (void*)pPlayer);
}

void MyField_Auth::PacketProc_HB(DWORD64 sessionID)
{
	auto iter = SIDToPlayer.find(sessionID);
	if (iter == SIDToPlayer.end())
	{
		DebugBreak();
		return;
	}

	Player* pPlayer = iter->second;

	pPlayer->heartbeat = GetTickCount64();
}
