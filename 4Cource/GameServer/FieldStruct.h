#pragma once

struct Player
{
	DWORD64 sessionID;
	INT64 accountNo;
	char sessionKey[64];

	DWORD64 heartbeat;
	PLAYER_STATE state;

	// RateLimit
	DWORD rateLimitTick;
	DWORD rateLimitMsgCount;
	DWORD rateLimitOutCount;
};