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

	_maxPlayerCount = 20000;

	InitializeSRWLock(&tmpPlayerArrLock);
	InitializeSRWLock(&tmpSIDToPlayerLock);
	InitializeSRWLock(&playerArrLock);
	InitializeSRWLock(&SIDToPlayerLock);
	InitializeSRWLock(&accountNoToSIDLock);

	hEvent_Quit = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (hEvent_Quit == 0)
		return 0;

	_hMonitoringThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&MonitorThreadRun, this, NULL, NULL);
	_hThread_Heartbeat = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&HeartbeatThreadRun, this, NULL, NULL);

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

		newPlayer->heartbeat = GetTickCount64();

		// rateLimit 초기화
		newPlayer->rateLimitTick = timeGetTime();
		newPlayer->rateLimitMsgCount = 0;
		newPlayer->rateLimitOutCount = 0;

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
		flag = PacketProc_SectorMove(sessionID, pPacket);
		Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::RecvMessageMoveTPS);
		break;

	case en_PACKET_CS_CHAT_REQ_MESSAGE:
		flag = PacketProc_Message(sessionID, pPacket);
		Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::RecvMessageChatTPS);
		break;

	case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
		flag = PacketProc_Heartbeat(sessionID);
		break;

	default:
		// attack #2 O
		flag = false;
		Disconnect(sessionID);
		_LOG(dfLOG_LEVEL_SYSTEM, L"%ls\n", L"attack #2 Disconnect");
		return;
	}

	if (flag == true)
	{
		// 하트비트 업데이트
		UpdateHeartbeat(sessionID);

		// 클라이언트가 1초에 10개 이상의 메시지를 보냈으면 킥. (단, 2OUT 시 킥)
		bool rateRet = CheckMessageRateLimit(sessionID);
		if (rateRet == false)
		{
			Disconnect(sessionID);
			_LOG(dfLOG_LEVEL_SYSTEM, L"%ls\n", L"attack #12 Disconnect");
		}
	}
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

	
	
	// TODO: 최대 유저에 대한 제한.
	// attack #6 O
	Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::LoginProcessingCount);
	LONG estimated =
		Monitoring::GetInstance()->GetInterlocked(MonitorType::PlayerCount) +
		Monitoring::GetInstance()->GetInterlocked(MonitorType::LoginProcessingCount);

	if (estimated > _maxPlayerCount)
	{
		Monitoring::GetInstance()->DecreaseInterlocked(MonitorType::LoginProcessingCount);
		Disconnect(sessionID);

		_LOG(dfLOG_LEVEL_SYSTEM, L"%ls\n", L"attack #6 Disconnect");

		return false;
	}

	// attack 14
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
			_LOG(dfLOG_LEVEL_SYSTEM, L"%ls\n", L"attack #14");
			Monitoring::GetInstance()->DecreaseInterlocked(MonitorType::LoginProcessingCount);

			return false;
		}
	}


	// 이미 로그인된 세션이 다시 로그인 요청
	// attack #13
	{
		AcquireSRWLockShared(&SIDToPlayerLock);
		auto iter = SIDToPlayer.find(sessionID);
		if (iter != SIDToPlayer.end())
		{
			ReleaseSRWLockShared(&SIDToPlayerLock);

			// attack #13
			Disconnect(sessionID);
			_LOG(dfLOG_LEVEL_SYSTEM, L"%ls\n", L"attack #LOGIN_DUP_SESSION");
			return false;
		}
		ReleaseSRWLockShared(&SIDToPlayerLock);
	}


	// 토큰 체크
	bool isTokenValid = IsTokenValid(accountNo, sessionKey);
	if (isTokenValid == false)
	{
		Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::TokenFailed);
		Monitoring::GetInstance()->DecreaseInterlocked(MonitorType::LoginProcessingCount);
		Disconnect(sessionID);

		return false;
	}

	// 중복로그인 검사 + 등록을 한번에.
	DWORD64 oldSID = 0;

	AcquireSRWLockExclusive(&accountNoToSIDLock);
	{
		auto iter = accountNoToSID.find(accountNo);
		if (iter != accountNoToSID.end())
		{
			oldSID = iter->second;
		}

		accountNoToSID[accountNo] = sessionID;
	}
	ReleaseSRWLockExclusive(&accountNoToSIDLock);


	// oldSID가 있고, 그게 지금 세션이 아리나면 중복로그인 -> old 끊기
	if (oldSID != 0 && oldSID != sessionID)
	{
		Disconnect(oldSID);
	}

	// 정상로그인 O
	{
		SerializePacketPtr newPacket = SerializePacketPtr::MakeSerializePacket();
		newPacket.Clear();

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
			Monitoring::GetInstance()->DecreaseInterlocked(MonitorType::LoginProcessingCount);
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

		SendPacket(sessionID, newPacket);
		Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::PlayerCount);
		Monitoring::GetInstance()->DecreaseInterlocked(MonitorType::LoginProcessingCount);

		return true;
	}
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
	INT64 accountNo = 0;
	WORD newSectorY = 0;
	WORD newSectorX = 0;

	pPacket >> accountNo;
	pPacket >> newSectorY;
	pPacket >> newSectorX;

	
	if (IsMyAccountNo(sessionID, accountNo) == false)
	{
		Disconnect(sessionID);
		_LOG(dfLOG_LEVEL_SYSTEM, L"%ls\n", L"attack #10 Disconnect");
		return false;
	}

	if (newSectorY >= MAX_SECTOR_Y || newSectorX >= MAX_SECTOR_X)
	{
		Disconnect(sessionID);
		_LOG(dfLOG_LEVEL_SYSTEM, L"%ls\n", L"attack #3 Disconnect");
		return false;
	}

	AcquireSRWLockShared(&SIDToPlayerLock);
	auto iter = SIDToPlayer.find(sessionID);
	if (iter == SIDToPlayer.end())
	{
		Disconnect(sessionID);
		_LOG(dfLOG_LEVEL_SYSTEM, L"%ls\n", L"attack #4 Disconnect");
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
	int len = pPacket.GetData((char*)msg, msgLen);

	if (IsMyAccountNo(sessionID, accountNo) == false)
	{
		Disconnect(sessionID);
		_LOG(dfLOG_LEVEL_SYSTEM, L"%ls\n", L"attack #10 Disconnect");
		return false;
	}

	if (len != msgLen)
	{
		Disconnect(sessionID);
		_LOG(dfLOG_LEVEL_SYSTEM, L"%ls\n", L"attack #3 Disconnect");
		return false;
	}

	SerializePacketPtr newPacket = SerializePacketPtr::MakeSerializePacket();
	newPacket.Clear();

	WORD type = en_PACKET_CS_CHAT_RES_MESSAGE;

	newPacket << type;
	newPacket << accountNo;

	AcquireSRWLockShared(&SIDToPlayerLock);
	auto iter = SIDToPlayer.find(sessionID);
	if (iter == SIDToPlayer.end())
	{
		Disconnect(sessionID);
		_LOG(dfLOG_LEVEL_SYSTEM, L"%ls\n", L"attack #4 Disconnect");
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
	ReleaseSRWLockShared(&removed->playerLock);


	AcquireSRWLockExclusive(&accountNoToSIDLock);
	auto iter2 = accountNoToSID.find(accountNo);
	if (iter2 != accountNoToSID.end() && iter2->second == sessionID)
	{
		accountNoToSID.erase(iter2);
	}
	ReleaseSRWLockExclusive(&accountNoToSIDLock);

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
		DWORD ret = WaitForSingleObject(hEvent_Quit, 1000 * 5);
		if (ret == WAIT_OBJECT_0)
			break;

		DWORD64 nowTime = GetTickCount64();

		// 로그인은 15초
		// 플레이는 30초

		vector<DWORD64> unresponsivePlayers;

		AcquireSRWLockShared(&tmpPlayerArrLock);
		for (int i = 0; i < tmpPlayerArr.size(); i++)
		{
			DWORD64 hb;
			DWORD64 sid;

			AcquireSRWLockShared(&tmpPlayerArr[i]->playerLock);
			hb = tmpPlayerArr[i]->heartbeat;
			sid = tmpPlayerArr[i]->sessionID;
			ReleaseSRWLockShared(&tmpPlayerArr[i]->playerLock);

			if (hb > nowTime)
				continue;

			DWORD64 diff = nowTime - hb;
			if (diff > 1000 * 15)
			{
				unresponsivePlayers.push_back(sid);
				_LOG(dfLOG_LEVEL_SYSTEM, L"%ls %llu\n", L"diff: ", diff);
			}
		}
		ReleaseSRWLockShared(&tmpPlayerArrLock);


		AcquireSRWLockShared(&playerArrLock);
		for (int i = 0; i < playerArr.size(); i++)
		{
			DWORD64 hb;
			DWORD64 sid;

			AcquireSRWLockShared(&playerArr[i]->playerLock);
			hb = playerArr[i]->heartbeat;
			sid = playerArr[i]->sessionID;
			ReleaseSRWLockShared(&playerArr[i]->playerLock);

			if (hb > nowTime)
				continue;

			DWORD64 diff = nowTime - hb;
			if (diff > 1000 * 30)
			{
				unresponsivePlayers.push_back(sid);
				_LOG(dfLOG_LEVEL_SYSTEM, L"%ls %llu\n", L"diff: ", diff);
			}

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


bool ChatServer::IsMyAccountNo(DWORD64 sessionID, INT64 accountNo)
{
	Player* pPlayer = NULL;

	AcquireSRWLockExclusive(&SIDToPlayerLock);

	auto iter = SIDToPlayer.find(sessionID);
	if (iter != SIDToPlayer.end())
	{
		pPlayer = iter->second;
	}

	ReleaseSRWLockExclusive(&SIDToPlayerLock);

	if (pPlayer == NULL)
	{
		DebugBreak();
		return false;
	}


	AcquireSRWLockExclusive(&pPlayer->playerLock);
	INT64 playerAccountNo = pPlayer->accountNo;
	ReleaseSRWLockExclusive(&pPlayer->playerLock);

	if (playerAccountNo != accountNo)
		return false;
	else
		return true;
}

bool ChatServer::CheckMessageRateLimit(DWORD64 sessionID)
{
	AcquireSRWLockExclusive(&SIDToPlayerLock);
	auto iter = SIDToPlayer.find(sessionID);
	if (iter == SIDToPlayer.end())
	{
		ReleaseSRWLockExclusive(&SIDToPlayerLock);
		return true;
	}

	Player* pPlayer = iter->second;
	ReleaseSRWLockExclusive(&SIDToPlayerLock);


	DWORD now = timeGetTime();
	bool ret = true;

	AcquireSRWLockExclusive(&pPlayer->playerLock);

	// 1초 갱신
	if (now - pPlayer->rateLimitTick >= 1000)
	{
		pPlayer->rateLimitTick = now;
		pPlayer->rateLimitMsgCount = 0;
	}

	// 메시지 갯수 증가
	pPlayer->rateLimitMsgCount++;
	
	// 아웃카운트 검사
	if (pPlayer->rateLimitMsgCount > 10)
	{
		pPlayer->rateLimitOutCount++;

		if (pPlayer->rateLimitOutCount >= 2)
		{
			ret = false;
		}
		else
		{
			pPlayer->rateLimitTick = now;
			pPlayer->rateLimitMsgCount = 0;
		}
	}

	ReleaseSRWLockExclusive(&pPlayer->playerLock);

	return ret;
}
