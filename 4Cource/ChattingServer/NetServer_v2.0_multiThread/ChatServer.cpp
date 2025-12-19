#include "stdafx.h"

#include "LogManager.h"
#include "Monitoring.h"
#include "NetServer.h"
#include "PacketProc.h"
#include "CommonProtocol.h"

#include "ChatServer.h"

using namespace std;

int dy[9] = { -1,-1,-1,0,0,0,1,1,1 };
int dx[9] = { -1,0,1,-1,0,1,-1,0,1 };

HANDLE hHeartbeatThread;
HANDLE hThread_Monitoring;
HANDLE hEvent_Quit;

LONG releaseCount;

bool ChatServer::Start(const WCHAR* ipAddress, unsigned short port, unsigned short workerThreadCount, unsigned short coreSkip, bool isNagle, unsigned int maximumSessionCount, bool codecOnOff = true)
{
	bool ret = CNetServer::Start(ipAddress, port, workerThreadCount, coreSkip, isNagle, maximumSessionCount, codecOnOff);
	if (ret == false)
	{
		return false;
	}

	for (int i = 0; i < MAX_SECTOR_Y; i++)
	{
		for (int j = 0; j < MAX_SECTOR_X; j++)
		{
			InitializeSRWLock(&sector_Lock[i][j]);
		}
	}

	InitializeSRWLock(&tmpPlayerArrLock);
	InitializeSRWLock(&tmpSIDToPlayerLock);
	InitializeSRWLock(&playerArrLock);
	InitializeSRWLock(&SIDToPlayerLock);
	InitializeSRWLock(&accountNoToPlayerLock);

	hEvent_Quit = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (hEvent_Quit == 0)
		return 0;

	hThread_Monitoring = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&MonitorThreadRun, this, NULL, NULL);

	return true;
}

void ChatServer::Stop()
{
	
}

bool ChatServer::OnConnectionRequest(SOCKADDR_IN clientAddr)
{
	return false;
}

void ChatServer::OnAccept(DWORD64 sessionID)
{
	Player* newPlayer = playerPool.Alloc();

	if (newPlayer->IsInitLock == false)
	{
		InitializeSRWLock(&newPlayer->playerLock);
		newPlayer->IsInitLock = true;
	}

	{
		AcquireSRWLockExclusive(&newPlayer->playerLock);
		newPlayer->sessionID = sessionID;
		newPlayer->state = PLAYER_STATE::ACCEPT;
		ReleaseSRWLockExclusive(&newPlayer->playerLock);
	}
	
	{
		AcquireSRWLockExclusive(&tmpPlayerArrLock);
		tmpPlayerArr.push_back(newPlayer);
		ReleaseSRWLockExclusive(&tmpPlayerArrLock);
	}

	{
		AcquireSRWLockExclusive(&tmpSIDToPlayerLock);
		tmpSIDToPlayer.insert({ sessionID, newPlayer });
		ReleaseSRWLockExclusive(&tmpSIDToPlayerLock);
	}
}

void ChatServer::OnRelease(DWORD64 sessionID)
{
	InterlockedIncrement(&releaseCount);

	bool ret = ReleaseTmpPlayer(sessionID);

	if (ret == true)
		return;

	ReleaseOriginPlayer(sessionID);

}

void ChatServer::OnMessage(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	PacketProc(sessionID, pPacket);
}

void ChatServer::OnError(int errorCode, WCHAR* errorComment)
{
}

void ChatServer::PacketProc(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	WORD msgType;
	pPacket >> msgType;

	switch (msgType)
	{
	case en_PACKET_CS_CHAT_REQ_LOGIN:
		PacketProc_Login(sessionID, pPacket);
		Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::RecvMessageLoginTPS);
		break;

	case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
		PacketProc_SectorMove(sessionID, pPacket);
		Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::RecvMessageMoveTPS);
		break;

	case en_PACKET_CS_CHAT_REQ_MESSAGE:
		PacketProc_Message(sessionID, pPacket);
		Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::RecvMessageChatTPS);
		break;

	case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
		PacketProc_Heartbeat(sessionID);
		break;
	}

	//UpdateHeartbeat(sessionID);
}

void ChatServer::PacketProc_Login(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	INT64 accountNo;
	pPacket >> accountNo;

	BYTE status = 1;

	SerializePacketPtr newPacket = SerializePacketPtr::MakeSerializePacket();
	newPacket.Clear();

	// 중복로그인 체크
	bool isAlreadyLoggedin = CheckDuplicateLogin(accountNo);

	// 중복로그인 O
	if (isAlreadyLoggedin == TRUE)
	{
		_LOG(dfLOG_LEVEL_DEBUG, L"%ls %ld\n", L"중복로그인 accountNo: ", accountNo);

		Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::DisconnectTotal_alreadyLogin);

		WORD type = en_PACKET_CS_CHAT_RES_LOGIN;
		newPacket << type;
		status = 0;
		newPacket << status;
		newPacket << accountNo;

		// new disconnect
		SendPacket(sessionID, newPacket);
		Disconnect(sessionID);
		
		// old disconnect
		AcquireSRWLockShared(&accountNoToPlayerLock);
		auto iter = accountNoToPlayer.find(accountNo);
		if (iter == accountNoToPlayer.end())
		{
			ReleaseSRWLockShared(&accountNoToPlayerLock);
			return;
		}

		Player* originPlayer = iter->second;
		ReleaseSRWLockShared(&accountNoToPlayerLock);
		
		AcquireSRWLockShared(&originPlayer->playerLock);
		if (originPlayer->accountNo != accountNo)
		{
			ReleaseSRWLockShared(&originPlayer->playerLock);
			return;
		}

		DWORD64 originSID = originPlayer->sessionID;

		ReleaseSRWLockShared(&originPlayer->playerLock);


		Disconnect(originSID);
	}

	// 정상로그인 O
	else
	{
		WORD type = en_PACKET_CS_CHAT_RES_LOGIN;
		newPacket << type;
		newPacket << status;
		newPacket << accountNo;


		AcquireSRWLockExclusive(&tmpSIDToPlayerLock);

		auto iter = tmpSIDToPlayer.find(sessionID);
		if (iter == tmpSIDToPlayer.end())
		{
			DebugBreak();
			ReleaseSRWLockExclusive(&tmpSIDToPlayerLock);
			return;
		}

		Player* loginPlayer = iter->second;

		tmpSIDToPlayer.erase(iter);
		ReleaseSRWLockExclusive(&tmpSIDToPlayerLock);


		AcquireSRWLockExclusive(&tmpPlayerArrLock);
		for (int i = 0; i < tmpPlayerArr.size(); i++)
		{
			if (tmpPlayerArr[i] != loginPlayer)
				continue;

			tmpPlayerArr[i] = tmpPlayerArr.back();
			tmpPlayerArr.pop_back();
			break;
		}
		ReleaseSRWLockExclusive(&tmpPlayerArrLock);

		/**************************************************************/

		AcquireSRWLockExclusive(&loginPlayer->playerLock);

		loginPlayer->sessionID = sessionID;
		loginPlayer->accountNo = accountNo;
		pPacket.GetData((char*)loginPlayer->ID, sizeof(WCHAR) * 20);
		pPacket.GetData((char*)loginPlayer->nickName, sizeof(WCHAR) * 20);
		pPacket.GetData(loginPlayer->sessionKey, sizeof(char) * 64);
		loginPlayer->heartbeat = timeGetTime();
		loginPlayer->state = PLAYER_STATE::LOGIN;

		ReleaseSRWLockExclusive(&loginPlayer->playerLock);


		AcquireSRWLockExclusive(&playerArrLock);
		playerArr.push_back(loginPlayer);
		ReleaseSRWLockExclusive(&playerArrLock);

		AcquireSRWLockExclusive(&SIDToPlayerLock);
		SIDToPlayer.insert({ sessionID,loginPlayer });
		ReleaseSRWLockExclusive(&SIDToPlayerLock);

		AcquireSRWLockExclusive(&accountNoToPlayerLock);
		accountNoToPlayer.insert({ accountNo, loginPlayer });
		ReleaseSRWLockExclusive(&accountNoToPlayerLock);

		SendPacket(sessionID, newPacket);
		Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::PlayerCount);
	}
}

bool ChatServer::CheckDuplicateLogin(INT64 accountNo)
{
	bool ret = false;

	AcquireSRWLockShared(&accountNoToPlayerLock);

	if (accountNoToPlayer.find(accountNo) != accountNoToPlayer.end())
		ret = true;

	ReleaseSRWLockShared(&accountNoToPlayerLock);

	return ret;
}

void ChatServer::PacketProc_SectorMove(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	INT64 accountNo;
	WORD newSectorY;
	WORD newSectorX;

	pPacket >> accountNo;
	pPacket >> newSectorY;
	pPacket >> newSectorX;

	AcquireSRWLockShared(&SIDToPlayerLock);
	auto iter = SIDToPlayer.find(sessionID);
	if (iter == SIDToPlayer.end())
	{
		ReleaseSRWLockShared(&SIDToPlayerLock);
		return;
	}

	Player* movePlayer = iter->second;
	ReleaseSRWLockShared(&SIDToPlayerLock);


	AcquireSRWLockExclusive(&movePlayer->playerLock);
	if (movePlayer->sessionID != sessionID)
	{
		ReleaseSRWLockExclusive(&movePlayer->playerLock);
		return;
	}

	if (movePlayer->state == PLAYER_STATE::LOGIN)
	{
		movePlayer->sectorY = newSectorY;
		movePlayer->sectorX = newSectorX;
		movePlayer->state = PLAYER_STATE::PLAY;

		AcquireSRWLockExclusive(&sector_Lock[newSectorY][newSectorX]);
		sector[newSectorY][newSectorX].push_back(sessionID);
		ReleaseSRWLockExclusive(&sector_Lock[newSectorY][newSectorX]);
	}
	else if (movePlayer->state == PLAYER_STATE::PLAY)
	{
		WORD oldSectorY = movePlayer->sectorY;
		WORD oldSectorX = movePlayer->sectorX;

		movePlayer->sectorY = newSectorY;
		movePlayer->sectorX = newSectorX;

		if (newSectorY == oldSectorY && newSectorX == oldSectorX)
		{
			int a = 3;
		}
		else
		{
			WORD firstY;
			WORD firstX;
			WORD secondY;
			WORD secondX;

			if (oldSectorY < newSectorY)
			{
				firstY = oldSectorY;
				firstX = oldSectorX;

				secondY = newSectorY;
				secondX = newSectorX;
			}
			else if (oldSectorY > newSectorY)
			{
				firstY = newSectorY;
				firstX = newSectorX;

				secondY = oldSectorY;
				secondX = oldSectorX;
			}
			else if (oldSectorX < newSectorX)
			{
				firstY = oldSectorY;
				firstX = oldSectorX;

				secondY = newSectorY;
				secondX = newSectorX;
			}
			else
			{
				firstY = newSectorY;
				firstX = newSectorX;

				secondY = oldSectorY;
				secondX = oldSectorX;
			}

			AcquireSRWLockExclusive(&sector_Lock[firstY][firstX]);
			AcquireSRWLockExclusive(&sector_Lock[secondY][secondX]);

			for (int i = 0; i < sector[oldSectorY][oldSectorX].size(); i++)
			{
				if (sector[oldSectorY][oldSectorX][i] != sessionID)
					continue;

				sector[oldSectorY][oldSectorX][i] = sector[oldSectorY][oldSectorX].back();
				sector[oldSectorY][oldSectorX].pop_back();
				break;
			}

			sector[newSectorY][newSectorX].push_back(sessionID);

			ReleaseSRWLockExclusive(&sector_Lock[firstY][firstX]);
			ReleaseSRWLockExclusive(&sector_Lock[secondY][secondX]);
		}
	}
	ReleaseSRWLockExclusive(&movePlayer->playerLock);


	SerializePacketPtr newPacket = SerializePacketPtr::MakeSerializePacket();
	newPacket.Clear();

	WORD type = en_PACKET_CS_CHAT_RES_SECTOR_MOVE;

	newPacket << type;
	newPacket << accountNo;
	newPacket << newSectorX;
	newPacket << newSectorY;

	SendPacket(sessionID, newPacket);
}

void ChatServer::PacketProc_Message(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	INT64 accountNo;
	WORD msgLen;
	WCHAR msg[MAX_MSGLEN];

	pPacket >> accountNo;
	pPacket >> msgLen;
	pPacket.GetData((char*)msg, msgLen);

	SerializePacketPtr newPacket = SerializePacketPtr::MakeSerializePacket();
	newPacket.Clear();

	WORD type = en_PACKET_CS_CHAT_RES_MESSAGE;

	newPacket << type;
	newPacket << accountNo;

	AcquireSRWLockShared(&SIDToPlayerLock);
	auto iter = SIDToPlayer.find(sessionID);
	if (iter == SIDToPlayer.end())
	{
		ReleaseSRWLockShared(&SIDToPlayerLock);
		return;
	}

	Player* msgPlayer = iter->second;
	ReleaseSRWLockShared(&SIDToPlayerLock);



	AcquireSRWLockShared(&msgPlayer->playerLock);
	if (msgPlayer->sessionID != sessionID)
	{
		ReleaseSRWLockShared(&msgPlayer->playerLock);
		return;
	}

	WORD sectorY = msgPlayer->sectorY;
	WORD sectorX = msgPlayer->sectorX;

	newPacket.Putdata((char*)msgPlayer->ID, sizeof(WCHAR) * 20);
	newPacket.Putdata((char*)msgPlayer->nickName, sizeof(WCHAR) * 20);

	ReleaseSRWLockShared(&msgPlayer->playerLock);

	newPacket << msgLen;
	newPacket.Putdata((char*)msg, msgLen);


	LockAroundSector_Shared(sectorY, sectorX);
	for (int i = 0; i < 9; i++)
	{
		WORD nextY = sectorY + dy[i];
		WORD nextX = sectorX + dx[i];

		if (nextY < 0 || nextY >= MAX_SECTOR_Y || nextX < 0 || nextX >= MAX_SECTOR_X)
			continue;

		for (int j = 0; j < sector[nextY][nextX].size(); j++)
		{
			SendPacket(sector[nextY][nextX][j], newPacket);
		}
	}

	UnLockAroundSector_Shared(sectorY, sectorX);

}

void ChatServer::PacketProc_Heartbeat(DWORD64 sessionID)
{
	UpdateHeartbeat(sessionID);
}

void ChatServer::UpdateHeartbeat(DWORD64 sessionID)
{
	AcquireSRWLockShared(&SIDToPlayerLock);
	auto iter = SIDToPlayer.find(sessionID);
	if (iter == SIDToPlayer.end())
	{
		ReleaseSRWLockShared(&SIDToPlayerLock);
		return;
	}

	Player* targetPlayer = iter->second;
	ReleaseSRWLockShared(&SIDToPlayerLock);


	AcquireSRWLockExclusive(&targetPlayer->playerLock);
	if (targetPlayer->sessionID != sessionID)
	{
		ReleaseSRWLockExclusive(&targetPlayer->playerLock);
		return;
	}

	targetPlayer->heartbeat = timeGetTime();
	ReleaseSRWLockExclusive(&targetPlayer->playerLock);
}

void ChatServer::DisconnectUnresponsivePlayers()
{
	

}

bool ChatServer::ReleaseTmpPlayer(DWORD64 sessionID)
{
	// release대상이 tmp인 상황 (playerState == accept)
	AcquireSRWLockExclusive(&tmpSIDToPlayerLock);

	auto iter = tmpSIDToPlayer.find(sessionID);
	if (iter == tmpSIDToPlayer.end())
	{
		//DebugBreak();
		ReleaseSRWLockExclusive(&tmpSIDToPlayerLock);
		return false;
	}

	Player* removed = iter->second;

	tmpSIDToPlayer.erase(iter);
	ReleaseSRWLockExclusive(&tmpSIDToPlayerLock);

	AcquireSRWLockExclusive(&tmpPlayerArrLock);
	for (int i = 0; i < tmpPlayerArr.size(); i++)
	{
		if (tmpPlayerArr[i] != removed)
			continue;

		tmpPlayerArr[i] = tmpPlayerArr.back();
		tmpPlayerArr.pop_back();
		break;
	}
	ReleaseSRWLockExclusive(&tmpPlayerArrLock);

	playerPool.Free(removed);

	return true;
}

bool ChatServer::ReleaseOriginPlayer(DWORD64 sessionID)
{
	// release대상이 origin인 상황 (playerstate == login or play)
	
	AcquireSRWLockExclusive(&SIDToPlayerLock);
	auto iter = SIDToPlayer.find(sessionID);
	if (iter == SIDToPlayer.end())
	{
		DebugBreak();
		ReleaseSRWLockExclusive(&SIDToPlayerLock);
		return true;
	}

	Player* removed = iter->second;

	SIDToPlayer.erase(iter);
	ReleaseSRWLockExclusive(&SIDToPlayerLock);

	AcquireSRWLockShared(&removed->playerLock);
	int accountNo = removed->accountNo;
	PLAYER_STATE state = removed->state;
	WORD sectorY = removed->sectorY;
	WORD sectorX = removed->sectorX;
	ReleaseSRWLockShared(&removed->playerLock);


	AcquireSRWLockExclusive(&accountNoToPlayerLock);
	auto iter2 = accountNoToPlayer.find(accountNo);
	if (iter2 == accountNoToPlayer.end())
	{
		DebugBreak();
		ReleaseSRWLockExclusive(&accountNoToPlayerLock);
		return false;
	}

	accountNoToPlayer.erase(iter2);
	ReleaseSRWLockExclusive(&accountNoToPlayerLock);

	if (state == PLAYER_STATE::PLAY)
	{
		AcquireSRWLockExclusive(&sector_Lock[sectorY][sectorX]);
		for (int i = 0; i < sector[sectorY][sectorX].size(); i++)
		{
			if (sector[sectorY][sectorX][i] != sessionID)
				continue;

			sector[sectorY][sectorX][i] = sector[sectorY][sectorX].back();
			sector[sectorY][sectorX].pop_back();
			break;
		}

		ReleaseSRWLockExclusive(&sector_Lock[sectorY][sectorX]);
	}

	playerPool.Free(removed);

	return true;
}

void ChatServer::LockAroundSector_Shared(WORD sectorY, WORD sectorX)
{
	for (int i = 0; i < 9; i++)
	{
		WORD nextY = sectorY + dy[i];
		WORD nextX = sectorX + dx[i];

		if (nextY < 0 || nextY >= MAX_SECTOR_Y || nextX < 0 || nextX >= MAX_SECTOR_X)
			continue;

		AcquireSRWLockShared(&sector_Lock[nextY][nextX]);
	}
}

void ChatServer::UnLockAroundSector_Shared(WORD sectorY, WORD sectorX)
{
	for (int i = 0; i < 9; i++)
	{
		WORD nextY = sectorY + dy[i];
		WORD nextX = sectorX + dx[i];

		if (nextY < 0 || nextY >= MAX_SECTOR_Y || nextX < 0 || nextX >= MAX_SECTOR_X)
			continue;

		ReleaseSRWLockShared(&sector_Lock[nextY][nextX]);
	}
}

void ChatServer::HeartbeatThreadRun(LPVOID* lParam)
{
	ChatServer* self = (ChatServer*)lParam;
	self->HeartbeatThread();
}

void ChatServer::HeartbeatThread()
{
	while (1)
	{
		DWORD ret = WaitForSingleObject(hEvent_Quit, 2000);
		if (ret == WAIT_OBJECT_0)
			break;

		DWORD nowTime = timeGetTime();

		
	}
}

void ChatServer::MonitorThreadRun(LPVOID* lParam)
{
	ChatServer* self = (ChatServer*)lParam;
	self->MonitorThread();
}

void ChatServer::MonitorThread()
{
	while (1)
	{
		DWORD ret = WaitForSingleObject(hEvent_Quit, 1000);
		if (ret == WAIT_OBJECT_0)
		{
			break;
		}

		Monitoring::GetInstance()->PrintMonitoring();
		Monitoring::GetInstance()->Clear();
	}
}
