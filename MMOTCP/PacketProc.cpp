#include "stdafx.h"

#include "Protocol.h"
#include "Network.h"
#include "SectorManager.h"
#include "CharacterManager.h"
#include "LogManager.h"
#include "MakePacket.h"
#include "SendPacket.h"
#include "SectorManager.h"

#include "PacketProc.h"

using namespace std;

bool netPacketProc_MoveStart(stSession* pSession, SerializePacket* sPacket);
bool netPacketProc_MoveStop(stSession* pSession, SerializePacket* sPacket);
bool netPacketProc_Attack1(stSession* pSession, SerializePacket* sPacket);
bool netPacketProc_Attack2(stSession* pSession, SerializePacket* sPacket);
bool netPacketProc_Attack3(stSession* pSession, SerializePacket* sPacket);
bool netPacketProc_Echo(stSession* pSession, SerializePacket* sPacket);
//void CharacterSectorUpdatePacket(stCharacter* pCharacter);

bool PacketProc(stSession* pSession, BYTE byPacketType, SerializePacket* sPacket)
{
	switch (byPacketType)
	{
	case dfPACKET_CS_MOVE_START:
		return netPacketProc_MoveStart(pSession, sPacket);
		break;

	case dfPACKET_CS_MOVE_STOP:
		return netPacketProc_MoveStop(pSession, sPacket);
		break;

	case dfPACKET_CS_ATTACK1:
		return netPacketProc_Attack1(pSession, sPacket);
		break;

	case dfPACKET_CS_ATTACK2:
		return netPacketProc_Attack2(pSession, sPacket);
		break;

	case dfPACKET_CS_ATTACK3:
		return netPacketProc_Attack3(pSession, sPacket);
		break;

	case dfPACKET_CS_ECHO:
		return netPacketProc_Echo(pSession, sPacket);
		break;
	}

	return true;
}

bool netPacketProc_MoveStart(stSession* pSession, SerializePacket* sPacket)
{
	BYTE byDirection;
	short shX;
	short shY;

	(*sPacket) >> byDirection;
	(*sPacket) >> shX;
	(*sPacket) >> shY;

	_LOG(dfLOG_LEVEL_DEBUG, L"# MOVESTART # SessionID:%d / Direction:%d / X:%d / Y:%d\n",
		pSession->dwSessionID, byDirection, shX, shY);

	stCharacter* pCharacter = FindCharacter(pSession->dwSessionID);
	if (pCharacter == NULL)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"# MOVESTART > SessionID:%d Character Not Found!\n", pSession->dwSessionID);

		return false;
	}

	int diffX = abs(shX - pCharacter->dX);
	int diffY = abs(shY - pCharacter->dY);
	if (diffX > dfERROR_RANGE || diffY > dfERROR_RANGE)
	{
		mpSync(sPacket, pCharacter->dwSessionID, pCharacter->shX, pCharacter->shY);
		SendPacket_Around(pSession, sPacket, true);

		shX = pCharacter->shX;
		shY = pCharacter->shY;
	}

	pCharacter->dwAction = byDirection;
	pCharacter->byMoveDirection = byDirection;

	switch (byDirection)
	{
	case dfPACKET_MOVE_DIR_RR:
	case dfPACKET_MOVE_DIR_RU:
	case dfPACKET_MOVE_DIR_RD:
		pCharacter->byDirection = dfPACKET_MOVE_DIR_RR;
		break;

	case dfPACKET_MOVE_DIR_LL:
	case dfPACKET_MOVE_DIR_LU:
	case dfPACKET_MOVE_DIR_LD:
		pCharacter->byDirection = dfPACKET_MOVE_DIR_LL;
		break;
	}
	pCharacter->shX = shX;
	pCharacter->shY = shY;


	if (UpdateSector(pCharacter))
	{
		int a = 3;
		// TODO: 클라들에게 패킷전송
		printf("2\n");
		CharacterSectorUpdatePacket(pCharacter);
	}

	sPacket->Clear();
	mpMoveStart(sPacket, pSession->dwSessionID, byDirection, shX, shY);
	SendPacket_Around(pSession, sPacket, false);

	pSession->dwLastRecvTime = GetTickCount();

	return true;
}

bool netPacketProc_MoveStop(stSession* pSession, SerializePacket* sPacket)
{
	
	BYTE byDirection;
	short shX;
	short shY;

	(*sPacket) >> byDirection;
	(*sPacket) >> shX;
	(*sPacket) >> shY;

	_LOG(dfLOG_LEVEL_DEBUG, L"# MOVESTOP # SessionID:%d / Direction:%d / X:%d / Y:%d\n",
		pSession->dwSessionID, byDirection, shX, shY);

	stCharacter* pCharacter = FindCharacter(pSession->dwSessionID);
	if (pCharacter == NULL)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"# MOVESTOP > SessionID:%d Character Not Found!\n", pSession->dwSessionID);

		return false;
	}

	int diffX = abs(shX - pCharacter->dX);
	int diffY = abs(shY - pCharacter->dY);
	if (diffX > dfERROR_RANGE || diffY > dfERROR_RANGE)
	{
		mpSync(sPacket, pCharacter->dwSessionID, pCharacter->shX, pCharacter->shY);
		SendPacket_Around(pSession, sPacket);

		shX = pCharacter->shX;
		shY = pCharacter->shY;
	}

	pCharacter->shX = shX;
	pCharacter->shY = shY;
	pCharacter->byDirection = byDirection;

	pCharacter->dwAction = dfMOVE_STOP;
	pCharacter->byMoveDirection = dfMOVE_STOP;

	if (UpdateSector(pCharacter))
	{
		// TODO: 클라들에게 패킷전송
		printf("3\n");
		CharacterSectorUpdatePacket(pCharacter);
	}

	sPacket->Clear();
	mpMoveStop(sPacket, pCharacter->dwSessionID, pCharacter->byDirection, pCharacter->shX, pCharacter->shY);
	SendPacket_Around(pCharacter->pSession, sPacket, false);

	pSession->dwLastRecvTime = GetTickCount();

	
	return true;
}

bool netPacketProc_Attack1(stSession* pSession, SerializePacket* sPacket)
{

	pSession->dwLastRecvTime = GetTickCount();
	return false;
}

bool netPacketProc_Attack2(stSession* pSession, SerializePacket* sPacket)
{

	pSession->dwLastRecvTime = GetTickCount();
	return false;
}

bool netPacketProc_Attack3(stSession* pSession, SerializePacket* sPacket)
{

	pSession->dwLastRecvTime = GetTickCount();
	return false;
}

bool netPacketProc_Echo(stSession* pSession, SerializePacket* sPacket)
{
	DWORD t;
	(*sPacket) >> t;
	sPacket->Clear();

	mpEcho(sPacket, t);

	SendPacket_Unicast(pSession, sPacket);
	
	return false;
}
