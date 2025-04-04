#include "BufferRender.h"
#include "Console.h"
#include <Windows.h>
#include <iostream>
#include "Data.h"

char szScreenBuffer[MAX_Y + 1][MAX_X + 2];

void Buffer_Flip(void)
{
	int iCnt;
	for (iCnt = 0; iCnt < MAX_Y + 1; iCnt++)
	{
		cs_MoveCursor(0, iCnt);
		printf(szScreenBuffer[iCnt]);
	}
}

void Buffer_Clear(void)
{
	int iCnt;
	memset(szScreenBuffer, ' ', (MAX_Y + 1) * (MAX_X + 2));

	for (iCnt = 0; iCnt < MAX_Y +1; iCnt++)
	{
		szScreenBuffer[iCnt][MAX_X + 1] = '\0';
	}
}

void Sprite_Draw(int iX, int iY, char chSprite)
{
	if (iX < 0 || iY < 0 || iX >= MAX_X + 1 || iY > MAX_Y + 1)
		return;

	szScreenBuffer[iY][iX] = chSprite;
}
