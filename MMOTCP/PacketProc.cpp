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

	int diffX = abs(shX - pCharacter->shX);
	int diffY = abs(shY - pCharacter->shY);
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
		_LOG(dfLOG_LEVEL_ERROR, L"# MOVESTOP # SessionID:%d Character Not Found!\n", pSession->dwSessionID);

		return false;
	}

	int diffX = abs(shX - pCharacter->shX);
	int diffY = abs(shY - pCharacter->shY);
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
	BYTE direction;
	short shX;
	short shY;

	*sPacket >> direction;
	*sPacket >> shX;
	*sPacket >> shY;


	_LOG(dfLOG_LEVEL_DEBUG, L"# ATTACK1 # SessionID:%d / Direction:%d / X:%d / Y:%d\n",
		pSession->dwSessionID, direction, shX, shY);

	DWORD sessionID = pSession->dwSessionID;
	stCharacter* pCharacter = FindCharacter(sessionID);
	if (pCharacter == NULL)
	{
		printf("ERROR: 캐릭터를 찾지 못했습니다.\n");
		_LOG(dfLOG_LEVEL_ERROR, L"# ATTACK1 # SessionID:%d Character Not Found!\n", pSession->dwSessionID);

		return false;
	}

	int diffX = abs(shX - pCharacter->shX);
	int diffY = abs(shY - pCharacter->shY);
	if (diffX > dfERROR_RANGE || diffY > dfERROR_RANGE)
	{
		mpSync(sPacket, pCharacter->dwSessionID, pCharacter->shX, pCharacter->shY);
		SendPacket_Around(pSession, sPacket);

		shX = pCharacter->shX;
		shY = pCharacter->shY;
	}

	pCharacter->byDirection = direction;
	pCharacter->shX = shX;
	pCharacter->shY = shY;

	// 캐릭터 공격 모션 패킷 전송
	{
		SerializePacket attack1Packet;
		mpAttack1(&attack1Packet, pSession->dwSessionID, direction, shX, shY);
		SendPacket_Around(pSession, &attack1Packet, false);
	}

	if (UpdateSector(pCharacter))
	{
		CharacterSectorUpdatePacket(pCharacter);
	}

	// 공격 판정
	int attackRangeMinX;
	int attackRangeMaxX;
	int attackRangeMinY = max(dfRANGE_MOVE_TOP, shY - dfATTACK1_RANGE_Y);
	int attackRangeMaxY = min(dfRANGE_MOVE_BOTTOM, shY + dfATTACK1_RANGE_Y);

	if (direction == dfPACKET_MOVE_DIR_LL)
	{
		attackRangeMinX = max(dfRANGE_MOVE_LEFT, shX - dfATTACK1_RANGE_X);
		attackRangeMaxX = shX;
	}
	else
	{
		attackRangeMinX = shX;
		attackRangeMaxX = min(dfRANGE_MOVE_RIGHT, shX + dfATTACK1_RANGE_X);
	}
	
	stSECTOR_AROUND sectorAround;
	GetSectorAround(pCharacter->curSector.iY, pCharacter->curSector.iX, &sectorAround);
	vector<stCharacter*> v;
	for (int i = 0; i < sectorAround.iCount; i++)
	{
		GetCharactersFromSector(sectorAround.around[i].iY, sectorAround.around[i].iX, v);
	}

	for (int i = 0; i < v.size(); i++)
	{
		if (v[i] == pCharacter)
			continue;

		short enemyY = v[i]->shY;
		short enemyX = v[i]->shX;

		if (enemyX < attackRangeMinX || enemyX > attackRangeMaxX)
			continue;
		if (enemyY < attackRangeMinY || enemyY > attackRangeMaxY)
			continue;

		v[i]->chHP -= dfATTACK1_DAMAGE;

		SerializePacket damagePacket;
		mpDamage(&damagePacket, sessionID, v[i]->dwSessionID, v[i]->chHP);

		SendPacket_Around(v[i]->pSession, &damagePacket, true);

		if (v[i]->chHP <= 0)
		{
			SerializePacket deadPacket;
			mpDeleteCharacter(&deadPacket, v[i]->dwSessionID);

			SendPacket_Around(v[i]->pSession, &deadPacket, false);

			PushQuitQ(v[i]->pSession);
		}

	}

	pSession->dwLastRecvTime = GetTickCount();
	return true;
}

bool netPacketProc_Attack2(stSession* pSession, SerializePacket* sPacket)
{
	BYTE direction;
	short shX;
	short shY;

	*sPacket >> direction;
	*sPacket >> shX;
	*sPacket >> shY;

	_LOG(dfLOG_LEVEL_DEBUG, L"# ATTACK2 # SessionID:%d / Direction:%d / X:%d / Y:%d\n",
		pSession->dwSessionID, direction, shX, shY);

	DWORD sessionID = pSession->dwSessionID;
	stCharacter* pCharacter = FindCharacter(sessionID);
	if (pCharacter == NULL)
	{
		printf("ERROR: 캐릭터를 찾지 못했습니다.\n");
		_LOG(dfLOG_LEVEL_ERROR, L"# ATTACK2 # SessionID:%d Character Not Found!\n", pSession->dwSessionID);

		return false;
	}

	int diffX = abs(shX - pCharacter->shX);
	int diffY = abs(shY - pCharacter->shY);
	if (diffX > dfERROR_RANGE || diffY > dfERROR_RANGE)
	{
		mpSync(sPacket, pCharacter->dwSessionID, pCharacter->shX, pCharacter->shY);
		SendPacket_Around(pSession, sPacket);

		shX = pCharacter->shX;
		shY = pCharacter->shY;
	}

	pCharacter->byDirection = direction;
	pCharacter->shX = shX;
	pCharacter->shY = shY;

	// 캐릭터 공격 모션 패킷 전송
	{
		SerializePacket attack2Packet;
		mpAttack2(&attack2Packet, pSession->dwSessionID, direction, shX, shY);
		SendPacket_Around(pSession, &attack2Packet, false);
	}

	if (UpdateSector(pCharacter))
	{
		CharacterSectorUpdatePacket(pCharacter);
	}

	// 공격 판정
	int attackRangeMinX;
	int attackRangeMaxX;
	int attackRangeMinY = max(dfRANGE_MOVE_TOP, shY - dfATTACK2_RANGE_Y);
	int attackRangeMaxY = min(dfRANGE_MOVE_BOTTOM, shY + dfATTACK2_RANGE_Y);

	if (direction == dfPACKET_MOVE_DIR_LL)
	{
		attackRangeMinX = max(dfRANGE_MOVE_LEFT, shX - dfATTACK2_RANGE_X);
		attackRangeMaxX = shX;
	}
	else
	{
		attackRangeMinX = shX;
		attackRangeMaxX = min(dfRANGE_MOVE_RIGHT, shX + dfATTACK2_RANGE_X);
	}

	stSECTOR_AROUND sectorAround;
	GetSectorAround(pCharacter->curSector.iY, pCharacter->curSector.iX, &sectorAround);
	vector<stCharacter*> v;
	for (int i = 0; i < sectorAround.iCount; i++)
	{
		GetCharactersFromSector(sectorAround.around[i].iY, sectorAround.around[i].iX, v);
	}

	for (int i = 0; i < v.size(); i++)
	{
		if (v[i] == pCharacter)
			continue;

		short enemyY = v[i]->shY;
		short enemyX = v[i]->shX;

		if (enemyX < attackRangeMinX || enemyX > attackRangeMaxX)
			continue;
		if (enemyY < attackRangeMinY || enemyY > attackRangeMaxY)
			continue;

		v[i]->chHP -= dfATTACK2_DAMAGE;

		SerializePacket damagePacket;
		mpDamage(&damagePacket, sessionID, v[i]->dwSessionID, v[i]->chHP);

		SendPacket_Around(v[i]->pSession, &damagePacket, true);

		if (v[i]->chHP <= 0)
		{
			SerializePacket deadPacket;
			mpDeleteCharacter(&deadPacket, v[i]->dwSessionID);

			SendPacket_Around(v[i]->pSession, &deadPacket, false);

			PushQuitQ(v[i]->pSession);
		}

	}

	pSession->dwLastRecvTime = GetTickCount();
	return false;
}

bool netPacketProc_Attack3(stSession* pSession, SerializePacket* sPacket)
{
	BYTE direction;
	short shX;
	short shY;

	*sPacket >> direction;
	*sPacket >> shX;
	*sPacket >> shY;

	_LOG(dfLOG_LEVEL_DEBUG, L"# ATTACK3 # SessionID:%d / Direction:%d / X:%d / Y:%d\n",
		pSession->dwSessionID, direction, shX, shY);

	DWORD sessionID = pSession->dwSessionID;
	stCharacter* pCharacter = FindCharacter(sessionID);
	if (pCharacter == NULL)
	{
		printf("ERROR: 캐릭터를 찾지 못했습니다.\n");
		_LOG(dfLOG_LEVEL_ERROR, L"# ATTACK3 # SessionID:%d Character Not Found!\n", pSession->dwSessionID);

		return false;
	}

	int diffX = abs(shX - pCharacter->shX);
	int diffY = abs(shY - pCharacter->shY);
	if (diffX > dfERROR_RANGE || diffY > dfERROR_RANGE)
	{
		mpSync(sPacket, pCharacter->dwSessionID, pCharacter->shX, pCharacter->shY);
		SendPacket_Around(pSession, sPacket);

		shX = pCharacter->shX;
		shY = pCharacter->shY;
	}

	pCharacter->byDirection = direction;
	pCharacter->shX = shX;
	pCharacter->shY = shY;

	// 캐릭터 공격 모션 패킷 전송
	{
		SerializePacket attack3Packet;
		mpAttack3(&attack3Packet, pSession->dwSessionID, direction, shX, shY);
		SendPacket_Around(pSession, &attack3Packet, false);
	}

	if (UpdateSector(pCharacter))
	{
		CharacterSectorUpdatePacket(pCharacter);
	}

	// 공격 판정
	int attackRangeMinX;
	int attackRangeMaxX;
	int attackRangeMinY = max(dfRANGE_MOVE_TOP, shY - dfATTACK3_RANGE_Y);
	int attackRangeMaxY = min(dfRANGE_MOVE_BOTTOM, shY + dfATTACK3_RANGE_Y);

	if (direction == dfPACKET_MOVE_DIR_LL)
	{
		attackRangeMinX = max(dfRANGE_MOVE_LEFT, shX - dfATTACK3_RANGE_X);
		attackRangeMaxX = shX;
	}
	else
	{
		attackRangeMinX = shX;
		attackRangeMaxX = min(dfRANGE_MOVE_RIGHT, shX + dfATTACK3_RANGE_X);
	}

	stSECTOR_AROUND sectorAround;
	GetSectorAround(pCharacter->curSector.iY, pCharacter->curSector.iX, &sectorAround);
	vector<stCharacter*> v;
	for (int i = 0; i < sectorAround.iCount; i++)
	{
		GetCharactersFromSector(sectorAround.around[i].iY, sectorAround.around[i].iX, v);
	}

	for (int i = 0; i < v.size(); i++)
	{
		if (v[i] == pCharacter)
			continue;

		short enemyY = v[i]->shY;
		short enemyX = v[i]->shX;

		if (enemyX < attackRangeMinX || enemyX > attackRangeMaxX)
			continue;
		if (enemyY < attackRangeMinY || enemyY > attackRangeMaxY)
			continue;

		v[i]->chHP -= dfATTACK3_DAMAGE;

		SerializePacket damagePacket;
		mpDamage(&damagePacket, sessionID, v[i]->dwSessionID, v[i]->chHP);

		SendPacket_Around(v[i]->pSession, &damagePacket, true);

		if (v[i]->chHP <= 0)
		{
			SerializePacket deadPacket;
			mpDeleteCharacter(&deadPacket, v[i]->dwSessionID);

			SendPacket_Around(v[i]->pSession, &deadPacket, false);

			PushQuitQ(v[i]->pSession);
		}

	}

	pSession->dwLastRecvTime = GetTickCount();
	return false;
}

bool netPacketProc_Echo(stSession* pSession, SerializePacket* sPacket)
{
	DWORD t;
	(*sPacket) >> t;
	sPacket->Clear();

	_LOG(dfLOG_LEVEL_DEBUG, L"# ECHO # SessionID:%d\n",pSession->dwSessionID);

	mpEcho(sPacket, t);

	SendPacket_Unicast(pSession, sPacket);

	pSession->dwLastRecvTime = GetTickCount();
	
	return false;
}
