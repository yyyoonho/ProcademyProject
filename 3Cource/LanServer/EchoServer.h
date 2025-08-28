#pragma once

class CEchoServer : public CLanServer
{
public:
	virtual bool Start(const WCHAR* ipAddress,
		unsigned short port,
		unsigned short workerThreadCount,
		unsigned short coreSkip,
		bool isNagle,
		unsigned int maximumSessionCount) override;

	virtual void Stop() override;

public:
	virtual bool OnConnectionRequest(SOCKADDR_IN clientAddr) override;

	virtual void OnAccept(DWORD64 sessionID) override;

	virtual void OnRelease(DWORD64 sessionID) override;

	virtual void OnMessage(DWORD64 sessionID, SerializePacket* SPacket) override;

	virtual void OnError(int errorCode, WCHAR* errorComment) override;

private:
	static void ContentThreadRun(LPVOID* lParam);
	void ContentThread();

private:
	// ¸â¹ö º¯¼ö: ÄÜÅÙÃ÷ Å¥, ÀÌº¥Æ®
	RingBuffer _contentQueue;
	HANDLE _hEvent_contentQueue;

	SRWLOCK _contentQueueLock;

	HANDLE _hThread_EchoThread;

private:
	// ¸â¹öº¯¼ö: ÀÌº¥Æ®
	HANDLE _hEvent_Quit;
};