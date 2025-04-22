#include <Windows.h>
#include <list>
#include <vector>
#include <queue>

#include "SectorManager.h"
#include "CharacterManager.h"

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
		else
			bOverlaped = false;
	}
	pAddSector->iCount = count;

	count = 0;
	for (int i = 0; i < tmpOld.iCount; i++)
	{
		bool bOverlaped = false;

		for (int j = 0; j < tmpCur.iCount; j++)
		{
			if (tmpCur.around[i].iY == tmpOld.around[j].iY && tmpCur.around[i].iX == tmpOld.around[j].iX)
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
		else
			bOverlaped = false;
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

void UpdateSector(stCharacter* pCharacter)
{
	stSECTOR_POS curSectorPos = pCharacter->curSector;
	short characterY = pCharacter->shY;
	short characterX = pCharacter->shX;

	int newSectorY = (int)characterY / dfSECTOR_SIZE;
	int newSectorX = (int)characterX / dfSECTOR_SIZE;

	if (newSectorY == curSectorPos.iY && newSectorX == curSectorPos.iX)
		return;

	list<stCharacter*>::iterator iter;
	for (iter = g_Sector[curSectorPos.iY][curSectorPos.iX].begin(); iter != g_Sector[curSectorPos.iY][curSectorPos.iX].end(); ++iter)
	{
		if ((*iter) != pCharacter)
			continue;

		g_Sector[curSectorPos.iY][curSectorPos.iX].erase(iter);
		break;
	}

	g_Sector[newSectorY][newSectorX].push_front(pCharacter);

	pCharacter->oldSector = curSectorPos;
	pCharacter->curSector.iY = newSectorY;
	pCharacter->curSector.iX = newSectorX;

	return;
}
