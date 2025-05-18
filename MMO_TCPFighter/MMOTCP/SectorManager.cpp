#include "pch.h"

#include "Protocol.h"
#include "SectorManager.h"
#include "CharacterManager.h"
#include "PacketProc.h"
#include "MakePacket.h"
#include "SendPacket.h"


using namespace std;

list<stCharacter*> g_Sector[dfSECTOR_MAX_Y][dfSECTOR_MAX_X];

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

void GetUpdateSectorAround(stCharacter* pCharacter, OUT stSECTOR_AROUND* pRemoveSector, OUT stSECTOR_AROUND* pAddSector)
{
	stSECTOR_AROUND tmpCur;
	stSECTOR_AROUND tmpOld;

	GetSectorAround(pCharacter->curSector.iY, pCharacter->curSector.iX, &tmpCur);
	GetSectorAround(pCharacter->oldSector.iY, pCharacter->oldSector.iX, &tmpOld);

	int count = 0;
	for (int i = 0; i < tmpCur.iCount; i++)
	{
		bool bOverlaped = false;

		for (int j = 0; j < tmpOld.iCount; j++)
		{
			if (tmpCur.around[i].iY == tmpOld.around[j].iY && tmpCur.around[i].iX == tmpOld.around[j].iX)
			{
				bOverlaped = true;
				break;
			}
		}

		if (bOverlaped == false)
		{
			pAddSector->around[count].iY = tmpCur.around[i].iY;
			pAddSector->around[count].iX = tmpCur.around[i].iX;
			count++;
		}
	}
	pAddSector->iCount = count;

	count = 0;
	for (int i = 0; i < tmpOld.iCount; i++)
	{
		bool bOverlaped = false;

		for (int j = 0; j < tmpCur.iCount; j++)
		{
			if (tmpCur.around[j].iY == tmpOld.around[i].iY && tmpCur.around[j].iX == tmpOld.around[i].iX)
			{
				bOverlaped = true;
				break;
			}
		}

		if (bOverlaped == false)
		{
			pRemoveSector->around[count].iY = tmpOld.around[i].iY;
			pRemoveSector->around[count].iX = tmpOld.around[i].iX;
			count++;
		}
	}
	pRemoveSector->iCount = count;

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
	stSECTOR_POS curSectorPos = pCharacter->curSector;
	short characterY = pCharacter->shY;
	short characterX = pCharacter->shX;

	int newSectorY = (int)characterY / dfSECTOR_SIZE;
	int newSectorX = (int)characterX / dfSECTOR_SIZE;

	g_Sector[newSectorY][newSectorX].push_front(pCharacter);
	
	pCharacter->curSector.iY = newSectorY;
	pCharacter->curSector.iX = newSectorX;
	pCharacter->oldSector.iY = newSectorY;
	pCharacter->oldSector.iX = newSectorX;

	return;
}

bool UpdateSector(stCharacter* pCharacter)
{
	stSECTOR_POS curSectorPos = pCharacter->curSector;
	short characterY = pCharacter->shY;
	short characterX = pCharacter->shX;

	int newSectorY = (int)characterY / dfSECTOR_SIZE;
	int newSectorX = (int)characterX / dfSECTOR_SIZE;

	if (newSectorY == curSectorPos.iY && newSectorX == curSectorPos.iX)
	{
		return false;
	}

	list<stCharacter*>::iterator iter;
	for (iter = g_Sector[curSectorPos.iY][curSectorPos.iX].begin(); iter != g_Sector[curSectorPos.iY][curSectorPos.iX].end(); ++iter)
	{
		if ((*iter) != pCharacter)
			continue;

		g_Sector[curSectorPos.iY][curSectorPos.iX].erase(iter);
		break;
	}

	g_Sector[newSectorY][newSectorX].push_front(pCharacter);

	pCharacter->oldSector.iY = curSectorPos.iY;
	pCharacter->oldSector.iX = curSectorPos.iX;

	pCharacter->curSector.iY = newSectorY;
	pCharacter->curSector.iX = newSectorX;

	return true;
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
			SendPacket_SectorOne(removeSectors.around[i].iY, removeSectors.around[i].iX, &sPacket, pCharacter->pSession);
		}
	}
	sPacket.Clear();

	// new에 remove할 캐릭터 알리기.
	{
		for (int i = 0; i < removeSectors.iCount; i++)
		{
			vector<stCharacter*> v;
			GetCharactersFromSector(removeSectors.around[i].iY, removeSectors.around[i].iX, v);

			for (int j = 0; j < v.size(); j++)
			{
				mpDeleteCharacter(&sPacket, v[j]->dwSessionID);
				SendPacket_Unicast(pCharacter->pSession, &sPacket);
				sPacket.Clear();
			}
		}
	}
	sPacket.Clear();

	// add에 new(섹터에 진입한) 존재 알리기.
	{
		mpCreateOtherCharacter(&sPacket, pCharacter->dwSessionID, pCharacter->byDirection, pCharacter->shX, pCharacter->shY, pCharacter->chHP);

		for (int i = 0; i < addSectors.iCount; i++)
		{
			SendPacket_SectorOne(addSectors.around[i].iY, addSectors.around[i].iX, &sPacket, pCharacter->pSession);
			printf("3\n");
		}
	}
	sPacket.Clear();

	// add에 new(섹터에 진입한) 액션 알리기.
	{
		if (pCharacter->dwAction != dfMOVE_STOP)
		{
			mpMoveStart(&sPacket, pCharacter->dwSessionID, pCharacter->byMoveDirection, pCharacter->shX, pCharacter->shY);

			for (int i = 0; i < addSectors.iCount; i++)
			{
				SendPacket_SectorOne(addSectors.around[i].iY, addSectors.around[i].iX, &sPacket, pCharacter->pSession);
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
				printf("4\n");
				SendPacket_Unicast(pCharacter->pSession, &sPacket);
				sPacket.Clear();
			}
		}
	}

	// new에 add에 있던 플레이어들 액션 알리기.
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

void DeleteCharacter(stCharacter* tmpCharacter)
{
	int nowSectorY = tmpCharacter->curSector.iY;
	int nowSectorX = tmpCharacter->curSector.iX;

	list<stCharacter*>::iterator iter;
	for (iter = g_Sector[nowSectorY][nowSectorX].begin(); iter != g_Sector[nowSectorY][nowSectorX].end(); ++iter)
	{
		if ((*iter) == tmpCharacter)
		{
			g_Sector[nowSectorY][nowSectorX].erase(iter);

			return;
		}
	}

	return;
}