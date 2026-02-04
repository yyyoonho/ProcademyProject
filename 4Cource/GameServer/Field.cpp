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


void Field::Disconnect(DWORD64 sessionID)
{
	pGameManager->PQCS_Disconnect(sessionID);
}

void Field::RegistGameManager(GameManager* pGM)
{
	pGameManager = pGM;
}