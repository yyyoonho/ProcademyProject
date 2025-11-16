#include "stdafx.h"
#include "NetServer.h"
#include "PacketProc.h"
#include "PlayerManager.h"
#include "SectorManager.h"
#include "CommonProtocol.h"

#include "ChatServer.h"

using namespace std;

bool ChatServer::Start(const WCHAR* ipAddress, unsigned short port, unsigned short workerThreadCount, unsigned short coreSkip, bool isNagle, unsigned int maximumSessionCount)
{
	bool ret = CNetServer::Start(ipAddress, port, workerThreadCount, coreSkip, isNagle, maximumSessionCount);
	if (ret == false)
	{
		return false;
	}

	// 종료 이벤트 생성
	_hEventQuit = CreateEvent(NULL, TRUE, FALSE, NULL);

	// 종료 이벤트 생성
	_hEventMsg = CreateEvent(NULL, FALSE, FALSE, NULL);

	// 메시지큐 동기화객체 초기화
	InitializeSRWLock(&_contentMSGLock);

	// 컨텐츠용 싱글쓰레드 생성
	_hThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&ChatServer::ContentThreadRun, this, NULL, NULL);


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
	SerializePacketPtr acceptMsg = SerializePacketPtr::MakeSerializePacket();
	acceptMsg.Clear();

	MSG_CATEGORY msgCategory = MSG_CATEGORY::ACCEPT;

	acceptMsg.PushExtraBuffer((char*)&msgCategory, sizeof(MSG_CATEGORY));
	acceptMsg.Putdata((char*)&sessionID, sizeof(DWORD64));

	AcquireSRWLockExclusive(&_contentMSGLock);
	_contentMSGQueue.Enqueue(acceptMsg);
	SetEvent(_hEventMsg);
	ReleaseSRWLockExclusive(&_contentMSGLock);

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
	_contentMSGQueue.Enqueue(releaseMsg);
	SetEvent(_hEventMsg);
	ReleaseSRWLockExclusive(&_contentMSGLock);
}

void ChatServer::OnMessage(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	pPacket.PushExtraBuffer((char*)&sessionID, sizeof(DWORD64));

	MSG_CATEGORY msgCategory = MSG_CATEGORY::MESSAGE;
	pPacket.PushExtraBuffer((char*)&msgCategory, sizeof(MSG_CATEGORY));

	AcquireSRWLockExclusive(&_contentMSGLock);
	_contentMSGQueue.Enqueue(pPacket);
	SetEvent(_hEventMsg);
	ReleaseSRWLockExclusive(&_contentMSGLock);
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

	int acceptOldTime = timeGetTime();
	int heartbeatOldTime = timeGetTime();

	while (1)
	{
		DWORD ret = WaitForMultipleObjects(2, hEventArr, FALSE, 3000);
		if (ret == WAIT_TIMEOUT)
		{
			int nowTime = timeGetTime();

			int acceptDiffTime = nowTime - acceptOldTime;
			int heartbeatDiffTime = nowTime - heartbeatOldTime;

			if (acceptDiffTime >= 3000)
			{
				NoReactAfterAcceptProc();
				acceptOldTime = nowTime;
			}
			if (heartbeatDiffTime >= 10000)
			{
				NoHeartbeatProc();
				heartbeatDiffTime = nowTime;
			}

			continue;
		}

		if (_contentMSGQueue.GetUseSize() > 0)
			SetEvent(_hEventMsg);

		SerializePacketPtr msg;
		_contentMSGQueue.Dequeue(msg);

		MSG_CATEGORY msgCategory;
		msg.GetData((char*)&msgCategory, sizeof(MSG_CATEGORY));

		switch (msgCategory)
		{
		case MSG_CATEGORY::ACCEPT:
			AcceptProc(msg);
			break;
		case MSG_CATEGORY::MESSAGE:
			PacketProc(msg);
			break;
		case MSG_CATEGORY::DISCONNECT:
			ReleaseProc(msg);
			break;
		}
	
		// 하트비트 처리 & 
		// Accept후 반응없는 애들 처리
		{
			int nowTime = timeGetTime();

			int acceptDiffTime = nowTime - acceptOldTime;
			int heartbeatDiffTime = nowTime - heartbeatOldTime;

			if (acceptDiffTime >= 3000)
			{
				NoReactAfterAcceptProc();
				acceptOldTime = nowTime;
			}
			if (heartbeatDiffTime >= 10000)
			{
				NoHeartbeatProc();
				heartbeatDiffTime = nowTime;
			}
		}
	}

	return;
}

void ChatServer::UnicastByAccountNo(INT64 accountNo, SerializePacketPtr sPacket)
{
	DWORD sessionID;
	bool ret = GetSessionID(accountNo, &sessionID);
	if (ret == false)
	{
		return;
	}

	SendPacket(sessionID, sPacket);
}

void ChatServer::UnicastBySessionID(DWORD64 sessionID, SerializePacketPtr sPacket)
{
	SendPacket(sessionID, sPacket);
}

void ChatServer::Multicast(std::vector<INT64>& accountNoArr, SerializePacketPtr sPacket)
{
	for (int i = 0; i < accountNoArr.size(); i++)
	{
		UnicastByAccountNo(accountNoArr[i], sPacket);
	}
}

void ChatServer::AcceptProc(SerializePacketPtr pPacket)
{
	DWORD64 sessionID;
	pPacket.GetData((char*)&sessionID, sizeof(DWORD64));

	CreatePlayer(sessionID);
}

void ChatServer::PacketProc(SerializePacketPtr pPacket)
{
	DWORD64 sessionID;
	pPacket.GetData((char*)&sessionID, sizeof(DWORD64));

	WORD msgType;
	pPacket >> msgType;

	// switch - case로 메시지별 처리 함수 찾아가기.
	switch (msgType)
	{
	case en_PACKET_CS_CHAT_REQ_LOGIN:
		PacketProc_Login(sessionID, pPacket);
		break;

	case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
		PacketProc_SectorMove(sessionID, pPacket);
		break;

	case en_PACKET_CS_CHAT_REQ_MESSAGE:
		PacketProc_Message(sessionID, pPacket);
		break;

	case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
		PacketProc_Heartbeat(sessionID);
		break;
	}
}

void ChatServer::PacketProc_Login(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	INT64 accountNo;
	WCHAR ID[20];
	WCHAR nickName[20];
	char sessionKey[64];

	pPacket >> accountNo;
	pPacket.GetData((char*)&ID, sizeof(WCHAR) * 20);
	pPacket.GetData((char*)&nickName, sizeof(WCHAR) * 20);
	pPacket.GetData(sessionKey, 64);

	// 중복로그인 체크
	bool isLoggedIn = IsLoggedIn(sessionID);
	if (isLoggedIn == TRUE)
	{
		// 1. (기존) 세션 먼저 disconnect 한 다음, Player지우기.
		DWORD originSessionID;
		GetSessionID(accountNo, &originSessionID);

		Disconnect(originSessionID);

		// 2. (New) 세션 먼저 disconnect 한 다음, Player 지우기.
		Disconnect(sessionID);
	}
	else
	{
		// 정상 로그인절차 진행
		LogInPlayer(sessionID, accountNo, ID, 20, nickName, 20, sessionKey, 64);
	}

	// Res 메시지 생성
	SerializePacketPtr newMsgPacket = SerializePacketPtr::MakeSerializePacket();
	newMsgPacket.Clear();

	WORD msgType = en_PACKET_CS_CHAT_RES_LOGIN;
	newMsgPacket << msgType;

	BYTE logInStatus;
	if (isLoggedIn == TRUE)
	{
		logInStatus = FALSE;
	}
	else
	{
		logInStatus = TRUE;
	}
	newMsgPacket << logInStatus;

	newMsgPacket << accountNo;


	// Res unicast()
	UnicastBySessionID(sessionID, newMsgPacket);
}

void ChatServer::PacketProc_SectorMove(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	INT64 accountNo;
	WORD newSectorX;
	WORD newSectorY;   

	pPacket >> accountNo;
	pPacket >> newSectorX;
	pPacket >> newSectorY;

	WORD oldSectorX;
	WORD oldSectorY;
	SetSector(accountNo, newSectorY, newSectorX, &oldSectorX, &oldSectorY);		// playerManager

	// 여기서 create인지 move 인지 결정
	PLAYER_STATE state;
	GetPlayerState(accountNo, &state);
	if (state == PLAYER_STATE::LOGGED_IN)
	{
		CreatePlayerToSector(accountNo, newSectorY, newSectorX);				// sectorManager
		SetPlayerState(accountNo, PLAYER_STATE::PLAY);
	}
	else
		MoveSector(accountNo, newSectorY, newSectorX, oldSectorX, oldSectorY);	// sectorManager
	
	// Res 메시지 생성
	SerializePacketPtr newMsgPacket = SerializePacketPtr::MakeSerializePacket();
	newMsgPacket.Clear();

	WORD msgType = en_PACKET_CS_CHAT_RES_SECTOR_MOVE;
	newMsgPacket << msgType;

	newMsgPacket << accountNo;

	newMsgPacket << newSectorX;
	newMsgPacket << newSectorY;

	// Res unicast( ) -> 라이브러리 지원
	UnicastBySessionID(sessionID, newMsgPacket);
}

void ChatServer::PacketProc_Message(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	INT64 accountNo;
	WORD messageLen;
	WCHAR message[100];

	pPacket >> accountNo;
	pPacket >> messageLen;
	pPacket.GetData((char*)&message, messageLen);

	// 메시지 생성
	stPlayer* pPlayer;
	bool ret = GetPlayer(accountNo, &pPlayer);
	if (ret == false)
		return;

	SerializePacketPtr newMsgPacket = SerializePacketPtr::MakeSerializePacket();
	newMsgPacket.Clear();

	WORD msgType = en_PACKET_CS_CHAT_RES_MESSAGE;
	newMsgPacket << msgType;

	newMsgPacket << accountNo;

	WCHAR* id = pPlayer->ID;
	WCHAR* nickName = pPlayer->nickname;

	newMsgPacket.Putdata((char*)id, 20);
	newMsgPacket.Putdata((char*)nickName, 20);

	newMsgPacket << messageLen;
	newMsgPacket.Putdata((char*)message, messageLen);
	 
	// Res multicast( ) -> 라이브러리 지원
	WORD sectorY;
	WORD sectorX;

	ret = GetSector(accountNo, &sectorY, &sectorX);
	if (ret == false)
		return;

	vector<INT64> adjacentPlayers;
	GetAdjacentSectorPlayers(sectorY, sectorX, adjacentPlayers);

	// multicast();
	Multicast(adjacentPlayers, newMsgPacket);
}

void ChatServer::PacketProc_Heartbeat(DWORD64 sessionID)
{
	UpdateHeartbeat(sessionID);
}

void ChatServer::ReleaseProc(SerializePacketPtr pPacket)
{
	// Disconnect 후 Release작업진행
	DWORD64 sessionID;
	pPacket.GetData((char*)&sessionID, sizeof(DWORD64));

	RemovePlayerFromWaitMap(sessionID);
	RemovePlayerFromPlayerMap(sessionID);


}

void ChatServer::NoHeartbeatProc()
{
	vector<DWORD64> NoHeartbeatSession;
	DisconnectNoLoginPlayer(NoHeartbeatSession);

	for (int i = 0; i < NoHeartbeatSession.size(); i++)
	{
		Disconnect(NoHeartbeatSession[i]);
	}
}

void ChatServer::NoReactAfterAcceptProc()
{
	vector<DWORD64> NoReactSession;
	DisconnectNoLoginPlayer(NoReactSession);

	for (int i = 0; i < NoReactSession.size(); i++)
	{
		Disconnect(NoReactSession[i]);
	}
}
