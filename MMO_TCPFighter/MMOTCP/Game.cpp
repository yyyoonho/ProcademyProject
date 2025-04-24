#include <Windows.h>
#include <iostream>
#include <list>
#include <vector>
#include <unordered_map>

#include "MemoryPool.h"

#include "SectorManager.h"
#include "CharacterManager.h"
#include "Game.h"

using namespace std;

#define FRAME 50
#define MSPERFRAME (1000/FRAME)

bool FrameControl()
{
	static int oldTime = timeGetTime();

	int diffTime = timeGetTime() - oldTime;

	if (diffTime < MSPERFRAME)
	{
		//Sleep(MSPERFRAME - diffTime); 
		return false;
	}
	else
	{
		oldTime += MSPERFRAME;
		return true;
	}
}

void ShowFrame()
{
	static int oldTime = timeGetTime();
	static int count = 0;
	count++;

	int diffTime = timeGetTime() - oldTime;
	if (diffTime >= 1000)
	{
		printf("Frame: %d\n", count);
		count = 0;

		oldTime += 1000;
	}
}

void GameUpdate()
{
	if (!FrameControl())
		return;
	



}