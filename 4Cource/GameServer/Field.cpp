#include "stdafx.h"

#include "Monitoring.h"
#include "NetServer.h"
#include "GameManager.h"
#include "Field.h"

Field::Field()
{
}

Field::~Field()
{
}

void Field::MoveToOtherField(FieldName fieldName, DWORD64 sessionID, void* pPlayer)
{
	FieldMovePack movePack;
	movePack.fieldName = fieldName;
	movePack.sessionID = sessionID;
	movePack.pPlayer = pPlayer;
	
	movePackageVec.push_back(movePack);
}

void Field::SendPacket(DWORD64 sessionID, SerializePacketPtr sPacket)
{
	// send담당 쓰레드에게 일감 넘기기.
	RawPtr r;
	sPacket.GetRawPtr(&r);
	r.IncreaseRefCount();

	SendPacketJob tmpJob;
	tmpJob.sid = sessionID;
	tmpJob.r = r;


	// 로드밸런스
	static int idx = 0;
	idx = (idx + 1) % 5;

	switch (idx)
	{
	case 0:
		Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::SendPacketQ_0);
		break;
	case 1:
		Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::SendPacketQ_1);
		break;
	case 2:
		Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::SendPacketQ_2);
		break;
	case 3:
		Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::SendPacketQ_3);
		break;
	case 4:
		Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::SendPacketQ_4);
		break;
	}

	Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::TotalSendPacketCount);

	pGameManager->sendPacketQ[idx]->Enqueue((char*)&tmpJob, sizeof(SendPacketJob));

	return;
}

void Field::Disconnect(DWORD64 sessionID)
{
	pGameManager->PQCS_Disconnect(sessionID);
}

void Field::RegistGameManager(GameManager* pGM)
{
	pGameManager = pGM;
}
