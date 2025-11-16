#include "stdafx.h"
#include "SectorManager.h"

using namespace std;

#define NONESECTOR 65535
#define MAX_SECTORY 32
#define MAX_SECTORX 32

int dy[9] = { -1,-1,-1,0,0,0,1,1,1 };
int dx[9] = { -1,0,1,-1,0,1,-1,0,1 };

list<INT64> playersInSector[30][30];

bool MoveSector(INT64 accountNo, WORD newSectorY, WORD newSectorX, WORD oldSectorY, WORD oldSectorX)
{
	bool flag = false;
	
	if (oldSectorY == NONESECTOR || oldSectorX == NONESECTOR)
	{
		playersInSector[newSectorY][newSectorX].push_back(accountNo);
		return true;
	}

	list<INT64>::iterator iter = playersInSector[oldSectorY][oldSectorX].begin();
	for ( ; iter != playersInSector[oldSectorY][oldSectorX].end(); iter++)
	{
		if (*iter != accountNo)
			continue;

		playersInSector[oldSectorY][oldSectorX].erase(iter);
		flag = true;
		break;
	}

	if (flag == true)
	{
		playersInSector[newSectorY][newSectorX].push_back(accountNo);
		return true;
	}
	else
	{
		return false;
	}
}

void GetAdjacentSectorPlayers(WORD sectorY, WORD sectorX, std::vector<INT64>& arr)
{
	for (int i = 0; i < 9; i++)
	{
		int nextY = (int)sectorY + dy[i];
		int nextX = (int)sectorX + dx[i];

		if (nextY < 0 || nextY >= MAX_SECTORY || nextX < 0 || nextX >= MAX_SECTORX)
			continue;

		list<INT64>::iterator iter = playersInSector[nextY][nextX].begin();
		for (; iter != playersInSector[nextY][nextX].end(); ++iter)
		{
			arr.push_back(*iter);
		}

	}
}

void CreatePlayerToSector(INT64 accountNo, WORD sectorY, WORD sectorX)
{
	playersInSector[sectorY][sectorX].push_back(accountNo);
}

bool RemovePlayerFromSector(INT64 accountNo, WORD sectorY, WORD sectorX)
{
	bool flag = false;

	list<INT64>::iterator iter = playersInSector[sectorY][sectorX].begin();
	for (; iter != playersInSector[sectorY][sectorX].end(); ++iter)
	{
		if (*iter != accountNo)
			continue;

		playersInSector[sectorY][sectorX].erase(iter);
		flag = true;
	}


	return flag;
}
