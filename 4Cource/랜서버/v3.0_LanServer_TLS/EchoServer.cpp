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

	for(int i=0; i< ECHOTHREADCOUNT; i++)
	{
		_contentQueue[i].Resize(90000);
		_hEvent_contentQueue[i] = CreateEvent(NULL, FALSE, FALSE, NULL);

		InitializeSRWLock(&_contentQueueLock[i]);
	}

	_hEvent_Quit = CreateEvent(NULL, TRUE, FALSE, NULL);
	
	for (int i = 0; i < ECHOTHREADCOUNT; i++)
	{
		_hThread_EchoThread[i] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&CEchoServer::ContentThreadRun, this, NULL, NULL);
		if (_hThread_EchoThread[i] == NULL)
			return false;

		threadQueueMap.insert({ GetThreadId(_hThread_EchoThread[i]), i });
	
	}

	return true;
}

void CEchoServer::Stop()
{
	SetEvent(_hEvent_Quit);
	CLanServer::Stop();
}

bool CEchoServer::OnConnectionRequest(SOCKADDR_IN clientAddr)
{
	return true;
}

void CEchoServer::OnAccept(DWORD64 sessionID)
{
	SerializePacketPtr pPacket = SerializePacketPtr::MakeSerializePacket();
	pPacket.Clear();

	stHeader header;
	stMessage loginMessage;

	header.len = sizeof(loginMessage);
	loginMessage.msg = 0x7fffffffffffffff;

	pPacket.PushHeader((char*)&header, sizeof(stHeader));
	pPacket.Putdata((char*)&loginMessage, sizeof(stMessage));

	SendPacket(sessionID, pPacket);
}

void CEchoServer::OnRelease(DWORD64 sessionID)
{
}

void CEchoServer::OnMessage(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	int threadNumber = sessionID % ECHOTHREADCOUNT;

	AcquireSRWLockExclusive(&_contentQueueLock[threadNumber]);

	_contentQueue[threadNumber].Enqueue((char*)&sessionID, sizeof(DWORD64));
	_contentQueue[threadNumber].Enqueue(pPacket.GetBufferPtr(), pPacket.GetDataSize());

	SetEvent(_hEvent_contentQueue[threadNumber]);

	ReleaseSRWLockExclusive(&_contentQueueLock[threadNumber]);

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
	DWORD idx = threadQueueMap[GetCurrentThreadId()];

	HANDLE hEventArr[2] = { _hEvent_Quit, _hEvent_contentQueue[idx]};

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

		AcquireSRWLockExclusive(&_contentQueueLock[idx]);

		_contentQueue[idx].Dequeue((char*)&sessionID, sizeof(DWORD64));
		_contentQueue[idx].Dequeue((char*)&msg, sizeof(stMessage));
		
		if (_contentQueue[idx].GetUseSize() > 0)
		{
			SetEvent(_hEvent_contentQueue[idx]);
		}

		ReleaseSRWLockExclusive(&_contentQueueLock[idx]);

		// 직렬화버퍼에 데이터 삽입
		SerializePacketPtr pPacket = SerializePacketPtr::MakeSerializePacket();
		pPacket.Clear();

		pPacket.Putdata((char*)&msg, sizeof(stMessage));

		SendPacket(sessionID, pPacket);
	}
}
