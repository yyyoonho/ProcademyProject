#include "stdafx.h"
#include "NetServer.h"
#include "CommonProtocol.h"
#include "MyConfig.h"

#include "MonitoringServer.h"

using namespace std;

MonitoringServer::MonitoringServer()
{
}

MonitoringServer::~MonitoringServer()
{
}

bool MonitoringServer::Start(const WCHAR* ipAddress, unsigned short port, unsigned short workerThreadCount, unsigned short coreSkip, bool isNagle, unsigned int maximumSessionCount, bool codecOnOff)
{
	bool ret = CNetServer::Start(ipAddress, port, workerThreadCount, coreSkip, isNagle, maximumSessionCount, codecOnOff);
	if (ret == false)
	{
		return false;
	}

	myConfig.Load("LoginConfig.ini");

	/*hEvent_Quit = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (hEvent_Quit == 0)
		return false;*/

	return false;
}

void MonitoringServer::Stop()
{
}

bool MonitoringServer::OnConnectionRequest(SOCKADDR_IN clientAddr)
{
	return false;
}

void MonitoringServer::OnAccept(DWORD64 sessionID, SOCKADDR_IN addr)
{
	Client* acceptClient = mp.Alloc();
	
	acceptClient->sessionID = sessionID;
	acceptClient->addr = addr;
	acceptClient->serverNo = -1;
	acceptClient->sessionRole = SESSION_ROLE::Accept;

	{
		lock_guard<mutex> lock(tmpClientMapLock);

		tmpClientMap.insert({ sessionID, acceptClient });
	}
}

void MonitoringServer::OnRelease(DWORD64 sessionID)
{
	bool flag = ReleaseTmpclient(sessionID);

	if (flag == false)
		flag = ReleaseOriginClient(sessionID);

	if (flag == false)
		ReleaseToolclient(sessionID);

	return;
}

void MonitoringServer::OnMessage(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	PacketProc(sessionID, pPacket);
}

void MonitoringServer::OnError(int errorCode, WCHAR* errorComment)
{
}


bool MonitoringServer::ReleaseTmpclient(DWORD64 sessionID)
{
	Client* removed;

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

bool MonitoringServer::ReleaseOriginClient(DWORD64 sessionID)
{
	Client* removed;

	{
		lock_guard<mutex> lock(clientMapLock);

		auto iter = clientMap.find(sessionID);
		if (iter == clientMap.end())
		{
			return false;
		}

		removed = iter->second;
		clientMap.erase(iter);
	}

	mp.Free(removed);
	return true;
}

bool MonitoringServer::ReleaseToolclient(DWORD64 sessionID)
{
	Client* removed = NULL;

	{
		lock_guard<mutex> lock(monitoringToolArrLock);

		for (int i = 0; i < monitoringToolArr.size(); i++)
		{
			if (monitoringToolArr[i]->sessionID != sessionID)
				continue;

			removed = monitoringToolArr[i];
			monitoringToolArr[i] = monitoringToolArr.back();
			monitoringToolArr.pop_back();
			break;
		}
	}

	if (removed == NULL)
	{
		DebugBreak();
		return false;
	}

	mp.Free(removed);
	return true;
}
