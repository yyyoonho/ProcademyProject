#pragma comment(lib,"winmm.lib")
#pragma comment(lib, "CustomProfiler.lib")

#include <Windows.h>
#include <iostream>
#include <thread>
#include <conio.h>

#include "CustomProfiler.h"

using namespace std;

LONG g_lock = 0;
LONG a = 0;

SRWLOCK srwLock;

void FuncA()
{
    int count = 0;

    while (count<10000000)
    {
        while (1)
        {
            if (InterlockedExchange(&g_lock, 1) == 0)
                break;

            //YieldProcessor();
        }

        a++;
        count++;
        InterlockedExchange(&g_lock, 0);
    }

    return;
}

void FuncB()
{
    int count = 0;

    while (count < 10000000)
    {
        AcquireSRWLockExclusive(&srwLock);
        a++;
        ReleaseSRWLockExclusive(&srwLock);

        count++;
    }

    return;
}

int main()
{
    timeBeginPeriod(1);

    InitializeSRWLock(&srwLock);

    HANDLE threadHandles1[4];
    HANDLE threadHandles2[4];

    /*for (int i = 0; i < 4; i++)
    {
        threadHandles1[i] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&FuncA, NULL, CREATE_SUSPENDED, NULL);
    }*/

    for (int i = 0; i < 4; i++)
    {
        threadHandles2[i] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&FuncB, NULL, CREATE_SUSPENDED, NULL);
    }

    while (1)
    {
        if (_kbhit())
        {
            /*for (int i = 0; i < 4; i++)
            {
                ResumeThread(threadHandles1[i]);
            }*/

            for (int i = 0; i < 4; i++)
            {
                ResumeThread(threadHandles2[i]);
            }

            break;
        }
    }

    PRO_BEGIN("SpinLock");
    //WaitForMultipleObjects(4, threadHandles1, TRUE, INFINITE);
    WaitForMultipleObjects(4, threadHandles2, TRUE, INFINITE);
    PRO_END("SpinLock");

    cout << a;

    while (1)
    {
        ProfilerInput();
    }

    return 0;
}
