#include "stdafx.h"
#include "Monitoring.h"
#include "NetServer.h"
#include "PacketProc.h"
#include "CommonProtocol.h"

#include "ChatServer.h"

using namespace std;

int dy[9] = { -1,-1,-1,0,0,0,1,1,1 };
int dx[9] = { -1,0,1,-1,0,1,-1,0,1 };

HANDLE hHeartbeatThread;

bool ChatServer::Start(const WCHAR* ipAddress, unsigned short port, unsigned short workerThreadCount, unsigned short coreSkip, bool isNagle, unsigned int maximumSessionCount, bool codecOnOff = true)
{
	bool ret = CNetServer::Start(ipAddress, port, workerThreadCount, coreSkip, isNagle, maximumSessionCount, codecOnOff);
	if (ret == false)
	{
		return false;
	}

	for (int i = 29999; i >= 0; i--)
	{
		tmpIdx_LockFreeStack.Push(i);
		tmpPlayerArr[i].sessionID = 0xffffffffffffffff;
		tmpPlayerArr[i].state = PLAYER_STATE::INVALID;
		//InitializeSRWLock(&tmpPlayerArr[i].playerLock);
	}

	for (int i = 19999; i >= 0; i--)
	{
		Idx_LockFreeStack.Push(i);
		playerArr[i].sessionID = 0xffffffffffffffff;
		playerArr[i].state = PLAYER_STATE::INVALID;
		//InitializeSRWLock(&playerArr[i].playerLock);
	}

	for (int i = 0; i < MAX_SECTOR_Y; i++)
	{
		for (int j = 0; j < MAX_SECTOR_X; j++)
		{
			InitializeSRWLock(&sector_Lock[i][j]);
		}
	}

	InitializeSRWLock(&tmpSessionIdToIndex_Lock);
	InitializeSRWLock(&accountToIndex_Lock);
	InitializeSRWLock(&sessionIdToAccountNo_Lock);
	InitializeSRWLock(&onlineAccounts_Lock);

	hHeartbeatThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&HeartbeatThreadRun, this, NULL, NULL);


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
	int idx;
	tmpIdx_LockFreeStack.Pop(&idx);

	//AcquireSRWLockExclusive(&tmpPlayerArr[idx].playerLock);

	tmpPlayerArr[idx].sessionID = sessionID;
	tmpPlayerArr[idx].waitLoginTime = timeGetTime();
	tmpPlayerArr[idx].state = PLAYER_STATE::ACCEPT;

	//ReleaseSRWLockExclusive(&tmpPlayerArr[idx].playerLock);


	AcquireSRWLockExclusive(&tmpSessionIdToIndex_Lock);

	tmpSessionIdToIndex.insert({ sessionID, idx });

	ReleaseSRWLockExclusive(&tmpSessionIdToIndex_Lock);
}

void ChatServer::OnRelease(DWORD64 sessionID)
{
	// *인덱스 반환전까지는 재사용되지 않는다.
	// 여기까지 왔다는건, 더이상 아무도 onAccept, OnMessage 안에 있지 않다는거다.
	
	// tmp부터 조지자.
	{
		int tmpIdx;

		AcquireSRWLockExclusive(&tmpSessionIdToIndex_Lock);
		unordered_map<DWORD64, int>::iterator iter = tmpSessionIdToIndex.find(sessionID);
		if (iter == tmpSessionIdToIndex.end())
		{
			tmpIdx = -1;
		}
		else
		{
			tmpIdx = iter->second;
			tmpSessionIdToIndex.erase(iter);
		}

		ReleaseSRWLockExclusive(&tmpSessionIdToIndex_Lock);

		if (tmpIdx != -1)
		{
			//AcquireSRWLockExclusive(&tmpPlayerArr[tmpIdx].playerLock);

			tmpPlayerArr[tmpIdx].sessionID = 0xffffffffffffffff;
			tmpPlayerArr[tmpIdx].state = PLAYER_STATE::INVALID;

			//ReleaseSRWLockExclusive(&tmpPlayerArr[tmpIdx].playerLock);

			tmpIdx_LockFreeStack.Push(tmpIdx);
		}
	}

	// real을 조지자.
	{
		INT64 accountNo = -1;
		int idx = -1;
		PLAYER_STATE state = PLAYER_STATE::LOGIN;
		WORD sectorY = -1;
		WORD sectorX = -1;

		AcquireSRWLockExclusive(&sessionIdToAccountNo_Lock);
		unordered_map<DWORD64, INT64>::iterator iter = sessionIdToAccountNo.find(sessionID);
		if (iter == sessionIdToAccountNo.end())
		{
			accountNo = -1;
		}
		else
		{
			accountNo = iter->second;
			sessionIdToAccountNo.erase(iter);
		}

		ReleaseSRWLockExclusive(&sessionIdToAccountNo_Lock);

		if (accountNo != -1)
		{
			AcquireSRWLockExclusive(&accountToIndex_Lock);
			unordered_map<INT64, int>::iterator	iter2 = accountToIndex.find(accountNo);
			if (iter2 == accountToIndex.end())
			{
				idx = -1;
			}
			else
			{
				idx = iter2->second;
				accountToIndex.erase(idx);
			}

			ReleaseSRWLockExclusive(&accountToIndex_Lock);
		}

		if (idx != -1)
		{
			//AcquireSRWLockExclusive(&playerArr[idx].playerLock);

			playerArr[idx].sessionID = 0xffffffffffffffff;
			state = playerArr[idx].state;
			sectorY = playerArr[idx].sectorY;
			sectorX = playerArr[idx].sectorX;

			playerArr[idx].state = PLAYER_STATE::INVALID;

			//ReleaseSRWLockExclusive(&playerArr[idx].playerLock);
		}

		if (accountNo != -1)
		{
			AcquireSRWLockExclusive(&onlineAccounts_Lock);
			onlineAccounts.erase(accountNo);

			ReleaseSRWLockExclusive(&onlineAccounts_Lock);
		}

		if (state == PLAYER_STATE::PLAY)
		{
			AcquireSRWLockExclusive(&sector_Lock[sectorY][sectorX]);
			for (int i = 0; i < sector[sectorY][sectorX].size(); i++)
			{
				if (sector[sectorY][sectorX][i] != sessionID)
					continue;
				
				sector[sectorY][sectorX][i] = sector[sectorY][sectorX].back();
				sector[sectorY][sectorX].pop_back();
			}

			ReleaseSRWLockExclusive(&sector_Lock[sectorY][sectorX]);
		}

		if (idx != -1)
		{
			Idx_LockFreeStack.Push(idx);
		}
	}

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
		Monitoring::GetInstance()->Increase(MonitorType::RecvMessageLoginTPS);
		break;

	case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
		PacketProc_SectorMove(sessionID, pPacket);
		Monitoring::GetInstance()->Increase(MonitorType::RecvMessageMoveTPS);
		break;

	case en_PACKET_CS_CHAT_REQ_MESSAGE:
		PacketProc_Message(sessionID, pPacket);
		Monitoring::GetInstance()->Increase(MonitorType::RecvMessageChatTPS);
		break;

	case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
		PacketProc_Heartbeat(sessionID);
		break;
	}

	UpdateHeartbeat(sessionID);
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
		WORD type = en_PACKET_CS_CHAT_RES_LOGIN;
		newPacket << type;
		status = 0;
		newPacket << status;
		newPacket << accountNo;

		// new disconnect
		AcquireSRWLockExclusive(&tmpSessionIdToIndex_Lock);
		unordered_map<DWORD64, int>::iterator iter = tmpSessionIdToIndex.find(sessionID);
		if (iter == tmpSessionIdToIndex.end())
		{
			ReleaseSRWLockExclusive(&tmpSessionIdToIndex_Lock);
			return;
		}

		int tmpIdx = iter->second;

		ReleaseSRWLockExclusive(&tmpSessionIdToIndex_Lock);


		//AcquireSRWLockExclusive(&tmpPlayerArr[tmpIdx].playerLock);
		if (tmpPlayerArr[tmpIdx].sessionID != sessionID)
		{
			//ReleaseSRWLockExclusive(&tmpPlayerArr[tmpIdx].playerLock);
			return;
		}
		tmpPlayerArr[tmpIdx].state = PLAYER_STATE::DISCONNECTING;

		//ReleaseSRWLockExclusive(&tmpPlayerArr[tmpIdx].playerLock);

		SendPacket(sessionID, pPacket);
		Disconnect(sessionID);


		// old disconnect
		AcquireSRWLockShared(&accountToIndex_Lock);
		unordered_map<INT64, int>::iterator iter2 = accountToIndex.find(accountNo);
		if (iter2 == accountToIndex.end())
		{
			ReleaseSRWLockShared(&accountToIndex_Lock);
			return;
		}

		int oldIdx = iter2->second;
		ReleaseSRWLockShared(&accountToIndex_Lock);

		//AcquireSRWLockShared(&playerArr[oldIdx].playerLock);
		if (playerArr[oldIdx].accountNo != accountNo)
		{
			//ReleaseSRWLockShared(&playerArr[oldIdx].playerLock);
			return;
		}

		DWORD64 oldPlayerSessionID = playerArr[oldIdx].sessionID;
		//ReleaseSRWLockShared(&playerArr[oldIdx].playerLock);

		Disconnect(oldPlayerSessionID);

	}
	// 정상로그인 O
	else
	{
		WORD type = en_PACKET_CS_CHAT_RES_LOGIN;
		newPacket << type;
		newPacket << status;
		newPacket << accountNo;

		AcquireSRWLockExclusive(&tmpSessionIdToIndex_Lock);
		unordered_map<DWORD64, int>::iterator iter = tmpSessionIdToIndex.find(sessionID);
		if (iter == tmpSessionIdToIndex.end())
		{
			ReleaseSRWLockExclusive(&tmpSessionIdToIndex_Lock);
			return;
		}

		int tmpIdx = iter->second;
		tmpSessionIdToIndex.erase(sessionID);

		ReleaseSRWLockExclusive(&tmpSessionIdToIndex_Lock);

		// 여기!

		//AcquireSRWLockExclusive(&tmpPlayerArr[tmpIdx].playerLock);
		if (tmpPlayerArr[tmpIdx].sessionID != sessionID)
		{
			//ReleaseSRWLockExclusive(&tmpPlayerArr[tmpIdx].playerLock);
			return;
		}

		tmpPlayerArr[tmpIdx].sessionID = 0xffffffffffffffff;
		tmpPlayerArr[tmpIdx].state = PLAYER_STATE::INVALID;

		tmpIdx_LockFreeStack.Push(tmpIdx);

		//ReleaseSRWLockExclusive(&tmpPlayerArr[tmpIdx].playerLock);

		int idx;
		Idx_LockFreeStack.Pop(&idx);

		//AcquireSRWLockExclusive(&playerArr[idx].playerLock);

		playerArr[idx].sessionID = sessionID;
		playerArr[idx].accountNo = accountNo;
		pPacket.GetData((char*)playerArr[idx].ID, sizeof(WCHAR) * 20);
		pPacket.GetData((char*)playerArr[idx].nickName, sizeof(WCHAR) * 20);
		pPacket.GetData(playerArr[idx].sessionKey, sizeof(char) * 64);
		playerArr[idx].state = PLAYER_STATE::LOGIN;

		//ReleaseSRWLockExclusive(&playerArr[idx].playerLock);


		AcquireSRWLockExclusive(&accountToIndex_Lock);
		accountToIndex.insert({ accountNo, idx });
		ReleaseSRWLockExclusive(&accountToIndex_Lock);


		AcquireSRWLockExclusive(&sessionIdToAccountNo_Lock);
		sessionIdToAccountNo.insert({ sessionID, accountNo });
		ReleaseSRWLockExclusive(&sessionIdToAccountNo_Lock);


		AcquireSRWLockExclusive(&onlineAccounts_Lock);
		onlineAccounts.insert(accountNo);
		ReleaseSRWLockExclusive(&onlineAccounts_Lock);


		SendPacket(sessionID, newPacket);
	}
}

bool ChatServer::CheckDuplicateLogin(INT64 accountNo)
{
	bool ret = false;

	AcquireSRWLockShared(&onlineAccounts_Lock);

	if (onlineAccounts.find(accountNo) != onlineAccounts.end())
		ret = true;

	ReleaseSRWLockShared(&onlineAccounts_Lock);

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

	AcquireSRWLockShared(&accountToIndex_Lock);
	unordered_map<INT64, int>::iterator iter = accountToIndex.find(accountNo);
	if (iter == accountToIndex.end())
	{
		ReleaseSRWLockShared(&accountToIndex_Lock);
		return;
	}

	int idx = iter->second;
	ReleaseSRWLockShared(&accountToIndex_Lock);


	//AcquireSRWLockExclusive(&playerArr[idx].playerLock);
	if (playerArr[idx].sessionID != sessionID)
	{
		//ReleaseSRWLockExclusive(&playerArr[idx].playerLock);
		return;
	}

	if (playerArr[idx].state == PLAYER_STATE::LOGIN)
	{
		playerArr[idx].sectorY = newSectorY;
		playerArr[idx].sectorX = newSectorX;
		playerArr[idx].state = PLAYER_STATE::PLAY;

		AcquireSRWLockExclusive(&sector_Lock[newSectorY][newSectorX]);
		sector[newSectorY][newSectorX].push_back(sessionID);

		ReleaseSRWLockExclusive(&sector_Lock[newSectorY][newSectorX]);
	}
	else if (playerArr[idx].state == PLAYER_STATE::PLAY)
	{
		WORD oldSectorY = playerArr[idx].sectorY;
		WORD oldSectorX = playerArr[idx].sectorX;

		playerArr[idx].sectorY = newSectorY;
		playerArr[idx].sectorX = newSectorX;

		AcquireSRWLockExclusive(&sector_Lock[oldSectorY][oldSectorX]);
		for (int i = 0; i < sector[oldSectorY][oldSectorX].size(); i++)
		{
			if (sector[oldSectorY][oldSectorX][i] != sessionID)
				continue;

			sector[oldSectorY][oldSectorX][i] = sector[oldSectorY][oldSectorX].back();
			sector[oldSectorY][oldSectorX].pop_back();
		}

		ReleaseSRWLockExclusive(&sector_Lock[oldSectorY][oldSectorX]);


		AcquireSRWLockExclusive(&sector_Lock[newSectorY][newSectorX]);
		sector[newSectorY][newSectorX].push_back(sessionID);

		ReleaseSRWLockExclusive(&sector_Lock[newSectorY][newSectorX]);
	}

	//ReleaseSRWLockExclusive(&playerArr[idx].playerLock);

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

	AcquireSRWLockShared(&accountToIndex_Lock);
	unordered_map<INT64, int>::iterator iter = accountToIndex.find(accountNo);
	if (iter == accountToIndex.end())
	{
		ReleaseSRWLockShared(&accountToIndex_Lock);
		return;
	}

	int idx = iter->second;

	ReleaseSRWLockShared(&accountToIndex_Lock);

	//AcquireSRWLockShared(&playerArr[idx].playerLock);
	if (playerArr[idx].sessionID != sessionID)
	{
		//ReleaseSRWLockShared(&playerArr[idx].playerLock);
		return;
	}

	WORD sectorY = playerArr[idx].sectorY;
	WORD sectorX = playerArr[idx].sectorX;

	newPacket.Putdata((char*)playerArr[idx].ID, sizeof(WCHAR) * 20);
	newPacket.Putdata((char*)playerArr[idx].nickName, sizeof(WCHAR) * 20);

	//ReleaseSRWLockShared(&playerArr[idx].playerLock);

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
	AcquireSRWLockShared(&sessionIdToAccountNo_Lock);
	unordered_map<DWORD64, INT64>::iterator iter = sessionIdToAccountNo.find(sessionID);
	if (iter == sessionIdToAccountNo.end())
	{
		ReleaseSRWLockShared(&sessionIdToAccountNo_Lock);
		return;
	}
	
	INT64 accountNo = iter->second;
	ReleaseSRWLockShared(&sessionIdToAccountNo_Lock);


	AcquireSRWLockShared(&accountToIndex_Lock);
	unordered_map<INT64, int>::iterator iter2 = accountToIndex.find(accountNo);
	if (iter2 == accountToIndex.end())
	{
		ReleaseSRWLockShared(&accountToIndex_Lock);
		return;
	}

	int idx = iter2->second;
	ReleaseSRWLockShared(&accountToIndex_Lock);


	//AcquireSRWLockShared(&playerArr[idx].playerLock);
	playerArr[idx].heartbeat = timeGetTime();

	//ReleaseSRWLockShared(&playerArr[idx].playerLock);
}

void ChatServer::DisconnectUnresponsivePlayers()
{
	

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
	
}
