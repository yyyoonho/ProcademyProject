#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "CustomProfiler.lib")
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"synchronization.lib")


#include <iostream>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <windows.h>
#include <iostream>
#include <synchapi.h>
#include <conio.h>

#include <thread>
#include <process.h>


using namespace std;

LONG g_lock = 0; // 0-> 아무도 없음, 1-> 누가 들어가 있음
LONG g_compare = 1;

int a = 0;

void Lock()
{
	while (1)
	{
		int ret = InterlockedExchange(&g_lock, 1);
		if (ret == 0)
		{
			return;
		}
		else if (ret == 1)
		{
			WaitOnAddress(&g_lock, &g_compare, sizeof(LONG), INFINITE);
		}
	}
}

void UnLock()
{
	InterlockedExchange(&g_lock, 0);

	WakeByAddressSingle(&g_lock);
}

void Func1()
{
	for (int i = 0; i < 1000000; i++)
	{
		Lock();

		a++;

		UnLock();
	}
}

int main()
{
	timeBeginPeriod(1);

	HANDLE threadHandles[10];
	for (int i = 0; i < 10; i++)
	{
		threadHandles[i] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type) & Func1, NULL, NULL, NULL);
	}

	WaitForMultipleObjects(10, threadHandles, TRUE, INFINITE);

	cout << a;
}