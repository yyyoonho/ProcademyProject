#include "stdafx.h"
#include "LanServerProtocol.h"

#include "LanServer.h"

using namespace std;

#define SERVERPORT 6000

LanServer::LanServer() : sessionPool(0,false)
{

}

LanServer::~LanServer()
{
}

bool LanServer::Start()
{
	NetInit();

	CreateAcceptThread();
	CreateIOCPWorkerThread();
	//CreateMonitorThread();

	return false;
}

void LanServer::Stop()
{
	closesocket(listenSocket);

	PostQueuedCompletionStatus(hIOCP, NULL, NULL, NULL);
}

int LanServer::GetSessionCount()
{
	// TODO: 꼭 이렇게 해야할까?
	int a = totalSessionCount;
	return a;
}

bool LanServer::Disconnect(DWORD64 sessionID)
{
	//TODO: recv를 못하게 일단 해야겠네. IOCount가 0이 되도록 유도해야겠어. 소켓을 끊을까?

	return false;
}

bool LanServer::SendPacket(DWORD64 sessionID, SerializePacket* sPacket)
{
	AcquireSRWLockExclusive(&sessionMapLock);
	unordered_map<DWORD64, Session*>::iterator iter = sessionMap.find(sessionID);
	ReleaseSRWLockExclusive(&sessionMapLock);

	if (iter == sessionMap.end())
	{
		printf("Error: No targetSession in sessionMap");
		return false;
	}
		

	Session* targetSession = iter->second;
	
	InterlockedIncrement((LONG*)&sendMessageTPS);

	stHeader header;
	header.len = sPacket->GetDataSize();

	EnterCriticalSection(&targetSession->cs);
	targetSession->sendQ.Enqueue((char*)&header, sizeof(stHeader));
	targetSession->sendQ.Enqueue(sPacket->GetBufferPtr(), sPacket->GetDataSize());
	LeaveCriticalSection(&targetSession->cs);

	return true;
}

int LanServer::GetAcceptTPS()
{
	return acceptTPS;
}

int LanServer::GetRecvMessageTPS()
{
	return recvMessageTPS;
}

int LanServer::GetSendMessageTPS()
{
	return sendMessageTPS;
}

void LanServer::NetInit()
{
	InitializeSRWLock(&sessionMapLock);

	WSADATA wsaData;
	int wsaDataRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsaDataRet != 0)
	{
		printf("ERROR: WSAStartup()\n");
		return;
	}

	// listen socket 생성
	listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == INVALID_SOCKET)
	{
		printf("ERROR: socket() %d\n", WSAGetLastError());
		return;
	}

	// 소켓 옵션 설정
	int optVal = 0;
	setsockopt(listenSocket, SOL_SOCKET, SO_SNDBUF, (char*)&optVal, sizeof(optVal));
	if (setSockOptRet == SOCKET_ERROR)
	{
		printf("ERROR: setsockopt() %d\n", GetLastError());
		return;
	}

	// bind
	SOCKADDR_IN serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(SERVERPORT);

	bindRet = bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (bindRet == SOCKET_ERROR)
	{
		printf("ERROR: bind() %d\n", GetLastError());
		return;
	}

	// listen
	listenRet = listen(listenSocket, SOMAXCONN);
	if (listenRet == SOCKET_ERROR)
	{
		printf("ERROR: listen() %d\n", GetLastError());
		return;
	}
}

void LanServer::CreateAcceptThread()
{
	// Accept 쓰레드 생성
	hAcceptThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&LanServer::AcceptThreadRun, this, NULL, NULL);
	if (hAcceptThread == NULL)
	{
		printf("ERROR: hAcceptThread %d\n", WSAGetLastError());
		return;
	}
}

void LanServer::CreateIOCPWorkerThread()
{
	// IOCP 생성
	hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 4);
	if (hIOCP == NULL)
	{
		printf("ERROR: CreateIoCompletionPort() %d\n", WSAGetLastError());
		return;
	}

	// CPU 갯수 확인
	SYSTEM_INFO si;
	GetSystemInfo(&si);

	// 워커쓰레드 생성
	HANDLE hThread;
	for (int i = 0; i < (int)si.dwNumberOfProcessors; i++)
	{
		hThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&LanServer::WorkerThreadRun, this, NULL, NULL);
		hWorkerThreads.push_back(hThread);
	}
}

void LanServer::CreateMonitorThread()
{
	hMonitorThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&LanServer::MonitorThreadRun, this, NULL, NULL);

	return;
}

void LanServer::WorkerThreadRun(LPVOID* lParam)
{
	LanServer* self = (LanServer*)lParam;
	self->WorkerThread();
}

void LanServer::WorkerThread()
{
	while (1)
	{
		// GQCS() OUT파라메터 초기화
		DWORD cbTransferred = 0;
		Session* pSession = NULL;
		OVERLAPPED* pOverlapped;

		// GQCS()
		int retVal = GetQueuedCompletionStatus(hIOCP, &cbTransferred, (PULONG_PTR)&pSession, &pOverlapped, INFINITE);
		if (cbTransferred == NULL && pSession == NULL && pOverlapped == NULL)
		{
			printf("\n[TCP 서버] IOCP 워커쓰레드 종료...\n");
			PostQueuedCompletionStatus(hIOCP, NULL, NULL, NULL);
			return;
		}
		if (cbTransferred == 0)
		{
			// TODO: 연결종료를 위한 something

		}

		if (((MyOverlapped*)pOverlapped)->type == RECV)
		{
			pSession->recvQ.MoveRear(cbTransferred);

			while (1)
			{
				if (pSession->recvQ.GetUseSize() < sizeof(stHeader))
				{
					// WSARecv
					RequestWSARecv(pSession);
					break;
				}

				char headerBuf[100 + 1];

				pSession->recvQ.Peek(headerBuf, sizeof(stHeader));
				short payLoadLen = ((stHeader*)headerBuf)->len;

				if (pSession->recvQ.GetUseSize() < sizeof(stHeader) + payLoadLen)
				{
					// WSARecv
					RequestWSARecv(pSession);
					break;
				}

				InterlockedIncrement((LONG*)&recvMessageTPS);

				SerializePacket sPacket;
				int ret = pSession->recvQ.Dequeue(sPacket.GetBufferPtr(), sizeof(stHeader) + payLoadLen);
				sPacket.MoveWritePos(ret);

				OnMessage(pSession->sessionID, (SerializePacket*)((BYTE*)&sPacket + sizeof(stHeader)));


				/* 에코를 위한 작업
				* 일부러 직렬화버퍼를 따로 또 만들어서 sendPacket을 해보자.
				*/
				{
					SerializePacket sPacket2;

					stHeader tmpHeader;
					__int64 tmpPayload;

					sPacket.GetData((char*)&tmpHeader, sizeof(stHeader));
					sPacket >> tmpPayload;

					sPacket2 << tmpPayload;
						
					SendPacket(pSession->sessionID, &sPacket2);
				}

				// WSASend
				if (InterlockedExchange(&pSession->sendFlag, false) == true)
				{
					EnterCriticalSection(&pSession->cs);
					RequestWSASend(pSession);
					LeaveCriticalSection(&pSession->cs);
				}
			}
		}
		else if (((MyOverlapped*)pOverlapped)->type == SEND)
		{
			EnterCriticalSection(&pSession->cs);

			pSession->sendQ.MoveFront(cbTransferred);

			if (pSession->sendQ.GetUseSize() > 0)
			{
				RequestWSASend(pSession);
			}
			else
			{
				InterlockedExchange(&pSession->sendFlag, true);
			}

			LeaveCriticalSection(&pSession->cs);
		}

		if (InterlockedDecrement(&pSession->IOCount) == 0)
		{
			DestroySession(pSession);
		}

	}
	return;
}

void LanServer::AcceptThreadRun(LPVOID* lParam)
{
	LanServer* self = (LanServer*)lParam;
	self->AcceptThread();
}


void LanServer::AcceptThread()
{
	while (1)
	{
		// accept
		SOCKADDR_IN clientAddr;
		memset(&clientAddr, 0, sizeof(clientAddr));
		int addrLen = sizeof(clientAddr);

		SOCKET clientSocket = accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
		if (clientSocket == INVALID_SOCKET)
		{
			if (WSAGetLastError() == WSAEINTR)
			{
				printf("Accept Thread 종료...\n");
				return;
			}
			else
			{
				printf("Error: accept() %d\n", WSAGetLastError());
				return;
			}
		}

		// 콘텐츠에 이 주소 접속 허용할지 말지 허락받기
		if (!OnConnectionRequest(clientAddr.sin_addr, clientAddr.sin_port))
			continue;

		InterlockedIncrement((LONG*)&acceptTPS);
		InterlockedIncrement(&totalSessionCount);

		// 세션 생성
		Session* newSession = sessionPool.Alloc();

		newSession->sock = clientSocket;
		newSession->sessionID = g_SessionId++;
		newSession->recvQ.Resize(10000);
		newSession->sendQ.Resize(10000);

		memset(&newSession->recvOverlapped.overlapped, 0, sizeof(OVERLAPPED));
		newSession->recvOverlapped.pSession = newSession;
		newSession->recvOverlapped.type = RECV;

		memset(&newSession->sendOverlapped.overlapped, 0, sizeof(OVERLAPPED));
		newSession->sendOverlapped.pSession = newSession;
		newSession->sendOverlapped.type = SEND;

		newSession->sendFlag = true;

		InitializeCriticalSection(&newSession->cs);

		AcquireSRWLockExclusive(&sessionMapLock);
		sessionMap.insert({ newSession->sessionID, newSession });
		ReleaseSRWLockExclusive(&sessionMapLock);

		// 소켓 - IOCP 연결
		CreateIoCompletionPort((HANDLE)clientSocket, hIOCP, (ULONG_PTR)newSession, 0);

		// 콘텐츠에 "새로운 세션이 만들어졌습니다" 알리기
		OnAccept(clientAddr.sin_addr, clientAddr.sin_port, newSession->sessionID);

		// 비동기 RECV 걸어버리기
		//EnterCriticalSection(&newSession->cs);
		RequestWSARecv(newSession);
		//LeaveCriticalSection(&newSession->cs);
	}

	return;
}

void LanServer::MonitorThreadRun(LPVOID* lParam)
{
	LanServer* self = (LanServer*)lParam;
	self->MonitorThread();
}

void LanServer::MonitorThread()
{
	static DWORD oldTime = timeGetTime();

	while (1)
	{
		printf("acceptTPS : %d \n", InterlockedExchange((LONG*)&acceptTPS, 0));
		printf("recvMessageTPS : %d \n", InterlockedExchange((LONG*)&recvMessageTPS, 0));
		printf("sendMessageTPS : %d \n", InterlockedExchange((LONG*)&sendMessageTPS, 0));
		printf("\n");

		Sleep(1000 - (timeGetTime() - oldTime));

		oldTime = timeGetTime();
	}

	return;
}

bool LanServer::RequestWSARecv(Session* pSession)
{
	DWORD recvBytes;
	DWORD flags = 0;

	WSABUF wsaBuf;
	wsaBuf.buf = pSession->recvQ.GetRearBufferPtr();
	wsaBuf.len = pSession->recvQ.DirectEnqueueSize();

	InterlockedIncrement(&pSession->IOCount);
	wsaRecvRet = WSARecv(pSession->sock, &wsaBuf, 1, &recvBytes, &flags, (LPWSAOVERLAPPED)&pSession->recvOverlapped, NULL);
	if (wsaRecvRet == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			if (WSAGetLastError() != 0)
			{
				InterlockedDecrement(&pSession->IOCount);
			}

			if (WSAGetLastError() != WSAECONNRESET)
				printf("ERROR: WSARecv() %d\n", WSAGetLastError());

			// TODO: 연결종료를 위한 something

			return false;
		}
	}

	return true;
}

bool LanServer::RequestWSASend(Session* pSession)
{
	DWORD sendBytes;

	WSABUF wsaBuf;
	wsaBuf.buf = pSession->sendQ.GetFrontBufferPtr();
	wsaBuf.len = pSession->sendQ.DirectDequeueSize();

	InterlockedIncrement(&pSession->IOCount);
	wsaSendRet = WSASend(pSession->sock, &wsaBuf, 1, &sendBytes, 0, (LPWSAOVERLAPPED)&pSession->sendOverlapped, NULL);
	if (wsaSendRet == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			if (WSAGetLastError() != WSAECONNRESET)
			{
				printf("ERROR: WSASend() %d\n", WSAGetLastError());
			}

			InterlockedDecrement(&pSession->IOCount);


			return false;
		}
	}

	return true;
}

void LanServer::DestroySession(Session* pSession)
{
	AcquireSRWLockExclusive(&sessionMapLock);
	sessionMap.erase(pSession->sessionID);
	ReleaseSRWLockExclusive(&sessionMapLock);

	closesocket(pSession->sock);

	sessionPool.Free(pSession);

	// 콘텐츠에 "해당 세션이 삭제되었습니다" 알리기.
	OnRelease(pSession->sessionID);

	InterlockedDecrement(&totalSessionCount);
}
