#include <Windows.h>
#include <iostream>	
#include <vector>
#include <unordered_map>

#include "Protocol.h"
#include "SerializeBuffer.h"
#include "Network.h"
#include "SectorManager.h"
#include "CharacterManager.h"
#include "LogManager.h"
#include "MakePacket.h"
#include "SendPacket.h"

#include "PacketProc.h"

using namespace std;

bool netPacketProc_MoveStart(stSession* pSession, SerializePacket* sPacket);
bool netPacketProc_MoveStop(stSession* pSession, SerializePacket* sPacket);
bool netPacketProc_Attack1(stSession* pSession, SerializePacket* sPacket);
bool netPacketProc_Attack2(stSession* pSession, SerializePacket* sPacket);
bool netPacketProc_Attack3(stSession* pSession, SerializePacket* sPacket);
bool netPacketProc_Echo(stSession* pSession, SerializePacket* sPacket);
void CharacterSectorUpdatePacket(stCharacter* pCharacter);

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
		SendPacket_Around(pSession, sPacket);

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
		// TODO: 클라들에게 패킷전송
		CharacterSectorUpdatePacket(pCharacter);
	}

	sPacket->Clear();
	mpMoveStart(sPacket, pSession->dwSessionID, byDirection, shX, shY);
	SendPacket_Around(pSession, sPacket, false);

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
	pCharacter->byMoveDirection = dfMOVE_STOP;

	if (UpdateSector(pCharacter))
	{
		// TODO: 클라들에게 패킷전송
		CharacterSectorUpdatePacket(pCharacter);
	}
	
	sPacket->Clear();
	mpMoveStop(sPacket, pCharacter->dwSessionID, pCharacter->byDirection, pCharacter->shX, pCharacter->shY);
	SendPacket_Around(pCharacter->pSession, sPacket, false);

	return true;
}

bool netPacketProc_Attack1(stSession* pSession, SerializePacket* sPacket)
{
	return false;
}

bool netPacketProc_Attack2(stSession* pSession, SerializePacket* sPacket)
{
	return false;
}

bool netPacketProc_Attack3(stSession* pSession, SerializePacket* sPacket)
{
	return false;
}

bool netPacketProc_Echo(stSession* pSession, SerializePacket* sPacket)
{
	return false;
}

void CharacterSectorUpdatePacket(stCharacter* pCharacter)
{
	stSECTOR_AROUND removeSectors;
	stSECTOR_AROUND addSectors;
	GetUpdateSectorAround(pCharacter, &removeSectors, &addSectors);

	SerializePacket sPacket;
	// remove에 delete보내기.
	{
		mpDeleteCharacter(&sPacket, pCharacter->dwSessionID);
		for (int i = 0; i < removeSectors.iCount; i++)
		{
			vector<stSession*> v;
			GetSessionsFromSector(removeSectors.around[i].iY, removeSectors.around[i].iX, v);

			for (int j = 0; j < v.size(); j++)
			{
				SendPacket_Unicast(v[j], &sPacket);
			}
		}
	}
	sPacket.Clear();

	// add에 new(섹터에 진입한) 존재 알리기.
	{
		mpCreateOtherCharacter(&sPacket, pCharacter->dwSessionID, pCharacter->byDirection, pCharacter->shX, pCharacter->shY, pCharacter->chHP);
		for (int i = 0; i < addSectors.iCount; i++)
		{
			vector<stSession*> v;
			GetSessionsFromSector(addSectors.around[i].iY, addSectors.around[i].iX, v);

			for (int j = 0; j < v.size(); j++)
			{
				SendPacket_Unicast(v[j], &sPacket);
			}
		}
	}
	sPacket.Clear();

	// new에 add에 있던 플레이어들 존재 알리기.
	{
		for (int i = 0; i < addSectors.iCount; i++)
		{
			vector<stCharacter*> v;
			GetCharactersFromSector(addSectors.around[i].iY, addSectors.around[i].iX, v);

			for (int j = 0; j < v.size(); j++)
			{
				mpCreateOtherCharacter(&sPacket, v[j]->dwSessionID, v[j]->byDirection, v[j]->shX, v[j]->shY, v[j]->chHP);
				SendPacket_Unicast(pCharacter->pSession, &sPacket);
				sPacket.Clear();
			}
		}
	}
	
	// new에 add에 있던 플레이어의 행동 알리기.
	{
		for (int i = 0; i < addSectors.iCount; i++)
		{
			vector<stCharacter*> v;
			GetCharactersFromSector(addSectors.around[i].iY, addSectors.around[i].iX, v);

			for (int j = 0; j < v.size(); j++)
			{
				if (v[j]->byMoveDirection == dfMOVE_STOP)
					continue;

				mpMoveStart(&sPacket, v[j]->dwSessionID, v[j]->byMoveDirection, v[j]->shX, v[j]->shY);
				SendPacket_Unicast(pCharacter->pSession, &sPacket);
				sPacket.Clear();
			}
		}
	}
}