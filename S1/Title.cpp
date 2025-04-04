#include <Windows.h>
#include <string.h>
#include <iostream>
#include "Data.h"

#include"S1.h"
#include "Title.h"
#include "Console.h"
#include "Load.h"
#include "BufferRender.h"

using namespace std;

int title_fileCount;
char title_fileName[100][64];

int titleCursorLocationY;
int titleCursorLocationX;

void TitleRender();
void TitleInput(OUT bool* enter, OUT int* selectStageIdx);
void TitleLogic(bool enter, int selectStageIdx);

void TitleUpdate()
{
	bool enter = false;
	static int selectStageIdx = 0;

	TitleInput(&enter, &selectStageIdx);

	TitleLogic(enter, selectStageIdx);

	if (RenderSkipCheck())
		return;

	TitleRender();
}


void TitleRender()
{
	RenderFPS++;

	Buffer_Clear();

	int strPosY = 0;

	const char* commentStr[] = { "\"Choose the stage you want to play",
											"and press ENTER button.\"" };

	int strSize = sizeof(commentStr) / sizeof(char*);

	for (int i = 0; i < strSize; i++)
	{
		int len = strlen(commentStr[i]);
		
		for (int j = 0; j < len; j++)
		{
			Sprite_Draw(j, strPosY, commentStr[i][j]);
		}

		strPosY++;
	}

	Sprite_Draw(titleCursorLocationX, titleCursorLocationY + strSize, '>');

	for (int i = 0; i < title_fileCount; i++)
	{
		int nameLen = strlen(&title_fileName[i][0]);

		const char* tmpStr = ".txt";
		int tmpStrLen = strlen(tmpStr);

		for (int j = 0; j < nameLen- tmpStrLen; j++)
		{
			Sprite_Draw(j + 1, strPosY, title_fileName[i][j]);
		}

		strPosY++;
	}

	ShowFPS();

	Buffer_Flip();
}

void TitleInput(OUT bool* enter, OUT int* selectStageIdx)
{
	if (GetAsyncKeyState(VK_UP))
	{
		if (titleCursorLocationY - 1 < 0)
			return;

		titleCursorLocationY = titleCursorLocationY - 1;
		*selectStageIdx = *selectStageIdx - 1;
	}
	if (GetAsyncKeyState(VK_DOWN))
	{
		if (titleCursorLocationY + 1 < 0 || titleCursorLocationY + 1 >= title_fileCount)
			return;

		titleCursorLocationY = titleCursorLocationY + 1;
		*selectStageIdx = *selectStageIdx + 1;
	}
	if (GetAsyncKeyState(VK_RETURN) & 0x8000)
	{
		*enter = true;
		return;
	}
}

void TitleLogic(bool enter, int selectStageIdx)
{
	logicFPS++;

	if (enter == false) return;

	PassStageName(&title_fileName[selectStageIdx][0]);

	ChangeScene(SCENE::GAME);
}
