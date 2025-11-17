#include "stdafx.h"
#include "NetServer.h"
#include "ChatServer.h"

using namespace std;

ChatServer chattingServer;

int main()
{

	bool serverSet = chattingServer.Start(L"127.0.0.1", 10004, 4, 2, TRUE, 10000, FALSE);
	if (serverSet == false)
	{
		printf("server start error\n");
		return 0;
	}
	
	while (1)
	{
		char inputKey = _getch();
		if (inputKey == 'Q' || inputKey == 'q')
		{
			chattingServer.Stop();

			break;
		}
	}


	return 0;
}