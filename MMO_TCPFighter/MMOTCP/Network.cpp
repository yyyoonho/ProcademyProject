#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <iostream>

#include "Data.h"
#include "MMOTCP.h"
#include "Network.h"

using namespace std;

/* 네트워크 전역변수 */
SOCKET listenSocket;

/* 함수 리턴용 전역변수 */
int wsaStartupRet;
int bindRet;
int listenRet;
int setSockOptRet;
int ioctlRet;

void NetInit()
{
	WSADATA wsaData;
	wsaStartupRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsaStartupRet != 0)
	{
		// TODO: 로그남기기
		printf("Error: WSAStartup() %d\n", WSAGetLastError());
		g_bShutdown = true;
		return;
	}

	listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == INVALID_SOCKET)
	{
		// TODO: 로그남기기
		printf("Error: socket() %d\n", WSAGetLastError());
		g_bShutdown = true;
		return;
	}

	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(SERVERPORT);

	bindRet = bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (bindRet == SOCKET_ERROR)
	{
		// TODO: 로그남기기
		printf("Error: bind() %d\n", WSAGetLastError());
		g_bShutdown = true;
		return;
	}

	// 강제종료 옵션 켜기
	LINGER linger;
	linger.l_onoff = 1;
	linger.l_linger = 0;
	setSockOptRet = setsockopt(listenSocket, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(LINGER));
	if (setSockOptRet == SOCKET_ERROR)
	{
		// TODO: 로그남기기
		printf("Error: setsockopt() %d\n", WSAGetLastError());
		g_bShutdown = true;
		return;
	}

	// 비동기 소켓 옵션 켜기
	u_long on = 1;
	ioctlRet = ioctlsocket(listenSocket, FIONBIO, &on);
	if (ioctlRet == SOCKET_ERROR)
	{
		// TODO: 로그남기기
		printf("Error: ioctlRet() %d\n", WSAGetLastError());
		g_bShutdown = true;
		return;
	}

	listenRet = listen(listenSocket, SOMAXCONN);
	if (listenRet == SOCKET_ERROR)
	{
		// TODO: 로그남기기
		printf("Error: listen() %d\n", WSAGetLastError());
		g_bShutdown = true;
		return;
	}
}