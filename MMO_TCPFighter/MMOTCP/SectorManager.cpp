#include <Windows.h>
#include <list>


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
		int newSectorY = iSectorY + dy[i];
		int newSectorX = iSectorX + dx[i];

		if (newSectorY < 0 || newSectorY >= dfSECTOR_MAX_Y || newSectorX < 0 || newSectorX <= dfSECTOR_MAX_X)
			continue;

		pSectorAround->around[count].iY = newSectorY;
		pSectorAround->around[count].iX = newSectorX;
		count++;
	}

	pSectorAround->iCount = count;
}

void GetUpdateSectorAround()
{

}
