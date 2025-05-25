#include "stdafx.h"

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
	int sectorY = pCharacter->shY / dfSECTOR_SIZE;
	int sectorX = pCharacter->shX / dfSECTOR_SIZE;

	pCharacter->curSector.iY = sectorY;
	pCharacter->curSector.iX = sectorX;

	g_Sector[sectorY][sectorX].push_back(pCharacter);
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
