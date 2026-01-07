#include "stdafx.h"
#include "Monitoring.h"
#include "NetServer.h"
#include "CommonProtocol.h"
#include "SendJob.h"

#include "MonitoringServer.h"

using namespace std;

void MonitoringServer::PacketProc(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	WORD type;
	pPacket >> type;

	switch (type)
	{
	case en_PACKET_CS_MONITOR_TOOL_REQ_LOGIN:
		PacketProc_MonitorToolLogin(sessionID, pPacket);
		break;
	}
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
		NetClient* newClient = NULL;
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

		Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::ConnectedMonitoringToolCount);
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