#include "stdafx.h"

#include <cpp_redis/cpp_redis>
#pragma comment (lib, "cpp_redis.lib")
#pragma comment (lib, "tacopie.lib")

#include "LogManager.h"
#include "Monitoring.h"
#include "NetServer.h"
#include "PacketProc.h"
#include "CommonProtocol.h"
#include "SendJob.h"

#include "NetClient.h"
#include "NetClient_Monitoring.h"

#include "LoginServer.h"

using namespace std;

void LoginServer::TossMonitoringData()
{
	// TODO: 데이터를 모아서 NetClient_Monitoring으로 토스하자.

	// 1. 실행여부
	{
		WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
		BYTE dataType = dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN;
		int dataValue = 1;
		int timeStamp = (int)time(nullptr);

		stSendJob job;
		job.dataType = dataType;
		job.dataValue = dataValue;
		job.timeStamp = timeStamp;

		pNetClient->EnqueueSendJob(job);
	}

	// 2. CPU사용률
	{
		WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
		BYTE dataType = dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU;
		int dataValue = Monitoring::GetInstance()->GetCpuUsage();
		int timeStamp = (int)time(nullptr);

		stSendJob job;
		job.dataType = dataType;
		job.dataValue = dataValue;
		job.timeStamp = timeStamp;

		pNetClient->EnqueueSendJob(job);
	}

	// 3. 메모리 사용 MByte
	{
		WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
		BYTE dataType = dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM;
		int dataValue = Monitoring::GetInstance()->GetPrivateBytes();
		int timeStamp = (int)time(nullptr);

		stSendJob job;
		job.dataType = dataType;
		job.dataValue = dataValue;
		job.timeStamp = timeStamp;

		pNetClient->EnqueueSendJob(job);
	}

	// 4. 세션 수
	{
		WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
		BYTE dataType = dfMONITOR_DATA_TYPE_LOGIN_SESSION;
		int dataValue = Monitoring::GetInstance()->_monitoringArr[(int)MonitorType::SessionNum];
		int timeStamp = (int)time(nullptr);

		stSendJob job;
		job.dataType = dataType;
		job.dataValue = dataValue;
		job.timeStamp = timeStamp;

		pNetClient->EnqueueSendJob(job);
	}

	// 5. AuthTPS
	{
		WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
		BYTE dataType = dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS;
		int dataValue = Monitoring::GetInstance()->_monitoringArr[(int)MonitorType::AuthTPS];
		int timeStamp = (int)time(nullptr);

		stSendJob job;
		job.dataType = dataType;
		job.dataValue = dataValue;
		job.timeStamp = timeStamp;

		pNetClient->EnqueueSendJob(job);
	}

	// 6. 패킷풀 사용량
	{
		WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
		BYTE dataType = dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL;
		int dataValue = Monitoring::GetInstance()->_monitoringArr[(int)MonitorType::PacketPool_FULL];
		int timeStamp = (int)time(nullptr);

		stSendJob job;
		job.dataType = dataType;
		job.dataValue = dataValue;
		job.timeStamp = timeStamp;

		pNetClient->EnqueueSendJob(job);
	}

}