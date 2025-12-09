#pragma once

union stNetHeader
{
	struct
	{
		BYTE code;
		BYTE len[2];
		BYTE randomKey;
		BYTE checkSum;
	};

	BYTE raw[5]; // 전체를 한 번에 접근하기 위한 배열
};

struct stMessage
{
	__int64 msg;
};

enum class MSG_CATEGORY
{
	ACCEPT,
	MESSAGE,
	DISCONNECT,

	NONE,
};

#define MAX_SECTOR_Y 50
#define MAX_SECTOR_X 50
#define NONE_SECTOR 65535

#define MAX_MSGLEN 255

enum class PLAYER_STATE
{
	ACCEPT,
	LOGIN,
	PLAY,
	DISCONNECTING,

	INVALID,
};

