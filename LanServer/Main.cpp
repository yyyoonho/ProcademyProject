#include "stdafx.h"
#include "LanServerProtocol.h"

#include "LanServer.h"
#include "EchoServer.h"

using namespace std;

// 에코 서버 전역객체
EchoServer server1;

// 함수 전방선언
void PrintTPSThread();
bool exitTPSThread = false;

int main()
{
	timeBeginPeriod(1);

	server1.Start();
	
	HANDLE hTPSThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&PrintTPSThread, NULL, NULL, NULL);
	if (hTPSThread == NULL)
	{
		return 0;
	}

	while (1)
	{
		if (_kbhit())
		{
			char input = _getch();

			if (input == 'Q' || input == 'q')
			{
				// TODO: 종료 유도.
				exitTPSThread = true;
				server1.Stop();

				break;
			}

			ProfilerInput();
		}
	}

	WaitForSingleObject(hTPSThread, INFINITE);

	return 0;
}

void PrintTPSThread()
{
	int count = 0;
	const char* fileName = "ServerTPS.txt";
	DWORD oldTime = timeGetTime();

	while (1)
	{
		if (exitTPSThread)
			break;

		FILE* fp;
		errno_t ret = fopen_s(&fp, fileName, "at");
		if (ret != 0)
		{
			printf("ERROR: fopen_s\n");
			return;
		}

		char buf[500];

		sprintf_s(buf, "[TPS: %d초]\n acceptTPS:%d \n recvMessageTPS:%d \n sendMessageTPS:%d \n disconnetTPS:%d\n\n",
			count, server1.GetAcceptTPS(), server1.GetRecvMessageTPS(), server1.GetSendMessageTPS(), server1.disconnetFromClient_Save);

		fwrite(buf, strlen(buf), 1, fp);

		fclose(fp);

		count++;

		Sleep(1000 - (timeGetTime()-oldTime));
		oldTime = timeGetTime();
	}

	printf("\nPrintTPSThread 종료중...\n");
	return;
}
