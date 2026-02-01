#include "stdafx.h"

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
	SendPacketJob* pSendPacketJob = sendPacketJobPool.Alloc();

	pSendPacketJob->sessionID = sessionID;
	pSendPacketJob->packet = sPacket;

	// 로드밸런스
	static int idx = 0;
	idx = (idx + 10) % 3;

	pGameManager->sendPacketJobQ[idx]->Enqueue((char*)&pSendPacketJob, sizeof(SendPacketJob*));

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
