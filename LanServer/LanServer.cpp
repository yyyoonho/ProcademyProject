#include "stdafx.h"
#include "LanServerProtocol.h"

#include "LanServer.h"

using namespace std;

#define SERVERPORT 6000

LanServer::LanServer()
{

}

LanServer::~LanServer()
{
}

bool LanServer::Start()
{
	InitializeCriticalSection(&stackLock);

	InitializeSRWLock(&LogLock);

	InitSessionArray();

	NetInit();

	CreateAcceptThread();
	CreateIOCPWorkerThread();
	CreateMonitorThread();

	return false;
}

void LanServer::Stop()
{
	closesocket(listenSocket);
	PostQueuedCompletionStatus(hIOCP, NULL, NULL, NULL);
	exitMonitorThread = true;

	WaitForSingleObject(hAcceptThread, INFINITE);
	WaitForMultipleObjects(hWorkerThreads.size(), hWorkerThreads.data(), TRUE, INFINITE);
	WaitForSingleObject(hMonitorThread, INFINITE);

	return;
}

int LanServer::GetSessionCount()
{
	// TODO: 꼭 이렇게 해야할까?
	int a = totalSessionCount;
	return a;
}

bool LanServer::Disconnect(DWORD64 sessionID)
{
	Session* pSession = FindSessionByID(sessionID);
	if (pSession == NULL)
	{
		printf("Error: FindSessionByID() No targetSession in sessionArray\n");
		return false;
	}

	closesocket(pSession->sock);
	// 소켓넘버 재사용?이지않을까?

	pSession->sock = -1;	

	return true;
}

bool LanServer::SendPacket(DWORD64 sessionID, SerializePacket* sPacket)
{
	Session* pSession = FindSessionByID(sessionID);
	if (pSession == NULL)
	{
		printf("Error: FindSessionByID() No targetSession in sessionArray\n");
		return false;
	}

	InterlockedIncrement((LONG*)&sendMessageTPS);

	stHeader header;
	header.len = sPacket->GetDataSize();

	SerializePacket* headerSPacket = new SerializePacket;
	headerSPacket->Putdata((char*)&header, sizeof(stHeader));
	
	EnterCriticalSection(&pSession->sendQLock);
	pSession->sendQ.push(headerSPacket);
	pSession->sendQ.push(sPacket);
	LeaveCriticalSection(&pSession->sendQLock);

	return true;
}

int LanServer::GetAcceptTPS()
{
	return acceptTPS_Save;
}

int LanServer::GetRecvMessageTPS()
{
	return recvMessageTPS_Save;
}

int LanServer::GetSendMessageTPS()
{
	return sendMessageTPS_Save;
}

void LanServer::InitSessionArray()
{
	for (int i = MAXARR-1; i >=0 ; i--)
	{
		sessionArray[i] = new Session;

		sessionArray[i]->recvQ.Resize(10000);
		//sessionArray[i]->sendQ.Resize(10000);

		myStack.Push(i);
		sessionArray[i]->idx = i;
	}

	return;
}

void LanServer::NetInit()
{
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
	//for (int i = 0; i < 1; i++)
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

				sPacket.MoveReadPos(sizeof(stHeader));

				OnMessage(pSession->sessionID, &sPacket);

				// WSASend
				if (InterlockedExchange(&pSession->sendFlag, false) == true)
				{
					RequestWSASend(pSession);
				}
			}
		}
		else if (((MyOverlapped*)pOverlapped)->type == SEND)
		{
			for (int i = 0; i < ((MyOverlapped*)pOverlapped)->IOCompletionWaitCount; i++)
			{
				delete pSession->IOCompletionWaitArr[i];
			}
			((MyOverlapped*)pOverlapped)->IOCompletionWaitCount = 0;

			EnterCriticalSection(&pSession->sendQLock);
			int sendQSize = pSession->sendQ.size();
			LeaveCriticalSection(&pSession->sendQLock);

			if (sendQSize > 0)
			{
				RequestWSASend(pSession);
			}
			else
			{
				InterlockedExchange(&pSession->sendFlag, true);
			}
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
				printf("\n Accept Thread 종료...\n");
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
		unsigned int idx;
		if (!FindNonActiveSession(&idx))
		{
			printf("Non-Active Session이 없습니다.\n");
			return;
		}

		_InterlockedIncrement(&sessionCount);

		sessionArray[idx]->sock = clientSocket;

		sessionArray[idx]->sessionID = idx;
		sessionArray[idx]->sessionID = sessionArray[idx]->sessionID << 48;
		sessionArray[idx]->sessionID |= g_SessionId++;

		InitializeCriticalSection(&sessionArray[idx]->sendQLock);

		memset(&sessionArray[idx]->recvOverlapped.overlapped, 0, sizeof(OVERLAPPED));
		sessionArray[idx]->recvOverlapped.pSession = sessionArray[idx];
		sessionArray[idx]->recvOverlapped.type = RECV;

		memset(&sessionArray[idx]->sendOverlapped.overlapped, 0, sizeof(OVERLAPPED));
		sessionArray[idx]->sendOverlapped.pSession = sessionArray[idx];
		sessionArray[idx]->sendOverlapped.type = SEND;
		sessionArray[idx]->sendOverlapped.IOCompletionWaitCount = 0;

		sessionArray[idx]->sendFlag = true;

		sessionArray[idx]->active = true;

		// 소켓 - IOCP 연결
		CreateIoCompletionPort((HANDLE)clientSocket, hIOCP, (ULONG_PTR)sessionArray[idx], 0);

		// 콘텐츠에 "새로운 세션이 만들어졌습니다" 알리기
		OnAccept(clientAddr.sin_addr, clientAddr.sin_port, sessionArray[idx]->sessionID);

		// 비동기 RECV 걸어버리기
		RequestWSARecv(sessionArray[idx]);
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
		if (exitMonitorThread)
		{
			printf("\n[TCP 서버] 모니터 쓰레드 종료...\n");
			break;
		}

		printf("sessionCount:%d\n", sessionCount);

		acceptTPS_Save = InterlockedExchange((LONG*)&acceptTPS, 0);
		recvMessageTPS_Save = InterlockedExchange((LONG*)&recvMessageTPS, 0);
		sendMessageTPS_Save = InterlockedExchange((LONG*)&sendMessageTPS, 0);
		disconnetFromClient_Save = InterlockedExchange((LONG*)&disconnetFromClient, 0);

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

	WSABUF wsaBuf[MAXWSABUF];

	int count = 0;

	EnterCriticalSection(&pSession->sendQLock);
	while (!pSession->sendQ.empty())
	{
		if (count > MAXWSABUF)
			break;

		wsaBuf[count].buf = (char*)(pSession->sendQ.front()->GetBufferPtr());
		wsaBuf[count].len = pSession->sendQ.front()->GetDataSize();

		pSession->IOCompletionWaitArr[count] = pSession->sendQ.front();
		pSession->sendQ.pop();

		count++;
	}
	LeaveCriticalSection(&pSession->sendQLock);

	pSession->sendOverlapped.IOCompletionWaitCount = count;

	InterlockedIncrement(&pSession->IOCount);

	bool flag=true;

	wsaSendRet = WSASend(pSession->sock, wsaBuf, count, &sendBytes, 0, (LPWSAOVERLAPPED)&pSession->sendOverlapped, NULL);
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

	return flag;
}

void LanServer::DestroySession(Session* pSession)
{
	closesocket(pSession->sock);
	pSession->sock = -1;

	pSession->recvQ.ClearBuffer();
	//pSession->sendQ.ClearBuffer();

	pSession->active = false;

	// 콘텐츠에 "해당 세션이 삭제되었습니다" 알리기.
	OnRelease(pSession->sessionID);

	_InterlockedDecrement(&sessionCount);

	EnterCriticalSection(&stackLock);
	myStack.Push(pSession->idx);
	LeaveCriticalSection(&stackLock);

	InterlockedDecrement(&totalSessionCount);

	// TEST
	InterlockedIncrement((LONG*) & disconnetFromClient);
}

bool LanServer::FindNonActiveSession(OUT unsigned int* idx)
{
	bool flag = myStack.Empty();

	if (!flag)
	{
		EnterCriticalSection(&stackLock);

		int tmpIdx = myStack.Top();
		myStack.Pop();

		LeaveCriticalSection(&stackLock);

		*idx = tmpIdx;
		
		return true;
	}
	else
	{
		printf("스택이 비었습니다.\n");
		return false;
	}
}

Session* LanServer::FindSessionByID(DWORD64 sessionID)
{
	unsigned int idx = (sessionID & 0xffff000000000000) >> 48;

	return sessionArray[idx];
}