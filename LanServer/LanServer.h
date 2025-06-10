#pragma once

using namespace std;

#define MAXARR 300

enum
{
	RECV,
	SEND,
};

struct Session;

struct MyOverlapped
{
	OVERLAPPED overlapped;
	int type;
	Session* pSession;
};

struct Session
{
	SOCKET sock = -1;
	
	// [2바이트 = idx][6바이트 = sessionID]
	DWORD64 sessionID;

	RingBuffer recvQ;
	RingBuffer sendQ;

	MyOverlapped recvOverlapped;
	MyOverlapped sendOverlapped;

	LONG sendFlag = true;
	LONG IOCount = 0;

	bool active = false;

	// 스택 테스트용
	int idx = -1;
};

class LanServer
{
public:
	LanServer();
	~LanServer();

public:
	bool Start();
	void Stop();

	int GetSessionCount();

	bool Disconnect(DWORD64 sessionID);
	bool SendPacket(DWORD64 sessionID, SerializePacket* sPacket);

	// 이벤트 함수
	virtual bool OnConnectionRequest(in_addr ipAddress, USHORT port) = 0;

	virtual void OnAccept(in_addr ipAddress, USHORT port, DWORD64 sessionID) = 0;
	virtual void OnRelease(DWORD64 sessionID) = 0;

	virtual void OnMessage(DWORD64 sessionID, SerializePacket* sPacket) = 0;

	virtual void OnError(int errorcode, WCHAR*) = 0;

	// 모니터링 함수
	int GetAcceptTPS();
	int GetRecvMessageTPS();
	int GetSendMessageTPS();

private:
	SOCKET listenSocket;

	HANDLE hIOCP;
	vector<HANDLE> hWorkerThreads;
	HANDLE hAcceptThread;
	HANDLE hMonitorThread;
	bool exitMonitorThread = false;


	DWORD64 g_SessionId = 0;
	DWORD64 totalSessionCount = 0;

	//unordered_map<DWORD64, Session*> sessionMap;
	//procademy::MemoryPool<Session> sessionPool;

	Session* sessionArray[MAXARR];

	// 리턴 체크용 전역변수
	int bindRet;
	int listenRet;
	int ioctlRet;
	int setSockOptRet;
	int wsaRecvRet;
	int wsaSendRet;

private:
	// TPS
	int acceptTPS = 0;
	int recvMessageTPS = 0;
	int sendMessageTPS = 0;

	int disconnetFromClient = 0;

	// TPS 대외용
	int acceptTPS_Save = 0;
	int recvMessageTPS_Save = 0;
	int sendMessageTPS_Save = 0;

public:
	int disconnetFromClient_Save = 0;

private:
	void InitSessionArray();

	void NetInit();
	void CreateAcceptThread();
	void CreateIOCPWorkerThread();
	void CreateMonitorThread();

	static void WorkerThreadRun(LPVOID* lParam);
	void WorkerThread();
	static void AcceptThreadRun(LPVOID* lParam);
	void AcceptThread();
	static void MonitorThreadRun(LPVOID* lParam);
	void MonitorThread();

	bool RequestWSARecv(Session* pSession);
	bool RequestWSASend(Session* pSession);
	void DestroySession(Session* pSession);

	bool FindNonActiveSession(OUT unsigned int* idx);
	Session* FindSessionByID(DWORD64 sessionID);

	// stack 테스트용
	stack<int> idxStack;
	SRWLOCK stackLock;

	// 성능측적용 락
	SRWLOCK LogLock;
};
