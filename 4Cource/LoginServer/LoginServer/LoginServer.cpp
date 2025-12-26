#include "stdafx.h"

#include <cpp_redis/cpp_redis>
#pragma comment (lib, "cpp_redis.lib")
#pragma comment (lib, "tacopie.lib")

#include "MyConfig.h"
#include "CommonProtocol.h"
#include "Monitoring.h"

#include "DBJob.h"
#include "DBConnector.h"

#include "Player.h"
#include "NetServer.h"
#include "LoginServer.h"



LoginServer::LoginServer()
{
	
}

LoginServer::~LoginServer()
{
}

void LoginServer::ConvertToUTF16()
{
	{
		std::wstring wideStr;

		int len = MultiByteToWideChar(
			CP_UTF8,
			0,
			myConfig.loginServerConfig.chatServer_ip1.data(),
			(int)myConfig.loginServerConfig.chatServer_ip1.size(),
			nullptr,
			0);

		wideStr.resize(len);

		MultiByteToWideChar(
			CP_UTF8,
			0,
			myConfig.loginServerConfig.chatServer_ip1.data(),
			(int)myConfig.loginServerConfig.chatServer_ip1.size(),
			&wideStr[0],
			len);

		wcscpy_s(_chattingServerIpStr1, wideStr.c_str());
	}

	{
		std::wstring wideStr;

		int len = MultiByteToWideChar(
			CP_UTF8,
			0,
			myConfig.loginServerConfig.chatServer_ip2.c_str(),
			(int)myConfig.loginServerConfig.chatServer_ip2.size(),
			nullptr,
			0);

		wideStr.resize(len);

		MultiByteToWideChar(
			CP_UTF8,
			0,
			myConfig.loginServerConfig.chatServer_ip2.c_str(),
			(int)myConfig.loginServerConfig.chatServer_ip2.size(),
			&wideStr[0],
			len);

		wcscpy_s(_chattingServerIpStr2, wideStr.c_str());
	}

	{
		std::wstring wideStr;

		int len = MultiByteToWideChar(
			CP_UTF8,
			0,
			myConfig.loginServerConfig.dummy_ip1.c_str(),
			(int)myConfig.loginServerConfig.dummy_ip1.size(),
			nullptr,
			0);

		wideStr.resize(len);

		MultiByteToWideChar(
			CP_UTF8,
			0,
			myConfig.loginServerConfig.dummy_ip1.c_str(),
			(int)myConfig.loginServerConfig.dummy_ip1.size(),
			&wideStr[0],
			len);

		//_dummyIpStr1 = wideStr.c_str();
		wcscpy_s(_dummyIpStr1, wideStr.c_str());
	}

	{
		std::wstring wideStr;

		int len = MultiByteToWideChar(
			CP_UTF8,
			0,
			myConfig.loginServerConfig.dummy_ip2.c_str(),
			(int)myConfig.loginServerConfig.dummy_ip2.size(),
			nullptr,
			0);

		wideStr.resize(len);

		MultiByteToWideChar(
			CP_UTF8,
			0,
			myConfig.loginServerConfig.dummy_ip2.c_str(),
			(int)myConfig.loginServerConfig.dummy_ip2.size(),
			&wideStr[0],
			len);

		//_dummyIpStr2 = wideStr.c_str();
		wcscpy_s(_dummyIpStr2, wideStr.c_str());
	}
}

bool LoginServer::Start(const WCHAR* ipAddress, unsigned short port, unsigned short workerThreadCount, unsigned short coreSkip, bool isNagle, unsigned int maximumSessionCount, bool codecOnOff)
{
	bool ret = CNetServer::Start(ipAddress, port, workerThreadCount, coreSkip, isNagle, maximumSessionCount, codecOnOff);
	if (ret == false)
	{
		return false;
	}

	myConfig.Load("LoginConfig.ini");
	ConvertToUTF16();

	InetPtonW(AF_INET, _dummyIpStr1, &_dummyIpAddr1);
	InetPtonW(AF_INET, _dummyIpStr2, &_dummyIpAddr2);

	hEvent_Quit = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (hEvent_Quit == 0)
		return false;

	hThread_Monitoring = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&MonitorThreadRun, this, NULL, NULL);
	hThread_Heartbeat = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&HeartbeatThreadRun, this, NULL, NULL);


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

void LoginServer::OnAccept(DWORD64 sessionID, SOCKADDR_IN addr)
{
	Player* newPlayer = mp.Alloc();

	{
		lock_guard<mutex> lock(newPlayer->playerLock);
		newPlayer->sessionID = sessionID;
		newPlayer->_addr = addr;
		newPlayer->heartbeat = GetTickCount64();
	}

	{
		lock_guard<mutex> lock(SIDToPlayerLock);
		SIDToPlayer.insert({sessionID, newPlayer});
	}
}

void LoginServer::OnRelease(DWORD64 sessionID)
{
	Player* removed = NULL;
	{
		lock_guard<mutex> lock(SIDToPlayerLock);

		auto iter = SIDToPlayer.find(sessionID);
		if (iter == SIDToPlayer.end())
		{
			DebugBreak();
		}

		removed = iter->second;
		SIDToPlayer.erase(iter);
	}

	mp.Free(removed);
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
	if (AuthorizeToken(accountNo, sessionKey) == false)
		return;

	// 2단계: Redis에 토큰 저장
	SaveTokenToRedis(accountNo, sessionKey);

	// 3단계: Send RES
	SendPacket_RES_LOGIN(accountNo, sessionID);
}

bool LoginServer::AuthorizeToken(INT64 accountNo, const char* token)
{
	// TODO: 플랫폼 API를 대신하여 회원DB를 한번 다녀오는 절차.
	// DB를 읽는것으로 블락 유도.

	DBCheckAccountInfo* job = new DBCheckAccountInfo;
	job->_accountNo = accountNo;

	DBResult queryResult;
	DBConnector::GetInstance()->QueryRead(job, queryResult);

	string tmpToken = queryResult[0]["sessionkey"];

	return true;
}

bool LoginServer::SaveTokenToRedis(INT64 accountNo, const char* sessionKey)
{
	/*static thread_local cpp_redis::client redisClient;

	if (!redisClient.is_connected())
	{
		redisClient.connect();
		redisClient.sync_commit();
	}

	string key = to_string(accountNo);
	string token = string(sessionKey, 64);

	redisClient.send({ "SET",key,token,"EX","10" });

	redisClient.sync_commit();

	return true;*/


	lock_guard<mutex> lock(redisLock);
	if (redisClient == NULL)
	{
		redisClient = new cpp_redis::client;
	}

	if (!redisClient->is_connected())
	{
		redisClient->connect();
		redisClient->sync_commit();
	}

	string key = to_string(accountNo);
	string token = string(sessionKey, 64);

	redisClient->send({ "SET",key,token,"EX","10" });

	redisClient->sync_commit();

	return true;
}

void LoginServer::SendPacket_RES_LOGIN(INT64 accountNo, DWORD64 sessionID)
{
	sockaddr_in playerAddr;
	Player* targetPlayer = NULL;

	{
		lock_guard<mutex> lock(SIDToPlayerLock);
		auto iter = SIDToPlayer.find(sessionID);
		if (iter == SIDToPlayer.end())
		{
			DebugBreak();
		}
		
		targetPlayer = iter->second;
	}

	playerAddr = targetPlayer->_addr;

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

	if (playerAddr.sin_addr.S_un.S_addr == _dummyIpAddr1.S_un.S_addr)
	{
		wcscpy_s(ChatServerIP, _chattingServerIpStr1);
	}
	else if (playerAddr.sin_addr.S_un.S_addr == _dummyIpAddr2.S_un.S_addr)
	{
		wcscpy_s(ChatServerIP, _chattingServerIpStr2);
	}
	else
	{
		wcscpy_s(ChatServerIP, L"127.0.0.1");
	}
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

void LoginServer::UpdateHeartbeat(DWORD64 sessionID)
{
	Player* pPlayer;

	{
		lock_guard<mutex> lock(SIDToPlayerLock);

		auto iter = SIDToPlayer.find(sessionID);
		if (iter == SIDToPlayer.end())
		{
			DebugBreak();
		}

		pPlayer = iter->second;
	}

	{
		lock_guard<mutex> lock(pPlayer->playerLock);

		if (pPlayer->sessionID != sessionID)
		{
			DebugBreak();
		}

		pPlayer->heartbeat = GetTickCount64();
	}
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

void LoginServer::HeartbeatThreadRun(LPVOID* lParam)
{
	LoginServer* self = (LoginServer*)lParam;
	self->HeartbeatThread();
}

void LoginServer::HeartbeatThread()
{
	while (1)
	{
		DWORD ret = WaitForSingleObject(hEvent_Quit, 10000);
		if (ret == WAIT_OBJECT_0)
		{
			break;
		}

		DWORD64 nowTime = GetTickCount64();
		vector<DWORD64> kickVec;

		{
			lock_guard<mutex> lock(SIDToPlayerLock);

			auto iter = SIDToPlayer.begin();
			for (; iter != SIDToPlayer.end(); ++iter)
			{
				Player* pPlayer = iter->second;
				{
					lock_guard<mutex> lock2(pPlayer->playerLock);
					DWORD64 heartbeat = pPlayer->heartbeat;

					if (nowTime - heartbeat >= 5000)
					{
						kickVec.push_back(pPlayer->sessionID);
					}
				}
			}
		}
		
		for (int i = 0; i < kickVec.size(); i++)
		{
			Disconnect(kickVec[i]);
		}
	}
}
