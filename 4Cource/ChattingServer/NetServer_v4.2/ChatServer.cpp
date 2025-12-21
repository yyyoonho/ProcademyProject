#include "stdafx.h"

#include <cpp_redis/cpp_redis>
#pragma comment (lib, "cpp_redis.lib")
#pragma comment (lib, "tacopie.lib")

#include "Monitoring.h"
#include "NetServer.h"
#include "PacketProc.h"
#include "CommonProtocol.h"

#include "ChatServer.h"

using namespace std;

thread_local cpp_redis::client* redisClient;

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

	//_contentMSGQueue.Enqueue(acceptMsg);
	RawPtr rawPtr;
	acceptMsg.GetRawPtr(&rawPtr);
	rawPtr.IncreaseRefCount();

	_contentMSGQueue.Enqueue(rawPtr);

	SetEvent(_hEventMsg);
	
	ReleaseSRWLockExclusive(&_contentMSGLock);


	Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::MSGQueueSize);

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

	RawPtr rawPtr;
	releaseMsg.GetRawPtr(&rawPtr);
	rawPtr.IncreaseRefCount();

	_contentMSGQueue.Enqueue(rawPtr);

	SetEvent(_hEventMsg);

	ReleaseSRWLockExclusive(&_contentMSGLock);

	Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::MSGQueueSize);
}

void ChatServer::OnMessage(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	pPacket.PushExtraBuffer((char*)&sessionID, sizeof(DWORD64));

	MSG_CATEGORY msgCategory = MSG_CATEGORY::MESSAGE;
	pPacket.PushExtraBuffer((char*)&msgCategory, sizeof(MSG_CATEGORY));

	AcquireSRWLockExclusive(&_contentMSGLock);
	
	RawPtr rawPtr;
	pPacket.GetRawPtr(&rawPtr);
	rawPtr.IncreaseRefCount();

	_contentMSGQueue.Enqueue(rawPtr);

	SetEvent(_hEventMsg);

	ReleaseSRWLockExclusive(&_contentMSGLock);

	Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::MSGQueueSize);
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
			//DisconnectUnresponsivePlayers();
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

		int size = _contentMSGQueue.Size();
		for (int i = 0; i < size; i++)
		{
			//SerializePacketPtr msg;
			RawPtr rawPtr;
			bool tmpRet = _contentMSGQueue.Dequeue(&rawPtr);
			if (tmpRet == false)
			{
				break;
			}

			SerializePacketPtr msg(rawPtr);
			rawPtr.DecreaseRefCount();

			Monitoring::GetInstance()->DecreaseInterlocked(MonitorType::MSGQueueSize);
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

		if (_contentMSGQueue.Size() > 0)
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
	tmpSIDToIdx.insert({ sessionID, tmpPlayerArr.size() - 1 });
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
	auto iter = tmpSIDToIdx.find(sessionID);
	if (iter == tmpSIDToIdx.end())
	{
		DebugBreak();
	}

	int idx = iter->second;
	Player* loginPlayer = tmpPlayerArr[idx];

	INT64 accountNo;
	WCHAR ID[20];
	WCHAR NicnkName[20];
	char sessionKey[64];

	pPacket >> accountNo;
	pPacket.GetData((char*)&ID, sizeof(WCHAR) * 20);
	pPacket.GetData((char*)&NicnkName, sizeof(WCHAR) * 20);
	pPacket.GetData(sessionKey, sizeof(char) * 64);

	// 토큰 체크
	bool isTokenValid = IsTokenValid(accountNo, sessionKey);
	if (isTokenValid == false)
		return;

	// 중복로그인 체크
	bool flag = CheckDuplicateLogin(accountNo);
	if (flag == false)
	{
		// 로그인 플레이어부터 끊기
		Disconnect(sessionID);
		Monitoring::GetInstance()->Increase(MonitorType::DuplicatedDisconnect_new);

		// 기존 플레이어 끊기
		auto iter2 = accountNoToIdx.find(accountNo);
		if (iter2 == accountNoToIdx.end())
		{
			DebugBreak();
		}

		DWORD64 oldSID = playerArr[iter2->second]->sessionID;
		Disconnect(oldSID);
		Monitoring::GetInstance()->Increase(MonitorType::DuplicatedDisconnect_old);

		return;
	}
	else
	{
		loginPlayer->accountNo = accountNo;
		loginPlayer->sessionID = sessionID;
		wcscpy_s(loginPlayer->ID, ID);
		wcscpy_s(loginPlayer->nickName, NicnkName);
		memcpy(loginPlayer->sessionKey, sessionKey, 64);
		
		loginPlayer->heartbeat = timeGetTime();
		loginPlayer->state = PLAYER_STATE::LOGIN;

		// tmp 자료구조 정리
		int lastIdx = tmpPlayerArr.size() - 1;
		if (idx != lastIdx)
		{
			Player* moved = tmpPlayerArr[lastIdx];
			tmpPlayerArr[idx] = moved;
			tmpSIDToIdx[moved->sessionID] = idx;
		}
		tmpPlayerArr.pop_back();
		tmpSIDToIdx.erase(sessionID);

		// new 자료구조 정리
		playerArr.push_back(loginPlayer);
		int newIdx = playerArr.size() - 1;
		SIDToIdx.insert({ sessionID, newIdx });
		accountNoToIdx.insert({ accountNo, newIdx });

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
	unordered_map<INT64, int>::iterator iter = accountNoToIdx.find(accountNo);
	if (iter == accountNoToIdx.end())
	{
		return true;
	}

	return false;
}

bool ChatServer::IsTokenValid(INT64 accountNo, const char* sessionKey)
{
	// redis에서 해당 accountNo에 대한 sessionKey과 일치하는지 확인.

	if (redisClient == NULL)
	{
		redisClient = new cpp_redis::client;
		redisClient->connect();
	}

	string key = to_string(accountNo);
	string token(sessionKey, 64);

	static const string lua_script = R"(
        local val = redis.call("GET", KEYS[1])
        if not val then
            return 0
        end

        if val ~= ARGV[1] then
            return 0
        end

        redis.call("DEL", KEYS[1])
        return 1
    )";

	auto future = redisClient->eval(lua_script, { key }, { token });
	redisClient->sync_commit();

	cpp_redis::reply reply = future.get();

	if (!reply.is_integer())
		return false;

	if (reply.as_integer() == 1)
	{
		return true;
	}
	else
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

	Player* pPlayer = playerArr[accountNoToIdx.find(accountNo)->second];

	WORD oldSectorY = pPlayer->sectorY;
	WORD oldSectorX = pPlayer->sectorX;

	pPlayer->sectorY = sectorY;
	pPlayer->sectorX = sectorX;

	if (pPlayer->state == PLAYER_STATE::PLAY)
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

	Player* pPlayer = playerArr[accountNoToIdx.find(accountNo)->second];

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
	// release대상이 tmp인 상황. (playerstate == accept)
	auto iter = tmpSIDToIdx.find(sessionID);
	if (iter != tmpSIDToIdx.end())
	{
		int tmpIdx = iter->second;
		Player* removed = tmpPlayerArr[tmpIdx];

		int tmpLastIdx = tmpPlayerArr.size() - 1;

		if (tmpIdx != tmpLastIdx)
		{
			Player* moved = tmpPlayerArr[tmpLastIdx];
			tmpPlayerArr[tmpIdx] = moved;
			
			tmpSIDToIdx[moved->sessionID] = tmpIdx;
		}

		tmpPlayerArr.pop_back();
		tmpSIDToIdx.erase(sessionID);
		playerPool.Free(removed);

		return;
	}

	// release대상이 origin인 상황. (playerstate == login or play)
	{
		auto iter2 = SIDToIdx.find(sessionID);
		if (iter2 == SIDToIdx.end())
		{
			DebugBreak();
		}

		int idx = iter2->second;
		Player* removed = playerArr[idx];
		INT64 accountNo = removed->accountNo;

		int lastIdx = playerArr.size() - 1;
		if (idx != lastIdx)
		{
			Player* moved = playerArr[lastIdx];
			playerArr[idx] = moved;

			SIDToIdx[moved->sessionID] = idx;
			accountNoToIdx[moved->accountNo] = idx;
		}
		playerArr.pop_back();

		if (removed->state == PLAYER_STATE::PLAY)
		{
			WORD sectorY = removed->sectorY;
			WORD sectorX = removed->sectorX;

			for (int i = 0; i < sector[sectorY][sectorX].size(); i++)
			{
				if (sector[sectorY][sectorX][i] != removed->sessionID)
					continue;

				sector[sectorY][sectorX][i] = sector[sectorY][sectorX].back();
				sector[sectorY][sectorX].pop_back();
				break;
			}
		}

		accountNoToIdx.erase(accountNo);
		SIDToIdx.erase(sessionID);
		playerPool.Free(removed);
	}

}

void ChatServer::UpdateHeartbeat(DWORD64 sessionID)
{
	
}

void ChatServer::DisconnectUnresponsivePlayers()
{
	static DWORD oldTime_waitLogin = timeGetTime();
	static DWORD oldTime_waitHB = timeGetTime();

	// TODO: 중복되서 처리되지않게 조심.(근데 어차피 disconnect에서 알아서 걸러주지않나)
}
