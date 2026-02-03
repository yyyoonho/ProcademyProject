#include "stdafx.h"

#include "LogManager.h"
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

bool CNetServer::Start(const WCHAR* ipAddress, unsigned short port, unsigned short workerThreadCount, unsigned short coreSkip, bool isNagle, unsigned int maximumSessionCount, bool codecOnOff, BYTE fixedKey, BYTE code)
{
	_ipAddress = ipAddress;
	_port = port;
	_workerThreadCount = workerThreadCount;
	_coreSkip = coreSkip;
	_isNagle = isNagle;
	_maximumSessionCount = maximumSessionCount;
	_codecOnOff = codecOnOff;

	_hEvent_Quit = CreateEvent(NULL, TRUE, FALSE, NULL);

	//for (int i = 19999; i >= 0; i--)
	//for (int i = 24999; i >= 0; i--)
	for (int i = 9999; i >= 0; i--)
	{
		_sessionArray[i].recvQ.Resize(2000);

		_releaseIdxLockFreeStack.Push(i);
	}

	bool ret = NetInit();
	if (ret == false)
		return false;

	_netCodec = new NetCodec;
	_netCodec->SetFixedKey(fixedKey);
	_netCodec->SetCode(code);

	_hThread_Accept = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&CNetServer::AcceptThreadRun, this, NULL, NULL);
	if (_hThread_Accept == NULL)
		return false;

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

	// NotifyDisconnect
	NotifyDisconnect(pSession);

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
				//Disconnect(pSession->sessionID);
				CancelIoEx((HANDLE)(pSession->sock), NULL);
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
			_netCodec->JustPushHeader(pPacket);
			pPacket.MarkEncoded();	// -> 여기서는 header를 push하는걸 1회만 하기위해 체크하는 용도.
		}
	}
	else if (_codecOnOff == TRUE)
	{
		if (!pPacket.IsEncoded())
		{
			_netCodec->EncodingPacket(pPacket);
			pPacket.MarkEncoded();
		}
	}
	
	RawPtr packetRawPtr;
	pPacket.GetRawPtr(&packetRawPtr);

	packetRawPtr.IncreaseRefCount();
	bool ret = pSession->LockFreeSendQ.Enqueue(packetRawPtr);

	// attack #11
	if (ret == false)
	{
		Disconnect(sessionID);
		_LOG(dfLOG_LEVEL_SYSTEM, L"%ls\n", L"attack #11 Disconnect");
		return false;
	}

	Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::SendMessageTPS);

	SendPost(pSession);

	DecreaseIO_Count(pSession);
	return true;
}

void CNetServer::PQCS_Disconnect(DWORD64 sessionID)
{
	PostQueuedCompletionStatus(_hIOCP, NULL, (ULONG_PTR)sessionID, LPOVERLAPPED(&disconnectReqToIOCP));
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

	// attack #8 테스트
	int tmp = pSession->recvQ.GetFreeSize();
	if (pSession->recvQ.GetFreeSize() == 0)
	{
		_LOG(dfLOG_LEVEL_SYSTEM, L"%ls\n", L"attack #8 Disconnect");
		return false;
	}

	IncreaseIO_Count(pSession);

	DWORD wsaRecvRet = WSARecv(pSession->sock, &wsaBuf, 1, &sendBytes, &flags, (LPWSAOVERLAPPED)&pSession->recvMyOverlapped, NULL);
	if (wsaRecvRet == SOCKET_ERROR)
	{
		DWORD tmp = WSAGetLastError();

		if (tmp != ERROR_IO_PENDING && tmp != 0)
		{
			DecreaseIO_Count(pSession);

			// NotifyDisconnect
			NotifyDisconnect(pSession);

			if (tmp != 10054 && tmp != 10038)
				printf("Error: WSARecv() %d\n", tmp);

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

		if (tmp != ERROR_IO_PENDING && tmp != 0)
		{
			for (int i = 0; i < sPacketCount; i++)
			{
				pSession->sendMyOverlapped.sendSerializePacketPtrArr[i].DecreaseRefCount();
				pSession->sendMyOverlapped.sendSerializePacketPtrArr[i]._ptr = NULL;
			}

			DecreaseIO_Count(pSession);

			// NotifyDisconnect
			NotifyDisconnect(pSession);

			if (tmp != 10054 && tmp != 10038)
				printf("Error: WSASend() %d\n", tmp);

			return false;
		}
	}

	return true;
}

void CNetServer::IncreaseIO_Count(Session* pSession)
{
	InterlockedIncrement(&(pSession->IOCountNReleaseCheck.IO_Count));
}

bool CNetServer::DecreaseIO_Count(Session* pSession)
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
			return false;
		}

		PQCS_Release(pSession);
		return true;
	}

	return false;
}

void CNetServer::PQCS_Release(Session* pSession)
{
	PostQueuedCompletionStatus(_hIOCP, NULL, (ULONG_PTR)pSession, LPOVERLAPPED(&releaseReqToIOCP));
}

void CNetServer::ReleaseProc(Session* pSession)
{
	closesocket(pSession->sock);

	Monitoring::GetInstance()->DecreaseInterlocked(MonitorType::SessionNum);

	// send링버퍼 돌기, 오버랩구조체 돌기
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

	pSession->recvQ.ClearBuffer();

	while (1)
	{
		if (pSession->contentMsgQ.GetUseSize() <= 0)
			break;

		RawPtr r;
		pSession->contentMsgQ.Dequeue((char*)&r, sizeof(RawPtr));

		r.DecreaseRefCount();
	}
	pSession->contentMsgQ.ClearBuffer();

	OnRelease(pSession->sessionID);
	OnRelease_GameManager(pSession->sessionID, pSession);

	unsigned int idx = GetIdxFromSessionID(pSession->sessionID);
	_releaseIdxLockFreeStack.Push(idx);
}

void CNetServer::NotifyDisconnect(Session* pSession)
{
	// 중복 방지
	if (InterlockedExchange(&pSession->disconnectNotified, TRUE) == TRUE)
		return;

	// GameManager에 알림
	OnRelease_GameManager(pSession->sessionID, pSession);
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
	LONG ret = Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::ActiveWorkerTh);

	while (1)
	{
		// 비동기 IO 완료통지 대기
		DWORD cbTransferred = NULL;
		Session* pSession = NULL;
		myOverlapped* pMyOverlapped = NULL;

		SOCKADDR_IN clientAddr;
		int addrLen = sizeof(clientAddr);

		Monitoring::GetInstance()->DecreaseInterlocked(MonitorType::ActiveWorkerTh);

		BOOL retVal = GetQueuedCompletionStatus(_hIOCP, &cbTransferred, (PULONG_PTR)&pSession, (LPWSAOVERLAPPED*)&pMyOverlapped, INFINITE);
		if (pMyOverlapped == NULL && cbTransferred == NULL && pSession == NULL)
		{
			// 종료통지 or IOCP 결함
			// => 워커쓰레드 종료.
			PostQueuedCompletionStatus(_hIOCP, NULL, NULL, NULL);

			return;
		}

		Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::ActiveWorkerTh);

		// TODO: ReleaseProc() 호출
		if (pMyOverlapped == &releaseReqToIOCP)
		{
			ReleaseProc(pSession);
			continue;
		}

		if (cbTransferred == 0 && pMyOverlapped == NULL)
		{
			// 클라에서 FIN or RST 를 던졌다.
			// 뭐.. 할수있느건 없다. 그저 DecreaseIO를 할 뿐
			int a = 3;

			// NotifyDisconnect
			NotifyDisconnect(pSession);
		}

		if (pMyOverlapped == &disconnectReqToIOCP)
		{
			DWORD64 sessionID = (DWORD64)pSession;

			Disconnect(sessionID);
			continue;
		}

		else if (pMyOverlapped->type == RECV)
		{
			pSession->recvQ.MoveRear(cbTransferred);

			while (1)
			{
				stNetHeader header;

				// 헤더 만큼도 안들어와 있는 경우.
				int tmp = pSession->recvQ.GetUseSize();
				if (pSession->recvQ.GetUseSize() < sizeof(stNetHeader))
				{
					// 비동기IO: Recv 요청 후 break
					// cancel IO 했는지 안했는지 검사.
					if (pSession->cancelIOCheck == TRUE)
						break;

					bool ret = RecvProc(pSession);
					if (ret == TRUE && pSession->cancelIOCheck == TRUE)
					{
						CancelIoEx((HANDLE)(pSession->sock), NULL);
					}
					//if (ret == false)
					//{
					//	Disconnect(pSession->sessionID);
					//	_LOG(dfLOG_LEVEL_SYSTEM, L"%ls\n", L"attack #8 Disconnect");
					//}


					break;
				}

				pSession->recvQ.Peek((char*)&header, sizeof(stNetHeader));
				unsigned short payloadLen;
				memcpy_s(&payloadLen, sizeof(BYTE) * 2, header.len, sizeof(BYTE) * 2);


				// 헤더에 기입된 len만큼 페이로드가 안들어온 경우.
				tmp = pSession->recvQ.GetUseSize();
				if (pSession->recvQ.GetUseSize() < sizeof(stNetHeader) + payloadLen)
				{
					// 비동기IO: Recv 요청 후 break
					// cancel IO 했는지 안했는지 검사.
					if (pSession->cancelIOCheck == TRUE)
						break;

					bool ret = RecvProc(pSession);
					if (ret == TRUE && pSession->cancelIOCheck == TRUE)
					{
						CancelIoEx((HANDLE)(pSession->sock), NULL);
					}
					//if (ret == false)
					//{
					//	Disconnect(pSession->sessionID);
					//	_LOG(dfLOG_LEVEL_SYSTEM, L"%ls\n", L"attack #8 Disconnect");
					//}


					break;
				}

				pSession->recvQ.MoveFront(sizeof(header));

				// attack #1 - code가 다른 패킷이 왔을 경우 킥. O
				{
					BYTE packetCode = header.code;
					if (_netCodec->isValidCode(packetCode) == false)
					{
						Disconnect(pSession->sessionID);
						_LOG(dfLOG_LEVEL_SYSTEM, L"%ls\n", L"attack #1 Disconnect");
						break;
					}
				}

				// attack #9
				// 직렬화패킷의 기본 버퍼사이즈는 1400
				// 그 이상 pPacket에 넣으려고 하면 오버런 가능성O
				if (payloadLen > Net_SerializePacket::eBUFFER_DEFAULT)
				{
					Disconnect(pSession->sessionID);
					_LOG(dfLOG_LEVEL_SYSTEM, L"%ls\n", L"attack #9 Disconnect");
					break;
				}

				SerializePacketPtr pPacket = SerializePacketPtr::MakeSerializePacket();
				pPacket.Clear();

				int ret = pSession->recvQ.Dequeue(pPacket.GetBufferPtr(), payloadLen);
				pPacket.MoveWritePos(ret);

				// 디코딩
				// attack #7 O
				if (_codecOnOff == TRUE)
				{
					bool decodingRet = _netCodec->DecodingPacket(pPacket, header);
					if (decodingRet == FALSE)
					{
						Disconnect(pSession->sessionID);
						_LOG(dfLOG_LEVEL_SYSTEM, L"%ls\n", L"attack #7 Disconnect");
						break;
					}
				}

				Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::RecvMessageTPS);

				// 컨텐츠에 메시지 전달
				OnMessage(pSession->sessionID, pPacket);
				OnMessage_GameManager(pSession->sessionID, pSession, pPacket);
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
						CancelIoEx((HANDLE)(pSession->sock), NULL);
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
								CancelIoEx((HANDLE)(pSession->sock), NULL);
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

		Monitoring::GetInstance()->Increase(MonitorType::AcceptTotal);
		Monitoring::GetInstance()->Increase(MonitorType::AcceptTPS);

		// OnConnectionRequest
		OnConnectionRequest(clientAddr);

		// TODO: 여기서 _maximumSessionCount 를 체크후, 초과 시 바로 disconnect
		// attack #5
		{
			LONG sessionCount = Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::SessionNum);
			if ((unsigned int)sessionCount > _maximumSessionCount)
			{
				closesocket(clientSocket);
				Monitoring::GetInstance()->DecreaseInterlocked(MonitorType::SessionNum);
				continue;
			}
		}

		

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
		if (_sessionArray[idx].LockFreeSendQ.Size() > 0)
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

		_sessionArray[idx].LockFreeSendQ.Clear();

		_sessionArray[idx].recvQ.ClearBuffer();

		_sessionArray[idx].loginCheck = FALSE;

		_sessionArray[idx].checkSend = TRUE;

		_sessionArray[idx].cancelIOCheck = FALSE;

		_sessionArray[idx].IOCountNReleaseCheck.releaseCheck = FALSE;

		_sessionArray[idx].disconnectNotified = FALSE;


		// Game용 변수 초기화
		{
			while (1)
			{
				if (_sessionArray[idx].contentMsgQ.GetUseSize() <= 0)
					break;

				RawPtr r;
				_sessionArray[idx].contentMsgQ.Dequeue((char*)&r, sizeof(RawPtr));

				r.DecreaseRefCount();
			}
			_sessionArray[idx].contentMsgQ.ClearBuffer();
			_sessionArray[idx].releaseWait = false;
		}

		getpeername(clientSocket, (SOCKADDR*)&clientAddr, &clientAddrSize);
		WCHAR addrBuf[40];
		InetNtop(AF_INET, &clientAddr.sin_addr, addrBuf, 40);

		//Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::SessionNum);

		// 소켓 <-> IOCP 연결
		CreateIoCompletionPort((HANDLE)_sessionArray[idx].sock, _hIOCP, (ULONG_PTR)&_sessionArray[idx], NULL);

		// OnAccept
		{
			OnAccept(_sessionArray[idx].sessionID);
			OnAccept_GameManager(_sessionArray[idx].sessionID, &_sessionArray[idx]);
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

void CNetServer::OnAccept_GameManager(DWORD64 sessionID, Session* pSession)
{
}

void CNetServer::OnMessage_GameManager(DWORD64 sessionID, Session* pSession, SerializePacketPtr pPacket)
{
}

void CNetServer::OnRelease_GameManager(DWORD64 sessionID, Session* pSession)
{
}
