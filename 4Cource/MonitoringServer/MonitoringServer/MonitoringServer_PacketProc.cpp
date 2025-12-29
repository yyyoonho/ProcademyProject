#include "stdafx.h"
#include "NetServer.h"
#include "CommonProtocol.h"
#include "MonitoringServer.h"

using namespace std;

void MonitoringServer::PacketProc(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	WORD type;
	pPacket >> type;

	switch (type)
	{
	case en_PACKET_SS_MONITOR_LOGIN:
		PacketProc_ServerLogin(sessionID, pPacket);
		break;
	case en_PACKET_SS_MONITOR_DATA_UPDATE:
		PacketProc_DataUpdate(sessionID, pPacket);
		break;
	case en_PACKET_CS_MONITOR_TOOL_REQ_LOGIN:
		PacketProc_MonitorToolLogin(sessionID, pPacket);
		break;
	}
}

void MonitoringServer::PacketProc_ServerLogin(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	int serverNo;
	pPacket >> serverNo;

	Client* pClient = NULL;
	{
		lock_guard<mutex> lock(tmpClientMapLock);

		auto iter = tmpClientMap.find(sessionID);
		if (iter == tmpClientMap.end())
		{
			DebugBreak();
			return;
		}

		pClient = iter->second;
		tmpClientMap.erase(iter);
	}

	pClient->serverNo = serverNo;
	
	if (serverNo < 10)
		DebugBreak();
	else if (serverNo < 20)
		pClient->sessionRole = SESSION_ROLE::ChatServer;
	else if (serverNo < 30)
		pClient->sessionRole = SESSION_ROLE::LoginServer;
	else if (serverNo < 40)
		pClient->sessionRole = SESSION_ROLE::GameServer;

	pClient->dataCount = 0;
	for (int i = 0; i < en_PACKET_SS_MONITOR_DATA_UPDATE::COUNT; i++)
	{
		pClient->sumData[i] = 0;
		pClient->minData[i] = MAXDWORD64;
		pClient->maxData[i] = 0;
	}

	{
		lock_guard<mutex> lock(clientMapLock);

		clientMap.insert({ sessionID, pClient });
	}
}

void MonitoringServer::PacketProc_DataUpdate(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	BYTE dataType;
	int dataValue;
	int timeStamp;

	pPacket >> dataType;
	pPacket >> dataValue;
	pPacket >> timeStamp;

	// client찾기.
	Client* pClient = NULL;
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


	// Tool에 데이터 전송
	SerializePacketPtr newPacket = SerializePacketPtr::MakeSerializePacket();
	newPacket.Clear();

	newPacket << en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE;
	newPacket << pClient->serverNo;
	newPacket << dataType;
	newPacket << dataValue;
	newPacket << timeStamp;

	vector<DWORD64> monitoringToolSIDs;
	{
		lock_guard<mutex> lock(monitoringToolArrLock);

		for (int i = 0; i < monitoringToolArr.size(); i++)
		{
			monitoringToolSIDs.push_back(monitoringToolArr[i]->sessionID);
		}
	}

	for (int i = 0; i < monitoringToolSIDs.size(); i++)
	{
		SendPacket(monitoringToolSIDs[i], newPacket);
	}

	// DB저장용 데이터 쌓기
	pClient->sumData[dataType] += dataValue;
	pClient->minData[dataType] = min(pClient->minData[dataType], dataValue);
	pClient->maxData[dataType] = max(pClient->minData[dataType], dataValue);
}

void MonitoringServer::PacketProc_MonitorToolLogin(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	char tmpSessionKey[32];
	pPacket.GetData(tmpSessionKey, 32);

	BYTE status;

	if (memcmp(loginSessionKey, loginSessionKey, 32) == 0)
		status = dfMONITOR_TOOL_LOGIN_OK;
	else
		status = dfMONITOR_TOOL_LOGIN_ERR_SESSIONKEY;


	// 정상 로그인
	if (status == dfMONITOR_TOOL_LOGIN_OK)
	{
		Client* newClient = NULL;
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

		newClient->sessionRole = SESSION_ROLE::MonitoringTool;

		{
			lock_guard<mutex> lock(monitoringToolArrLock);

			monitoringToolArr.push_back(newClient);
		}	

		SerializePacketPtr newPacket = SerializePacketPtr::MakeSerializePacket();
		newPacket.Clear();

		WORD type = en_PACKET_CS_MONITOR_TOOL_RES_LOGIN;

		newPacket << type;
		newPacket << status;

		SendPacket(sessionID, newPacket);
	}

	// 비정상 로그인 : loginSessionKey Invalid
	else
	{
		SerializePacketPtr newPacket = SerializePacketPtr::MakeSerializePacket();
		newPacket.Clear();

		WORD type = en_PACKET_CS_MONITOR_TOOL_RES_LOGIN;

		newPacket << type;
		newPacket << status;

		SendPacket(sessionID, newPacket);
		Disconnect(sessionID);
	}

}