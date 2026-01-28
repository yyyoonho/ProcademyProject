#include "stdafx.h"

#include "CommonProtocol.h"
#include "FieldStruct.h"
#include "Field.h"
#include "NetServer.h"
#include "GameManager.h"
#include "MyField_Echo.h"


void MyField_Echo::PacketProc_Echo(DWORD64 sessionID, SerializePacketPtr sPacket)
{
	WORD type;
	INT64 accountNo;
	LONGLONG sendTick;

	type = en_PACKET_CS_GAME_RES_ECHO;
	sPacket >> accountNo;
	sPacket >> sendTick;

	SerializePacketPtr newPacket = SerializePacketPtr::MakeSerializePacket();
	newPacket.Clear();

	newPacket << type;
	newPacket << accountNo;
	newPacket << sendTick;

	SendPacket(sessionID, newPacket);
}

void MyField_Echo::PacketProc_HB(DWORD64 sessionID)
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
