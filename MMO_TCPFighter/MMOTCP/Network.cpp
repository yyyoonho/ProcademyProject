#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <iostream>
#include <list>
#include <unordered_map>

#include "RingBuffer.h"
#include "MemoryPool.h"
#include "SerializeBuffer.h"

#include "Data.h"
#include "Struct.h"
#include "MMOTCP.h"
#include "Network.h"

using namespace std;

/* 네트워크 전역변수 */
SOCKET listenSocket;
procademy::MemoryPool<stSession> sessionMP(0, false);
unordered_map<SOCKET, stSession* > sessionMap;

int totalSession = 0;
int sessionId = 0;

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

void NetCleanUp()
{
	closesocket(listenSocket);
}

void DestroySession()
{

}

void AcceptProc()
{
	SOCKADDR_IN clientAddr;
	memset(&clientAddr, 0, sizeof(clientAddr));
	int addrLen = sizeof(clientAddr);

	SOCKET newSocket = accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
	if (newSocket == INVALID_SOCKET)
	{
		printf("Error: accecpt() %d\n", WSAGetLastError());
		return;
	}

	stSession* newSession = sessionMP.Alloc();
	//sessionList.push_front(newSession);
	/*newSession->_socket = newSocket;
	newSession->_clientAddr = clientAddr;
	newSession->_id = g_id++;
	newSession->_x = RANGE_MOVE_RIGHT / 2;
	newSession->_y = RANGE_MOVE_BOTTOM / 2;
	newSession->_hp = 100;
	newSession->_dX = RANGE_MOVE_RIGHT / 2;
	newSession->_dY = RANGE_MOVE_BOTTOM / 2;
	newSession->_action = PACKET_STOP;
	newSession->_characterDirection = PACKET_MOVE_DIR_LL;*/
}

void SelectFunc(FD_SET* pReadSet, FD_SET* pWriteSet)
{
	timeval time;
	time.tv_sec = 0;
	time.tv_usec = 0;

	int result = select(0, pReadSet, pWriteSet, 0, &time);

	if (result > 0)
	{
		if (FD_ISSET(listenSocket, pReadSet))
		{
			AcceptProc();
		}

		unordered_map<SOCKET, stSession* >::iterator iter;
		for (iter = sessionMap.begin(); iter != sessionMap.end(); ++iter)
		{
			stSession* nowSession = iter->second;

			if (FD_ISSET(nowSession->socket, pReadSet))
			{
				--result;
				//RecvProc(nowSession);
			}

			if (FD_ISSET(nowSession->socket, pWriteSet))
			{
				--result;
				//SendProc(nowSession);
			}
		}

	}
	else if (result == SOCKET_ERROR)
	{
		// TODO: 로그남기기
		printf("ERROR: select() %d\n", WSAGetLastError());
		return;
	}
}

void NetworkUpdate()
{
	fd_set readSet;
	fd_set writeSet;

	FD_ZERO(&readSet);
	FD_ZERO(&writeSet);

	FD_SET(listenSocket, &readSet);
	int setCount = 1;

	unordered_map<SOCKET, stSession* >::iterator iter;
	for (iter = sessionMap.begin(); iter != sessionMap.end(); ++iter)
	{
		stSession* nowSession = iter->second;

		FD_SET(nowSession->socket, &readSet);
		if(nowSession->sendQ.GetUseSize() > 0)
		{
			FD_SET(nowSession->socket, &writeSet);
		}

		setCount++;

		if (setCount >= 64)
		{
			SelectFunc(&readSet, &writeSet);

			FD_ZERO(&readSet);
			FD_ZERO(&writeSet);

			setCount = 0;
		}
	}
	

	DestroySession();
}
