#pragma comment(lib, "winmm.lib")
#pragma comment(lib,"ws2_32.lib")


#include <iostream>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <windows.h>

#define SERVERPORT 9000
#define SERVERIP L"127.0.0.1"

int connectRet;
int ioctlRet;

int main()
{
	timeBeginPeriod(1);

	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
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

	FD_SET fd_read;
	while (1)
	{
		FD_ZERO(&fd_read);
		FD_SET(sock, &fd_read);

		timeval timev;
		timev.tv_sec = 0;
		timev.tv_usec = 0;

		printf("메시지 입력: ");
		char buf[100];
		fgets(buf, 100, stdin);
		if (buf[strlen(buf) - 1] == '\n')
			buf[strlen(buf) - 1] = '\0';

		send(sock, buf, strlen(buf), 0);

		int selectRet = select(0, &fd_read, NULL, NULL, NULL);

		if (selectRet > 0)
		{
			if (FD_ISSET(sock, &fd_read))
			{
				//TODO: 읽기처리
				char recvbuf[100];
				recv(sock, recvbuf, 100, 0);

				printf("받은 메시지: %s\n", recvbuf);
			}
		}


	}

}