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

void CreateCharacter(stSession* pSession, DWORD dwSessionID, OUT stCharacter** ppNewCharacter);

void DestroyCharacter(DWORD sessionId);
void PushCharacterToMap();

void GetCurSectorPos(stSession* pSession, OUT stSECTOR_POS* pSectorPos );

stCharacter* FindCharacter(BYTE sessionID);