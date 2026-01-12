#include "stdafx.h"

#include "Monitoring.h"
#include "NetServer.h"
#include "CommonProtocol.h"
#include "MyConfig.h"
#include "SendJob.h"
#include "DBJob.h"
#include "DBConnector.h"

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

	hEvent_Quit = CreateEvent(NULL, TRUE, FALSE, NULL);

	hThread_DB = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&DBContentRun, this, NULL, NULL);
	if (hThread_DB == NULL)
	{
		return false;
	}

	return true;
}

void LanMonitoringServer::Stop()
{
	SetEvent(hEvent_Quit);

	CNetServer::Stop();
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

void LanMonitoringServer::DBContentRun(LPVOID* lParam)
{
	LanMonitoringServer* self = (LanMonitoringServer*)lParam;
	self->DBContentThread();
}

void LanMonitoringServer::DBContentThread()
{
	while (1)
	{
		DWORD ret = WaitForSingleObject(hEvent_Quit, 1000 * 60 * 10);
		if (ret == WAIT_OBJECT_0)
		{
			break;
		}

		// DB 저장할 Job 만들어서 DBWorker에 토스.
		
		vector<MonitoringSnapshot> snapShots;

		{
			lock_guard<mutex> lock(clientMapLock);

			auto iter = clientMap.begin();
			for (; iter != clientMap.end(); ++iter)
			{
				LanClient* client = iter->second;

				lock_guard<mutex> lock2(client->LanClientLock);

				for (int i = 0; i < en_PACKET_SS_MONITOR_DATA_UPDATE::COUNT; i++)
				{
					if (client->activeData[i] != true)
						continue;

					snapShots.push_back({
						client->serverNo,
						i,
						client->avgData[i],
						client->minData[i],
						client->maxData[i]
						});

					client->ResetData(i);
				}
			}
		}

		for (int i = 0; i < snapShots.size(); i++)
		{
			auto job = new DBMonitoringLog;

			job->_serverNo = snapShots[i].serverNo;
			job->_dataType = snapShots[i].dataType;
			job->_avg = snapShots[i].avg;
			job->_min = snapShots[i].min;
			job->_max = snapShots[i].max;

			DBConnector::GetInstance()->QueryWrite(0, job);
		}
	}
}


void LanClient::ResetData(int i)
{
	count[i] = 0;
	activeData[i] = false;
	avgData[i] = 0;
	minData[i] = INT32_MAX;
	maxData[i] = 0;
}
