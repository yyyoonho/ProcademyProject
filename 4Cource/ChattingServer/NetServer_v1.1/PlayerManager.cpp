#include "stdafx.h"
#include "PlayerManager.h"

using namespace std;

#define NONESECTOR 65535

unordered_map<DWORD, stPlayer*> waitPlayerMap;	// Key = sessionID

unordered_map<INT64, stPlayer*> playerMap;		// Key = accountNo
unordered_map<DWORD64, INT64> sessionToAccountMap;

procademy::MemoryPool_TLS<stPlayer> playerMemoryPool(0, false);

void CreatePlayer(DWORD sessionID)
{
	stPlayer* newPlayer = playerMemoryPool.Alloc();
	newPlayer->sessionID = sessionID;
	
	newPlayer->accountNo = -1;
	newPlayer->ID[0] = NULL;
	newPlayer->nickname[0] = NULL;

	newPlayer->sectorY = NONESECTOR;
	newPlayer->sectorY = NONESECTOR;

	newPlayer->state = PLYAER_STATE::ACCEPT;

	waitPlayerMap.insert({ sessionID, newPlayer });

	return;
}

bool RemovePlayer(INT64 accountNo)
{
	unordered_map<INT64, stPlayer*>::iterator iter = playerMap.find(accountNo);
	if (iter == playerMap.end())
	{
		return false;
	}

	stPlayer* targetPlayer = iter->second;
	playerMap.erase(accountNo);

	playerMemoryPool.Free(targetPlayer);
}

bool SetSector(INT64 accountNo, WORD newSectorY, WORD newSectorX, OUT WORD* oldSectorY, OUT WORD* oldSectorX)
{
	unordered_map<INT64, stPlayer*>::iterator iter = playerMap.find(accountNo);
	if (iter == playerMap.end())
	{
		*oldSectorY = NULL;
		*oldSectorX = NULL;
		return false;
	}


	stPlayer* targetPlayer = iter->second;

	*oldSectorY = targetPlayer->sectorY;
	*oldSectorX = targetPlayer->sectorX;

	targetPlayer->sectorY = newSectorY;
	targetPlayer->sectorX = newSectorX;

	return true;
}

bool GetSector(INT64 accountNo, WORD* sectorY, WORD* sectorX)
{
	unordered_map<INT64, stPlayer*>::iterator iter = playerMap.find(accountNo);
	if (iter == playerMap.end())
	{
		return false;
	}

	stPlayer* targetPlayer = iter->second;

	*sectorY = targetPlayer->sectorY;
	*sectorX = targetPlayer->sectorX;

	return true;
}

bool GetSessionID(INT64 accountNo, OUT DWORD* sessionID)
{
	unordered_map<INT64, stPlayer*>::iterator iter = playerMap.find(accountNo);
	if (iter == playerMap.end())
	{
		sessionID = NULL;
		return false;
	}

	*sessionID = iter->second->sessionID;
	return true;
}
bool IsLoggedIn(INT64 accountNo)
{
	unordered_map<INT64, stPlayer*>::iterator iter = playerMap.find(accountNo);
	if (iter == playerMap.end())
	{
		return false;
	}

	return true;
}

void LogInPlayer(DWORD sessionID, INT64 accountNo, WCHAR* id, int idLen, WCHAR* nickName, int nickNameLen, char* sessionKey, int sessionKeyLen)
{
	unordered_map<DWORD, stPlayer*>::iterator iter = waitPlayerMap.find(sessionID);
	if (iter == waitPlayerMap.end())
	{
		return;
	}

	stPlayer* newPlayer = iter->second;
	waitPlayerMap.erase(iter);

	// TODO: 플레이어 세팅
	newPlayer->accountNo = accountNo;
	newPlayer->sessionID = sessionID;

	wcscpy_s(newPlayer->ID, id);
	wcscpy_s(newPlayer->nickname, nickName);
	strcpy_s(newPlayer->SessionKey, sessionKey);

	newPlayer->heartbeat = timeGetTime();

	newPlayer->state = PLYAER_STATE::LOGGED_IN;

	sessionToAccountMap.insert({ sessionID, accountNo });
}

void UpdateHeartbeat(DWORD sessionID)
{
	unordered_map<DWORD64, INT64>::iterator iter = sessionToAccountMap.find(sessionID);
	if (iter == sessionToAccountMap.end())
		return;

	INT64 accountNo = iter->second;

	unordered_map<INT64, stPlayer*>::iterator iter2 = playerMap.find(accountNo);
	if (iter2 == playerMap.end())
		return;

	stPlayer* targetPlayer = iter2->second;

	targetPlayer->heartbeat = timeGetTime();
}

bool GetPlayer(INT64 accountNo, stPlayer* player)
{
	unordered_map<INT64, stPlayer*>::iterator iter = playerMap.find(accountNo);
	if (iter == playerMap.end())
	{
		return false;
	}

	*player = *(iter->second);

	return true;
}
