#include "stdafx.h"
#include "NetServer.h"
#include "PacketProc.h"
#include "PlayerManager.h"
#include "SectorManager.h"
#include "CommonProtocol.h"

#include "ChatServer.h"

using namespace std;

bool FrameControl();

bool ChatServer::Start(const WCHAR* ipAddress, unsigned short port, unsigned short workerThreadCount, unsigned short coreSkip, bool isNagle, unsigned int maximumSessionCount)
{
	bool ret = CNetServer::Start(ipAddress, port, workerThreadCount, coreSkip, isNagle, maximumSessionCount);
	if (ret == false)
	{
		return false;
	}

	// 종료 이벤트 생성
	_hEventQuit = CreateEvent(NULL, TRUE, FALSE, NULL);


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
	ReleaseSRWLockExclusive(&_contentMSGLock);

	return;
}

void ChatServer::OnRelease(DWORD64 sessionID)
{
}

void ChatServer::OnMessage(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	pPacket.PushExtraBuffer((char*)&sessionID, sizeof(DWORD64));

	MSG_CATEGORY msgCategory = MSG_CATEGORY::MESSAGE;
	pPacket.PushExtraBuffer((char*)&msgCategory, sizeof(MSG_CATEGORY));

	AcquireSRWLockExclusive(&_contentMSGLock);
	_contentMSGQueue.Enqueue(pPacket);
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
	while (!_bShutdown)
	{
		if(!FrameControl())
			continue;

		int msgQSize = _contentMSGQueue.GetUseSize();
		for (int i = 0; i < msgQSize; i++)
		{
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
				break;
			}

		}

		// TODO: 하트비트

		// TODO: accept한것들중 30초이상 안되는것들 처리
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
	pPacket.GetData((char*)&ID, 20);
	pPacket.GetData((char*)&nickName, 20);
	pPacket.GetData(sessionKey, 64);

	// 중복로그인 체크
	bool isLoggedIn = IsLoggedIn(sessionID);
	if (isLoggedIn == TRUE)
	{
		// TODO: 기존에 들어와있던 녀석도 팅기게.
		// TODO: 새로운놈도 팅기게. (wait pool에 만들어놓은것도 찾아서 free)

		// 1. remove 존에 옮기고, (기존 + 새로)세션 먼저 disconnect 한다음, player지우기.
	}
	else
	{
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
	SetSector(accountNo, newSectorY, newSectorX, &oldSectorX, &oldSectorY); // playerManager
	MoveSector(accountNo, newSectorY, newSectorX, oldSectorX, oldSectorY);
	
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
	stPlayer player;
	bool ret = GetPlayer(accountNo, &player);
	if (ret == false)
		return;

	SerializePacketPtr newMsgPacket = SerializePacketPtr::MakeSerializePacket();
	newMsgPacket.Clear();

	WORD msgType = en_PACKET_CS_CHAT_RES_MESSAGE;
	newMsgPacket << msgType;

	newMsgPacket << accountNo;

	WCHAR* id = player.ID;
	WCHAR* nickName = player.nickname;

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

bool FrameControl()
{
	static int oldTime = timeGetTime();

	int diffTime = timeGetTime() - oldTime;

	if (diffTime < 40) // 1000/40 = 25fps
	{
		return false;
	}
	else
	{
		oldTime += 40;
		return true;
	}
}
