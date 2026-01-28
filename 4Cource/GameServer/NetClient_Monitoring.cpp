#include "stdafx.h"
#include "Monitoring.h"
#include "NetClient.h"
#include "SendJob.h"
#include "CommonProtocol.h"

#include "NetClient_Monitoring.h"

using namespace std;

NetClient_Monitoring::NetClient_Monitoring()
{
}

NetClient_Monitoring::~NetClient_Monitoring()
{
}

bool NetClient_Monitoring::Connect(const WCHAR* ipAddress, unsigned short port, unsigned short workerThreadCount, unsigned short coreSkip, bool isNagle, bool codecOnOff, BYTE fixedKey, BYTE code)
{
	bool ret = CNetClient::Connect(ipAddress, port, workerThreadCount, coreSkip, isNagle, codecOnOff, fixedKey, code);
	if(ret == false)
	{
		return false;
	}

	return true;
}

void NetClient_Monitoring::OnEnterJoin()
{
	SerializePacketPtr newPacket = SerializePacketPtr::MakeSerializePacket();
	newPacket.Clear();

	WORD type = en_PACKET_SS_MONITOR_LOGIN;
	int serverNo = _serverNo++;

	newPacket << type;
	newPacket << serverNo;

	SendPacket(newPacket);

	sendJobQ.Clear();

	connected = true;
}

void NetClient_Monitoring::OnLeaveServer()
{
	connected = false;
	sendJobQ.Clear();
}

void NetClient_Monitoring::OnMessage(SerializePacketPtr pPacket)
{
}

void NetClient_Monitoring::OnError(int errorCode, WCHAR* errorComment)
{
}

void NetClient_Monitoring::OnSendJob()
{
	// 3. NetServer의 IOCP워커 쓰레드가 queue에 있는 구조체 꺼내서 모니터렁서버로 sendPacket();

	stSendJob job;
	bool ret = sendJobQ.Dequeue(&job);
	if (ret == false)
		return;

	Monitoring::GetInstance()->DecreaseInterlocked(MonitorType::SendJobQ);

	SerializePacketPtr newPacket = SerializePacketPtr::MakeSerializePacket();
	newPacket.Clear();

	// TODO: 수정예정
	WORD type = en_PACKET_SS_MONITOR_DATA_UPDATE;
	
	newPacket << type;
	newPacket << job.dataType;
	newPacket << job.dataValue;
	newPacket << job.timeStamp;

	SendPacket(newPacket);
}

void NetClient_Monitoring::EnqueueSendJob(stSendJob sendJob)
{
	// TODO: 
	// 1. queue로 구조체 집어넣기. 
	// 2. 쓰레드가 PQCS호출.(?)
	// 3. NetServer의 IOCP워커 쓰레드가 queue에 있는 구조체 꺼내서 MonitoringServer로 sendPacket();

	if (connected == false)
		return;

	// 1.
	sendJobQ.Enqueue(sendJob);

	Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::SendJobQ);

	// 2.
	PQCSSendJob();
}
