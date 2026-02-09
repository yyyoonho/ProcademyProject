#include "stdafx.h"

#include "CommonProtocol.h"
#include "Monitoring.h"
#include "FieldStruct.h"
#include "Field.h"
#include "NetServer.h"
#include "GameManager.h"
#include "MyField_Auth.h"

bool MyField_Auth::PacketProc_Login(DWORD64 sessionID, SerializePacketPtr sPacket)
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
		Disconnect(sessionID);
		//_LOG(dfLOG_LEVEL_SYSTEM, L"%ls\n", L"attack #3 Disconnect");
		return false;
	}

	LONG authPlayerCount = Monitoring::GetInstance()->GetInterlocked(MonitorType::AuthPlayerCount);
	LONG echoPlayerCount = Monitoring::GetInstance()->GetInterlocked(MonitorType::GamePlayerCount);
	if (authPlayerCount + echoPlayerCount > MAXPLAYER)
	{
		Disconnect(sessionID);
		//_LOG(dfLOG_LEVEL_SYSTEM, L"%ls\n", L"attack #6 Disconnect");
		return false;
	}

	// attack #14
	// 비정상 accountNo에 대한 컷
	{
		bool valid = false;

		// 첫 구간: 1 ~ 5001
		if (accountNo >= 1 && accountNo <= 5001)
		{
			valid = true;
		}
		else if (accountNo >= 10000 && accountNo <= 95000)
		{
			INT64 base = (accountNo / 10000) * 10000;
			if (accountNo >= base && accountNo <= base + 5000)
			{
				valid = true;
			}
		}

		if (!valid)
		{
			Disconnect(sessionID);
			//_LOG(dfLOG_LEVEL_SYSTEM, L"%ls\n", L"attack #14");

			return false;
		}
	}


	Player* pPlayer = iter->second;

	pPlayer->accountNo = accountNo;
	pPlayer->heartbeat = GetTickCount64();

	MoveToOtherField(FieldName::Echo, sessionID, (void*)pPlayer);

	return true;
}

bool MyField_Auth::PacketProc_HB(DWORD64 sessionID)
{
	auto iter = SIDToPlayer.find(sessionID);
	if (iter == SIDToPlayer.end())
	{
		Disconnect(sessionID);
		//_LOG(dfLOG_LEVEL_SYSTEM, L"%ls\n", L"attack #3 Disconnect");
		return false;
	}

	Player* pPlayer = iter->second;

	pPlayer->heartbeat = GetTickCount64();

	return true;
}
