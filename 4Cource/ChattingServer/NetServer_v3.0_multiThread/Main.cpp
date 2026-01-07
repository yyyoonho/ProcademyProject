#include "stdafx.h"
#include "LogManager.h"
#include "NetClient.h"
#include "NetServer.h"
#include "SendJob.h"
#include "NetClient_Monitoring.h"
#include "ChatServer.h"

using namespace std;

procademy::CCrashDump dump;

ChatServer				chattingServer;
NetClient_Monitoring	MonitoringClient;

int main()
{
	timeBeginPeriod(1);

	InitLog();

	bool serverSet = chattingServer.Start(L"127.0.0.1", 20601, 20, 10, TRUE, 40000, TRUE, 0x32, 0x77);
	if (serverSet == false)
	{
		printf("server start error\n");

		return 0;
	}

	bool clientSet = MonitoringClient.Connect(L"127.0.0.1", 20604, 10, 20, TRUE, TRUE, 0x32, 0x77);
	if (clientSet == false)
	{
		printf("server start error\n");

		return 0;
	}

	chattingServer.RegisterNetServer(&MonitoringClient);







	while (1)
	{
		ProfilerInput();

		char inputKey = _getch();
		if (inputKey == 'Q' || inputKey == 'q')
		{
			chattingServer.Stop();

			break;
		}
	}

	return 0;
}