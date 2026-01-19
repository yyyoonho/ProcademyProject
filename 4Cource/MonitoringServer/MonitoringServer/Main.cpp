#include "stdafx.h"
#include "Monitoring.h"
#include "MyConfig.h"

#include "LogManager.h"
#include "NetServer.h"

#include "SendJob.h"
#include "CommonProtocol.h"
#include "MonitoringServer.h"
#include "LanMonitoringServer.h"
#include "LocalMonitoring.h"

using namespace std;

procademy::CCrashDump dump;


int main()
{
	timeBeginPeriod(1);

	InitLog();

	myConfig.Load("MonitoringConfig.ini");

	MonitoringServer	Net_monitoringServer;
	LanMonitoringServer Lan_monitoringServer;
	LocalMonitoring		Local_monitoring;

	Lan_monitoringServer.RegisterNetServer(&Net_monitoringServer);
	Local_monitoring.RegisterNetServer(&Net_monitoringServer);

	bool serverRet = Net_monitoringServer.Start(L"127.0.0.1", 20603, 6, 24, TRUE, 100, TRUE, 30, 109);
	if (serverRet == false)
	{
		printf("NetServer start error\n");
		return 0;
	}

	serverRet = Lan_monitoringServer.Start(L"127.0.0.1", 20604, 6, 24, TRUE, 20000, TRUE, 0x32, 0x77);
	if (serverRet == false)
	{
		printf("NetServer start error\n");
		return 0;
	}

	Local_monitoring.Start();


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