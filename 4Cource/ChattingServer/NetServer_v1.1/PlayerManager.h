#pragma once

enum class PLYAER_STATE
{
	ACCEPT,
	LOGGED_IN,
};

struct stPlayer
{
	INT64 accountNo;
	DWORD sessionID;

	WCHAR ID[20];				// null 포함
	WCHAR nickname[20];			// null 포함
	char SessionKey[64];

	WORD sectorY;
	WORD sectorX;

	DWORD heartbeat;

	PLYAER_STATE state;
};


void CreatePlayer(DWORD sessionID);
bool RemovePlayer(INT64 accountNo);

bool SetSector(INT64 accountNo, WORD newSectorY, WORD newSectorX, OUT WORD* oldSectorY, OUT WORD* oldSectorX);
bool GetSector(INT64 accountNo, OUT WORD* sectorY, OUT WORD* sectorX);

bool GetSessionID(INT64 accountNo, OUT DWORD* sessionID);

bool IsLoggedIn(INT64 accountNo);
void LogInPlayer(DWORD sessionID, INT64 accountNo, WCHAR* id, int idLen, WCHAR* nickName, int nickNameLen, char* sessionKey, int sessionKeyLen);

void UpdateHeartbeat(DWORD sessionID);

bool GetPlayer(INT64 accountNo, OUT stPlayer* player);