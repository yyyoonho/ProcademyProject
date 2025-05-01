#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "CustomProfiler.lib")
#pragma comment(lib,"ws2_32.lib")

#include <iostream>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <windows.h>
#include <iostream>

#include <thread>
#include <process.h>


using namespace std;

int a = 0;
LONG flag = 0;

LONG ticket = 0;

void ThreadFunc1()
{
	int cap_A;

	for (int i = 0; i < 100000; i++)
	{
		while (1)
		{
			if ((cap_A = flag) == 0)
				break;
		}

		if (InterlockedExchange(&ticket, 1) != 0)
			DebugBreak();
		flag = 1;

		a++;

		if (InterlockedExchange(&ticket, 0) != 1)
			DebugBreak();

		flag = 0;
	}

	return;
}

void ThreadFunc2()
{
	int cap_B;

	for (int i = 0; i < 100000; i++)
	{
		while (1)
		{
			if ((cap_B = flag) == 0)
				break;
		}

		if (InterlockedExchange(&ticket, 2) != 0)
			DebugBreak();

		flag = 1;
		a++;

		if (InterlockedExchange(&ticket, 0) != 2)
			DebugBreak();

		flag = 0;
	}

	return;
}

void ThreadFunc3()
{
	int cap_B;

	for (int i = 0; i < 100000; i++)
	{
		while (1)
		{
			if ((cap_B = InterlockedExchange(&flag, 1)) == 0)
				break;
		}

		if (InterlockedExchange(&ticket, 1) != 0)
			DebugBreak();

		a++;

		if (InterlockedExchange(&ticket, 0) != 1)
			DebugBreak();

		flag = 0;
	}

	return;
}

void ThreadFunc4()
{
	int cap_B;

	for (int i = 0; i < 100000; i++)
	{
		while (1)
		{
			if ((cap_B = InterlockedExchange(&flag, 1)) == 0)
				break;
		}

		if (InterlockedExchange(&ticket, 2) != 0)
			DebugBreak();

		a++;

		if (InterlockedExchange(&ticket, 0) != 2)
			DebugBreak();

		flag = 0;
	}

	return;
}

int main()
{
	timeBeginPeriod(1);

	HANDLE threadHandle[2];

	threadHandle[0] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&ThreadFunc3, NULL, NULL, NULL);
	threadHandle[1] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&ThreadFunc4, NULL, NULL, NULL);

	WaitForMultipleObjects(2, threadHandle, true, INFINITE);

	cout << a;
}