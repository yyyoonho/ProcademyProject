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
		printf("acceptTPS : %d\n", server1.GetAcceptTPS());
		printf("recvMessageTPS : %d\n", server1.GetRecvMessageTPS());
		printf("sendMessageTPS : %d\n", server1.GetSendMessageTPS());
		printf("-----------------------------------\n");
		printf("\n");
	}
}

int main()
{
	bool serverSet = server1.Start(L"127.0.0.1", 6000, 5, 8, TRUE, 10000);
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