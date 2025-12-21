#include "stdafx.h"

#include <cpp_redis/cpp_redis>
#pragma comment (lib, "cpp_redis.lib")
#pragma comment (lib, "tacopie.lib")

#include "LogManager.h"

#include "NetServer.h"
#include "LoginServer.h"

using namespace std;

procademy::CCrashDump dump;

LoginServer loginServer1;

int main()
{
	timeBeginPeriod(1);

	InitLog();

	loginServer1.Start(L"127.0.0.1", 20602, 100, 2, TRUE, 40000, TRUE);

	while (1)
	{
		//ProfilerInput();

		char inputKey = _getch();
		if (inputKey == 'Q' || inputKey == 'q')
		{
			loginServer1.Stop();

			break;
		}
	}

	return 0;
}