#include "stdafx.h"
#include "Monitoring.h"
#include "NetServer.h"
#include "CommonProtocol.h"
#include "MyConfig.h"
#include "SendJob.h"
#include "MonitoringServer.h"
#include "LanMonitoringServer.h"

using namespace std;

void LanMonitoringServer::PacketProc(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	WORD type;
	pPacket >> type;

	switch (type)
	{
	case en_PACKET_SS_MONITOR_LOGIN:
		PakcetProc_Login(sessionID, pPacket);
		break;
	case en_PACKET_SS_MONITOR_DATA_UPDATE:
		PakcetProc_DataUpdate(sessionID, pPacket);
		break;
	}
}

void LanMonitoringServer::PakcetProc_Login(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	int serverNo;
	pPacket >> serverNo;

	LanClient* newClient = NULL;
	{
		lock_guard<mutex> lock(tmpClientMapLock);

		auto iter = tmpClientMap.find(sessionID);
		if (iter == tmpClientMap.end())
		{
			DebugBreak();
			return;
		}

		newClient = iter->second;
		tmpClientMap.erase(iter);
	}

	newClient->serverNo = serverNo;
	

	for (int i = 0; i < en_PACKET_SS_MONITOR_DATA_UPDATE::COUNT; i++)
	{
		newClient->count[i] = 0;
		newClient->activeData[i] = false;
		newClient->avgData[i] = 0;
		newClient->minData[i] = INT32_MAX;
		newClient->maxData[i] = 0;
	}

	{
		lock_guard<mutex> lock(clientMapLock);
		clientMap.insert({ sessionID, newClient });
	}

	if (serverNo < 100)
	{
		Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::ConnectedChatServerCount);
	}
	else if (serverNo < 1000)
	{
		Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::ConnectedLoginServerCount);
	}
	else if (serverNo < 10000)
	{
		Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::ConnectedGameServerCount);
	}
}

void LanMonitoringServer::PakcetProc_DataUpdate(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	BYTE dataType;
	int dataValue;
	int timeStamp;

	pPacket >> dataType;
	pPacket >> dataValue;
	pPacket >> timeStamp;

	LanClient* pClient;
	{
		lock_guard<mutex> lock(clientMapLock);
		auto iter = clientMap.find(sessionID);
		if (iter == clientMap.end())
		{
			DebugBreak();
			return;
		}

		pClient = iter->second;
	}

	stSendJob job;
	job.dataType = dataType;
	job.dataValue = dataValue;
	job.timeStamp = timeStamp;

	{
		lock_guard<mutex> lock(pClient->LanClientLock);
		
		pClient->count[dataType]++;
		pClient->activeData[dataType] = true;
		pClient->avgData[dataType] += (dataValue - pClient->avgData[dataType]) / pClient->count[dataType];
		pClient->minData[dataType] = min(pClient->minData[dataType], dataValue);
		pClient->maxData[dataType] = max(pClient->maxData[dataType], dataValue);

		job.serverNo = pClient->serverNo;
	}

	// TODO: Tool·Î sendPacketÇÏ±â.
	pNetMonitoringServer->EnqueueSendJob(job);
}