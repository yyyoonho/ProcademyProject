#include <Windows.h>
#include <thread>
#include <iostream>

#include "CCrashDump.h"
#include "LockFreeStack.h"

using namespace std;

//procademy::CCrashDump g_crashDump;

LockFreeStack<int> g_Stack;

void ThreadFunc()
{
    while (1)
    {
        for (int i = 0; i < 100; i++)
        {
            g_Stack.Push(i);
        }
        
        for (int i = 0; i < 100; i++)
        {
            int tmp;
            g_Stack.Pop(&tmp);
        }
    }
}

int main()
{
    HANDLE hThreads[2];
    for (int i = 0; i < 2; i++)
    {
        hThreads[i] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&ThreadFunc, NULL, NULL, NULL);
    }

    WaitForMultipleObjects(2, hThreads, NULL, INFINITE);

    return 0;
}