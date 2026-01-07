#include "stdafx.h"
#include "NetCodec.h"
#include "NetClient.h"

using namespace std;

CNetClient::CNetClient()
{
}

CNetClient::~CNetClient()
{
}


bool CNetClient::Connect(const WCHAR* ipAddress, unsigned short port, unsigned short workerThreadCount, unsigned short coreSkip, bool isNagle, bool codecOnOff, BYTE fixedKey, BYTE code)
{

	_ipAddress = ipAddress;
	_port = port;
	_workerThreadCount = workerThreadCount;
	_coreSkip = coreSkip;
	_isNagle = isNagle;
	_codecOnOff = codecOnOff;

	_netCodec = new NetCodec;
	_netCodec->SetFixedKey(fixedKey);
	_netCodec->SetCode(code);

	_hEvent_Quit = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (_hEvent_Quit == NULL)
	{
		printf("Handle 생성실패 : _hEvent_Quit\n");
		return false;
	}
	_hEvent_Connect = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (_hEvent_Connect == NULL)
	{
		printf("Handle 생성실패 : _hEvent_Connect\n");
		return false;
	}

	SYSTEM_INFO si;
	GetSystemInfo(&si);

	_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, (int)si.dwNumberOfProcessors - coreSkip);
	if (_hIOCP == NULL)
		return false;

	HANDLE hThread;
	for (int i = 0; i < _workerThreadCount; i++)
	{
		hThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&CNetClient::WorkerThreadRun, this, NULL, NULL);
		if (hThread == NULL)
			return false;
		_workerThreadHandles.push_back(hThread);
	}

	_hConnectThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&CNetClient::ConnectThreadRun, this, NULL, NULL);
	if (_hConnectThread == NULL)
		return false;

	bool ret = NetInit();
	if (ret == false)
		return false;

	SetEvent(_hEvent_Connect);

	return true;
}

void CNetClient::InitSession()
{

	memset(&(_session.recvMyOverlapped.overlapped), 0, sizeof(WSAOVERLAPPED));
	_session.recvMyOverlapped.pSession = &_session;
	_session.recvMyOverlapped.type = RECV;

	memset(&(_session.sendMyOverlapped.overlapped), 0, sizeof(WSAOVERLAPPED));
	_session.sendMyOverlapped.pSession = &_session;
	_session.sendMyOverlapped.type = SEND;

	for (int i = 0; i < 100; i++)
	{
		if (_session.sendMyOverlapped.sendSerializePacketPtrArr[i]._ptr != NULL)
			_session.sendMyOverlapped.sendSerializePacketPtrArr[i].DecreaseRefCount();

		_session.sendMyOverlapped.sendSerializePacketPtrArr[i]._ptr = NULL;
		_session.sendMyOverlapped.sendSerializePacketPtrArr[i]._RCBPtr = NULL;
	}

	_session.sendMyOverlapped.sPacketCount = 0;

	_session.recvQ.ClearBuffer();
	if (_session.LockFreeSendQ.Size() > 0)
	{
		while (1)
		{
			if (_session.LockFreeSendQ.Size() <= 0)
				break;

			RawPtr r;
			_session.LockFreeSendQ.Dequeue(&r);

			r.DecreaseRefCount();
		}

		_session.LockFreeSendQ.Clear();
	}

	_session.LockFreeSendQ.Clear();

	_session.recvQ.ClearBuffer();

	_session.loginCheck = FALSE;

	_session.checkSend = TRUE;

	_session.cancelIOCheck = FALSE;

	_session.IOCountNReleaseCheck.releaseCheck = FALSE;
}

bool CNetClient::Disconnect()
{
	// 세션 검색
	Session* pSession = &_session;

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
	//if (sessionID != pSession->sessionID)
	//{
	//	// 중단 - 이미 IO_Count 증가 전에 릴리즈되서 재사용된 세션입니다.
	//	DecreaseIO_Count(pSession);
	//	return false;
	//}

	// 세션 사용 부
	pSession->cancelIOCheck = TRUE;

	CancelIoEx((HANDLE)(pSession->sock), NULL);

	DecreaseIO_Count(pSession);

	return true;
}

bool CNetClient::SendPacket(SerializePacketPtr pPacket)
{
	// 세션 검색
	Session* pSession = &_session;

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
	//if (sessionID != pSession->sessionID)
	//{
	//	// 중단 - 이미 IO_Count 증가 전에 릴리즈되서 재사용된 세션입니다.
	//	DecreaseIO_Count(pSession);
	//	return false;
	//}

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
	pSession->LockFreeSendQ.Enqueue(packetRawPtr);

	//Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::SendMessageTPS);

	SendPost(pSession);

	DecreaseIO_Count(pSession);
	return true;
}

bool CNetClient::NetInit()
{
	WSADATA wsaData;
	wsaStartupRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wsaStartupRet != 0)
		return false;

	return true;
}

bool CNetClient::TryConnect(int retryCount)
{
	int ret = 0;

	for (int i = 0; i < retryCount; i++)
	{
		_session.sock = socket(AF_INET, SOCK_STREAM, 0);
		if (_session.sock == INVALID_SOCKET)
		{
			printf("Error: socket() %d\n", WSAGetLastError());
			return false;
		}

		SOCKADDR_IN serverAddr;
		memset(&serverAddr, 0, sizeof(SOCKADDR_IN));
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(_port);
		InetPton(AF_INET, _ipAddress, &serverAddr.sin_addr);

		LINGER linger;
		linger.l_onoff = 1;
		linger.l_linger = 0;
		setSockOptRet = setsockopt(_session.sock, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(LINGER));
		if (setSockOptRet == SOCKET_ERROR)
		{
			printf("ERROR: setsockopt() %d\n", GetLastError());
			return false;
		}

		int optVal = 0;
		setSockOptRet = setsockopt(_session.sock, SOL_SOCKET, SO_SNDBUF, (char*)&optVal, sizeof(optVal));
		if (setSockOptRet == SOCKET_ERROR)
		{
			printf("ERROR: setsockopt() %d\n", GetLastError());
			return false;
		}

		connectRet = connect(_session.sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
		if (connectRet == 0)
		{
			printf("Success: connect()\n");
			_session.sessionID = ++_uniqueSessionID;
			return true;
		}

		ret = WSAGetLastError();
		closesocket(_session.sock);
		_session.sock = INVALID_SOCKET;

		Sleep(1000);
	}

	printf("Error: connect() %d\n", ret);
	return false;
}

bool CNetClient::RecvProc(Session* pSession)
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
		DWORD tmp = WSAGetLastError();

		if (tmp != ERROR_IO_PENDING && tmp != 0)
		{
			DecreaseIO_Count(pSession);

			if (tmp != 10054 && tmp != 10038)
				printf("Error: WSARecv() %d\n", tmp);

			return false;
		}
	}

	return true;
}

bool CNetClient::SendProc(Session* pSession)
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

			if (tmp != 10054 && tmp != 10038)
				printf("Error: WSASend() %d\n", tmp);

			return false;
		}
	}

	return true;
}

void CNetClient::SendPost(Session* pSession)
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

void CNetClient::IncreaseIO_Count(Session* pSession)
{
	InterlockedIncrement(&(pSession->IOCountNReleaseCheck.IO_Count));
}

bool CNetClient::DecreaseIO_Count(Session* pSession)
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

void CNetClient::PQCS_Release(Session* pSession)
{
	PostQueuedCompletionStatus(_hIOCP, NULL, (ULONG_PTR)pSession, NULL);
}

void CNetClient::ReleaseProc(Session* pSession)
{
	closesocket(pSession->sock);

	//Monitoring::GetInstance()->DecreaseInterlocked(MonitorType::SessionNum);

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

	OnLeaveServer();

	// TODO: 재연결시도
	SetEvent(_hEvent_Connect);
}

void CNetClient::WorkerThreadRun(LPVOID* lParam)
{
	CNetClient* self = (CNetClient*)lParam;
	self->WorkerThread();
}

void CNetClient::WorkerThread()
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

		// ReleaseProc() 호출
		if (pMyOverlapped == &sendJobIOCP)
		{
			OnSendJob();
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
						CancelIoEx((HANDLE)(pSession->sock), NULL);
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
						CancelIoEx((HANDLE)(pSession->sock), NULL);
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
					bool decodingRet = _netCodec->DecodingPacket(pPacket, header);
					if (decodingRet == FALSE)
					{
						continue;
					}
				}

				//Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::RecvMessageTPS);
				// 컨텐츠에 메시지 전달
				OnMessage(pPacket);
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

void CNetClient::ConnectThreadRun(LPVOID* lParam)
{
	CNetClient* self = (CNetClient*)lParam;
	self->ConnectThread();
}

void CNetClient::ConnectThread()
{
	HANDLE hEvents[2] = { _hEvent_Quit , _hEvent_Connect };

	while (1)
	{
		DWORD ret = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
		if (ret == WAIT_OBJECT_0)
		{
			break;
		}

		IncreaseIO_Count(&_session);

		InitSession();

		if (TryConnect(5) == false)
		{
			printf("TryConnect failed: connect retry 5times...\n");
			printf("TryConnect failed: connect retry 5times...\n");
			printf("TryConnect failed: connect retry 5times...\n");
			printf("TryConnect failed: connect retry 5times...\n");
			printf("TryConnect failed: connect retry 5times...\n");

			continue;
		}

		// 소켓 <-> IOCP 연결
		CreateIoCompletionPort((HANDLE)_session.sock, _hIOCP, (ULONG_PTR)&_session, NULL);

		OnEnterJoin();

		// 비동기 IO 걸어두기
		bool recvRet = RecvProc(&_session);

		DecreaseIO_Count(&_session);
	}
}

void CNetClient::PQCSSendJob()
{
	PostQueuedCompletionStatus(_hIOCP, NULL, NULL, (LPOVERLAPPED)&sendJobIOCP);
}
