#include "stdafx.h"

#include <cpp_redis/cpp_redis>
#pragma comment (lib, "cpp_redis.lib")
#pragma comment (lib, "tacopie.lib")

#include "CommonProtocol.h"
#include "Monitoring.h"
#include "NetServer.h"
#include "LoginServer.h"

LoginServer::LoginServer()
{
	
}

LoginServer::~LoginServer()
{
}

bool LoginServer::Start(const WCHAR* ipAddress, unsigned short port, unsigned short workerThreadCount, unsigned short coreSkip, bool isNagle, unsigned int maximumSessionCount, bool codecOnOff)
{
	bool ret = CNetServer::Start(ipAddress, port, workerThreadCount, coreSkip, isNagle, maximumSessionCount, codecOnOff);
	if (ret == false)
	{
		return false;
	}

	client = new cpp_redis::client;
	client->connect();

	hEvent_Quit = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (hEvent_Quit == 0)
		return false;

	hThread_Monitoring = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&MonitorThreadRun, this, NULL, NULL);

	return true;
}

void LoginServer::Stop()
{
	CNetServer::Stop();

	SetEvent(hEvent_Quit);
	WaitForSingleObject(hThread_Monitoring, INFINITE);
}

bool LoginServer::OnConnectionRequest(SOCKADDR_IN clientAddr)
{
	return false;
}

void LoginServer::OnAccept(DWORD64 sessionID)
{
}

void LoginServer::OnRelease(DWORD64 sessionID)
{
}

void LoginServer::OnMessage(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	PacketProc(sessionID, pPacket);
}

void LoginServer::OnError(int errorCode, WCHAR* errorComment)
{
}

void LoginServer::PacketProc(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	WORD msgType;
	pPacket >> msgType;

	switch (msgType)
	{
	case en_PACKET_CS_LOGIN_REQ_LOGIN:
		PacketProc_Login(sessionID, pPacket);
		break;
	}


}

void LoginServer::PacketProc_Login(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	INT64 accountNo;
	char sessionKey[64];

	pPacket >> accountNo;
	pPacket.GetData(sessionKey, sizeof(char) * 64);

	// 1단계: 토큰인증
	if (AuthorizeToken(sessionKey) == false)
		return;

	// 2단계: Redis에 토큰 저장
	SaveTokenToRedis(accountNo, sessionKey);

	// 3단계: Send RES
	SendPacket_RES_LOGIN(accountNo, sessionID);
}

bool LoginServer::AuthorizeToken(const char* token)
{
	// TODO: 플랫폼 API를 대신하여 회원DB를 한번 다녀오는 절차.
	// DB를 읽는것으로 블락 유도.

	return true;
}

bool LoginServer::SaveTokenToRedis(INT64 accountNo, const char* sessionKey)
{
	string key = to_string(accountNo);
	string token = string(sessionKey);

	client->set(key, token);
	/*client->get(key, [](cpp_redis::reply& reply) {
		std::cout << reply << std::endl;
		});*/

	client->sync_commit();

	return true;
}

void LoginServer::SendPacket_RES_LOGIN(INT64 accountNo, DWORD64 sessionID)
{
	SerializePacketPtr newPacket = SerializePacketPtr::MakeSerializePacket();
	newPacket.Clear();

	WORD type = en_PACKET_CS_LOGIN_RES_LOGIN;
	BYTE status = dfGAME_LOGIN_OK;

	WCHAR ID[20] = { 0 };
	WCHAR Nickname[20] = { 0 };

	WCHAR GameServerIP[16] = { 0 };
	USHORT GameServerPort;
	WCHAR ChatServerIP[16] = { 0 };
	USHORT ChatServerPort;

	swprintf_s(ID, 20, L"ID_%lld", accountNo);
	swprintf_s(Nickname, 20, L"Nickname_%lld", accountNo);

	wcscpy_s(GameServerIP, L"7.7.7.7");
	GameServerPort = 7777;

	wcscpy_s(ChatServerIP, L"127.0.0.1");
	ChatServerPort = 20601;

	newPacket << type;

	newPacket << accountNo;
	newPacket << status;

	newPacket.Putdata((char*)ID, sizeof(WCHAR) * 20);
	newPacket.Putdata((char*)Nickname, sizeof(WCHAR) * 20);
	
	newPacket.Putdata((char*)GameServerIP, sizeof(WCHAR) * 16);
	newPacket << GameServerPort;
	newPacket.Putdata((char*)ChatServerIP, sizeof(WCHAR) * 16);
	newPacket << ChatServerPort;

	SendPacket(sessionID, newPacket);

	return;
}

void LoginServer::MonitorThreadRun(LPVOID* lParam)
{
	LoginServer* self = (LoginServer*)lParam;
	self->MonitorThread();
}

void LoginServer::MonitorThread()
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
