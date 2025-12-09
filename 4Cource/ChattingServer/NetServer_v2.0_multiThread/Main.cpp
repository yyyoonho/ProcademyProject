#include "stdafx.h"
#include "LogManager.h"
#include "NetServer.h"
#include "ChatServer.h"

using namespace std;

procademy::CCrashDump dump;

ChatServer chattingServer;

int main()
{
	timeBeginPeriod(1);

	InitLog();

	bool serverSet = chattingServer.Start(L"127.0.0.1", 20601, 20, 2, TRUE, 40000, TRUE);
	if (serverSet == false)
	{
		printf("server start error\n");

		return 0;
	}

	while (1)
	{
		//ProfilerInput();

		char inputKey = _getch();
		if (inputKey == 'Q' || inputKey == 'q')
		{
			chattingServer.Stop();

			break;
		}
	}

	return 0;
}