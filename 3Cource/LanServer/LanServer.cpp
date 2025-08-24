#include "stdafx.h"
#include "LanServer.h"
#include "Struct.h"

using namespace std;

CLanServer::CLanServer()
{
}

CLanServer::~CLanServer()
{
}

bool CLanServer::Start(const WCHAR* ipAddress, unsigned short port, unsigned short workerThreadCount, unsigned short coreSkip, bool isNagle, unsigned int maximumSessionCount)
{
	_ipAddress = ipAddress;
	_port = port;
	_workerThreadCount = workerThreadCount;
	_coreSkip = coreSkip;
	_isNagle = isNagle;
	_maximumSessionCount = maximumSessionCount;

	
	InitializeSRWLock(&_sessionMapLock);
	_hEvent_Quit = CreateEvent(NULL, TRUE, FALSE, NULL);


	bool ret = NetInit();
	if (ret == false)
		return false;

	_hThread_Accept = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&CLanServer::AcceptThreadRun, this, NULL, NULL);
	if (_hThread_Accept == NULL)
		return false;

	_hThread_Monitor = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&CLanServer::MonitorThreadRun, this, NULL, NULL);
	if (_hThread_Monitor == NULL)
		return false;

	SYSTEM_INFO si;
	GetSystemInfo(&si);

	_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, (int)si.dwNumberOfProcessors - coreSkip);
	if (_hIOCP == NULL)
		return false;

	HANDLE hThread;
	for (int i = 0; i < workerThreadCount; i++)
	{
		hThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&CLanServer::WorkerThreadRun, this, NULL, NULL);
		if (hThread == NULL)
			return false;
		_workerThreadHandles.push_back(hThread);
	}

	return true;
}

void CLanServer::Stop()
{
	SetEvent(_hEvent_Quit);
	PostQueuedCompletionStatus(_hIOCP, NULL, NULL, NULL);
}

int CLanServer::GetSessionCount()
{
	return _sessionCount;
}

bool CLanServer::Disconnect(INT64 sessionID)
{
	return false;
}

void CLanServer::SendPost(Session* pSession)
{
	if (pSession->sendQ.DirectDequeueSize() > 0)
	{
		if (InterlockedExchange(&pSession->checkSend, false) == true)
		{
			SendProc(pSession);
		}
	}
}

bool CLanServer::SendPacket(INT64 sessionID, SerializePacket* pSPacket)
{
	Session* pSession = NULL;

	AcquireSRWLockShared(&_sessionMapLock);

	unordered_map<INT64, Session*>::iterator iter;
	iter = _sessionMap.find(sessionID);

	if (iter == _sessionMap.end())
	{
		printf("Error: SendPacket() Can not find a session by ID(Key) in sessionMap\n");

		return false;
	}

	pSession = iter->second;

	AcquireSRWLockExclusive(&pSession->sessionLock);

	ReleaseSRWLockShared(&_sessionMapLock);

	stHeader header;
	header.len = pSPacket->GetDataSize();
	
	// TODO: 직렬화버퍼를 하나 더 선언하는걸 고치자.
	SerializePacket headerSPacket;
	headerSPacket.Putdata((char*)&header, sizeof(stHeader));

	InterlockedIncrement(&_sendMessageTPS);

	pSession->sendQ.Enqueue(headerSPacket.GetBufferPtr(), headerSPacket.GetDataSize());
	pSession->sendQ.Enqueue(pSPacket->GetBufferPtr(), pSPacket->GetDataSize());

	SendPost(pSession);

	ReleaseSRWLockExclusive(&pSession->sessionLock);

	return false;
}

bool CLanServer::NetInit()
{
	WSADATA wsaData;
	wsaStartupRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsaStartupRet != 0)
		return false;

	_listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenSocket == INVALID_SOCKET)
	{
		printf("Error: listen socket()\n");
		return false;
	}

	int optVal = 0;
	setSockOptRet = setsockopt(_listenSocket, SOL_SOCKET, SO_SNDBUF, (char*)&optVal, sizeof(optVal));
	if (setSockOptRet == SOCKET_ERROR)
	{
		printf("ERROR: setsockopt() %d\n", GetLastError());
		return false;
	}

	SOCKADDR_IN serverAddr;
	memset(&serverAddr, 0, sizeof(SOCKADDR_IN));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(_port);
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	bindRet = bind(_listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (bindRet == SOCKET_ERROR)
	{
		printf("Error: bind()\n");
		return false;
	}

	listenRet = listen(_listenSocket, SOMAXCONN);
	if (listenRet == SOCKET_ERROR)
	{
		printf("Error: listen()\n");
		return false;
	}
}

void CLanServer::RecvProc(Session* pSession)
{
	DWORD sendBytes;
	DWORD flags = 0;

	WSABUF wsaBuf;
	wsaBuf.buf = pSession->recvQ.GetRearBufferPtr();
	wsaBuf.len = pSession->recvQ.DirectEnqueueSize();

	IncreaseIO_Count(pSession);

	DWORD wsaRecvRet = WSARecv(pSession->sock, &wsaBuf, 1, &sendBytes, &flags, (LPWSAOVERLAPPED)&pSession->recvMyOverlapped, NULL);
	if (wsaRecvRet == SOCKET_ERROR)
	{
		if (WSAGetLastError() != ERROR_IO_PENDING)
		{
			if (WSAGetLastError() != 0)
			{
				DecreaseIO_Count(pSession);
			}

			printf("Error: WSARecv() %d\n", WSAGetLastError());
			return;
		}
	}
}

void CLanServer::SendProc(Session* pSession)
{
	if (pSession->sendQ.DirectDequeueSize() > 0)
	{
		DWORD sendBytes;

		WSABUF wsaBuf;
		wsaBuf.buf = pSession->sendQ.GetFrontBufferPtr();
		wsaBuf.len = pSession->sendQ.DirectDequeueSize();

		// TODO: 나중에 세션에 대한 락이 없어지면 이렇게 해결해야할듯 싶다.
		if (wsaBuf.len == 0)
		{
			DebugBreak();

			//InterlockedExchange(&pSession->checkSend, TRUE);
			//return;
		}

		IncreaseIO_Count(pSession);

		DWORD wsaSendRet = WSASend(pSession->sock, &wsaBuf, 1, &sendBytes, 0, (LPWSAOVERLAPPED)&pSession->sendMyOverlapped, NULL);
		DWORD tmp2 = WSAGetLastError();
		if (wsaSendRet == SOCKET_ERROR)
		{
			DWORD tmp = WSAGetLastError();

			if (WSAGetLastError() != ERROR_IO_PENDING && WSAGetLastError() != 0)
			{
				DecreaseIO_Count(pSession);

				printf("Error: WSASend() %d\n", WSAGetLastError());
				return;
			}
		}
	}
}

void CLanServer::IncreaseIO_Count(Session* pSession)
{
	InterlockedIncrement(&(pSession->IO_Count));
}

void CLanServer::DecreaseIO_Count(Session* pSession)
{
	LONG ret = InterlockedDecrement(&(pSession->IO_Count));
	if (ret == 0)
	{
		// Session Release

		AcquireSRWLockExclusive(&_sessionMapLock);

		_sessionMap.erase(pSession->sessionID);

		ReleaseSRWLockExclusive(&_sessionMapLock);

		AcquireSRWLockExclusive(&pSession->sessionLock);
		ReleaseSRWLockExclusive(&pSession->sessionLock);

		closesocket(pSession->sock);
		OnRelease(pSession->sessionID);

		delete pSession;
	}
}

void CLanServer::WorkerThreadRun(LPVOID* lParam)
{
	CLanServer* self = (CLanServer*)lParam;
	self->WorkerThread();
}

void CLanServer::WorkerThread()
{
	while (1)
	{
		// 비동기 IO 완료통지 대기
		DWORD cbTransferred = NULL;
		Session* pSession = NULL;
		myOverlapped* pMyOverlapped = NULL;

		SOCKADDR_IN clientAddr;
		int addrLen = sizeof(clientAddr);
		WCHAR addrBuf[40];

		BOOL retVal = GetQueuedCompletionStatus(_hIOCP, &cbTransferred, (PULONG_PTR)&pSession, (LPWSAOVERLAPPED*)&pMyOverlapped, INFINITE);
		if (pMyOverlapped == NULL && cbTransferred == NULL && pSession == NULL)
		{
			// 종료통지 or IOCP 결함
			// => 워커쓰레드 종료.
			PostQueuedCompletionStatus(_hIOCP, NULL, NULL, NULL);

			return;
		}

		if (cbTransferred == 0)
		{
			// 클라에서 FIN or RST 를 던졌다.
			getpeername(pSession->sock, (SOCKADDR*)&clientAddr, &addrLen);
			InetNtop(AF_INET, &clientAddr.sin_addr, addrBuf, 40);

			//printf("\n[TCP 서버] 클라이언트 종료신호 (FIN or SRT): IP주소=%ls, 포트번호=%d\n", addrBuf, ntohs(clientAddr.sin_port));
		}

		else if (pMyOverlapped->type == RECV)
		{
			pSession->recvQ.MoveRear(cbTransferred);

			while (1)
			{
				stHeader header;
				stMessage msg;

				if (pSession->recvQ.GetUseSize() < sizeof(stHeader))
				{
					// 비동기IO: Recv 요청 후 break
					RecvProc(pSession);
					break;
				}

				pSession->recvQ.Peek((char*)&header, sizeof(stHeader));
				short payLoadLen = header.len;

				if (pSession->recvQ.GetUseSize() < sizeof(stHeader) + payLoadLen)
				{
					// 비동기IO: Recv 요청 후 break
					RecvProc(pSession);
					break;
				}

				pSession->recvQ.MoveFront(sizeof(header));

				SerializePacket sPacket;
				int ret = pSession->recvQ.Dequeue(sPacket.GetBufferPtr(), payLoadLen);
				sPacket.MoveWritePos(ret);

				InterlockedIncrement(&_recvMessageTPS);

				// 비동기IO: Send 요청
				// 이제 컨텐츠 단에서 요청
				OnMessage(pSession->sessionID, &sPacket);
			}
		}

		else if (pMyOverlapped->type == SEND)
		{
			pSession->sendQ.MoveFront(cbTransferred);

			AcquireSRWLockExclusive(&pSession->sessionLock);

			// 비동기IO: Send 요청
			if (pSession->sendQ.DirectDequeueSize() > 0)
			{
				SendProc(pSession);
			}
			else
			{
				InterlockedExchange(&pSession->checkSend, TRUE);
			}


			ReleaseSRWLockExclusive(&pSession->sessionLock);
		}

		DecreaseIO_Count(pSession);
	}
}

void CLanServer::AcceptThreadRun(LPVOID* lParam)
{
	CLanServer* self = (CLanServer*)lParam;
	self->AcceptThread();
}

void CLanServer::AcceptThread()
{
	while (1)
	{
		SOCKADDR_IN clientAddr;
		memset(&clientAddr, 0, sizeof(SOCKADDR_IN));
		int clientAddrSize = sizeof(clientAddr);

		SOCKET clientSocket = accept(_listenSocket, (SOCKADDR*)&clientAddr, &clientAddrSize);
		if (clientSocket == INVALID_SOCKET)
		{
			printf("Error: accept() %d\n", WSAGetLastError());
			return;
		}

		InterlockedIncrement(&_acceptTPS);

		// OnConnectionRequest
		OnConnectionRequest(clientAddr);

		// 세션 생성 및 세팅
		Session* newSession = new Session;

		newSession->sock = clientSocket;
		newSession->sessionID = ++_g_sessionID;

		memset(&(newSession->recvMyOverlapped.overlapped), 0, sizeof(WSAOVERLAPPED));
		newSession->recvMyOverlapped.pSession = newSession;
		newSession->recvMyOverlapped.type = RECV;

		newSession->recvQ.Resize(20000);

		memset(&(newSession->sendMyOverlapped.overlapped), 0, sizeof(WSAOVERLAPPED));
		newSession->sendMyOverlapped.pSession = newSession;
		newSession->sendMyOverlapped.type = SEND;

		newSession->sendQ.Resize(20000);

		InitializeSRWLock(&newSession->sessionLock);

		AcquireSRWLockExclusive(&_sessionMapLock);
		_sessionMap.insert({ newSession->sessionID, newSession });
		ReleaseSRWLockExclusive(&_sessionMapLock);

		getpeername(clientSocket, (SOCKADDR*)&clientAddr, &clientAddrSize);
		WCHAR addrBuf[40];
		InetNtop(AF_INET, &clientAddr.sin_addr, addrBuf, 40);
		//printf("\n[TCP 서버] 클라이언트 접속: IP주소=%ls, 포트번호=%d\n", addrBuf, ntohs(clientAddr.sin_port));

		// 소켓 <-> IOCP 연결
		CreateIoCompletionPort((HANDLE)newSession->sock, _hIOCP, (ULONG_PTR)newSession, NULL);

		// OnAccept
		OnAccept(newSession->sessionID);

		// 비동기 IO 걸어두기
		RecvProc(newSession);
	}
}

void CLanServer::MonitorThreadRun(LPVOID* lParam)
{
	CLanServer* self = (CLanServer*)lParam;
	self->MonitorThread();
}

void CLanServer::MonitorThread()
{
	while (1)
	{
		DWORD ret = WaitForSingleObject(_hEvent_Quit, 1000);
		if (ret == WAIT_OBJECT_0)
		{
			return;
		}

		_acceptTPS_BackUp = _acceptTPS;
		_recvMessageTPS_BackUp = _recvMessageTPS;
		_sendMessageTPS_BackUp = _sendMessageTPS;

		InterlockedExchange(&_acceptTPS, 0);
		InterlockedExchange(&_recvMessageTPS, 0);
		InterlockedExchange(&_sendMessageTPS, 0);
	}
}

int CLanServer::GetAcceptTPS()
{
	return _acceptTPS_BackUp;
}

int CLanServer::GetRecvMessageTPS()
{
	return _recvMessageTPS_BackUp;
}

int CLanServer::GetSendMessageTPS()
{
	return _sendMessageTPS_BackUp;
}
