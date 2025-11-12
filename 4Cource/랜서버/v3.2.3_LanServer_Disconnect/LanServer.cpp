#include "stdafx.h"
#include "LanServer.h"

using namespace std;

INT64 tmpA = 0;
LONG tmpBB = 0;

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
	//_workerThreadCount = 2;
	_coreSkip = coreSkip;
	_isNagle = isNagle;
	_maximumSessionCount = maximumSessionCount;

	_hEvent_Quit = CreateEvent(NULL, TRUE, FALSE, NULL);

	for (int i = 19999; i >= 0; i--)
	{
		_sessionArray[i].recvQ.Resize(20000);

		_releaseIdxLockFreeStack.Push(i);
	}

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
	for (int i = 0; i < _workerThreadCount; i++)
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

bool CLanServer::Disconnect(DWORD64 sessionID)
{
	// 세션 검색
	unsigned int idx = GetIdxFromSessionID(sessionID);
	Session* pSession = NULL;
	pSession = &_sessionArray[idx];

	// 세션 IO_Count ++ (=SessionRefCount의 역할)
	IncreaseIO_Count(pSession);

	// release flag 체크
	if (pSession->IOCountNReleaseCheck.releaseCheck == TRUE)
	{
		// 중단 - 이미 릴리즈된 세션입니다.
		DecreaseIO_Count(pSession);
		return false;
	}

	// 세션 ID체크 
	if (sessionID != pSession->sessionID)
	{
		// 중단 - 이미 IO_Count 증가 전에 릴리즈되서 재사용된 세션입니다.
		DecreaseIO_Count(pSession);
		return false;
	}

	// 세션 사용 부
	pSession->cancelIOCheck = TRUE;

	CancelIoEx((HANDLE)(pSession->sock), NULL);

	DecreaseIO_Count(pSession);

	return true;
}

void CLanServer::SendPost(Session* pSession)
{
	if (pSession->LockFreeSendQ.Size() > 0)
	{
		if (InterlockedExchange(&pSession->checkSend, FALSE) == TRUE)
		{
			// cancel IO 했는지 안했는지 검사.
			if (pSession->cancelIOCheck == TRUE)
				return;

			bool ret = SendProc(pSession);

			if (ret == TRUE && pSession->cancelIOCheck == TRUE)
			{
				Disconnect(pSession->sessionID);
			}
		}
	}
}

bool CLanServer::SendPacket(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	// 세션 검색
	unsigned int idx = GetIdxFromSessionID(sessionID);
	Session* pSession = NULL;
	pSession = &_sessionArray[idx];

	// 세션 IO_Count ++ (=SessionRefCount의 역할)
	IncreaseIO_Count(pSession);

	// release flag 체크
	if (pSession->IOCountNReleaseCheck.releaseCheck == TRUE)
	{
		// 중단 - 이미 릴리즈된 세션입니다.
		DecreaseIO_Count(pSession);
		return false;
	}

	// 세션 ID체크 
	if (sessionID != pSession->sessionID)
	{
		// 중단 - 이미 IO_Count 증가 전에 릴리즈되서 재사용된 세션입니다.
		DecreaseIO_Count(pSession);
		return false;
	}

	// 세션 사용 부

	stHeader header;
	header.len = pPacket.GetDataSize();
	pPacket.PushHeader((char*)&header, sizeof(stHeader));

	RawPtr packetRawPtr;
	pPacket.GetRawPtr(&packetRawPtr);

	packetRawPtr.IncreaseRefCount();
	pSession->LockFreeSendQ.Enqueue(packetRawPtr);

	InterlockedIncrement(&_sendMessageTPS);

	SendPost(pSession);

	DecreaseIO_Count(pSession);

	return true;
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

bool CLanServer::RecvProc(Session* pSession)
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
		if (WSAGetLastError() != ERROR_IO_PENDING && WSAGetLastError() != 0)
		{
			DecreaseIO_Count(pSession);

			if (WSAGetLastError() != 10054)
				printf("Error: WSARecv() %d\n", WSAGetLastError());

			return false;
		}
	}

	return true;
}

bool CLanServer::SendProc(Session* pSession)
{
	WSABUF wsaBuf[100];
	int sPacketCount = 0;
	for (int i = 0; i < 100; i++)
	{
		if (pSession->LockFreeSendQ.Size() <= 0)
			break;

		RawPtr packet;
		pSession->LockFreeSendQ.Dequeue(&packet);

		wsaBuf[i].buf = packet._ptr->GetBufferPtr();
		wsaBuf[i].len = packet._ptr->GetDataSize();

		pSession->sendMyOverlapped.sendSerializePacketPtrArr[i] = packet;

		sPacketCount++;
	}

	if (sPacketCount == 0)
	{
		InterlockedExchange(&pSession->checkSend, TRUE);
		return false;
	}
		
	pSession->sendMyOverlapped.sPacketCount = sPacketCount;

	IncreaseIO_Count(pSession);

	DWORD wsaSendRet = WSASend(pSession->sock, wsaBuf, sPacketCount, NULL, 0, (LPWSAOVERLAPPED)&pSession->sendMyOverlapped, NULL);
	if (wsaSendRet == SOCKET_ERROR)
	{
		DWORD tmp = WSAGetLastError();

		if (WSAGetLastError() != ERROR_IO_PENDING && WSAGetLastError() != 0)
		{
			DecreaseIO_Count(pSession);

			if (WSAGetLastError() != 10054)
				printf("Error: WSASend() %d\n", WSAGetLastError());

			return false;
		}
		else
		{
			int a = 3;
		}
	}

	return true;
}

void CLanServer::IncreaseIO_Count(Session* pSession)
{
	InterlockedIncrement(&(pSession->IOCountNReleaseCheck.IO_Count));
}

void CLanServer::DecreaseIO_Count(Session* pSession)
{
	LONG ret = InterlockedDecrement(&(pSession->IOCountNReleaseCheck.IO_Count));

	// Session Release
	if (ret == 0)
	{
		// releaseFlag 0 -> 1
		// IO_Count == 0 
		// 딱 위 두조건에만 실질적인 릴리즈 진행.

		IOReleasePair expected = { 0, FALSE };
		IOReleasePair desired = { 0, TRUE };
		
		LONG64 result = InterlockedCompareExchange64((LONG64*)&pSession->IOCountNReleaseCheck, *((LONG64*)&desired), *((LONG64*)&expected));
		IOReleasePair oldValue = *((IOReleasePair*)&result);
		if (oldValue.IO_Count != expected.IO_Count || oldValue.releaseCheck != expected.releaseCheck)
		{
			return;
		}

		closesocket(pSession->sock);
		
		unsigned int idx = GetIdxFromSessionID(pSession->sessionID);
		_releaseIdxLockFreeStack.Push(idx);

		InterlockedDecrement(&_sessionCount);

		OnRelease(pSession->sessionID);
	}
}

unsigned int CLanServer::GetIdxFromSessionID(DWORD64 sessionID)
{
	unsigned int idx = (sessionID & 0xffff000000000000) >> 48;

	return idx;
}

void CLanServer::SetIdxToSessionID(DWORD64* pSessionID, unsigned int idx)
{
	DWORD64 sessionID = *pSessionID;

	DWORD64 shiftedIdx = (DWORD64)idx << 48;

	*pSessionID = sessionID | shiftedIdx;

	return;
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

				if (pSession->recvQ.GetUseSize() < sizeof(stHeader))
				{
					// 비동기IO: Recv 요청 후 break
					
					// cancel IO 했는지 안했는지 검사.
					if (pSession->cancelIOCheck == TRUE)
						break;

					bool ret = RecvProc(pSession);

					if (ret == TRUE && pSession->cancelIOCheck == TRUE)
					{
						Disconnect(pSession->sessionID);
					}

					break;
				}

				pSession->recvQ.Peek((char*)&header, sizeof(stHeader));
				short payLoadLen = header.len;

				if (pSession->recvQ.GetUseSize() < sizeof(stHeader) + payLoadLen)
				{
					// 비동기IO: Recv 요청 후 break

					// cancel IO 했는지 안했는지 검사.
					if (pSession->cancelIOCheck == TRUE)
						break;

					bool ret = RecvProc(pSession);

					if (ret == TRUE && pSession->cancelIOCheck == TRUE)
					{
						Disconnect(pSession->sessionID);
					}

					break;
				}

				pSession->recvQ.MoveFront(sizeof(header));

				//SerializePacket sPacket;
				SerializePacketPtr pPacket = SerializePacketPtr::MakeSerializePacket();
				pPacket.Clear();

				int ret = pSession->recvQ.Dequeue(pPacket.GetBufferPtr(), payLoadLen);
				pPacket.MoveWritePos(ret);

				InterlockedIncrement(&_recvMessageTPS);

				// 비동기IO: Send 요청
				// 이제 컨텐츠 단에서 요청
				OnMessage(pSession->sessionID, pPacket);
			}
		}

		else if (pMyOverlapped->type == SEND)
		{
			for (int i = 0; i < pMyOverlapped->sPacketCount; i++)
			{
				pMyOverlapped->sendSerializePacketPtrArr[i].DecreaseRefCount();
				pMyOverlapped->sendSerializePacketPtrArr[i]._ptr = NULL;
			}

			// 비동기IO: Send 요청
			if (pSession->LockFreeSendQ.Size() > 0)
			{
				// cancel IO 했는지 안했는지 검사
				if (pSession->cancelIOCheck != TRUE)
				{
					bool ret = SendProc(pSession);

					if (ret == TRUE && pSession->cancelIOCheck == TRUE)
					{
						Disconnect(pSession->sessionID);
					}
				}
			}
			else
			{
				InterlockedExchange(&pSession->checkSend, TRUE);

				if (pSession->LockFreeSendQ.Size() > 0)
				{
					if (InterlockedExchange(&pSession->checkSend, FALSE) == TRUE)
					{
						// cancel IO 했는지 안했는지 검사.
						if (pSession->cancelIOCheck != TRUE)
						{
							bool ret = SendProc(pSession);

							if (ret == TRUE && pSession->cancelIOCheck == TRUE)
							{
								Disconnect(pSession->sessionID);
							}
						}
					}
				}
			}

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

		// 세션 할당 및 초기화
		unsigned int idx;
		_releaseIdxLockFreeStack.Pop(&idx);

		_sessionArray[idx].sock = clientSocket;
		_sessionArray[idx].sessionID = ++_g_sessionID;

		SetIdxToSessionID(&_sessionArray[idx].sessionID, idx);

		unsigned int tmp = GetIdxFromSessionID(_sessionArray[idx].sessionID);

		memset(&(_sessionArray[idx].recvMyOverlapped.overlapped), 0, sizeof(WSAOVERLAPPED));
		_sessionArray[idx].recvMyOverlapped.pSession = &_sessionArray[idx];
		_sessionArray[idx].recvMyOverlapped.type = RECV;

		memset(&(_sessionArray[idx].sendMyOverlapped.overlapped), 0, sizeof(WSAOVERLAPPED));
		_sessionArray[idx].sendMyOverlapped.pSession = &_sessionArray[idx];
		_sessionArray[idx].sendMyOverlapped.type = SEND;

		for (int i = 0; i < 100; i++)
		{
			if (_sessionArray[idx].sendMyOverlapped.sendSerializePacketPtrArr[i]._ptr != NULL)
				_sessionArray[idx].sendMyOverlapped.sendSerializePacketPtrArr[i].DecreaseRefCount();

			_sessionArray[idx].sendMyOverlapped.sendSerializePacketPtrArr[i]._ptr = NULL;
			_sessionArray[idx].sendMyOverlapped.sendSerializePacketPtrArr[i]._RCBPtr = NULL;
		}

		_sessionArray[idx].sendMyOverlapped.sPacketCount = 0;

		_sessionArray[idx].recvQ.ClearBuffer();
		if(_sessionArray[idx].LockFreeSendQ.Size() > 0)
		{
			_sessionArray[idx].LockFreeSendQ.Clear();
		}

		_sessionArray[idx].loginCheck = FALSE;

		_sessionArray[idx].cancelIOCheck = FALSE;

		_sessionArray[idx].checkSend = TRUE;

		//_sessionArray[idx].releaseCheck = FALSE;
		_sessionArray[idx].IOCountNReleaseCheck.releaseCheck = FALSE;

		getpeername(clientSocket, (SOCKADDR*)&clientAddr, &clientAddrSize);
		WCHAR addrBuf[40];
		InetNtop(AF_INET, &clientAddr.sin_addr, addrBuf, 40);
		//printf("\n[TCP 서버] 클라이언트 접속: IP주소=%ls, 포트번호=%d\n", addrBuf, ntohs(clientAddr.sin_port));

		InterlockedIncrement(&_sessionCount);

		// 소켓 <-> IOCP 연결
		CreateIoCompletionPort((HANDLE)_sessionArray[idx].sock, _hIOCP, (ULONG_PTR)&_sessionArray[idx], NULL);

		// OnAccept
		{
			IncreaseIO_Count(&_sessionArray[idx]);
			OnAccept(_sessionArray[idx].sessionID);
		}

		// 비동기 IO 걸어두기
		bool recvRet = RecvProc(&_sessionArray[idx]);

		// 로그인패킷을 send했지만, recv를 실패한 경우.
		if (recvRet == false)
		{
			DecreaseIO_Count(&_sessionArray[idx]);
		}
		else
		{
			DecreaseIO_Count(&_sessionArray[idx]);
			_sessionArray[idx].loginCheck = TRUE;
		}
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
