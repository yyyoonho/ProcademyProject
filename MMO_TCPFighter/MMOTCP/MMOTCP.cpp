#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "RingBuffer.lib")
#pragma comment(lib, "SerializeBuffer.lib")

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

int main()
{
	timeBeginPeriod(1);

	NetInit();

	while (g_bShutdown == false)
	{
		NetworkUpdate();

		GameUpdate();

		ServerControl();
		Monitor();
	}

	NetCleanUp();

	timeEndPeriod(1);
}