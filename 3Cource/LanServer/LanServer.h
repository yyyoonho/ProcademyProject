#pragma once

class Session;

class CLanServer
{
public:
	CLanServer();
	virtual ~CLanServer();

public:
	virtual bool Start(
		const WCHAR* ipAddress, 
		unsigned short port, 
		unsigned short workerThreadCount, 
		unsigned short coreSkip, 
		bool isNagle, 
		unsigned int maximumSessionCount);

	virtual void Stop();
	int GetSessionCount();

	bool Disconnect(DWORD64 sessionID);

	bool SendPacket(DWORD64 sessionID, SerializePacket* pSPacket);
	
private:
	// 클래스 내부 함수
	bool NetInit();

	void RecvProc(Session* pSession);
	void SendProc(Session* pSession);
	void SendPost(Session* pSession);

	void IncreaseIO_Count(Session* pSession);
	void DecreaseIO_Count(Session* pSession);

	unsigned int GetIdxFromSessionID(DWORD64 sessionID);
	void SetIdxToSessionID(DWORD64* pSessionID, unsigned int idx);

private:
	// 쓰레드 함수
	static void WorkerThreadRun(LPVOID* lParam);
	void WorkerThread();
	static void AcceptThreadRun(LPVOID* lParam);
	void AcceptThread();
	static void MonitorThreadRun(LPVOID* lParam);
	void MonitorThread();
	
public:
	// 핸들링 함수
	virtual bool OnConnectionRequest(SOCKADDR_IN clientAddr) = 0;
	virtual void OnAccept(DWORD64 sessionID) = 0;
	virtual void OnRelease(DWORD64 sessionID) = 0;
	virtual void OnMessage(DWORD64 sessionID, SerializePacket* pSPacket) = 0;
	virtual void OnError(int errorCode, WCHAR* errorComment) = 0;

	// 모니터링 함수
	int GetAcceptTPS();
	int GetRecvMessageTPS();
	int GetSendMessageTPS();

private:
	// 멤버변수: 쓰레드
	HANDLE _hIOCP;
	HANDLE _hThread_Monitor;
	HANDLE _hThread_Accept;
	std::vector<HANDLE> _workerThreadHandles;

private:
	// 멤버변수: 네트워크
	const WCHAR* _ipAddress;
	unsigned short _port;
	unsigned short _workerThreadCount;
	unsigned short _coreSkip;
	bool _isNagle;
	unsigned int _maximumSessionCount;

	SOCKET _listenSocket;
	DWORD64 _g_sessionID = 0;

public:
	// 멤버 변수: 세션배열관리
	Session _sessionArray[20000];
	std::stack<unsigned int> _releaseIdxStack;
	SRWLOCK _releaseStackLock;

private:
	// 멤버변수: 네트워크 리턴값체크
	DWORD wsaStartupRet;
	DWORD bindRet;
	DWORD listenRet;
	DWORD setSockOptRet;

private:
	// 멤버변수: 이벤트
	HANDLE _hEvent_Quit;

private:
	// 멤버변수: 모니터링
	LONG _acceptTPS = 0;
	LONG _recvMessageTPS = 0;
	LONG _sendMessageTPS = 0;

	LONG _acceptTPS_BackUp = 0;
	LONG _recvMessageTPS_BackUp = 0;
	LONG _sendMessageTPS_BackUp = 0;

	LONG _sessionCount = 0;
};