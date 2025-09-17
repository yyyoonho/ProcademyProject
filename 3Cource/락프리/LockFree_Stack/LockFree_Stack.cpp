#include <Windows.h>
#include <thread>
#include <iostream>

#include "CCrashDump.h"
#include "MemoryPool.h"

#include "LockFreeStack.h"

using namespace std;

//procademy::CCrashDump g_crashDump;

LockFreeStack<int> g_Stack;

void ThreadFunc()
{
    while (1)
    {
        for (int i = 0; i < 100000; i++)
        {
            g_Stack.Push(i);
        }
        
        for (int i = 0; i < 100000; i++)
        {
            int tmp;
            g_Stack.Pop(&tmp);
        }
    }
}

void MonitorThreadFunc()
{
    while (1)
    {
        printf("==================================\n");
        printf("Capacity: %d\n", g_Stack.mp.GetCapacity());
        printf("UseCount: %d\n", g_Stack.mp.GetUseCount());
        printf("StackSize: %d\n", g_Stack.Size());
        printf("==================================\n");

        Sleep(1000);
    }
}

int main()
{
    HANDLE hThreads[4];
    for (int i = 0; i < 4; i++)
    {
        hThreads[i] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&ThreadFunc, NULL, NULL, NULL);
    }

    HANDLE monitorThread;
    monitorThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&MonitorThreadFunc, NULL, NULL, NULL);

    WaitForMultipleObjects(4, hThreads, TRUE, INFINITE);
    WaitForSingleObject(monitorThread, INFINITE);

    return 0;
}