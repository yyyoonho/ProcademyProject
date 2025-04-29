#pragma comment(lib,"winmm.lib")

#include <Windows.h>
#include <iostream>
#include <thread>

using namespace std;

LONG g_flag[2];
LONG g_turn;

LONG cap_A;
LONG cap_B;
LONG cap_C;
LONG cap_D;

int a = 0;

LONG lock = 0;

UINT FuncA(LPVOID lpThreadParameter)
{
    int count = 0;
    while (count++ < 10000000)
    {
        g_flag[0] = true;   // store flag[0]
        g_turn = 0;         // store turn 

        while (1)
        {
            if (g_turn == 99)   // load turn
                int tmp = 3;

            cap_A = g_flag[1];  // load flag[1]
                                // store cap_A

            cap_B = g_turn;     // load turn
                                // store cap_B

            if (cap_A == false) // load cap_A     
                break;         

            if (cap_B != 0)     // load cap_B
                break;

        }

        // 임 계 영 역
        LONG ret1 = InterlockedExchange(&lock, 1);
        if (ret1 != 0)
        {
            DebugBreak();
        }

        a++;


        ret1 = InterlockedExchange(&lock, 0);
        if (ret1 != 1)
        {
            DebugBreak();
        }
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
            if (g_turn == 99)
                int tmp = 3;

            cap_C = g_flag[0];
            cap_D = g_turn;

            if (cap_C == false)
                break;
            if (cap_D != 1)
                break;
        }

        // 임 계 영 역
        LONG ret2 = InterlockedExchange(&lock, 2);
        if (ret2 != 0)
        {
            DebugBreak();
        }

        a++;

        ret2 = InterlockedExchange(&lock, 0);
        if (ret2 != 2)
        {
            DebugBreak();
        }
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
