#include <Windows.h>
#include <iostream>
#include <list>
#include <vector>
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
queue<stCharacter*> waitingQ;

void CreateCharacter(stSession* pSession, DWORD dwSessionID, OUT stCharacter** ppNewCharacter)
{
	stCharacter* newCharacter = characterMP.Alloc();

	newCharacter->pSession = pSession;
	newCharacter->dwSessionID = dwSessionID;

	newCharacter->dwAction = dfMOVE_STOP;
	newCharacter->byDirection = dfRANGE_MOVE_LEFT;
	newCharacter->byMoveDirection = dfRANGE_MOVE_LEFT;

	//newCharacter->shX = rand() % (dfRANGE_MOVE_RIGHT - 1);
	//newCharacter->shY = rand() % (dfRANGE_MOVE_BOTTOM - 1);
	
	// TODO: 테스트코드
	static short shX = 20;
	static short shY = 20;
	newCharacter->shX = shX;
	newCharacter->shY = shY;
	shX += 20;
	shY += 20;

	newCharacter->dX = (double)newCharacter->shX;
	newCharacter->dY = (double)newCharacter->shY;
	
	newCharacter->curSector.iY = -1;
	newCharacter->curSector.iX = -1;
	newCharacter->oldSector.iY = -1;
	newCharacter->oldSector.iX = -1;

	newCharacter->chHP = 100;

	waitingQ.push(newCharacter);

	*ppNewCharacter = newCharacter;

	return;
}

void DestroyCharacter(DWORD sessionId)
{
	stCharacter* tmpCharacter = characterMap.find(sessionId)->second;
	characterMap.erase(sessionId);

	characterMP.Free(tmpCharacter);
}

void PushCharacterToMap()
{
	while (!waitingQ.empty())
	{
		characterMap.insert({ waitingQ.front()->dwSessionID, waitingQ.front() });
		SetSector(waitingQ.front());
		waitingQ.pop();
	}
}

void GetCurSectorPos(stSession* pSession, OUT stSECTOR_POS* pSectorPos)
{
	stCharacter* tmpCharacter = NULL;
	unordered_map<DWORD, stCharacter* >::iterator iter = characterMap.find(pSession->dwSessionID);
	if (iter == characterMap.end())
	{
		int a = 3;
	}

	tmpCharacter = (*iter).second;

	pSectorPos->iY = tmpCharacter->curSector.iY; // tmpCharacter 가 NULLPTR 에러발생
	pSectorPos->iX = tmpCharacter->curSector.iX;
	return;
}

stCharacter* FindCharacter(BYTE sessionID)
{
	unordered_map<DWORD, stCharacter* >::iterator iter = characterMap.find(sessionID);
	if (iter == characterMap.end())
	{
		return NULL;
	}

	return (*iter).second;
}
