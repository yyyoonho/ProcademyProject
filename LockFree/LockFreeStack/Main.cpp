#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "Dbghelp.lib")

#include <iostream>
#include <list>
#include <Windows.h>
#include <thread>
#include <DbgHelp.h>
#include <minidumpapiset.h>
#include "CrashDump.h"

#include "LockFreeStack.h"

using namespace std;

procademy::CrashDump cCrashDump;

MyStack<int> g_Stack;

void ThreadFunc()
{
	while (1)
	{
		for (int i = 0; i < 5; i++)
		{
			g_Stack.Push(i);
		}

		for (int i = 0; i < 5; i++)
		{
			int tmp;
			g_Stack.Pop(&tmp);
		}
	}
}


HANDLE hThread[5];

int main()
{
	timeBeginPeriod(1);

	for (int i = 0; i < 5; i++)
	{
		hThread[i] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&ThreadFunc, NULL, NULL, NULL);
	}

	WaitForMultipleObjects(5, hThread, TRUE, INFINITE);
	
	return 0;
}
