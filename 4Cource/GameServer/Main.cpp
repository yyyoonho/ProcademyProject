#include "stdafx.h"
#include "LogManager.h"
#include "NetClient.h"
#include "NetServer.h"
#include "SendJob.h"
#include "FieldStruct.h"
#include "Field.h"
#include "MyField_Auth.h"
#include "MyField_Echo.h"
#include "GameManager.h"
#include "NetClient_Monitoring.h"

using namespace std;

procademy::CCrashDump dump;

GameManager				gameManager;
NetClient_Monitoring	MonitoringClient;

int main()
{
	timeBeginPeriod(1);

	InitLog();

	gameManager.RegistMonitoring(&MonitoringClient);

	MyField_Auth* authField = new MyField_Auth;
	MyField_Echo* echoField = new MyField_Echo;

	gameManager.RegistField(FieldName::Auth, authField);
	gameManager.RegistField(FieldName::Echo, echoField);


	bool serverSet = gameManager.Start(L"127.0.0.1", 20609, 40, 10, TRUE, 25000, TRUE, 0x32, 0x77);
	if (serverSet == false)
	{
		printf("server start error\n");

		return 0;
	}

	bool clientRet = MonitoringClient.Connect(L"127.0.0.1", 20604, 10, 20, TRUE, TRUE, 0x32, 0x77);
	if (clientRet == false)
	{
		printf("server start error\n");

		return 0;
	}

	while (1)
	{
		ProfilerInput();

		char inputKey = _getch();
		if (inputKey == 'Q' || inputKey == 'q')
		{
			gameManager.Stop();

			break;
		}
	}

	return 0;
}