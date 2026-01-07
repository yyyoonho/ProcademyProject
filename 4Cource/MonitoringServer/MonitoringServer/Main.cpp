#include "stdafx.h"
#include "Monitoring.h"

#include "LogManager.h"
#include "NetServer.h"

#include "SendJob.h"
#include "CommonProtocol.h"
#include "MonitoringServer.h"
#include "LanMonitoringServer.h"

using namespace std;

procademy::CCrashDump dump;


int main()
{
	timeBeginPeriod(1);

	InitLog();

	

	MonitoringServer Net_monitoringServer;
	LanMonitoringServer Lan_monitoringServer;

	Lan_monitoringServer.RegisterNetServer(&Net_monitoringServer);

	bool serverRet = Net_monitoringServer.Start(L"127.0.0.1", 20603, 4, 20, TRUE, 40000, TRUE, 30, 109);
	if (serverRet == false)
	{
		printf("NetServer start error\n");
		return 0;
	}

	serverRet = Lan_monitoringServer.Start(L"127.0.0.1", 20604, 4, 20, TRUE, 40000, TRUE, 0x32, 0x77);
	if (serverRet == false)
	{
		printf("NetServer start error\n");
		return 0;
	}


	while (1)
	{
		Sleep(1000);

		Monitoring::GetInstance()->PrintMonitoring();
		Monitoring::GetInstance()->Clear();
	}

	//while (1)
	//{
	//	//ProfilerInput();

	//	char inputKey = _getch();
	//	if (inputKey == 'Q' || inputKey == 'q')
	//	{
	//		//chattingServer.Stop();

	//		break;
	//	}
	//}

	return 0;
}