#pragma once

struct Player
{
	DWORD64 sessionID;
	SOCKADDR_IN _addr;

	INT64 accountNo;

	WCHAR ID[20];
	WCHAR NickName[20];

	std::mutex playerLock;
	DWORD64 heartbeat;


	// RateLimit
	DWORD rateLimitTick;
	DWORD rateLimitMsgCount;
	DWORD rateLimitOutCount;
};