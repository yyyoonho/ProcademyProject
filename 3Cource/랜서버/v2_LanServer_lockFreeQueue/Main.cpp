#include "stdafx.h"
#include "LanServer.h"
#include "EchoServer.h"

using namespace std;

CEchoServer server1;

HANDLE hMonitoringThread;
HANDLE hEvent_Quit;

void MonitoringThread()
{
	while (1)
	{
		DWORD ret = WaitForSingleObject(hEvent_Quit, 1000);
		if (ret == WAIT_OBJECT_0)
		{
			return;
		}

		printf("-----------------------------------\n");
		printf("sessionCount : %d\n\n", server1.GetSessionCount());

		printf("acceptTPS : %d\n", server1.GetAcceptTPS());
		printf("recvMessageTPS : %d\n", server1.GetRecvMessageTPS());
		printf("sendMessageTPS : %d\n", server1.GetSendMessageTPS());
		printf("-----------------------------------\n");
		printf("\n");
	}
}

int main()
{
	// 4개의 코어만 사용 (예: 코어 0~3)
	//DWORD_PTR affinityMask = 0xF; // (0b1111) → CPU 0,1,2,3

	//HANDLE hProcess = GetCurrentProcess();

	//if (SetProcessAffinityMask(hProcess, affinityMask))
	//	std::cout << "CPU affinity set successfully.\n";
	//else
	//	std::cout << "Failed to set CPU affinity.\n";

	timeBeginPeriod(1);

	bool serverSet = server1.Start(L"127.0.0.1", 6000, 2, 2, TRUE, 10000);
	if (serverSet == false)
	{
		printf("server start error\n");
		return 0;
	}

	hEvent_Quit = CreateEvent(NULL, TRUE, FALSE, NULL);

	hMonitoringThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type) & MonitoringThread, NULL, NULL, NULL);
	if (hMonitoringThread == NULL)
	{
		printf("hMonitoringThread create error\n");
		return 0;
	}

	while (1)
	{
		char inputKey = _getch();
		if (inputKey == 'Q' || inputKey == 'q')
		{
			server1.Stop();
			SetEvent(hEvent_Quit);

			break;
		}
	}

	WaitForSingleObject(hMonitoringThread, INFINITE);

	return 0;
}