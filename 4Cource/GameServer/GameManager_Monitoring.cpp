#include "stdafx.h"

#include "CommonProtocol.h"
#include "Monitoring.h"
#include "Field.h"
#include "NetServer.h"
#include "SendJob.h"
#include "NetClient.h"
#include "NetClient_Monitoring.h"
#include "GameManager.h"


void GameManager::TossMonitoringData()
{
	// TODO: 데이터를 모아서 NetClient_Monitoring으로 토스하자.

	// 1. GameServer 실행여부
	{
		WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
		BYTE dataType = dfMONITOR_DATA_TYPE_GAME_SERVER_RUN;
		int dataValue = 1;
		int timeStamp = (int)time(nullptr);

		stSendJob job;
		job.dataType = dataType;
		job.dataValue = dataValue;
		job.timeStamp = timeStamp;

		pNetClient->EnqueueSendJob(job);
	}

	// 2. GameServer CPU사용률
	{
		WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
		BYTE dataType = dfMONITOR_DATA_TYPE_GAME_SERVER_CPU;
		int dataValue = Monitoring::GetInstance()->GetCpuUsage();
		int timeStamp = (int)time(nullptr);

		stSendJob job;
		job.dataType = dataType;
		job.dataValue = dataValue;
		job.timeStamp = timeStamp;

		pNetClient->EnqueueSendJob(job);
	}

	// 3. GameServer 메모리 사용 MByte
	{
		WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
		BYTE dataType = dfMONITOR_DATA_TYPE_GAME_SERVER_MEM;
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
		BYTE dataType = dfMONITOR_DATA_TYPE_GAME_SESSION;
		int dataValue = Monitoring::GetInstance()->_monitoringArr[(int)MonitorType::SessionNum];
		int timeStamp = (int)time(nullptr);

		stSendJob job;
		job.dataType = dataType;
		job.dataValue = dataValue;
		job.timeStamp = timeStamp;

		pNetClient->EnqueueSendJob(job);
	}

	// 5. Auth 플레이어 수
	{
		WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
		BYTE dataType = dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER;
		int dataValue = Monitoring::GetInstance()->_monitoringArr[(int)MonitorType::AuthPlayerCount];
		int timeStamp = (int)time(nullptr);

		stSendJob job;
		job.dataType = dataType;
		job.dataValue = dataValue;
		job.timeStamp = timeStamp;

		pNetClient->EnqueueSendJob(job);
	}

	// 6.Game 플레이어 수
	{
		WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
		BYTE dataType = dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER;
		int dataValue = Monitoring::GetInstance()->_monitoringArr[(int)MonitorType::GamePlayerCount];
		int timeStamp = (int)time(nullptr);

		stSendJob job;
		job.dataType = dataType;
		job.dataValue = dataValue;
		job.timeStamp = timeStamp;

		pNetClient->EnqueueSendJob(job);
	}

	// 7. accept TPS
	{
		WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
		BYTE dataType = dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS;
		int dataValue = Monitoring::GetInstance()->_monitoringArr[(int)MonitorType::AcceptTPS];
		int timeStamp = (int)time(nullptr);

		stSendJob job;
		job.dataType = dataType;
		job.dataValue = dataValue;
		job.timeStamp = timeStamp;

		pNetClient->EnqueueSendJob(job);
	}

	// 8. recv TPS
	{
		WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
		BYTE dataType = dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS;
		int dataValue = Monitoring::GetInstance()->_monitoringArr[(int)MonitorType::RecvMessageTPS];
		int timeStamp = (int)time(nullptr);

		stSendJob job;
		job.dataType = dataType;
		job.dataValue = dataValue;
		job.timeStamp = timeStamp;

		pNetClient->EnqueueSendJob(job);
	}

	// 9. send 완료 TPS
	{
		WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
		BYTE dataType = dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS;
		int dataValue = Monitoring::GetInstance()->_monitoringArr[(int)MonitorType::SendMessageTPS];
		int timeStamp = (int)time(nullptr);

		stSendJob job;
		job.dataType = dataType;
		job.dataValue = dataValue;
		job.timeStamp = timeStamp;

		pNetClient->EnqueueSendJob(job);
	}

	// 10. DB 저장 메시지 초당 처리횟수
	{

	}

	// 11. DB 저장 메시지 큐 갯수 (남은 수)
	{

	}

	// 12. Auth field 초당 프레임
	{
		WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
		BYTE dataType = dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS;
		int dataValue = Monitoring::GetInstance()->_monitoringArr[(int)MonitorType::AuthFieldFrame];
		int timeStamp = (int)time(nullptr);

		stSendJob job;
		job.dataType = dataType;
		job.dataValue = dataValue;
		job.timeStamp = timeStamp;

		pNetClient->EnqueueSendJob(job);
	}

	// 13. Game field 초당 프레임
	{
		WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
		BYTE dataType = dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS;
		int dataValue = Monitoring::GetInstance()->_monitoringArr[(int)MonitorType::EchoFieldFrame];
		int timeStamp = (int)time(nullptr);

		stSendJob job;
		job.dataType = dataType;
		job.dataValue = dataValue;
		job.timeStamp = timeStamp;

		pNetClient->EnqueueSendJob(job);
	}

	// 14. GameServer 패킷 풀 사용량
	{
		WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
		BYTE dataType = dfMONITOR_DATA_TYPE_GAME_PACKET_POOL;
		int dataValue = (int)Monitoring::GetInstance()->_monitoringArr[(int)MonitorType::PacketPool_FULL];
		int timeStamp = (int)time(nullptr);

		stSendJob job;
		job.dataType = dataType;
		job.dataValue = dataValue;
		job.timeStamp = timeStamp;

		pNetClient->EnqueueSendJob(job);
	}
}