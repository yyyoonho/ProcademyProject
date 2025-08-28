#include "stdafx.h"
#include "LanServer.h"
#include "EchoServer.h"

using namespace std;

bool CEchoServer::Start(const WCHAR* ipAddress, unsigned short port, unsigned short workerThreadCount, unsigned short coreSkip, bool isNagle, unsigned int maximumSessionCount)
{
	bool ret = CLanServer::Start(ipAddress, port, workerThreadCount, coreSkip, isNagle, maximumSessionCount);
	if (ret == false)
	{
		return false;
	}

	_contentQueue.Resize(20000);
	_hEvent_contentQueue = CreateEvent(NULL, FALSE, FALSE, NULL);

	InitializeSRWLock(&_contentQueueLock);

	_hEvent_Quit = CreateEvent(NULL, TRUE, FALSE, NULL);

	_hThread_EchoThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&CEchoServer::ContentThreadRun, this, NULL, NULL);

	return true;
}

void CEchoServer::Stop()
{
	CLanServer::Stop();

	SetEvent(_hEvent_Quit);
}

bool CEchoServer::OnConnectionRequest(SOCKADDR_IN clientAddr)
{
	return true;
}

void CEchoServer::OnAccept(DWORD64 sessionID)
{
}

void CEchoServer::OnRelease(DWORD64 sessionID)
{
}

void CEchoServer::OnMessage(DWORD64 sessionID, SerializePacket* pSPacket)
{
	AcquireSRWLockExclusive(&_contentQueueLock);

	_contentQueue.Enqueue((char*)&sessionID, sizeof(DWORD64));
	_contentQueue.Enqueue((char*)pSPacket->GetBufferPtr(), pSPacket->GetDataSize());

	SetEvent(_hEvent_contentQueue);

	ReleaseSRWLockExclusive(&_contentQueueLock);

	return;
}

void CEchoServer::OnError(int errorCode, WCHAR* errorComment)
{
}

void CEchoServer::ContentThreadRun(LPVOID* lParam)
{
	CEchoServer* self = (CEchoServer*)lParam;
	self->ContentThread();
}

void CEchoServer::ContentThread()
{
	HANDLE hEventArr[2] = { _hEvent_Quit, _hEvent_contentQueue };

	while (1)
	{
		DWORD ret = WaitForMultipleObjects(2, hEventArr, FALSE, INFINITE);
		if (ret == WAIT_OBJECT_0)
		{
			return;
		}
		
		// 데이터 추출
		DWORD64 sessionID;
		stMessage msg;

		AcquireSRWLockExclusive(&_contentQueueLock);

		_contentQueue.Dequeue((char*)&sessionID, sizeof(DWORD64));
		_contentQueue.Dequeue((char*)&msg, sizeof(stMessage));
		
		if (_contentQueue.GetUseSize() > 0)
		{
			SetEvent(_hEvent_contentQueue);
		}

		ReleaseSRWLockExclusive(&_contentQueueLock);


		// 직렬화버퍼에 데이터 삽입
		SerializePacket newSPacket;
		newSPacket.Putdata((char*)&msg, sizeof(stMessage));

		SendPacket(sessionID, &newSPacket);
	}
}
