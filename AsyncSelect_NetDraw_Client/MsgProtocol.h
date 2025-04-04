#pragma once

struct stHeader
{
	unsigned short len;
};

struct stDrawPacket
{
	int startX;
	int startY;
	int endX;
	int endY;
};