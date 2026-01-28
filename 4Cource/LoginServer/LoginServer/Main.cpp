#include "stdafx.h"

#include <cpp_redis/cpp_redis>
#pragma comment (lib, "cpp_redis.lib")
#pragma comment (lib, "tacopie.lib")

#include "LogManager.h"

#include "SendJob.h"

#include "NetClient.h"
#include "NetClient_Monitoring.h"

#include "NetServer.h"
#include "LoginServer.h"

#include "DB_Profiler.h"
using namespace std;

procademy::CCrashDump dump;

LoginServer				loginServer1;
NetClient_Monitoring	MonitoringClient;

int main()
{
	timeBeginPeriod(1);

	InitLog();

	bool serverSet = loginServer1.Start(L"127.0.0.1", 20602, 20, 20, TRUE, 20000, TRUE, 0x32, 0x77);
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

	loginServer1.RegisterNetServer(&MonitoringClient);



	while (1)
	{
		//ProfilerInput();

		char inputKey = _getch();
		if (inputKey == 'Q' || inputKey == 'q')
		{
			loginServer1.Stop();
			DBProfilerRecordStop();

			break;
		}
	}

	return 0;
}