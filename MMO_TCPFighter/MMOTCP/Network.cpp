#include "pch.h"

#include "MMOTCP.h"
#include "SectorManager.h"
#include "CharacterManager.h"
#include "Protocol.h"
#include "PacketProc.h"
#include "MakePacket.h"
#include "SendPacket.h"

#include "Network.h"

using namespace std;

/* 네트워크 전역변수 */
SOCKET listenSocket;
procademy::MemoryPool<stSession> sessionMP(0, false);
unordered_map<SOCKET, stSession* > sessionMap;
queue<pair<stSession*, stCharacter*> > waitingQ;
queue<stSession*> destroyQ;

int totalSession = 0;
int g_id = 0;

/* 함수 리턴용 전역변수 */
int wsaStartupRet;
int bindRet;
int listenRet;
int setSockOptRet;
int ioctlRet;
int recvRet;
int sendRet;

/* 함수 전방 선언 */
void NetInit();
void NetCleanUp();
void RecvProc(SOCKET socket);
void DestroySession();
void PushSessionToMap();
void SelectFunc(FD_SET* pReadSet, FD_SET* pWriteSet);
void NetworkUpdate();
void RecvProc(SOCKET socket);
void AcceptProc();
void SendProc(SOCKET socket);


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
	serverAddr.sin_port = htons(dfNETWORK_PORT);

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
	while (!destroyQ.empty())
	{
		
		stSession* tmpSession = destroyQ.front();
		destroyQ.pop();

		int sessionId = tmpSession->dwSessionID;

		// 캐릭터 삭제
		DestroyCharacter(sessionId);

		// 세션 삭제
		SerializePacket sPacket;
		//mpDeleteCharacter(&sPacket, tmpSession->_id);
		//SendBroadcast(NULL, &sPacket);

		closesocket(tmpSession->socket);
		sessionMap.erase(tmpSession->socket);
		sessionMP.Free(tmpSession);
	}
}

void PushSessionToMap()
{
	while (!waitingQ.empty())
	{
		stSession* newSession = waitingQ.front().first;
		stCharacter* newCharacter = waitingQ.front().second;
		waitingQ.pop();
		sessionMap.insert({ newSession->socket, newSession });

		// new에게 자신의 생성 메시지 보내기.
		{
			SerializePacket sPacket;

			mpCreateMyCharacter(&sPacket, newCharacter->dwSessionID, newCharacter->byDirection, newCharacter->shX, newCharacter->shY);
			SendPacket_Unicast(newSession, &sPacket);

			printf("New <- mpCreateMyCharacter Send\n");
		}

		// new에게 기존 멤버들 생성 메시지 보내기.
		{
			stSECTOR_AROUND sectorAround;
			GetSectorAround(newCharacter->curSector.iY, newCharacter->curSector.iX, &sectorAround);

			vector<stCharacter*> v;
			for (int i = 0; i < sectorAround.iCount; i++)
			{
				GetCharactersFromSector(sectorAround.around[i].iY, sectorAround.around[i].iX, v);

				for (int j = 0; j < v.size(); j++)
				{
					if (v[j] == newCharacter)
						continue;

					SerializePacket sPacket;
					mpCreateOtherCharacter(&sPacket, v[j]->dwSessionID, v[j]->byDirection, v[j]->shX, v[j]->shY, v[j]->chHP);
					SendPacket_Unicast(newSession, &sPacket);
				}

				v.clear();
			}

			printf("New <- mpCreateOtherCharacter Send\n");
		}

		// new에게 기존 멤버들 액션 메시지 보내기.
		{
			stSECTOR_AROUND sectorAround;
			GetSectorAround(newCharacter->curSector.iY, newCharacter->curSector.iX, &sectorAround);

			vector<stCharacter*> v;
			for (int i = 0; i < sectorAround.iCount; i++)
			{
				GetCharactersFromSector(sectorAround.around[i].iY, sectorAround.around[i].iX, v);

				for (int j = 0; j < v.size(); j++)
				{
					if (v[j]->dwAction == dfMOVE_STOP)
						continue;

					SerializePacket sPacket;
					mpMoveStart(&sPacket, v[j]->dwSessionID, v[j]->byMoveDirection, v[j]->shX, v[j]->shY);
					SendPacket_Unicast(newSession, &sPacket);
				}

				v.clear();
			}

			printf("New <- mpMoveStart Send\n");
		}

		// 기존 멤버들에게 new 생성 메시지 보내기.
		{
			SerializePacket sPacket;
			mpCreateOtherCharacter(&sPacket, newSession->dwSessionID, newCharacter->byDirection, newCharacter->shX, newCharacter->shY, newCharacter->chHP);
		
			SendPacket_Around(newSession, &sPacket, false);
			printf("Other <- mpCreateOtherCharacter Send\n");
		}
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
		printf("Error: accecpt() %d\n", WSAGetLastError());
		return;
	}

	stSession* newSession = sessionMP.Alloc();

	newSession->socket = newSocket;
	newSession->clientAddr = clientAddr;
	newSession->dwSessionID = g_id++;
	newSession->recvQ.Resize(5000);
	newSession->sendQ.Resize(10000);
	newSession->dwLastRecvTime = GetTickCount();

	stCharacter* pNewCharacter = NULL;
	CreateCharacter(newSession, newSession->dwSessionID, &pNewCharacter);
	waitingQ.push({ newSession, pNewCharacter });
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
				RecvProc(nowSession->socket);
			}

			if (FD_ISSET(nowSession->socket, pWriteSet))
			{
				--result;
				SendProc(nowSession->socket);
			}

			if (result <= 0)
				break;
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

	if (setCount > 0)
		SelectFunc(&readSet, &writeSet);
	
	PushCharacterToMap();
	PushSessionToMap();

	DestroySession();
}

void PushSessionToDestroyQ(stSession* pSession)
{
	destroyQ.push(pSession);
}

void RecvProc(SOCKET socket)
{
	stSession* pSession = sessionMap.find(socket)->second;

	recvRet = recv(pSession->socket, pSession->recvQ.GetRearBufferPtr(), pSession->recvQ.DirectEnqueueSize(), 0);
	if (recvRet == SOCKET_ERROR)
	{
		if (WSAGetLastError() == 10054)
		{
			// TODO: 
			
			printf("[상대방의 비 정상 연결 종료] id : %d\n", pSession->dwSessionID);
			//destroyQ.push(pSession);
			PushSessionToDestroyQ(pSession);
			return;
			
		}

		if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			// TODO: 
			
			printf("ERROR: recv() %d\n", WSAGetLastError());
			return;
			
		}
	}
	if (recvRet == 0)
	{
		// TODO: 종료처리 (상대 FIN보낸거지)
		
		printf("[상대방의 정상 연결 종료] id : %d\n", pSession->dwSessionID);
		//destroyQ.push(pSession);
		PushSessionToDestroyQ(pSession);
		return;
		
	}

	pSession->recvQ.MoveRear(recvRet);

	while (1)
	{
		if (pSession->recvQ.GetUseSize() <= sizeof(st_PACKET_HEADER))
			break;

		// TODO: 직렬화 버퍼도 메모리풀로 관리해도 될지도?
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
			// TODO:
			printf("ERROR: send() %d\n", WSAGetLastError());
			return;
		}
	}

	pSession->sendQ.MoveFront(sendRet);

	return;

}

