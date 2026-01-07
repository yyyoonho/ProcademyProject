#pragma once

class NetCodec;

class CNetClient
{
public:
	CNetClient();
	virtual ~CNetClient();

public:
	virtual bool Connect(
		const WCHAR* ipAddress,
		unsigned short port,
		unsigned short workerThreadCount,
		unsigned short coreSkip,
		bool isNagle,
		bool codecOnOff,
		BYTE fixedKey,
		BYTE code);

	void InitSession();

	// 콘텐츠에서 호출하는 함수
	bool Disconnect();
	bool SendPacket(SerializePacketPtr pPacket);


private:
	// 클래스 내부 함수
	bool NetInit();
	bool TryConnect(int retryCount);

	bool RecvProc(Session* pSession);
	bool SendProc(Session* pSession);
	void SendPost(Session* pSession);

	void IncreaseIO_Count(Session* pSession);
	bool DecreaseIO_Count(Session* pSession);

	void PQCS_Release(Session* pSession);
	void ReleaseProc(Session* pSession);

private:
	// 쓰레드 함수
	static void WorkerThreadRun(LPVOID* lParam);
	void WorkerThread();

	static void ConnectThreadRun(LPVOID* lParam);
	void ConnectThread();

public:
	// 핸들링 함수
	virtual void OnEnterJoin() = 0;		// 서버와의 연결 성공 후
	virtual void OnLeaveServer() = 0;	// 서버와의 연결이 끊어졌을 때
	virtual void OnMessage(SerializePacketPtr pPacket) = 0;
	virtual void OnError(int errorCode, WCHAR* errorComment) = 0;

private:
	// 멤버변수: 네트워크
	const WCHAR* _ipAddress;
	unsigned short _port;
	unsigned short _workerThreadCount;
	unsigned short _coreSkip;
	bool _isNagle;
	bool _codecOnOff;

private:
	// 멤버변수: 쓰레드
	HANDLE _hIOCP;
	std::vector<HANDLE> _workerThreadHandles;

	HANDLE _hConnectThread;

public:
	DWORD64 _uniqueSessionID = 0;
	Session _session;

private:
	// 멤버변수: 네트워크 리턴값체크
	DWORD wsaStartupRet;
	DWORD connectRet;
	DWORD setSockOptRet;

private:
	// 멤버변수: 이벤트
	HANDLE _hEvent_Quit;
	HANDLE _hEvent_Connect;

private:
	// PQCS용 오버랩객체
	virtual void OnSendJob() = 0;
	myOverlapped sendJobIOCP;
public:
	void PQCSSendJob();

private:
	NetCodec* _netCodec;
};