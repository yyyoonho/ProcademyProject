#include "stdafx.h"
#include "Monitoring.h"
#include "NetServer.h"
#include "CommonProtocol.h"
#include "MyConfig.h"
#include "SendJob.h"

#include "LanMonitoringServer.h"

LanMonitoringServer::LanMonitoringServer()
{
}

LanMonitoringServer::~LanMonitoringServer()
{
}

bool LanMonitoringServer::Start(const WCHAR* ipAddress, unsigned short port, unsigned short workerThreadCount, unsigned short coreSkip, bool isNagle, unsigned int maximumSessionCount, bool codecOnOff, BYTE fixedKey, BYTE code)
{
	bool ret = CNetServer::Start(ipAddress, port, workerThreadCount, coreSkip, isNagle, maximumSessionCount, codecOnOff, fixedKey, code);
	if (ret == false)
	{
		return false;
	}

	// TODO: 수정예정
	//myConfig.Load("LoginConfig.ini");


	return true;
}

void LanMonitoringServer::Stop()
{
}

bool LanMonitoringServer::OnConnectionRequest(SOCKADDR_IN clientAddr)
{
	return false;
}

void LanMonitoringServer::OnAccept(DWORD64 sessionID, SOCKADDR_IN addr)
{
	LanClient* newClient = mp.Alloc();

	newClient->sessionID = sessionID;
	newClient->addr = addr;
	newClient->serverNo = -1;

	{
		lock_guard<mutex> lock(tmpClientMapLock);
		tmpClientMap.insert({ sessionID, newClient });
	}
}

void LanMonitoringServer::OnRelease(DWORD64 sessionID)
{
	bool flag = ReleaseTmpclient(sessionID);

	if (flag == false)
	{
		ReleaseOriginclient(sessionID);

	}
		
}

void LanMonitoringServer::OnMessage(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	PacketProc(sessionID, pPacket);
}

void LanMonitoringServer::OnError(int errorCode, WCHAR* errorComment)
{
}

void LanMonitoringServer::OnSendJob()
{
}

void LanMonitoringServer::RegisterNetServer(MonitoringServer* pNetMonitoringServer)
{
	this->pNetMonitoringServer = pNetMonitoringServer;
}


bool LanMonitoringServer::ReleaseTmpclient(DWORD64 sessionID)
{
	LanClient* removed = NULL;

	{
		lock_guard<mutex> lock(tmpClientMapLock);

		auto iter = tmpClientMap.find(sessionID);
		if (iter == tmpClientMap.end())
		{
			return false;
		}

		removed = iter->second;
		tmpClientMap.erase(iter);
	}

	mp.Free(removed);
	return true;
}

bool LanMonitoringServer::ReleaseOriginclient(DWORD64 sessionID)
{
	LanClient* removed = NULL;

	{
		lock_guard<mutex> lock(clientMapLock);

		auto iter = clientMap.find(sessionID);
		if (iter == clientMap.end())
		{
			DebugBreak();
			return false;
		}

		removed = iter->second;
		clientMap.erase(iter);
	}

	BYTE serverNo;
	{
		lock_guard<mutex> lock(removed->LanClientLock);

		serverNo = removed->serverNo;
	}

	if (serverNo < 100)
	{
		Monitoring::GetInstance()->DecreaseInterlocked(MonitorType::ConnectedChatServerCount);
	}
	else if (serverNo < 1000)
	{
		Monitoring::GetInstance()->DecreaseInterlocked(MonitorType::ConnectedLoginServerCount);
	}
	else if (serverNo < 10000)
	{
		Monitoring::GetInstance()->DecreaseInterlocked(MonitorType::ConnectedGameServerCount);
	}

	mp.Free(removed);
	return true;
}
