#pragma once

struct stSession;

struct stCharacter
{
	stSession* pSession;
	DWORD dwSessionID;

	DWORD dwAction;
	BYTE byDirection;
	BYTE byMoveDirection;

	short shX;
	short shY;

	double dX;
	double dY;

	stSECTOR_POS curSector;
	stSECTOR_POS oldSector;

	char chHP;
};

extern std::unordered_map<DWORD, stCharacter* > characterMap;

void CreateCharacter(stSession* pSession, DWORD dwSessionID);

void DestroyCharacter(DWORD sessionId);