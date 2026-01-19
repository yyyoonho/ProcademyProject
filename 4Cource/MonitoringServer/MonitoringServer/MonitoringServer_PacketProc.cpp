#include "stdafx.h"

#include "LogManager.h"
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

	bool flag = true;

	switch (type)
	{
	case en_PACKET_CS_MONITOR_TOOL_REQ_LOGIN:
		PacketProc_MonitorToolLogin(sessionID, pPacket);
		break;

	default:
		// attack #2
		flag = false;
		Disconnect(sessionID);
		_LOG(dfLOG_LEVEL_SYSTEM, L"%ls\n", L"attack #2 Disconnect");
		return;
	}
}

void MonitoringServer::PacketProc_MonitorToolLogin(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	char tmpSessionKey[32];
	pPacket.GetData(tmpSessionKey, 32);

	BYTE status;

	// 최대 유저에 대한 제한.
	// attack #6
	Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::LoginProcessingCount);
	LONG estimated =
		Monitoring::GetInstance()->GetInterlocked(MonitorType::ConnectedMonitoringToolCount) +
		Monitoring::GetInstance()->GetInterlocked(MonitorType::LoginProcessingCount);


	if (estimated > _maxPlayerCount)
	{
		Monitoring::GetInstance()->DecreaseInterlocked(MonitorType::LoginProcessingCount);
		Disconnect(sessionID);

		_LOG(dfLOG_LEVEL_SYSTEM, L"%ls\n", L"attack #6 Disconnect");

		return;
	}


	// 이미 로그인된 세션이 다시 로그인 요청
	// attack #13
	{
		lock_guard<mutex> lock(monitoringToolArrLock);

		bool flag = true;
		for (int i = 0; i < monitoringToolArr.size(); i++)
		{
			if (monitoringToolArr[i]->sessionID == sessionID)
			{
				flag = false;
				break;
			}
		}

		if (flag == false)
		{
			// attack #13
			Disconnect(sessionID);
			_LOG(dfLOG_LEVEL_SYSTEM, L"%ls\n", L"attack #LOGIN_DUP_SESSION");

			Monitoring::GetInstance()->DecreaseInterlocked(MonitorType::LoginProcessingCount);
			return;
		}
	}


	if (memcmp(loginSessionKey, tmpSessionKey, 32) == 0)
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
				Monitoring::GetInstance()->DecreaseInterlocked(MonitorType::LoginProcessingCount);
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
		Monitoring::GetInstance()->DecreaseInterlocked(MonitorType::LoginProcessingCount);
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

		Monitoring::GetInstance()->DecreaseInterlocked(MonitorType::LoginProcessingCount);

		Disconnect(sessionID);
	}

}