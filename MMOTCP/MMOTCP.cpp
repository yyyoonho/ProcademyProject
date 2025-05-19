#include "stdafx.h"

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "RingBuffer.lib")

#include "LogManager.h"
#include "Network.h"

using namespace std;

/* 서버 메인 전역변수 */
bool g_bShutdown = false;

void Init();

int main()
{
    timeBeginPeriod(1);

    Init();

    NetInit();

    while (!g_bShutdown)
    {
        NetworkUpdate();

        //GameUpdate();

        //ServerControl();
        //Monitor();
    }

    NetCleanUp();

    timeEndPeriod(1);
}

void Init()
{
    srand(time(NULL));
    InitLog();
}
