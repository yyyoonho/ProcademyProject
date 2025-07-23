#pragma comment(lib, "winmm.lib")

#include <iostream>
#include <Windows.h>
#include <thread>
#include <conio.h>

using namespace std;

// 전역변수
int g_Data = 0;
int g_Connect = 0;
bool g_Shutdown = false;

CRITICAL_SECTION cs;

void AcceptThread()
{
	srand(time(NULL));

	while (!g_Shutdown)
	{
		int randNum = rand() % 901 + 100;
		Sleep(randNum);

		InterlockedIncrement((LONG*)&g_Connect);
	}

	return;
}

void DisconnectThread()
{
	srand(time(NULL));

	while (!g_Shutdown)
	{
		int randNum = rand() % 901 + 100;
		Sleep(randNum);

		InterlockedDecrement((LONG*)&g_Connect);
	}

	return;
}

void UpdateThread()
{
	while (!g_Shutdown)
	{
		Sleep(10);

		EnterCriticalSection(&cs);

		g_Data++;
		if (g_Data % 1000 == 0)
		{
			cout << g_Data << endl;
		}

		LeaveCriticalSection(&cs);
	}

	return;
}

int main()
{
	timeBeginPeriod(1);

	InitializeCriticalSection(&cs);

	HANDLE threadHandles[5];
	threadHandles[0] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&AcceptThread, NULL, NULL, NULL);
	Sleep(1000);
	threadHandles[1] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&DisconnectThread, NULL, NULL, NULL);
	threadHandles[2] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&UpdateThread, NULL, NULL, NULL);
	threadHandles[3] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&UpdateThread, NULL, NULL, NULL);
	threadHandles[4] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&UpdateThread, NULL, NULL, NULL);

	int count = 0;
	while (1)
	{
		Sleep(1000);
		cout << g_Connect << endl;

		count += 1000;
		if (count == 20000)
		{
			g_Shutdown = true;
			break;
		}
	}

	WaitForMultipleObjects(5, threadHandles, TRUE, INFINITE);

	return 0;
}
