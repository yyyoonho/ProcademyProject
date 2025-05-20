#pragma comment(lib, "winmm.lib")
#pragma comment(lib,"ws2_32.lib")


#include <iostream>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <windows.h>
#include <thread>
#include <conio.h>

#define SERVERPORT 9000
#define SERVERIP L"127.0.0.1"

int connectRet;
int ioctlRet;
SOCKET sock;

HANDLE hSendThread;

char str[2000] = "Look if you had one shot or one opportunityTo seize everything you ever wantedIn one momentWould you capture it or just let it slip ?Yo his palms are sweaty knees weakArms are heavyThere's vomit on his sweater alreadyMom's spaghettiHe's nervous but on the surfaceHe looks calm and readyTo drop bombs but he keeps on forgettingWhat he wrote downThe whole crowd goes so loudHe opens his mouthBut the words won't come outHe's choking how?Everybody's joking nowThe clock's run outTime's up over blaowSnap back to realityOpe there goes gravity opeThere goes rabbit he chokedHe's so mad but he won'tGive up that easy noHe won't have it he knowsHis whole back's to these ropesIt don't matter he's dopeHe knows that but he's brokeHe's so stagnant he knowsWhen he goes back to this mobile homeThat's when it'sBack to the lab again yo this whole rhapsodyBetter go capture this momentAnd hope it don't pass him andYou better lose yourself in the musicThe moment you own itYou better never let it go";

void SendThread()
{
	bool autoFlag = true;

	int idx = 0;

	while (1)
	{
		if (autoFlag == false)
		{
			char buf[200];
			fgets(buf, 200, stdin);

			if (buf[strlen(buf) - 1] == '\n')
				buf[strlen(buf) - 1] = '\0';

			send(sock, buf, strlen(buf) + 1, 0);
		}
		else if (autoFlag == true)
		{
			char input = _getch();

			for (int i = 0; i < 10; i++)
			{
				int len = 50;

				int sendRet = send(sock, str + idx, len, 0);
				if (sendRet == SOCKET_ERROR)
				{
					if (GetLastError() != WSAEWOULDBLOCK)
					{
						printf("ERROR: send() %d\n", WSAGetLastError());
						return;
					}
				}

				idx += len;
				if (idx + len > strlen(str))
					idx = 0;
			}
		}
	}

	return;
}

int main()
{
	timeBeginPeriod(1);

	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		printf("ERROR: socket%d\n", GetLastError());
		return 0;
	}

	SOCKADDR_IN serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVERPORT);
	InetPton(AF_INET, SERVERIP, &serverAddr.sin_addr);

	connectRet = connect(sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (connectRet == SOCKET_ERROR)
	{
		printf("ERROR: connect%d\n", GetLastError());
		return 0;
	}

	u_long l = 1;
	ioctlRet = ioctlsocket(sock, FIONBIO, &l);
	if (ioctlRet == SOCKET_ERROR)
	{
		printf("ERROR: ioctlsocket%d\n", GetLastError());
		return 0;
	}

	hSendThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&SendThread, NULL, NULL, NULL);

	FD_SET fd_read;
	FD_ZERO(&fd_read);
	while (1)
	{
		FD_SET(sock, &fd_read);

		timeval timev;
		timev.tv_sec = 0;
		timev.tv_usec = 0;

		int selectRet = select(0, &fd_read, NULL, NULL, &timev);

		if (selectRet > 0)
		{
			if (FD_ISSET(sock, &fd_read))
			{
				//TODO: 읽기처리
				char recvbuf[10000 + 1];
				int ret = recv(sock, recvbuf, 10000, 0);
				if (ret == SOCKET_ERROR)
				{
					if (WSAGetLastError() != WSAEWOULDBLOCK)
					{
						printf("ERROR: recv() %d\n", WSAGetLastError());
						return 0;
					}
				}
				recvbuf[ret] = '\0';

				printf("#RECV [ 받은 바이트수: %d ]\n %s\n\n", ret, recvbuf);
			}
		}
	}

	return 0;
}