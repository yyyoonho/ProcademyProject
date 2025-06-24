#include "stdafx.h"

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "RingBuffer.lib")
#pragma comment(lib, "SerializeBuffer.lib")
#pragma comment(lib, "CustomProfiler.lib")

#include "LogManager.h"
#include "MonitorManager.h"
#include "Game.h"
#include "Network.h"
#include "ServerControlManager.h"

#include "MMOTCP.h"

using namespace std;

/* 서버 메인 전역변수 */
bool g_bShutdown = false;

void Init();
bool FrameControl();

int g_FrameCount = 0;

int main()
{
    timeBeginPeriod(1);

    Init();

    NetInit();

    while (!g_bShutdown)
    {
        ProfilerInput();

        NetworkUpdate();

        while (FrameControl())
        {
            GameUpdate();
        }

        ServerControl();

        Monitor();
    }

    NetCleanUp();

    timeEndPeriod(1);
}

void Init()
{
    srand(time(NULL));
    InitLog();
}


bool FrameControl()
{
    static int oldTime = timeGetTime();

    int diffTime = timeGetTime() - oldTime;

    if (diffTime < 40) // 20ms
    {
        return false;
    }
    else
    {
        PushDeltaTime(diffTime);
        oldTime += 40;
        g_FrameCount++;
        return true;
    }
}