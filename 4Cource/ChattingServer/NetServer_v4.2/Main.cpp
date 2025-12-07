#include "stdafx.h"
#include "Monitoring.h"
#include "NetServer.h"
#include "ChatServer.h"

using namespace std;

procademy::CCrashDump dump;

ChatServer chattingServer;
HANDLE hEvent_Quit;

void MonitorThread()
{
	while (1)
	{
		DWORD ret = WaitForSingleObject(hEvent_Quit, 1000);
		if (ret == WAIT_OBJECT_0)
		{
			break;
		}

		Monitoring::GetInstance()->PrintMonitoring();
		Monitoring::GetInstance()->Clear();
	}
}

int main()
{
	timeBeginPeriod(1);

	bool serverSet = chattingServer.Start(L"127.0.0.1", 20601, 20, 2, TRUE, 50000, TRUE);
	if (serverSet == false)
	{
		printf("server start error\n");
		return 0;
	}

	hEvent_Quit = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (hEvent_Quit == 0)
		return 0;

	HANDLE hThread_Monitoring = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&MonitorThread, NULL, NULL, NULL);
	if (hThread_Monitoring == 0)
		return 0;

	while (1)
	{
		ProfilerInput();

		/*char inputKey = _getch();
		if (inputKey == 'Q' || inputKey == 'q')
		{
			chattingServer.Stop();
			SetEvent(hEvent_Quit);

			break;
		}*/
	}

	WaitForSingleObject(hThread_Monitoring, INFINITE);

	return 0;
}