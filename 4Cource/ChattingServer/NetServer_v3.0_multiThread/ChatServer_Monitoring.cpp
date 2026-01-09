#include "stdafx.h"

#include "LogManager.h"
#include "Monitoring.h"
#include "NetServer.h"
#include "PacketProc.h"
#include "CommonProtocol.h"
#include "SendJob.h"

#include "NetClient.h"
#include "NetClient_Monitoring.h"

#include "ChatServer.h"

using namespace std;


void ChatServer::TossMonitoringData()
{
	// TODO: 데이터를 모아서 NetClient_Monitoring으로 토스하자.

	// 1. 실행여부
	{
		WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
		BYTE dataType = dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN;
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
		BYTE dataType = dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU;
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
		BYTE dataType = dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM;
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
		BYTE dataType = dfMONITOR_DATA_TYPE_CHAT_SESSION;
		int dataValue = Monitoring::GetInstance()->_monitoringArr[(int)MonitorType::SessionNum];
		int timeStamp = (int)time(nullptr);

		stSendJob job;
		job.dataType = dataType;
		job.dataValue = dataValue;
		job.timeStamp = timeStamp;

		pNetClient->EnqueueSendJob(job);
	}

	// 5. 플레이어 수
	{
		WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
		BYTE dataType = dfMONITOR_DATA_TYPE_CHAT_PLAYER;
		int dataValue = Monitoring::GetInstance()->_monitoringArr[(int)MonitorType::PlayerCount];
		int timeStamp = (int)time(nullptr);

		stSendJob job;
		job.dataType = dataType;
		job.dataValue = dataValue;
		job.timeStamp = timeStamp;

		pNetClient->EnqueueSendJob(job);
	}

	// 6. RecvTPS
	{
		WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
		BYTE dataType = dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS;
		int dataValue = Monitoring::GetInstance()->_monitoringArr[(int)MonitorType::RecvMessageTPS];
		int timeStamp = (int)time(nullptr);

		stSendJob job;
		job.dataType = dataType;
		job.dataValue = dataValue;
		job.timeStamp = timeStamp;

		pNetClient->EnqueueSendJob(job);
	}

	// 7. 패킷풀 사용량
	{
		WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
		BYTE dataType = dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL;
		int dataValue = (int)Monitoring::GetInstance()->_monitoringArr[(int)MonitorType::PacketPool_FULL];
		int timeStamp = (int)time(nullptr);

		stSendJob job;
		job.dataType = dataType;
		job.dataValue = dataValue;
		job.timeStamp = timeStamp;

		pNetClient->EnqueueSendJob(job);
	}

	// 8. 기타 풀 사용량
	{

	}
}