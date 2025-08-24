#include "stdafx.h"
#include "LanServer.h"
#include "EchoServer.h"

using namespace std;

int main()
{
	CEchoServer server1;
	server1.Start(L"127.0.0.1", 6000, 5, 8, TRUE, 10000);

	while (1)
	{
		char inputKey = _getch();
		if (inputKey == 'Q' || inputKey == 'q')
		{
			server1.Stop();
		}
	}

	return 0;
}