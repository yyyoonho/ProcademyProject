#pragma comment(lib,"winmm.lib")

#include <Windows.h>
#include <iostream>
#include <thread>

using namespace std;

LONG g_flag[2];
LONG g_turn;

LONG flagInterlocked_0 = 4;
LONG flagInterlocked_1 = 4;
LONG turnInterlocked_0 = 4;
LONG turnInterlocked_1 = 4;

bool enterZone[2];

LONG a = 0;

UINT FuncA(LPVOID lpThreadParameter)
{
    int count = 0;
    while (count++ < 10000000)
    {
        g_flag[0] = true;

        g_turn = 0;

        while (1)
        {
            if ((flagInterlocked_1 = InterlockedCompareExchange(&g_flag[1], 0, 0)) == false)
                break;
                
            if ((turnInterlocked_0 = InterlockedCompareExchange(&g_turn, 0, 0)) != 0)
                break;
                
        }

        // 임 계 영 역
        enterZone[0] = true;

        if (enterZone[1] == true && enterZone[0] == true)
        {
            DebugBreak();
        }

        a++;

        enterZone[0] = false;
        // 임 계 영 역

        g_flag[0] = false;
    }

    return 0;
}

UINT FuncB(LPVOID lpThreadParameter)
{
    int count = 0;
    while (count++ < 10000000)
    {
        g_flag[1] = true;
        
        g_turn = 1;

        while (1)
        {
            if (g_flag[0] == false)
                break;
            if ((turnInterlocked_1 = InterlockedCompareExchange(&g_turn, 0, 0)) != 1)
                break;
        }

        // 임 계 영 역
        enterZone[1] = true;

        a++;

        Sleep(3);

        enterZone[1] = false;
        // 임 계 영 역

        g_flag[1] = false;
    }


    return 0;
}

int main()
{
    HANDLE threadHandles[2];

    threadHandles[0] = (HANDLE)_beginthreadex(NULL, 0, &FuncA, NULL, NULL, NULL);
    threadHandles[1] = (HANDLE)_beginthreadex(NULL, 0, &FuncB, NULL, NULL, NULL);

    WaitForMultipleObjects(2, threadHandles, true, INFINITE);

    printf("%d\n", a);

}
