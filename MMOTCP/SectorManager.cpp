#include "stdafx.h"

#include "SectorManager.h"
#include "CharacterManager.h"
#include "MakePacket.h"
#include "SendPacket.h"
#include "LogManager.h"

using namespace std;

list<stCharacter*> g_Sector[dfSECTOR_MAX_Y + 1][dfSECTOR_MAX_X + 1];

int dy[9] = { 0,0,-1,-1,-1,0, 1,1,1 };
int dx[9] = { 0,-1,-1,0,1,1, 1,0,-1 };

void GetSectorAround(int iSectorY, int iSectorX, OUT stSECTOR_AROUND* pSectorAround)
{
	int count = 0;

	for (int i = 0; i < 9; i++)
	{
		int aroundSectorY = iSectorY + dy[i];
		int aroundSectorX = iSectorX + dx[i];

		if (aroundSectorY < 0 || aroundSectorY >= dfSECTOR_MAX_Y || aroundSectorX < 0 || aroundSectorX >= dfSECTOR_MAX_X)
			continue;

		pSectorAround->around[count].iY = aroundSectorY;
		pSectorAround->around[count].iX = aroundSectorX;
		count++;
	}

	pSectorAround->iCount = count;
}

void GetSessionsFromSector(int sectorY, int sectorX, OUT std::vector<stSession*>& v)
{
	list<stCharacter*>::iterator iter;
	for (iter = g_Sector[sectorY][sectorX].begin(); iter != g_Sector[sectorY][sectorX].end(); ++iter)
	{
		v.push_back((*iter)->pSession);
	}

	return;
}

void GetCharactersFromSector(int sectorY, int sectorX, OUT std::vector<stCharacter*>& v)
{
	list<stCharacter*>::iterator iter;
	for (iter = g_Sector[sectorY][sectorX].begin(); iter != g_Sector[sectorY][sectorX].end(); ++iter)
	{
		v.push_back((*iter));
	}

	return;
}

void SetSector(stCharacter* pCharacter)
{
	_LOG(dfLOG_LEVEL_DEBUG, L"# SetSector # SessionID:%d\n", pCharacter->dwSessionID);

	int sectorY = pCharacter->shY / dfSECTOR_SIZE;
	int sectorX = pCharacter->shX / dfSECTOR_SIZE;

	pCharacter->curSector.iY = sectorY;
	pCharacter->curSector.iX = sectorX;

	g_Sector[sectorY][sectorX].push_back(pCharacter);
}

bool DeleteInSector(int sectorY, int sectorX, stCharacter* destroyCharacter)
{
	_LOG(dfLOG_LEVEL_DEBUG, L"# DeleteInSector # SessionID:%d\n", destroyCharacter->dwSessionID);

	list<stCharacter*>::iterator iter;
	for (iter = g_Sector[sectorY][sectorX].begin(); iter != g_Sector[sectorY][sectorX].end(); ++iter)
	{
		if ((*iter) != destroyCharacter)
			continue;

		g_Sector[sectorY][sectorX].erase(iter);
		return true;
	}

	return false;
}

bool UpdateSector(stCharacter* pCharacter)
{
	int sectorY = pCharacter->shY / dfSECTOR_SIZE;
	int sectorX = pCharacter->shX / dfSECTOR_SIZE;

	if (sectorY == pCharacter->curSector.iY && sectorX == pCharacter->curSector.iX)
		return false;

	list<stCharacter*>::iterator iter;
	for (iter = g_Sector[pCharacter->curSector.iY][pCharacter->curSector.iX].begin(); iter != g_Sector[pCharacter->curSector.iY][pCharacter->curSector.iX].end(); ++iter)
	{
		if ((*iter) != pCharacter)
			continue;

		g_Sector[pCharacter->curSector.iY][pCharacter->curSector.iX].erase(iter);
		break;
	}

	pCharacter->oldSector.iY = pCharacter->curSector.iY;
	pCharacter->oldSector.iX = pCharacter->curSector.iX;

	pCharacter->curSector.iY = sectorY;
	pCharacter->curSector.iX = sectorX;

	g_Sector[sectorY][sectorX].push_front(pCharacter);

	

	return true;
}

void CharacterSectorUpdatePacket(stCharacter* pCharacter)
{
	stSECTOR_AROUND removeSectors;
	stSECTOR_AROUND addSectors;
	GetUpdateSectorAround(pCharacter, &removeSectors, &addSectors);

	// 나에게 RemoveSector에 위치한 캐릭터들의 삭제정보를 보낸다.
	{
		for (int i = 0; i < removeSectors.iCount; i++)
		{
			int sectorY = removeSectors.around[i].iY;
			int sectorX = removeSectors.around[i].iX;

			list<stCharacter*>::iterator iter;
			for (iter = g_Sector[sectorY][sectorX].begin(); iter != g_Sector[sectorY][sectorX].end(); ++iter)
			{
				SerializePacket sPacket;
				mpDeleteCharacter(&sPacket, (*iter)->dwSessionID);
				SendPacket_Unicast(pCharacter->pSession, &sPacket);
			}
		}
	}

	// RemoveSector에 위치한 클라이언트들에게 나의 삭제정보를 보낸다.
	{
		SerializePacket sPacket;
		mpDeleteCharacter(&sPacket, pCharacter->dwSessionID);

		for (int i = 0; i < removeSectors.iCount; i++)
		{
			int sectorY = removeSectors.around[i].iY;
			int sectorX = removeSectors.around[i].iX;

			SendPacket_SectorOne(sectorY, sectorX, &sPacket, NULL);
		}
	}

	// AddSector에 있는 캐릭터들 얻기
	vector<stCharacter*> v;
	for (int i = 0; i < addSectors.iCount; i++)
	{
		GetCharactersFromSector(addSectors.around[i].iY, addSectors.around[i].iX, v);
	}

	// 나에게 AddSector에 위치한 클라이언트들의 생성정보를 보낸다.
	{
		for (int j = 0; j < v.size(); j++)
		{
			SerializePacket sPacket;
			mpCreateOtherCharacter(&sPacket, v[j]->dwSessionID, v[j]->byDirection, v[j]->shX, v[j]->shY, v[j]->chHP);

			SendPacket_Unicast(pCharacter->pSession, &sPacket);
		}
	}

	// 나에게 AddSector에 위치한 클라이언트들의 액션정보를 보낸다.
	{
		for (int j = 0; j < v.size(); j++)
		{
			if (v[j]->byMoveDirection == dfMOVE_STOP)
				continue;

			SerializePacket sPacket;
			mpMoveStart(&sPacket, v[j]->dwSessionID, v[j]->byMoveDirection, v[j]->shX, v[j]->shY);

			SendPacket_Unicast(pCharacter->pSession, &sPacket);
		}
	}

	// AddSector에 위치한 클라이언트들에게 나의 생성정보 보낸다.
	{
		SerializePacket sPacket;
		mpCreateOtherCharacter(&sPacket, pCharacter->dwSessionID, pCharacter->byDirection, pCharacter->shX, pCharacter->shY,pCharacter->chHP);
		for (int i = 0; i < addSectors.iCount; i++)
		{
			SendPacket_SectorOne(addSectors.around[i].iY, addSectors.around[i].iX, &sPacket, pCharacter->pSession);
		}
	}

	// AddSector에 위치한 클라이언트들에게 나의 액션정보 보낸다.
	{
		if (pCharacter->byMoveDirection != dfMOVE_STOP)
		{
			SerializePacket sPacket;
			mpMoveStart(&sPacket, pCharacter->dwSessionID, pCharacter->byMoveDirection, pCharacter->shX, pCharacter->shY);

			for (int i = 0; i < addSectors.iCount; i++)
			{
				SendPacket_SectorOne(addSectors.around[i].iY, addSectors.around[i].iX, &sPacket, pCharacter->pSession);
			}
		}
	}
 }

void GetUpdateSectorAround(stCharacter* pCharacter, OUT stSECTOR_AROUND* pRemoveSector, OUT stSECTOR_AROUND* pAddSector)
{
	stSECTOR_POS curSector = pCharacter->curSector;
	stSECTOR_POS oldSector = pCharacter->oldSector;

	stSECTOR_AROUND curSectorAround;
	GetSectorAround(curSector.iY, curSector.iX, &curSectorAround);

	stSECTOR_AROUND oldSectorAround;
	GetSectorAround(oldSector.iY, oldSector.iX, &oldSectorAround);

	// addSectors 구하기
	int count = 0;
	for (int i = 0; i < curSectorAround.iCount; i++)
	{
		bool bOverlaped = false;

		for (int j = 0; j < oldSectorAround.iCount; j++)
		{
			if (curSectorAround.around[i].iY == oldSectorAround.around[j].iY &&
				curSectorAround.around[i].iX == oldSectorAround.around[j].iX)
			{
				bOverlaped = true;
				break;
			}
		}

		if(bOverlaped == false)
		{
			pAddSector->around[count].iY = curSectorAround.around[i].iY;
			pAddSector->around[count].iX = curSectorAround.around[i].iX;
			count++;
		}
	}
	pAddSector->iCount = count;

	// removeSectors 구하기
	count = 0;
	for (int i = 0; i < oldSectorAround.iCount; i++)
	{
		bool bOverlaped = false;

		for (int j = 0; j < curSectorAround.iCount; j++)
		{
			if (oldSectorAround.around[i].iY == curSectorAround.around[j].iY &&
				oldSectorAround.around[i].iX == curSectorAround.around[j].iX)
			{
				bOverlaped = true;
				break;
			}
		}

		if (bOverlaped == false)
		{
			pRemoveSector->around[count].iY = oldSectorAround.around[i].iY;
			pRemoveSector->around[count].iX = oldSectorAround.around[i].iX;
			count++;
		}
	}
	pRemoveSector->iCount = count;



	//TODO:Debug Log print
	/*printf("#CurSectorPos  (y,x) = (%d,%d)\n", curSector.iY, curSector.iX);

	printf("#OldSectorPos  (y,x) = (%d,%d)\n", oldSector.iY, oldSector.iX);
	
	for (int i = 0; i < pAddSector->iCount; i++)
	{
		printf("#addSectors  (y,x) = (%d,%d)\n", pAddSector->around[i].iY, pAddSector->around[i].iX);
	}
	
	for (int i = 0; i < pRemoveSector->iCount; i++)
	{
		printf("#removeSectors  (y,x) = (%d,%d)\n", pRemoveSector->around[i].iY, pRemoveSector->around[i].iX);
	}*/

	return;
}
