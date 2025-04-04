#include <Windows.h>
#include "Data.h"

#include "S1.h"
#include "Game.h"
#include "GameOver.h"
#include "BufferRender.h"
#include "Load.h"


#define GAMEOVERSTRWIDTH 20
#define GAMEOVERSTRHEIGHT 20

void GameOverInput();
void GameOverLogic();
void GameOverRender();

void LoseRender();
void WinRender();

int clearTime;

void GameOverUpdate()
{
	GameOverInput();
	GameOverLogic();

	if (RenderSkipCheck())
		return;

	GameOverRender();
}

void SetEndTime(int time)
{
	clearTime = time/1000;
}

void GameOverInput()
{
	if (GetAsyncKeyState(VK_SPACE) & 0x8000)
	{
		ChangeScene(SCENE::TITLE);
	}
}

void GameOverLogic()
{
	logicFPS++;
}

void GameOverRender()
{
	RenderFPS++;

	Buffer_Clear();

	if (stageClear == true)
	{
		WinRender();
	}
	else
		LoseRender();

	ShowFPS();

	Buffer_Flip();
}

void LoseRender()
{
	const char* gameOverStr = "G A M E O V E R ...";
	const char* PressBtnStr = "Press Spacebar button";

	int len = strlen(gameOverStr);

	for (int i = 0; i < len; i++)
	{
		Sprite_Draw(GAMEOVERSTRWIDTH + i, GAMEOVERSTRHEIGHT, gameOverStr[i]);
	}

	len = strlen(PressBtnStr);
	for (int i = 0; i < len; i++)
	{
		Sprite_Draw(GAMEOVERSTRWIDTH + i, GAMEOVERSTRHEIGHT + 3, PressBtnStr[i]);
	}
}

void WinRender()
{
	const char* gameOverStr = "STAGE CLEAR!";
	const char* clearTimeStr = "Clear time : ";
	const char* PressBtnStr = "Press Spacebar button";

	int len = strlen(gameOverStr);
	for (int i = 0; i < len; i++)
	{
		Sprite_Draw(GAMEOVERSTRWIDTH + i, GAMEOVERSTRHEIGHT, gameOverStr[i]);
	}

	len = strlen(clearTimeStr);
	int j = 0;
	for (; j < len; j++)
	{
		Sprite_Draw(GAMEOVERSTRWIDTH + j, GAMEOVERSTRHEIGHT+1, clearTimeStr[j]);
	}

	int min = clearTime / 60;
	int sec = clearTime % 60;

	Sprite_Draw(GAMEOVERSTRWIDTH + j, GAMEOVERSTRHEIGHT + 1, (min / 10 + '0'));
	j++;
	Sprite_Draw(GAMEOVERSTRWIDTH + j, GAMEOVERSTRHEIGHT + 1, (char)(min % 10 + '0'));
	j++;
	Sprite_Draw(GAMEOVERSTRWIDTH + j, GAMEOVERSTRHEIGHT + 1, ':');
	j++;
	Sprite_Draw(GAMEOVERSTRWIDTH + j, GAMEOVERSTRHEIGHT + 1, (char)(sec / 10 + '0'));
	j++;
	Sprite_Draw(GAMEOVERSTRWIDTH + j, GAMEOVERSTRHEIGHT + 1, (char)(sec % 10 + '0'));

	int a = 30;

	len = strlen(PressBtnStr);
	for (int i = 0; i < len; i++)
	{
		Sprite_Draw(GAMEOVERSTRWIDTH + i, GAMEOVERSTRHEIGHT + 3, PressBtnStr[i]);
	}
}

