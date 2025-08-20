#pragma once

struct stHeader
{
	unsigned short len;
};

struct stMessage
{
	__int64 data;
};

struct stMessageForMsgQ
{
	DWORD64 sessionId;
	stMessage msg;
};