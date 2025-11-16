#pragma once

#define NONESECTOR 65535

enum class PLAYER_STATE
{
	ACCEPT,
	LOGGED_IN,
	PLAY,
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

	PLAYER_STATE state;
};


void CreatePlayer(DWORD sessionID);
bool RemovePlayerFromPlayerMap(INT64 accountNo);
bool RemovePlayerFromPlayerMap(DWORD64 sessionID);
bool RemovePlayerFromWaitMap(DWORD sessionID);

bool SetSector(INT64 accountNo, WORD newSectorY, WORD newSectorX, OUT WORD* oldSectorY, OUT WORD* oldSectorX);
bool GetSector(INT64 accountNo, OUT WORD* sectorY, OUT WORD* sectorX);

bool GetSessionID(INT64 accountNo, OUT DWORD* sessionID);

bool IsLoggedIn(INT64 accountNo);
bool LogInPlayer(DWORD sessionID, INT64 accountNo, WCHAR* id, int idLen, WCHAR* nickName, int nickNameLen, char* sessionKey, int sessionKeyLen);

void UpdateHeartbeat(DWORD sessionID);

bool GetPlayer(INT64 accountNo, OUT stPlayer** player);

bool GetPlayerState(INT64 accountNo, OUT PLAYER_STATE* state);
bool SetPlayerState(INT64 accountNo, PLAYER_STATE state);

void DisconnectNoLoginPlayer(OUT std::vector<DWORD64>& sessionIds);
void DisconnectNoHeartbeatPlayer(OUT std::vector<DWORD64>& sessionIds);
