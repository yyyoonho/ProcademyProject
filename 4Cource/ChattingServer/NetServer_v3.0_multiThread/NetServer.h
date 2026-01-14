#pragma once

class Session;
class NetCodec;

class CNetServer
{
public:
	CNetServer();
	virtual ~CNetServer();

public:
	virtual bool Start(
		const WCHAR* ipAddress, 
		unsigned short port, 
		unsigned short workerThreadCount, 
		unsigned short coreSkip, 
		bool isNagle, 
		unsigned int maximumSessionCount,
		bool codecOnOff,
		BYTE fixedKey,
		BYTE code);

	virtual void Stop();

	// 콘텐츠에서 호출하는 함수
	bool Disconnect(DWORD64 sessionID);
	bool SendPacket(DWORD64 sessionID, SerializePacketPtr pPacket);
	
private:
	// 클래스 내부 함수
	bool NetInit();

	bool RecvProc(Session* pSession);
	bool SendProc(Session* pSession);
	void SendPost(Session* pSession);

	void IncreaseIO_Count(Session* pSession);
	bool DecreaseIO_Count(Session* pSession);

	void PQCS_Release(Session* pSession);
	void ReleaseProc(Session* pSession);

	unsigned int GetIdxFromSessionID(DWORD64 sessionID);
	void SetIdxToSessionID(DWORD64* pSessionID, unsigned int idx);

private:
	// 쓰레드 함수
	static void WorkerThreadRun(LPVOID* lParam);
	void WorkerThread();
	static void AcceptThreadRun(LPVOID* lParam);
	void AcceptThread();
	
public:
	// 핸들링 함수
	virtual bool OnConnectionRequest(SOCKADDR_IN clientAddr) = 0;
	virtual void OnAccept(DWORD64 sessionID) = 0;
	virtual void OnRelease(DWORD64 sessionID) = 0;
	virtual void OnMessage(DWORD64 sessionID, SerializePacketPtr pPacket) = 0;
	virtual void OnError(int errorCode, WCHAR* errorComment) = 0;


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
	bool _codecOnOff;

	SOCKET _listenSocket;
	DWORD64 _g_sessionID = 0;

public:
	// 멤버 변수: 세션배열관리
	Session _sessionArray[30000];
	LockFreeStack<unsigned int> _releaseIdxLockFreeStack;

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
	NetCodec* _netCodec;
};