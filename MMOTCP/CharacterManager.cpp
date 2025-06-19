#include "stdafx.h"

#include "MMOTCP.h"
#include "Network.h"
#include "SectorManager.h"
#include "LogManager.h"
#include "MakePacket.h"
#include "SendPacket.h"

#include "CharacterManager.h"

using namespace std;

procademy::MemoryPool<stCharacter> characterMP(0, false);
unordered_map<DWORD, stCharacter* > characterMap;

bool EnterWorld(stCharacter* pNewCharacter);


void CreateCharacter(stSession* pSession, DWORD dwSessionID)
{
	_LOG(dfLOG_LEVEL_DEBUG, L"# CreateCharacter # SessionID:%d\n", dwSessionID);

	stCharacter* newCharacter = characterMP.Alloc();

	newCharacter->pSession = pSession;
	newCharacter->dwSessionID = dwSessionID;

	newCharacter->dwAction = dfMOVE_STOP;
	newCharacter->byDirection = dfRANGE_MOVE_LEFT;
	newCharacter->byMoveDirection = dfMOVE_STOP;

	// TODO: 테스트코드
	/*static short shX = 20;
	static short shY = 20;*/

	short shX = rand() % dfRANGE_MOVE_RIGHT;
	short shY = rand() % dfRANGE_MOVE_BOTTOM;

	newCharacter->shX = shX;
	newCharacter->shY = shY;
	/*shX += 20;
	shY += 20;*/

	newCharacter->dX = (double)newCharacter->shX;
	newCharacter->dY = (double)newCharacter->shY;

	SetSector(newCharacter);
	newCharacter->oldSector.iY = -1;
	newCharacter->oldSector.iX = -1;

	newCharacter->chHP = 100;

	characterMap.insert({ newCharacter->dwSessionID, newCharacter });

	EnterWorld(newCharacter);

	return;
}

bool EnterWorld(stCharacter* pNewCharacter)
{
	_LOG(dfLOG_LEVEL_DEBUG, L"# EnterWorld # SessionID:%d\n", pNewCharacter->dwSessionID);

	stSession* pNewSession = pNewCharacter->pSession;

	// new에게 자신의 생성 메시지 보내기.
	{
		_LOG(dfLOG_LEVEL_DEBUG, L"# new에게 자신의 생성 메시지 보내기 # SessionID:%d\n", pNewCharacter->dwSessionID);

		SerializePacket sPacket;

		mpCreateMyCharacter(&sPacket, pNewCharacter->dwSessionID, pNewCharacter->byDirection, pNewCharacter->shX, pNewCharacter->shY);
		SendPacket_Unicast(pNewSession, &sPacket);
	}

	// new에게 기존 멤버들 생성 메시지 보내기.
	{
		_LOG(dfLOG_LEVEL_DEBUG, L"# new에게 기존 멤버들 생성 메시지 보내기 # SessionID:%d\n", pNewCharacter->dwSessionID);

		stSECTOR_AROUND sectorAround;
		GetSectorAround(pNewCharacter->curSector.iY, pNewCharacter->curSector.iX, &sectorAround);

		vector<stCharacter*> v;
		for (int i = 0; i < sectorAround.iCount; i++)
		{
			GetCharactersFromSector(sectorAround.around[i].iY, sectorAround.around[i].iX, v);

			for (int j = 0; j < v.size(); j++)
			{
				if (v[j] == pNewCharacter)
					continue;

				SerializePacket sPacket;
				mpCreateOtherCharacter(&sPacket, v[j]->dwSessionID, v[j]->byDirection, v[j]->shX, v[j]->shY, v[j]->chHP);
				SendPacket_Unicast(pNewSession, &sPacket);
			}

			v.clear();
		}

		//printf("New <- mpCreateOtherCharacter Send\n");
	}

	// new에게 기존 멤버들 액션 메시지 보내기.
	{
		_LOG(dfLOG_LEVEL_DEBUG, L"# new에게 기존 멤버들 액션 메시지 보내기 # SessionID:%d\n", pNewCharacter->dwSessionID);

		stSECTOR_AROUND sectorAround;
		GetSectorAround(pNewCharacter->curSector.iY, pNewCharacter->curSector.iX, &sectorAround);

		vector<stCharacter*> v;
		for (int i = 0; i < sectorAround.iCount; i++)
		{
			GetCharactersFromSector(sectorAround.around[i].iY, sectorAround.around[i].iX, v);

			for (int j = 0; j < v.size(); j++)
			{
				if (v[j]->dwAction == dfMOVE_STOP)
					continue;

				SerializePacket sPacket;
				mpMoveStart(&sPacket, v[j]->dwSessionID, v[j]->byMoveDirection, v[j]->shX, v[j]->shY);
				SendPacket_Unicast(pNewSession, &sPacket);
			}

			v.clear();
		}

		//printf("New <- mpMoveStart Send\n");
	}

	// 기존 멤버들에게 new 생성 메시지 보내기.
	{
		_LOG(dfLOG_LEVEL_DEBUG, L"# 기존 멤버들에게 new 생성 메시지 보내기 # SessionID:%d\n", pNewCharacter->dwSessionID);

		SerializePacket sPacket;
		mpCreateOtherCharacter(&sPacket, pNewSession->dwSessionID, pNewCharacter->byDirection, pNewCharacter->shX, pNewCharacter->shY, pNewCharacter->chHP);

		SendPacket_Around(pNewSession, &sPacket, false);
		//printf("Other <- mpCreateOtherCharacter Send\n");
	}

	return true;
}

void DestroyCharacter(DWORD sessionId)
{
	_LOG(dfLOG_LEVEL_DEBUG, L"# DestroyCharacter # SessionID:%d\n", sessionId);

	stCharacter* destroyCharacter = characterMap.find(sessionId)->second;
	if (destroyCharacter == NULL)
	{
		DebugBreak();
		return;
	}

	DeleteInSector(destroyCharacter->curSector.iY, destroyCharacter->curSector.iX, destroyCharacter);

	characterMap.erase(sessionId);	
	_LOG(dfLOG_LEVEL_ERROR, L"# EraseCharacter # SessionID:%d\n", sessionId);

	bool ret = characterMP.Free(destroyCharacter);
	if (!ret)
	{
		_LOG(dfLOG_LEVEL_ERROR, L"Error: characterMP.Free\n");
	}
}

void GetCurSector(stSession* pSession, OUT stSECTOR_POS* pSectorPos)
{
	stCharacter* tmpCharacter = NULL;
	unordered_map<DWORD, stCharacter* >::iterator iter = characterMap.find(pSession->dwSessionID);
	if (iter == characterMap.end())
	{
		_LOG(dfLOG_LEVEL_DEBUG, L"ERROR: GetCurSector() 캐릭터가 없습니다.\n");

		g_bShutdown = true;
		return;
	}

	tmpCharacter = (*iter).second;
	if (tmpCharacter == NULL)
	{
		_LOG(dfLOG_LEVEL_DEBUG, L"ERROR: GetCurSector() tmpCharacter == NULL\n");

		g_bShutdown = true;
		return;
	}

	pSectorPos->iY = tmpCharacter->curSector.iY;
	pSectorPos->iX = tmpCharacter->curSector.iX;
	return;
}

stCharacter* FindCharacter(DWORD sessionID)
{
	unordered_map<DWORD, stCharacter* >::iterator iter = characterMap.find(sessionID);
	if (iter == characterMap.end())
	{
		return NULL;
	}

	return (*iter).second;
}

int GetCharacterSize()
{
	return characterMap.size();
}
