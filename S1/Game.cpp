#include <Windows.h>
#include <iostream>
#include "Data.h"

#include "Game.h"
#include "BufferRender.h"
#include "Player.h"
#include "Enemy.h"
#include "Item.h"
#include "S1.h"
#include "Load.h"
#include "GameOver.h"

DWORD startTime;

bool stageClear;

void GameInput();
void GameLogic();
void GameRender();
void ClearLogic();

void GameUpdate()
{
	GameInput();

	GameLogic();

	if (RenderSkipCheck())
		return;

	GameRender();
}

void GameReset()
{
	stageClear = false;
}

void GameInput()
{
	EnemyInput();
	PlayerInput();
}

void GameLogic()
{
	logicFPS++;

	EnemyLogic();
	PlayerLogic();
	ItemLogic();
	ClearLogic();
}

void GameRender()
{
	RenderFPS++;

	Buffer_Clear();

	EnemyRender();
	PlayerRender();
	ItemRender();

	ShowFPS();
	
	Buffer_Flip();
}

void ClearLogic()
{
	if (enemyCount <= 0)
	{
		stageClear = true;

		SetEndTime(timeGetTime() - startTime);

		ChangeScene(SCENE::GAMEOVER);
	}
}
