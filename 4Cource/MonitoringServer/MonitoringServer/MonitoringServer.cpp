#include "stdafx.h"
#include "Monitoring.h"
#include "NetServer.h"
#include "CommonProtocol.h"
#include "MyConfig.h"
#include "SendJob.h"

#include "MonitoringServer.h"

using namespace std;

MonitoringServer::MonitoringServer()
{
}

MonitoringServer::~MonitoringServer()
{
}

bool MonitoringServer::Start(const WCHAR* ipAddress, unsigned short port, unsigned short workerThreadCount, unsigned short coreSkip, bool isNagle, unsigned int maximumSessionCount, bool codecOnOff, BYTE fixedKey, BYTE code)
{
	bool ret = CNetServer::Start(ipAddress, port, workerThreadCount, coreSkip, isNagle, maximumSessionCount, codecOnOff, fixedKey, code);
	if (ret == false)
	{
		return false;
	}

	//myConfig.Load("LoginConfig.ini");

	/*hEvent_Quit = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (hEvent_Quit == 0)
		return false;*/



	return true;
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
	NetClient* acceptClient = mp.Alloc();
	
	acceptClient->sessionID = sessionID;
	acceptClient->addr = addr;

	{
		lock_guard<mutex> lock(tmpClientMapLock);

		tmpClientMap.insert({ sessionID, acceptClient });
	}

	return;
}

void MonitoringServer::OnRelease(DWORD64 sessionID)
{
	bool flag = ReleaseTmpclient(sessionID);

	if (flag == false)
	{
		ReleaseToolclient(sessionID);
		Monitoring::GetInstance()->DecreaseInterlocked(MonitorType::ConnectedMonitoringToolCount);
	}
	
	return;
}

void MonitoringServer::OnMessage(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	PacketProc(sessionID, pPacket);
}

void MonitoringServer::OnError(int errorCode, WCHAR* errorComment)
{
}

void MonitoringServer::OnSendJob()
{
	// 3. NetServer의 IOCP워커 쓰레드가 queue에 있는 구조체 꺼내서 Tool로 sendPacket();
	stSendJob job;
	bool ret = sendJobQ.Dequeue(&job);
	if (ret == false)
		return;

	Monitoring::GetInstance()->DecreaseInterlocked(MonitorType::SendJobQ);

	SerializePacketPtr newPacket = SerializePacketPtr::MakeSerializePacket();
	newPacket.Clear();

	WORD type = en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE;

	newPacket << type;
	newPacket << job.serverNo;
	newPacket << job.dataType;
	newPacket << job.dataValue;
	newPacket << job.timeStamp;

	vector<DWORD64> toolSIDs;
	{
		lock_guard<mutex> lock(monitoringToolArrLock);

		for (int i = 0; i < monitoringToolArr.size(); i++)
		{
			toolSIDs.push_back(monitoringToolArr[i]->sessionID);
		}
	}

	for (int i = 0; i < toolSIDs.size(); i++)
	{
		SendPacket(toolSIDs[i], newPacket);
	}
}

void MonitoringServer::EnqueueSendJob(stSendJob sendJob)
{
	// TODO: 
	// 1. queue로 구조체 집어넣기. 
	// 2. 쓰레드가 PQCS호출.(?)
	// 3. NetServer의 IOCP워커 쓰레드가 queue에 있는 구조체 꺼내서 Tool로 sendPacket();

	// 1.
	sendJobQ.Enqueue(sendJob);
	Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::SendJobQ);

	// 2.
	PQCSSendJob();
}


bool MonitoringServer::ReleaseTmpclient(DWORD64 sessionID)
{
	NetClient* removed;

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

bool MonitoringServer::ReleaseToolclient(DWORD64 sessionID)
{
	NetClient* removed = NULL;

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
