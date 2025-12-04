#include "stdafx.h"
#include "Monitoring.h"
#include "NetServer.h"
#include "PacketProc.h"
#include "CommonProtocol.h"

#include "ChatServer.h"

using namespace std;

bool ChatServer::Start(const WCHAR* ipAddress, unsigned short port, unsigned short workerThreadCount, unsigned short coreSkip, bool isNagle, unsigned int maximumSessionCount, bool codecOnOff = true)
{
	bool ret = CNetServer::Start(ipAddress, port, workerThreadCount, coreSkip, isNagle, maximumSessionCount, codecOnOff);
	if (ret == false)
	{
		return false;
	}

	// 종료 이벤트 생성
	_hEventQuit = CreateEvent(NULL, TRUE, FALSE, NULL);

	// 메시지 이벤트 생성
	_hEventMsg = CreateEvent(NULL, FALSE, FALSE, NULL);

	// 메시지큐 동기화객체 초기화
	InitializeSRWLock(&_contentMSGLock);

	// 컨텐츠용 싱글쓰레드 생성
	_hThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&ChatServer::ContentThreadRun, this, NULL, NULL);

	return true;
}

void ChatServer::Stop()
{
	SetEvent(_hEventQuit);
}

bool ChatServer::OnConnectionRequest(SOCKADDR_IN clientAddr)
{
	return false;
}

void ChatServer::OnAccept(DWORD64 sessionID)
{
	SerializePacketPtr acceptMsg = SerializePacketPtr::MakeSerializePacket();
	acceptMsg.Clear();

	MSG_CATEGORY msgCategory = MSG_CATEGORY::ACCEPT;

	acceptMsg.PushExtraBuffer((char*)&msgCategory, sizeof(MSG_CATEGORY));
	acceptMsg.Putdata((char*)&sessionID, sizeof(DWORD64));

	AcquireSRWLockExclusive(&_contentMSGLock);

	int size = _contentMSGQueue.GetUseSize();
	_contentMSGQueue.Enqueue(acceptMsg);

	SetEvent(_hEventMsg);
	
	ReleaseSRWLockExclusive(&_contentMSGLock);


	Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::UpdateMessageQueue);

	return;
}

void ChatServer::OnRelease(DWORD64 sessionID)
{
	SerializePacketPtr releaseMsg = SerializePacketPtr::MakeSerializePacket();
	releaseMsg.Clear();

	MSG_CATEGORY msgCategory = MSG_CATEGORY::DISCONNECT;

	releaseMsg.PushExtraBuffer((char*)&msgCategory, sizeof(MSG_CATEGORY));
	releaseMsg.Putdata((char*)&sessionID, sizeof(DWORD64));

	AcquireSRWLockExclusive(&_contentMSGLock);

	int size = _contentMSGQueue.GetUseSize();
	_contentMSGQueue.Enqueue(releaseMsg);

	SetEvent(_hEventMsg);

	ReleaseSRWLockExclusive(&_contentMSGLock);

	Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::UpdateMessageQueue);
}

void ChatServer::OnMessage(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	pPacket.PushExtraBuffer((char*)&sessionID, sizeof(DWORD64));

	MSG_CATEGORY msgCategory = MSG_CATEGORY::MESSAGE;
	pPacket.PushExtraBuffer((char*)&msgCategory, sizeof(MSG_CATEGORY));

	AcquireSRWLockExclusive(&_contentMSGLock);
	
	int size = _contentMSGQueue.GetUseSize();
	_contentMSGQueue.Enqueue(pPacket);

	SetEvent(_hEventMsg);

	ReleaseSRWLockExclusive(&_contentMSGLock);

	Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::UpdateMessageQueue);
}

void ChatServer::OnError(int errorCode, WCHAR* errorComment)
{
}

void ChatServer::ContentThreadRun(LPVOID* lParam)
{
	ChatServer* self = (ChatServer*)lParam;
	self->ContentThread();
}

void ChatServer::ContentThread()
{
	HANDLE hEventArr[2] = { _hEventMsg, _hEventQuit };

	while (1)
	{
		// ------------------------
		// Wait 시작 시간 기록
		// ------------------------
		DWORD waitStart = timeGetTime();

		DWORD ret = WaitForMultipleObjects(2, hEventArr, FALSE, 1000);

		// ------------------------
		// Wait 종료 → 블록된 시간 계산
		// ------------------------
		DWORD waitEnd = timeGetTime();
		DWORD waitDuration = waitEnd - waitStart;

		Monitoring::GetInstance()->AddWaitTime(waitDuration);

		if (ret == WAIT_TIMEOUT)
		{
			DisconnectUnresponsivePlayers();
			continue;
		}
		if (ret == WAIT_OBJECT_0 + 1)
		{
			break;
		}

		// ------------------------
		// 메시지 처리 시작 시간
		// ------------------------
		DWORD workStart = timeGetTime();

		int size = _contentMSGQueue.GetUseSize();
		for (int i = 0; i < size; i++)
		{
			SerializePacketPtr msg;
			int tmpRet = _contentMSGQueue.Dequeue(msg);
			if (tmpRet == 0)
			{
				break;
			}

			Monitoring::GetInstance()->DecreaseInterlocked(MonitorType::UpdateMessageQueue);
			Monitoring::GetInstance()->Increase(MonitorType::UpdateTPS);

			MSG_CATEGORY msgCategory;
			msg.GetData((char*)&msgCategory, sizeof(MSG_CATEGORY));

			DWORD64 sessionID;
			msg.GetData((char*)&sessionID, sizeof(DWORD64));

			switch (msgCategory)
			{
			case MSG_CATEGORY::ACCEPT:
				AcceptProc(sessionID, msg);
				break;
			case MSG_CATEGORY::MESSAGE:
				PacketProc(sessionID, msg);
				break;
			case MSG_CATEGORY::DISCONNECT:
				ReleaseProc(sessionID, msg);
				break;
			}
		}

		//DisconnectUnresponsivePlayers();

		if (_contentMSGQueue.GetUseSize() > 0)
		{
			SetEvent(_hEventMsg);
		}

		// ------------------------
		// 메시지 처리 끝 → 실제 작업 시간
		// ------------------------
		DWORD workEnd = timeGetTime();
		DWORD workDuration = workEnd - workStart;

		Monitoring::GetInstance()->AddWorkTime(workDuration);
	}

	return;
}

void ChatServer::AcceptProc(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	Player* newPlayer = playerPool.Alloc();

	newPlayer->sessionID = sessionID;
	newPlayer->heartbeat = timeGetTime();
	newPlayer->state = PLAYER_STATE::ACCEPT;

	tmpPlayerArr.push_back(newPlayer);
}

void ChatServer::PacketProc(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	WORD msgType;
	pPacket >> msgType;

	// switch - case로 메시지별 처리 함수 찾아가기.
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

	//UpdateHeartbeat(sessionID);
}

void ChatServer::PacketProc_Login(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	Player* loginPlayer = NULL;
	for (int i = 0; i < tmpPlayerArr.size(); i++)
	{
		if (tmpPlayerArr[i]->sessionID != sessionID)
			continue;

		loginPlayer = tmpPlayerArr[i];
		tmpPlayerArr[i] = tmpPlayerArr.back();
		tmpPlayerArr.pop_back();
	}

	if (loginPlayer == NULL)
		return;

	INT64 accountNo;
	pPacket >> accountNo;

	// 중복로그인 체크
	if (CheckDuplicateLogin(accountNo) == false)
	{
		// 로그인 플레이어부터 끊기
		Disconnect(loginPlayer->sessionID);

		// 기존 플레이어 끊기
		unordered_map<INT64, int>::iterator iter = accountToIndex.find(accountNo);
		if (iter == accountToIndex.end())
			return;

		int idx = iter->second;
		Disconnect(playerArr[idx]->sessionID);

		return;
	}
	else
	{
		pPacket.GetData((char*)loginPlayer->ID, sizeof(WCHAR) * 20);
		pPacket.GetData((char*)loginPlayer->nickName, sizeof(WCHAR) * 20);
		pPacket.GetData(loginPlayer->sessionKey, 64);

		loginPlayer->accountNo = accountNo;
		loginPlayer->sectorY = 12345;
		loginPlayer->sectorX = 12345;
		loginPlayer->state = PLAYER_STATE::LOGIN;

		playerArr.push_back(loginPlayer);
		sessionIDAccountNoMap.insert({ sessionID, accountNo });
		accountToIndex.insert({ accountNo, playerArr.size() - 1 });
		onlineAccounts.insert(accountNo);

		SerializePacketPtr newPacket = SerializePacketPtr::MakeSerializePacket();
		newPacket.Clear();

		WORD type = en_PACKET_CS_CHAT_RES_LOGIN;
		BYTE status = 1;

		newPacket << type;
		newPacket << status;
		newPacket << accountNo;

		SendPacket(sessionID, newPacket);

		Monitoring::GetInstance()->Increase(MonitorType::PlayerCount);
	}

}

bool ChatServer::CheckDuplicateLogin(INT64 accountNo)
{
	unordered_set<INT64>::iterator iter = onlineAccounts.find(accountNo);
	if (iter == onlineAccounts.end())
	{
		return true;
	}

	return false;
}

void ChatServer::PacketProc_SectorMove(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	INT64 accountNo;
	WORD sectorX;
	WORD sectorY;

	pPacket >> accountNo;
	pPacket >> sectorX;
	pPacket >> sectorY;

	Player* pPlayer = playerArr[accountToIndex.find(accountNo)->second];

	WORD oldSectorY = pPlayer->sectorY;
	WORD oldSectorX = pPlayer->sectorX;

	pPlayer->sectorY = sectorY;
	pPlayer->sectorX = sectorX;

	if (oldSectorY != 12345)
	{
		for (int i = 0; i < sector[oldSectorY][oldSectorX].size(); i++)
		{
			if (sector[oldSectorY][oldSectorX][i] != sessionID)
				continue;

			sector[oldSectorY][oldSectorX][i] = sector[oldSectorY][oldSectorX].back();
			sector[oldSectorY][oldSectorX].pop_back();
			break;
		}
	}
	else
	{
		pPlayer->state = PLAYER_STATE::PLAY;
	}

	sector[sectorY][sectorX].push_back(sessionID);



	SerializePacketPtr newPacket = SerializePacketPtr::MakeSerializePacket();
	newPacket.Clear();

	WORD type = en_PACKET_CS_CHAT_RES_SECTOR_MOVE;

	newPacket << type;
	newPacket << accountNo;
	newPacket << sectorX;
	newPacket << sectorY;

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

	Player* pPlayer = playerArr[accountToIndex.find(accountNo)->second];

	SerializePacketPtr newPacket = SerializePacketPtr::MakeSerializePacket();
	newPacket.Clear();

	WORD type = en_PACKET_CS_CHAT_RES_MESSAGE;

	newPacket << type;
	newPacket << accountNo;

	WCHAR* id = pPlayer->ID;
	WCHAR* nickname = pPlayer->nickName;

	newPacket.Putdata((char*)id, sizeof(WCHAR) * 20);
	newPacket.Putdata((char*)nickname, sizeof(WCHAR) * 20);
	newPacket << msgLen;
	newPacket.Putdata((char*)msg, msgLen);

	WORD nowY = pPlayer->sectorY;
	WORD nowX = pPlayer->sectorX;
	int dy[9] = { -1,-1,-1,0,0,0,1,1,1 };
	int dx[9] = { -1,0,1,-1,0,1,-1,0,1 };

	for (int i = 0; i < 9; i++)
	{
		WORD nextY = nowY + dy[i];
		WORD nextX = nowX + dx[i];

		if (nextY < 0 || nextY >= MAX_SECTOR_Y || nextX < 0 || nextX >= MAX_SECTOR_X)
			continue;

		for (int j = 0; j < sector[nextY][nextX].size(); j++)
		{
			SendPacket(sector[nextY][nextX][j], newPacket);
		}
	}
}

void ChatServer::PacketProc_Heartbeat(DWORD64 sessionID)
{
	UpdateHeartbeat(sessionID);
}

void ChatServer::ReleaseProc(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	unordered_map<DWORD64, INT64>::iterator iter = sessionIDAccountNoMap.find(sessionID);
	if (iter != sessionIDAccountNoMap.end())
	{
		INT64 accountNo = iter->second;
		sessionIDAccountNoMap.erase(iter);

		unordered_map<INT64, int>::iterator iter2 = accountToIndex.find(accountNo);
		if (iter2 != accountToIndex.end())
		{
			int idx = iter2->second;
			accountToIndex.erase(iter2);

			WORD sectorY = playerArr[idx]->sectorY;
			WORD sectorX = playerArr[idx]->sectorX;

			if (sectorY != 12345)
			{
				for (int i = 0; i < sector[sectorY][sectorX].size(); i++)
				{
					if (sector[sectorY][sectorX][i] != sessionID)
						continue;

					sector[sectorY][sectorX][i] = sector[sectorY][sectorX].back();
					sector[sectorY][sectorX].pop_back();
					break;
				}
			}

			Player* pPlayer = playerArr[idx];
			playerArr[idx] = playerArr.back();

			unordered_map<INT64, int>::iterator iter3 = accountToIndex.find(playerArr[idx]->accountNo);
			if (iter3 != accountToIndex.end())
			{
				accountToIndex[playerArr[idx]->accountNo] = idx;
			}

			playerArr.pop_back();
			onlineAccounts.erase(accountNo);
			playerPool.Free(pPlayer);

			Monitoring::GetInstance()->Decrease(MonitorType::PlayerCount);
		}

	}
	else
	{
		for (int i = 0; i < tmpPlayerArr.size(); i++)
		{
			if (tmpPlayerArr[i]->sessionID != sessionID)
				continue;

			Player* pPlayer = tmpPlayerArr[i];
			tmpPlayerArr[i] = tmpPlayerArr.back();
			tmpPlayerArr.pop_back();

			playerPool.Free(pPlayer);
		}
	}

}

void ChatServer::UpdateHeartbeat(DWORD64 sessionID)
{
	unordered_map<DWORD64, INT64>::iterator iter = sessionIDAccountNoMap.find(sessionID);
	if (iter == sessionIDAccountNoMap.end())
		return;

	INT64 accountNo = iter->second;

	unordered_map<INT64, int>::iterator iter2 = accountToIndex.find(accountNo);
	if (iter2 == accountToIndex.end())
		return;

	int idx = iter2->second;

	playerArr[idx]->heartbeat = timeGetTime();
}

void ChatServer::DisconnectUnresponsivePlayers()
{
	static DWORD oldTime_waitLogin = timeGetTime();
	static DWORD oldTime_waitHB = timeGetTime();

	DWORD nowTime = timeGetTime();
	if (nowTime - oldTime_waitLogin >= 2000)
	{
		for (int i = 0; i < tmpPlayerArr.size(); i++)
		{
			if (nowTime - tmpPlayerArr[i]->heartbeat >= 4000 && tmpPlayerArr[i]->state != PLAYER_STATE::DISCONNECTING)
			{
				DWORD64 sessionID = tmpPlayerArr[i]->sessionID;
				tmpPlayerArr[i]->state = PLAYER_STATE::DISCONNECTING;

				Disconnect(sessionID);

				cout << "로그인시간 초과" << endl;
			}
		}

		oldTime_waitLogin = nowTime;
	}


	if (nowTime - oldTime_waitHB >= 20000)
	{
		for (int i = 0; i < playerArr.size(); i++)
		{
			if (nowTime - playerArr[i]->heartbeat >= 40000 && playerArr[i]->state != PLAYER_STATE::DISCONNECTING)
			{
				DWORD64 sessionID = playerArr[i]->sessionID;
				playerArr[i]->state = PLAYER_STATE::DISCONNECTING;

				Disconnect(sessionID);

				cout << "하트비트시간 초과" << endl;
			}
		}

		oldTime_waitHB = nowTime;
	}

}
