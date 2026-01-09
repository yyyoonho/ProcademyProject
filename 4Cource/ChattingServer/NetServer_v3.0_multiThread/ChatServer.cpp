#include "stdafx.h"

#include <cpp_redis/cpp_redis>
#pragma comment (lib, "cpp_redis.lib")
#pragma comment (lib, "tacopie.lib")

#include "LogManager.h"
#include "Monitoring.h"
#include "NetServer.h"
#include "PacketProc.h"
#include "CommonProtocol.h"
#include "SendJob.h"
#include "NetClient.h"
#include "NetClient_Monitoring.h"

#include "ChatServer.h"

using namespace std;

int dy[9] = { -1,-1,-1,0,0,0,1,1,1 };
int dx[9] = { -1,0,1,-1,0,1,-1,0,1 };

HANDLE hHeartbeatThread;
HANDLE hThread_Monitoring;
HANDLE hEvent_Quit;



bool ChatServer::Start(const WCHAR* ipAddress, unsigned short port, unsigned short workerThreadCount, unsigned short coreSkip, bool isNagle, unsigned int maximumSessionCount, bool codecOnOff, BYTE fixedKey, BYTE code)
{
	bool ret = CNetServer::Start(ipAddress, port, workerThreadCount, coreSkip, isNagle, maximumSessionCount, codecOnOff, fixedKey, code);
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

	{
		// TODO:  모니터링 서버와 연결
	}

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
		newPlayer->duplicateKick = false;

		newPlayer->heartbeat = GetTickCount64();
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
	bool ret = ReleaseTmpPlayer(sessionID);

	if(ret ==false)
		ReleaseOriginPlayer(sessionID);

	return;
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
	PRO_BEGIN("Proc");

	WORD msgType;
	pPacket >> msgType;

	bool flag = true;
	switch (msgType)
	{
	case en_PACKET_CS_CHAT_REQ_LOGIN:
		flag = PacketProc_Login(sessionID, pPacket);
		Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::RecvMessageLoginTPS);
		break;

	case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
		PRO_BEGIN("Move");
		flag = PacketProc_SectorMove(sessionID, pPacket);
		PRO_END("Move");
		Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::RecvMessageMoveTPS);
		break;

	case en_PACKET_CS_CHAT_REQ_MESSAGE:
		PRO_BEGIN("Message");
		flag = PacketProc_Message(sessionID, pPacket);
		PRO_END("Message");
		Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::RecvMessageChatTPS);
		break;

	case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
		PRO_BEGIN("HB");
		flag = PacketProc_Heartbeat(sessionID);
		PRO_END("HB");
		break;
	}

	if (flag == true)
	{
		UpdateHeartbeat(sessionID);
	}

	PRO_END("Proc");
}

bool ChatServer::PacketProc_Login(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	INT64 accountNo;
	WCHAR ID[20];
	WCHAR NicnkName[20];
	char sessionKey[64];

	pPacket >> accountNo;
	pPacket.GetData((char*)&ID, sizeof(WCHAR) * 20);
	pPacket.GetData((char*)&NicnkName, sizeof(WCHAR) * 20);
	pPacket.GetData(sessionKey, sizeof(char) * 64);

	BYTE status = 1;

	SerializePacketPtr newPacket = SerializePacketPtr::MakeSerializePacket();
	newPacket.Clear();

	// 토큰 체크
	bool isTokenValid = IsTokenValid(accountNo, sessionKey);
	if (isTokenValid == false)
	{
		Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::TokenFailed);
		Disconnect(sessionID);
		return false;
	}
		

	// 중복로그인 체크
	bool isAlreadyLoggedin = CheckDuplicateLogin(accountNo);

	// 중복로그인 O
	// 테스트
	do
	{
		if (isAlreadyLoggedin == TRUE)
		{
			//_LOG(dfLOG_LEVEL_DEBUG, L"%ls %ld\n", L"중복로그인 accountNo: ", accountNo);

			// old disconnect
			AcquireSRWLockShared(&accountNoToPlayerLock);
			auto iter = accountNoToPlayer.find(accountNo);
			if (iter == accountNoToPlayer.end())
			{
				ReleaseSRWLockShared(&accountNoToPlayerLock);
				break;
			}

			Player* originPlayer = iter->second;
			ReleaseSRWLockShared(&accountNoToPlayerLock);


			AcquireSRWLockExclusive(&originPlayer->playerLock);
			if (originPlayer->accountNo != accountNo)
			{
				ReleaseSRWLockExclusive(&originPlayer->playerLock);
				break;
			}

			DWORD64 originSID = originPlayer->sessionID;
			originPlayer->duplicateKick = true;
			ReleaseSRWLockExclusive(&originPlayer->playerLock);

			Disconnect(originSID);
		}
	} while (0);

	// 정상로그인 O
	//else
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
			return false;
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
		wcscpy_s(loginPlayer->ID, ID);
		wcscpy_s(loginPlayer->nickName, NicnkName);
		memcpy(loginPlayer->sessionKey, sessionKey, 64);

		loginPlayer->heartbeat = GetTickCount64();
		loginPlayer->state = PLAYER_STATE::LOGIN;

		ReleaseSRWLockExclusive(&loginPlayer->playerLock);


		AcquireSRWLockExclusive(&playerArrLock);
		playerArr.push_back(loginPlayer);
		ReleaseSRWLockExclusive(&playerArrLock);

		AcquireSRWLockExclusive(&SIDToPlayerLock);
		SIDToPlayer.insert({ sessionID,loginPlayer });
		ReleaseSRWLockExclusive(&SIDToPlayerLock);

		AcquireSRWLockExclusive(&accountNoToPlayerLock);
		//accountNoToPlayer.insert({ accountNo, loginPlayer });
		accountNoToPlayer[accountNo] = loginPlayer;
		ReleaseSRWLockExclusive(&accountNoToPlayerLock);

		SendPacket(sessionID, newPacket);
		Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::PlayerCount);

		return true;
	}
}

bool ChatServer::CheckDuplicateLogin(INT64 accountNo)
{
	bool ret = false;

	Player* oldPlayer = NULL;

	AcquireSRWLockShared(&accountNoToPlayerLock);
	auto iter = accountNoToPlayer.find(accountNo);
	if (iter == accountNoToPlayer.end())
	{
		ret = false;
	}
	else
	{
		oldPlayer = iter->second;
	}
	ReleaseSRWLockShared(&accountNoToPlayerLock);

	if (oldPlayer == NULL)
	{
		return ret;
	}

	AcquireSRWLockExclusive(&oldPlayer->playerLock);
	if (oldPlayer->duplicateKick == true)
	{
		// 테스트
		//ret = true;
		ret = false;
	}
	else
	{
		// 테스트
		//ret = false;
		ret = true;
	}
	ReleaseSRWLockExclusive(&oldPlayer->playerLock);

	return ret;
}

bool ChatServer::IsTokenValid(INT64 accountNo, const char* sessionKey)
{
	// redis에서 해당 accountNo에 대한 sessionKey과 일치하는지 확인.

	static thread_local cpp_redis::client redisClient;

	if (!redisClient.is_connected())
	{
		redisClient.connect();
		redisClient.sync_commit();
	}
		

	string key = to_string(accountNo);
	string token(sessionKey, 64);

	auto future = redisClient.get(key);
	redisClient.sync_commit();

	auto reply = future.get();
	if (!reply.is_string())
		return false;

	return reply.as_string() == token;
}

bool ChatServer::PacketProc_SectorMove(DWORD64 sessionID, SerializePacketPtr pPacket)
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
		return false;
	}

	Player* movePlayer = iter->second;
	ReleaseSRWLockShared(&SIDToPlayerLock);


	AcquireSRWLockExclusive(&movePlayer->playerLock);
	if (movePlayer->sessionID != sessionID)
	{
		ReleaseSRWLockExclusive(&movePlayer->playerLock);
		return false;
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

	return true;
}

bool ChatServer::PacketProc_Message(DWORD64 sessionID, SerializePacketPtr pPacket)
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
		return false;
	}

	Player* msgPlayer = iter->second;
	ReleaseSRWLockShared(&SIDToPlayerLock);



	AcquireSRWLockShared(&msgPlayer->playerLock);
	if (msgPlayer->sessionID != sessionID)
	{
		ReleaseSRWLockShared(&msgPlayer->playerLock);
		return false;
	}

	WORD sectorY = msgPlayer->sectorY;
	WORD sectorX = msgPlayer->sectorX;

	newPacket.Putdata((char*)msgPlayer->ID, sizeof(WCHAR) * 20);
	newPacket.Putdata((char*)msgPlayer->nickName, sizeof(WCHAR) * 20);

	ReleaseSRWLockShared(&msgPlayer->playerLock);

	newPacket << msgLen;
	newPacket.Putdata((char*)msg, msgLen);

	vector<DWORD64> targets;
	LockAroundSector_Shared(sectorY, sectorX);
	for (int i = 0; i < 9; i++)
	{
		WORD nextY = (int)sectorY + dy[i];
		WORD nextX = (int)sectorX + dx[i];

		if (nextY < 0 || nextY >= MAX_SECTOR_Y || nextX < 0 || nextX >= MAX_SECTOR_X)
			continue;

		for (int j = 0; j < sector[nextY][nextX].size(); j++)
		{
			targets.push_back(sector[nextY][nextX][j]);
			//SendPacket(sector[nextY][nextX][j], newPacket);
		}
	}

	UnLockAroundSector_Shared(sectorY, sectorX);

	for (int i = 0; i < targets.size(); i++)
	{
		SendPacket(targets[i], newPacket);
	}

	return true;
}

bool ChatServer::PacketProc_Heartbeat(DWORD64 sessionID)
{
	UpdateHeartbeat(sessionID);

	return false;
}

void ChatServer::UpdateHeartbeat(DWORD64 sessionID)
{
	AcquireSRWLockShared(&SIDToPlayerLock);
	auto iter = SIDToPlayer.find(sessionID);
	if (iter == SIDToPlayer.end())
	{
		DebugBreak();
	}

	Player* pPlayer = iter->second;
	ReleaseSRWLockShared(&SIDToPlayerLock);

	AcquireSRWLockExclusive(&pPlayer->playerLock);
	pPlayer->heartbeat = GetTickCount64();
	ReleaseSRWLockExclusive(&pPlayer->playerLock);
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

	AcquireSRWLockExclusive(&playerArrLock);
	for (int i = 0; i < playerArr.size(); i++)
	{
		if (playerArr[i] != removed)
			continue;

		playerArr[i] = playerArr.back();
		playerArr.pop_back();
		break;
	}
	ReleaseSRWLockExclusive(&playerArrLock);

	AcquireSRWLockShared(&removed->playerLock);
	INT64 accountNo = removed->accountNo;
	PLAYER_STATE state = removed->state;
	WORD sectorY = removed->sectorY;
	WORD sectorX = removed->sectorX;
	bool dup = removed->duplicateKick;
	if (dup == true)
	{
		Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::DisconnectTotal_alreadyLogin);
	}
	ReleaseSRWLockShared(&removed->playerLock);


	AcquireSRWLockExclusive(&accountNoToPlayerLock);
	auto iter2 = accountNoToPlayer.find(accountNo);
	if (iter2 != accountNoToPlayer.end() && iter2->second == removed)
	{
		accountNoToPlayer.erase(iter2);
	}
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

	Monitoring::GetInstance()->DecreaseInterlocked(MonitorType::PlayerCount);

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
		DWORD ret = WaitForSingleObject(hEvent_Quit, 5000);
		if (ret == WAIT_OBJECT_0)
			break;

		DWORD64 nowTime = GetTickCount64();

		// 로그인은 5초
		// 플레이는 30초

		vector<DWORD64> unresponsivePlayers;

		AcquireSRWLockShared(&tmpPlayerArrLock);
		for (int i = 0; i < tmpPlayerArr.size(); i++)
		{
			AcquireSRWLockShared(&tmpPlayerArr[i]->playerLock);
			if (nowTime - tmpPlayerArr[i]->heartbeat >= 5000)
			{
				//Disconnect(tmpPlayerArr[i]->sessionID);
				unresponsivePlayers.push_back(tmpPlayerArr[i]->sessionID);
			}

			ReleaseSRWLockShared(&tmpPlayerArr[i]->playerLock);
		}
		ReleaseSRWLockShared(&tmpPlayerArrLock);


		AcquireSRWLockShared(&playerArrLock);
		for (int i = 0; i < playerArr.size(); i++)
		{
			AcquireSRWLockShared(&playerArr[i]->playerLock);
			if (nowTime - playerArr[i]->heartbeat >= 30000)
			{
				//Disconnect(playerArr[i]->sessionID);
				unresponsivePlayers.push_back(playerArr[i]->sessionID);
			}

			ReleaseSRWLockShared(&playerArr[i]->playerLock);
		}
		ReleaseSRWLockShared(&playerArrLock);

		for (int i = 0; i < unresponsivePlayers.size(); i++)
		{
			Disconnect(unresponsivePlayers[i]);
		}
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

		Monitoring::GetInstance()->_monitoringArr[(int)MonitorType::tmpPlayerArrSize] = tmpPlayerArr.size();
		Monitoring::GetInstance()->_monitoringArr[(int)MonitorType::PlayerArrSize] = playerArr.size();

		Monitoring::GetInstance()->UpdatePDHnCpuUsage();

		TossMonitoringData();

		Monitoring::GetInstance()->PrintMonitoring();
		Monitoring::GetInstance()->Clear();		
	}
}


void ChatServer::RegisterNetServer(NetClient_Monitoring* pNetClient)
{
	this->pNetClient = pNetClient;
}
