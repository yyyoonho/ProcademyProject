#include "stdafx.h"
#include "NetServer.h"
#include "PacketProc.h"
#include "PlayerManager.h"
#include "SectorManager.h"
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

	// PlayerManeger 생성
	_playerManager = new PlayerManager(this);

	// PlayerManeger 생성
	_sectorManager = new SectorManager(this);

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

	while (1)
	{
		DWORD ret = WaitForMultipleObjects(2, hEventArr, FALSE, 1000);
		if (ret == WAIT_TIMEOUT)
		{
			DisconnectUnresponsivePlayers();
			continue;
		}
		if (ret == WAIT_OBJECT_0 + 1)
		{
			break;
		}

		SerializePacketPtr msg;
		int ret1 = _contentMSGQueue.Dequeue(msg);

		if (_contentMSGQueue.GetUseSize() > 0)
			SetEvent(_hEventMsg);

		MSG_CATEGORY msgCategory = MSG_CATEGORY::NONE;
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
		case MSG_CATEGORY::NONE:
			break;
		}

		DisconnectUnresponsivePlayers();
	}

	return;
}

void ChatServer::AcceptProc(SerializePacketPtr pPacket)
{
	DWORD64 sessionID;
	pPacket.GetData((char*)&sessionID, sizeof(DWORD64));

	_playerManager->CreatePlayerInfo(sessionID);
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
	WCHAR id[20];
	WCHAR nickName[20];
	char sessionKey[64];

	pPacket >> accountNo;
	pPacket.GetData((char*)id, sizeof(WCHAR) * 20);
	pPacket.GetData((char*)nickName, sizeof(WCHAR) * 20);
	pPacket.GetData(sessionKey, 64);

	// LOG
	cout << "#Recv -> en_PACKET_CS_CHAT_REQ_LOGIN - accountNo:" << accountNo << endl;

	SerializePacketPtr resPacket = SerializePacketPtr::MakeSerializePacket();
	resPacket.Clear();

	WORD msgType = en_PACKET_CS_CHAT_RES_LOGIN;
	BYTE status;

	bool ret = _playerManager->LoginPlayer(sessionID, accountNo, id, nickName, sessionKey);
	if (ret)
		status = 1; // 성공
	else
		status = 0; // 실패

	resPacket << msgType;
	resPacket << status;
	resPacket << accountNo;
	
	// LOG
	cout << "#Send <- en_PACKET_CS_CHAT_RES_LOGIN - accountNo:" << accountNo << endl;
	SendPacket(sessionID, resPacket);
}

void ChatServer::PacketProc_SectorMove(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	INT64 accountNo;
	WORD sectorX;
	WORD sectorY;

	pPacket >> accountNo;
	pPacket >> sectorX;
	pPacket >> sectorY;

	// LOG
	cout << "#Recv -> en_PACKET_CS_CHAT_REQ_SECTOR_MOVE - accountNo:" << accountNo << endl;

	bool ret = _playerManager->IsLoggedIn(accountNo);
	if (ret == false)
		return;

	_sectorManager->MovePlayer(accountNo, sectorY, sectorX);

	SerializePacketPtr resPacket = SerializePacketPtr::MakeSerializePacket();
	resPacket.Clear();

	WORD msgType = en_PACKET_CS_CHAT_RES_SECTOR_MOVE;

	resPacket << msgType;
	resPacket << accountNo;
	resPacket << sectorX;
	resPacket << sectorY;

	// LOG
	cout << "#Send <- en_PACKET_CS_CHAT_RES_SECTOR_MOVE - accountNo:" << accountNo << endl;

	SendPacket(sessionID, resPacket);
}

void ChatServer::PacketProc_Message(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	INT64 accountNo;
	WORD messageLen;
	WCHAR message[MAX_MSGLEN];

	pPacket >> accountNo;
	pPacket >> messageLen;
	pPacket.GetData((char*)message, messageLen);

	// LOG
	cout << "#Recv -> en_PACKET_CS_CHAT_REQ_MESSAGE - accountNo:" << accountNo << endl;

	SerializePacketPtr msgPacket = SerializePacketPtr::MakeSerializePacket();
	msgPacket.Clear();

	WORD type = en_PACKET_CS_CHAT_RES_MESSAGE;
	msgPacket << type;

	msgPacket << accountNo;

	bool ret = _playerManager->SerializeIDnNickname(accountNo, pPacket);
	if (ret == false)
		return;

	msgPacket << messageLen;
	msgPacket.Putdata((char*)message, messageLen);

	vector<INT64> aroundPlayer;
	_sectorManager->GetAroundAccountNo(accountNo, aroundPlayer);

	for (int i = 0; i < aroundPlayer.size(); i++)
	{
		DWORD64 aroundPlayerSessionID = _playerManager->GetSessionID(aroundPlayer[i]);
		if (aroundPlayerSessionID == -1)
			continue;

		// LOG
		cout << "#Send <- en_PACKET_CS_CHAT_RES_MESSAGE - accountNo:" << accountNo << endl;

		SendPacket(aroundPlayerSessionID, msgPacket);
	}
}

void ChatServer::PacketProc_Heartbeat(DWORD64 sessionID)
{
	// LOG
	INT64 accountNo = _playerManager->GetAccountNo(sessionID);
	cout << "#Recv -> en_PACKET_CS_CHAT_REQ_HEARTBEAT - accountNo:" << accountNo << endl;

	_playerManager->UpdateHeartbeat(sessionID);
}

void ChatServer::ReleaseProc(SerializePacketPtr pPacket)
{
	DWORD64 sessionID;
	pPacket.GetData((char*)&sessionID, sizeof(DWORD64));

	INT64 accountNo = _playerManager->ReleaseProc(sessionID);
	if (accountNo != -1)
	{
		_sectorManager->ReleaseProc(accountNo);
		cout << "# Release 완료 - accountNo:" << accountNo << endl;
	}
	else
	{
		cout << "# Release 완료 - sessionID:" << sessionID << endl;
	}
}

void ChatServer::DisconnectUnresponsivePlayers()
{
	static DWORD acceptOldTime = timeGetTime();
	static DWORD heartbeatOldTime = timeGetTime();

	// TODO: 하트비트 & Accept후 NoLogin
	DWORD nowTime = timeGetTime();
	DWORD acceptDiffTime = nowTime - acceptOldTime;
	DWORD heatbeatDiffTime = nowTime - heartbeatOldTime;

	if (acceptDiffTime >= 1000)
	{
		_playerManager->CheckAcceptTime(nowTime);

		acceptOldTime = nowTime;
	}

	if (heatbeatDiffTime >= 10000)
	{
		_playerManager->CheckHeartbeat(nowTime);

		heartbeatOldTime = nowTime;
	}
}
