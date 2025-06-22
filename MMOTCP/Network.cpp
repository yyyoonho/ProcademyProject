#include "stdafx.h"

#include "MMOTCP.h"
#include "LogManager.h"
#include "SectorManager.h"
#include "CharacterManager.h"
#include "PacketProc.h"
#include "MakePacket.h"
#include "SendPacket.h"

#include "Network.h"

using namespace std;

/* 네트워크 전역변수 */
SOCKET listenSocket;
procademy::MemoryPool<stSession> sessionMP(0, false);
unordered_map<SOCKET, stSession* > sessionMap;
queue<SOCKET> entryQ;
queue<stSession*> quitQ;

int totalSession = 0;
int g_id = 1;

/* 함수 리턴용 전역변수 */
int wsaStartupRet;
int bindRet;
int listenRet;
int setSockOptRet;
int ioctlRet;
int recvRet;
int sendRet;

/* 함수 전방선언 */
void SelectFunc(FD_SET* pReadSet, FD_SET* pWriteSet);
void RecvProc(SOCKET socket);
void AcceptProc();
void SendProc(SOCKET socket);
void CreateSessionNCharacter();
void DestroySessionNCharacter();

void NetInit()
{
	WSADATA wsaData;
	wsaStartupRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsaStartupRet != 0)
	{
		printf("Error: WSAStartup() %d\n", WSAGetLastError());

		_LOG(dfLOG_LEVEL_SYSTEM, L"Error: WSAStartup() %d\n", WSAGetLastError());

		g_bShutdown = true;
		return;
	}
	_LOG(dfLOG_LEVEL_SYSTEM, L"Success: WSAStartup()\n");

	listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == INVALID_SOCKET)
	{
		printf("Error: socket() %d\n", WSAGetLastError());

		_LOG(dfLOG_LEVEL_SYSTEM, L"Error: socket() %d\n", WSAGetLastError());

		g_bShutdown = true;
		return;
	}
	_LOG(dfLOG_LEVEL_SYSTEM, L"Success: socket()\n");

	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(dfNETWORK_PORT);

	bindRet = bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (bindRet == SOCKET_ERROR)
	{
		printf("Error: bind() %d\n", WSAGetLastError());

		_LOG(dfLOG_LEVEL_SYSTEM, L"Error: bind() %d\n", WSAGetLastError());

		g_bShutdown = true;
		return;
	}
	_LOG(dfLOG_LEVEL_SYSTEM, L"Success: bind()\n");

	// 강제종료 옵션 켜기
	LINGER linger;
	linger.l_onoff = 1;
	linger.l_linger = 0;
	setSockOptRet = setsockopt(listenSocket, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(LINGER));
	if (setSockOptRet == SOCKET_ERROR)
	{
		printf("Error: setsockopt() %d\n", WSAGetLastError());

		_LOG(dfLOG_LEVEL_SYSTEM, L"Error: SO_LINGER() %d\n", WSAGetLastError());

		g_bShutdown = true;
		return;
	}
	_LOG(dfLOG_LEVEL_SYSTEM, L"Success: SO_LINGER\n");

	// 논블락킹 소켓 옵션 켜기
	u_long on = 1;
	ioctlRet = ioctlsocket(listenSocket, FIONBIO, &on);
	if (ioctlRet == SOCKET_ERROR)
	{
		printf("Error: ioctlRet() %d\n", WSAGetLastError());

		_LOG(dfLOG_LEVEL_SYSTEM, L"Error: ioctlRet() %d\n", WSAGetLastError());

		g_bShutdown = true;
		return;
	}
	_LOG(dfLOG_LEVEL_SYSTEM, L"Success: ioctlRet\n");

	// listen()
	listenRet = listen(listenSocket, SOMAXCONN);
	if (listenRet == SOCKET_ERROR)
	{
		printf("Error: listen() %d\n", WSAGetLastError());

		_LOG(dfLOG_LEVEL_SYSTEM, L"Error: listen() %d\n", WSAGetLastError());

		g_bShutdown = true;
		return;
	}
	_LOG(dfLOG_LEVEL_SYSTEM, L"Success: listen\n");
}

void NetCleanUp()
{
	closesocket(listenSocket);
}

void NetworkUpdate()
{
	PRO_BEGIN("Select");

	// Select()
	fd_set readSet;
	fd_set writeSet;

	FD_ZERO(&readSet);
	FD_ZERO(&writeSet);

	FD_SET(listenSocket, &readSet);
	int setCount = 1;
	bool isListenAdded = true;

	unordered_map<SOCKET, stSession* >::iterator iter = sessionMap.begin();

	while (iter != sessionMap.end() || isListenAdded == true)
	{
		if (isListenAdded == false)
		{
			FD_SET(listenSocket, &readSet);
			setCount = 1;
		}

		// 64개씩 끊어서 select
		for (; iter != sessionMap.end() && setCount < 64; ++iter)
		{
			FD_SET(iter->first, &readSet);
			if (iter->second->sendQ.GetUseSize() > 0)
			{
				FD_SET(iter->first, &writeSet);
			}

			++setCount;
		}

		SelectFunc(&readSet, &writeSet);
		FD_ZERO(&readSet);
		FD_ZERO(&writeSet);
		isListenAdded = false;
		setCount = 0;
	}

	PRO_END("Select");

	// TODO: 세션&캐릭터 생성하기
	CreateSessionNCharacter();

	// TODO: 세션&캐릭터 파괴하기
	DestroySessionNCharacter();
}

void PushQuitQ(stSession* pSession)
{
	quitQ.push(pSession);
}

int GetSessionSize()
{
	return sessionMap.size();
}

void SelectFunc(FD_SET* pReadSet, FD_SET* pWriteSet)
{
	timeval time;
	time.tv_sec = 0;
	time.tv_usec = 0;

	int result = select(0, pReadSet, pWriteSet, 0, &time);

	if (result == SOCKET_ERROR)
	{
		_LOG(dfLOG_LEVEL_SYSTEM, L"Error: select() %d\n", WSAGetLastError());
		printf("ERROR: select() %d\n", WSAGetLastError());
		return;
	}

	for (int i = 0; i < pReadSet->fd_count; i++)
	{
		SOCKET sock = pReadSet->fd_array[i];

		if (sock == listenSocket)
		{
			AcceptProc();
		}
		else
		{
			RecvProc(sock);
		}
	}

	for (int i = 0; i < pWriteSet->fd_count; i++)
	{
		SOCKET sock = pWriteSet->fd_array[i];
		SendProc(sock);
	}

}

void AcceptProc()
{
	SOCKADDR_IN clientAddr;
	memset(&clientAddr, 0, sizeof(clientAddr));
	int addrLen = sizeof(clientAddr);

	SOCKET newSocket = accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
	if (newSocket == INVALID_SOCKET)
	{
		_LOG(dfLOG_LEVEL_SYSTEM, L"Error: accept() %d\n", WSAGetLastError());
		printf("Error: accecpt() %d\n", WSAGetLastError());
		return;
	}

	entryQ.push(newSocket);
}

void RecvProc(SOCKET socket)
{
	stSession* pSession = sessionMap.find(socket)->second;

	recvRet = recv(pSession->socket, pSession->recvQ.GetRearBufferPtr(), pSession->recvQ.DirectEnqueueSize(), 0);
	if (recvRet == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			//printf("[상대방의 비 정상 연결 종료] id : %d\n", pSession->dwSessionID);

			//TEST
			_LOG(dfLOG_LEVEL_SYSTEM, L"# [상대방의 비 정상 연결 종료] # SessionID:%d\n", pSession->dwSessionID);
			quitQ.push(pSession);

			_LOG(dfLOG_LEVEL_DEBUG, L"# Disconnet... # SessionID:%d\n", pSession->dwSessionID);
			return;
		}
	}
	if (recvRet == 0)
	{
		//printf("[상대방의 정상 연결 종료] id : %d\n", pSession->dwSessionID);

		//TEST
		_LOG(dfLOG_LEVEL_SYSTEM, L"# [상대방의 정상 연결 종료] # SessionID:%d\n", pSession->dwSessionID);
		quitQ.push(pSession);

		_LOG(dfLOG_LEVEL_DEBUG, L"# Disconnet... # SessionID:%d\n", pSession->dwSessionID);
		return;
	}

	pSession->recvQ.MoveRear(recvRet);

	while (1)
	{
		if (pSession->recvQ.GetUseSize() < sizeof(st_PACKET_HEADER))
			break;

		SerializePacket sPacket;

		// 헤더 뽑기
		int headerPeekSize = pSession->recvQ.Peek(sPacket.GetBufferPtr(), sizeof(st_PACKET_HEADER));
		sPacket.MoveWritePos(headerPeekSize);

		st_PACKET_HEADER header;
		sPacket.GetData((char*)&header, sizeof(st_PACKET_HEADER));

		BYTE payloadLen = header.bySize;

		// 페이로드 뽑기
		if (pSession->recvQ.GetUseSize() < sizeof(st_PACKET_HEADER) + payloadLen)
			break;

		pSession->recvQ.MoveFront(headerPeekSize);

		sPacket.Clear();

		int payloadPeekSize = pSession->recvQ.Peek(sPacket.GetBufferPtr(), payloadLen);
		sPacket.MoveWritePos(payloadPeekSize);

		pSession->recvQ.MoveFront(payloadPeekSize);

		PacketProc(pSession, header.byType, &sPacket);
	}
}

void SendProc(SOCKET socket)
{
	stSession* pSession = sessionMap.find(socket)->second;

	sendRet = send(pSession->socket, pSession->sendQ.GetFrontBufferPtr(), pSession->sendQ.DirectDequeueSize(), 0);
	if (sendRet == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			if (WSAGetLastError() != 10054)
			{
				printf("ERROR: send() %d\n", WSAGetLastError());

				quitQ.push(pSession);

				return;
			}
		}
	}

	pSession->sendQ.MoveFront(sendRet);

	return;
}

void CreateSessionNCharacter()
{
	while (!entryQ.empty())
	{
		SOCKET newSocket = entryQ.front();
		entryQ.pop();

		stSession* newSession = sessionMP.Alloc();
		
		newSession->socket = newSocket;
		newSession->dwSessionID = g_id++;

		newSession->recvQ.Resize(5000);
		newSession->recvQ.ClearBuffer();

		newSession->sendQ.Resize(10000);
		newSession->sendQ.ClearBuffer();

		newSession->dwLastRecvTime = GetTickCount();

		//printf("[접속] SessionID: %d\n", newSession->dwSessionID);

		sessionMap.insert({ newSocket, newSession });

		CreateCharacter(newSession, newSession->dwSessionID);
	}
}

void DestroySessionNCharacter()
{
	while (!quitQ.empty())
	{
		stSession* destroySession = quitQ.front();
		quitQ.pop();

		{
			SerializePacket sPacket;
			mpDeleteCharacter(&sPacket, destroySession->dwSessionID);

			SendPacket_Around(destroySession, &sPacket, false);
		}

		DestroyCharacter(destroySession->dwSessionID);

		sessionMap.erase(destroySession->socket);

		closesocket(destroySession->socket);

		bool ret = sessionMP.Free(destroySession);
		if (!ret)
		{
			_LOG(dfLOG_LEVEL_ERROR, L"Error: sessionMP.Free\n");
		}

		//delete destroySession;
	}
}
