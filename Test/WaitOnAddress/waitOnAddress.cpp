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

LONG g_lock = 1; // 0-> 아무도 없음, 1-> 누가 들어가 있음
LONG g_compare = 1;

CRITICAL_SECTION g_CS;

void Func1()
{
	while (1)
	{
		cout << "ThreadID: " << GetCurrentThreadId() << endl;
		//Sleep(100);
		WaitOnAddress(&g_lock, &g_compare, sizeof(LONG), INFINITE);
	}
}

int main()
{
	timeBeginPeriod(1);

	HANDLE threadHandles[10];
	for (int i = 0; i < 10; i++)
	{
		threadHandles[i] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type) & Func1, NULL, NULL, NULL);
		Sleep(10);
	}

	cout<<"***********************"<< endl;

	while (1)
	{
		if (_getch())
		{
			WakeByAddressSingle(&g_lock);

		}

		
	}

}