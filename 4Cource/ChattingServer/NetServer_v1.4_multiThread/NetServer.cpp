#include "stdafx.h"
#include "Monitoring.h"
#include "NetCodec.h"
#include "NetServer.h"

using namespace std;

CNetServer::CNetServer()
{
}

CNetServer::~CNetServer()
{
}

bool CNetServer::Start(const WCHAR* ipAddress, unsigned short port, unsigned short workerThreadCount, unsigned short coreSkip, bool isNagle, unsigned int maximumSessionCount, bool codecOnOff=true)
{
	_ipAddress = ipAddress;
	_port = port;
	_workerThreadCount = workerThreadCount;
	//_workerThreadCount = 2;
	_coreSkip = coreSkip;
	_isNagle = isNagle;
	_maximumSessionCount = maximumSessionCount;
	_codecOnOff = codecOnOff;

	_hEvent_Quit = CreateEvent(NULL, TRUE, FALSE, NULL);

	for (int i = 49999; i >= 0; i--)
	{
		_sessionArray[i].recvQ.Resize(50000);

		_releaseIdxLockFreeStack.Push(i);
	}

	bool ret = NetInit();
	if (ret == false)
		return false;

	_hThread_Accept = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&CNetServer::AcceptThreadRun, this, NULL, NULL);
	if (_hThread_Accept == NULL)
		return false;

	//_hThread_Monitor = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&CNetServer::MonitorThreadRun, this, NULL, NULL);
	//if (_hThread_Monitor == NULL)
	//	return false;

	SYSTEM_INFO si;
	GetSystemInfo(&si);

	_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, (int)si.dwNumberOfProcessors - coreSkip);
	if (_hIOCP == NULL)
		return false;

	HANDLE hThread;
	for (int i = 0; i < _workerThreadCount; i++)
	{
		hThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&CNetServer::WorkerThreadRun, this, NULL, NULL);
		if (hThread == NULL)
			return false;
		_workerThreadHandles.push_back(hThread);
	}

	return true;
}

void CNetServer::Stop()
{
	SetEvent(_hEvent_Quit);
	PostQueuedCompletionStatus(_hIOCP, NULL, NULL, NULL);
}

int CNetServer::GetSessionCount()
{
	return _sessionCount;
}

bool CNetServer::Disconnect(DWORD64 sessionID)
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

void CNetServer::SendPost(Session* pSession)
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

bool CNetServer::SendPacket(DWORD64 sessionID, SerializePacketPtr pPacket)
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

	// 인코딩 + Header넣기
	if (_codecOnOff == FALSE)
	{
		bool ret = pPacket.IsEncoded();

		if (!pPacket.IsEncoded())
		{
			JustPushHeader(pPacket);
			pPacket.MarkEncoded();	// -> 여기서는 header를 push하는걸 1회만 하기위해 체크하는 용도.
		}
	}
	else if (_codecOnOff == TRUE)
	{
		if (!pPacket.IsEncoded())
		{
			EncodingPacket(pPacket);
			pPacket.MarkEncoded();
		}
	}
	
	RawPtr packetRawPtr;
	pPacket.GetRawPtr(&packetRawPtr);

	packetRawPtr.IncreaseRefCount();
	pSession->LockFreeSendQ.Enqueue(packetRawPtr);

	//InterlockedIncrement(&_sendMessageTPS);
	Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::SendMessageTPS);

	SendPost(pSession);

	DecreaseIO_Count(pSession);
	return true;
}

bool CNetServer::NetInit()
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

	LINGER linger;
	linger.l_onoff = 1;
	linger.l_linger = 0;
	setSockOptRet = setsockopt(_listenSocket, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(LINGER));
	if (setSockOptRet == SOCKET_ERROR)
	{
		printf("ERROR: setsockopt() %d\n", GetLastError());
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

	listenRet = listen(_listenSocket, SOMAXCONN_HINT(65535));
	if (listenRet == SOCKET_ERROR)
	{
		printf("Error: listen()\n");
		return false;
	}

	return true;
}

bool CNetServer::RecvProc(Session* pSession)
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

			if (WSAGetLastError() != 10054 && WSAGetLastError() != 10038)
				printf("Error: WSARecv() %d\n", WSAGetLastError());

			return false;
		}
	}

	return true;
}

bool CNetServer::SendProc(Session* pSession)
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
			for (int i = 0; i < sPacketCount; i++)
			{
				pSession->sendMyOverlapped.sendSerializePacketPtrArr[i].DecreaseRefCount();
				pSession->sendMyOverlapped.sendSerializePacketPtrArr[i]._ptr = NULL;
			}

			DecreaseIO_Count(pSession);

			if (WSAGetLastError() != 10054 && WSAGetLastError() != 10038)
				printf("Error: WSASend() %d\n", WSAGetLastError());

			return false;
		}
	}

	return true;
}

void CNetServer::IncreaseIO_Count(Session* pSession)
{
	InterlockedIncrement(&(pSession->IOCountNReleaseCheck.IO_Count));
}

void CNetServer::DecreaseIO_Count(Session* pSession)
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

		PQCS_Release(pSession);
		//ReleaseProc(pSession);
	}
}

void CNetServer::PQCS_Release(Session* pSession)
{
	PostQueuedCompletionStatus(_hIOCP, NULL, (ULONG_PTR)pSession, NULL);
}

void CNetServer::ReleaseProc(Session* pSession)
{
	closesocket(pSession->sock);

	Monitoring::GetInstance()->DecreaseInterlocked(MonitorType::SessionNum);

	// TODO: send링버퍼 돌기, 오버랩구조체 돌기
	{
		while (1)
		{
			if (pSession->LockFreeSendQ.Size() <= 0)
				break;

			RawPtr r;
			pSession->LockFreeSendQ.Dequeue(&r);
			r.DecreaseRefCount();
		}

		for (int i = 0; i < pSession->sendMyOverlapped.sPacketCount; i++)
		{
			if (pSession->sendMyOverlapped.sendSerializePacketPtrArr[i]._ptr == NULL)
				continue;

			pSession->sendMyOverlapped.sendSerializePacketPtrArr[i].DecreaseRefCount();
			pSession->sendMyOverlapped.sendSerializePacketPtrArr[i]._ptr = NULL;
		}
	}

	OnRelease(pSession->sessionID);

	unsigned int idx = GetIdxFromSessionID(pSession->sessionID);
	_releaseIdxLockFreeStack.Push(idx);
}

unsigned int CNetServer::GetIdxFromSessionID(DWORD64 sessionID)
{
	unsigned int idx = (sessionID & 0xffff000000000000) >> 48;

	return idx;
}

void CNetServer::SetIdxToSessionID(DWORD64* pSessionID, unsigned int idx)
{
	DWORD64 sessionID = *pSessionID;

	DWORD64 shiftedIdx = (DWORD64)idx << 48;

	*pSessionID = sessionID | shiftedIdx;

	return;
}

void CNetServer::WorkerThreadRun(LPVOID* lParam)
{
	CNetServer* self = (CNetServer*)lParam;
	self->WorkerThread();
}

void CNetServer::WorkerThread()
{
	while (1)
	{
		// 비동기 IO 완료통지 대기
		DWORD cbTransferred = NULL;
		Session* pSession = NULL;
		myOverlapped* pMyOverlapped = NULL;

		SOCKADDR_IN clientAddr;
		int addrLen = sizeof(clientAddr);

		BOOL retVal = GetQueuedCompletionStatus(_hIOCP, &cbTransferred, (PULONG_PTR)&pSession, (LPWSAOVERLAPPED*)&pMyOverlapped, INFINITE);
		if (pMyOverlapped == NULL && cbTransferred == NULL && pSession == NULL)
		{
			// 종료통지 or IOCP 결함
			// => 워커쓰레드 종료.
			PostQueuedCompletionStatus(_hIOCP, NULL, NULL, NULL);

			return;
		}


		// TODO: ReleaseProc() 호출
		if (pMyOverlapped == NULL && pSession != NULL)
		{
			ReleaseProc(pSession);
			continue;
		}

		if (cbTransferred == 0)
		{
			// 클라에서 FIN or RST 를 던졌다.
			// 뭐.. 할수있느건 없다. 그저 DecreaseIO를 할 뿐
		}

		else if (pMyOverlapped->type == RECV)
		{
			pSession->recvQ.MoveRear(cbTransferred);

			while (1)
			{
				stNetHeader header;

				if (pSession->recvQ.GetUseSize() < sizeof(stNetHeader))
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

				pSession->recvQ.Peek((char*)&header, sizeof(stNetHeader));
				unsigned short payloadLen;
				memcpy_s(&payloadLen, sizeof(BYTE) * 2, header.len, sizeof(BYTE) * 2);

				if (pSession->recvQ.GetUseSize() < sizeof(stNetHeader) + payloadLen)
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

				SerializePacketPtr pPacket = SerializePacketPtr::MakeSerializePacket();
				pPacket.Clear();

				int ret = pSession->recvQ.Dequeue(pPacket.GetBufferPtr(), payloadLen);
				pPacket.MoveWritePos(ret);

				// 디코딩
				if (_codecOnOff == TRUE)
				{
					bool decodingRet = DecodingPacket(pPacket, header);
					if (decodingRet == FALSE)
					{
						continue;
					}
				}

				Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::RecvMessageTPS);
				// 컨텐츠에 메시지 전달
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

void CNetServer::AcceptThreadRun(LPVOID* lParam)
{
	CNetServer* self = (CNetServer*)lParam;
	self->AcceptThread();
}

void CNetServer::AcceptThread()
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

		//InterlockedIncrement(&_acceptTPS);
		Monitoring::GetInstance()->Increase(MonitorType::AcceptTotal);
		Monitoring::GetInstance()->Increase(MonitorType::AcceptTPS);

		// OnConnectionRequest
		OnConnectionRequest(clientAddr);

		// 세션 할당 및 초기화
		unsigned int idx;
		_releaseIdxLockFreeStack.Pop(&idx);

		// 디버깅
		IncreaseIO_Count(&_sessionArray[idx]);

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
			while (1)
			{
				if (_sessionArray[idx].LockFreeSendQ.Size() <= 0)
					break;

				RawPtr r;
				_sessionArray[idx].LockFreeSendQ.Dequeue(&r);
				r.DecreaseRefCount();
			}

			_sessionArray[idx].LockFreeSendQ.Clear();
		}

		_sessionArray[idx].loginCheck = FALSE;

		_sessionArray[idx].checkSend = TRUE;

		//_sessionArray[idx].releaseCheck = FALSE;
		_sessionArray[idx].IOCountNReleaseCheck.releaseCheck = FALSE;

		getpeername(clientSocket, (SOCKADDR*)&clientAddr, &clientAddrSize);
		WCHAR addrBuf[40];
		InetNtop(AF_INET, &clientAddr.sin_addr, addrBuf, 40);

		Monitoring::GetInstance()->Increase(MonitorType::SessionNum);

		// 소켓 <-> IOCP 연결
		CreateIoCompletionPort((HANDLE)_sessionArray[idx].sock, _hIOCP, (ULONG_PTR)&_sessionArray[idx], NULL);

		// OnAccept
		{
			//IncreaseIO_Count(&_sessionArray[idx]);
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

void CNetServer::MonitorThreadRun(LPVOID* lParam)
{
	CNetServer* self = (CNetServer*)lParam;
	self->MonitorThread();
}

void CNetServer::MonitorThread()
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

int CNetServer::GetAcceptTPS()
{
	return _acceptTPS_BackUp;
}

int CNetServer::GetRecvMessageTPS()
{
	return _recvMessageTPS_BackUp;
}

int CNetServer::GetSendMessageTPS()
{
	return _sendMessageTPS_BackUp;
}
