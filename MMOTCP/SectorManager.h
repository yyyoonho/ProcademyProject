#pragma once

struct stCharacter;
struct stSession;

// 섹터 하나당 크기 200 pixel * 200 pixel
#define dfSECTOR_MAX_Y 32
#define dfSECTOR_MAX_X 32

#define dfSECTOR_SIZE (6400/dfSECTOR_MAX_Y)

struct stSECTOR_POS
{
	int iY;
	int iX;
};

struct stSECTOR_AROUND
{
	int iCount;
	stSECTOR_POS around[9];
};