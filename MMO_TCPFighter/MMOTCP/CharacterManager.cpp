#include <Windows.h>
#include <iostream>
#include <list>
#include <unordered_map>
#include <queue>

#include "MemoryPool.h"
#include "Protocol.h"
#include "Network.h"
#include "SectorManager.h"

#include "CharacterManager.h"

using namespace std;

procademy::MemoryPool<stCharacter> characterMP(0, false);
unordered_map<DWORD, stCharacter* > characterMap;
queue<stCharacter*> watingQueue;

void CreateCharacter(stSession* pSession, DWORD dwSessionID)
{
	stCharacter* newCharacter = characterMP.Alloc();

	newCharacter->pSession = pSession;
	newCharacter->dwSessionID = dwSessionID;

	newCharacter->dwAction = dfACTION_STOP;
	newCharacter->byDirection = dfRANGE_MOVE_LEFT;
	newCharacter->byMoveDirection = dfRANGE_MOVE_LEFT;

	newCharacter->shX = rand() % (dfRANGE_MOVE_RIGHT - 1);
	newCharacter->shY = rand() % (dfRANGE_MOVE_BOTTOM - 1);
	
	newCharacter->dX = (double)newCharacter->shX;
	newCharacter->dY = (double)newCharacter->shY;
	
	newCharacter->curSector.iY = (int)newCharacter->shY / dfSECTOR_SIZE;
	newCharacter->curSector.iX = (int)newCharacter->shX / dfSECTOR_SIZE;
	newCharacter->oldSector.iY = (int)newCharacter->shY / dfSECTOR_SIZE;
	newCharacter->oldSector.iX = (int)newCharacter->shX / dfSECTOR_SIZE;

	newCharacter->chHP = 100;

	watingQueue.push(newCharacter);
}

void DestroyCharacter()
{

}
