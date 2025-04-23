#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "RingBuffer.lib")

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <iostream>

#include "Network.h"
#include "Game.h"
#include "ServerControl.h"
#include "Monitor.h"

#include "MMOTCP.h"

using namespace std;

/* 서버 메인 전역변수 */
bool g_bShutdown = false;

#define FRAME 50
#define MSPERFRAME (1000/FRAME)

void Init()
{
	srand(time(NULL));
}

void FrameControl()
{
	static int oldTime = timeGetTime();

	int diffTime = timeGetTime() - oldTime;

	if (diffTime < MSPERFRAME)
	{
		Sleep(MSPERFRAME - diffTime);	
	}

	oldTime += MSPERFRAME;
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

int main()
{
	timeBeginPeriod(1);
	
	Init();
	NetInit();

	while (g_bShutdown == false)
	{
		FrameControl();
		ShowFrame();

		NetworkUpdate();

		GameUpdate();

		ServerControl();
		Monitor();
	}

	NetCleanUp();

	timeEndPeriod(1);
}