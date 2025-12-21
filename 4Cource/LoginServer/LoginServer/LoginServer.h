#pragma once

class Player;

class LoginServer : public CNetServer
{
public:
	LoginServer();
	~LoginServer();

	virtual bool Start(
		const WCHAR* ipAddress,
		unsigned short port,
		unsigned short workerThreadCount,
		unsigned short coreSkip,
		bool isNagle,
		unsigned int maximumSessionCount,
		bool codecOnOff);

	virtual void Stop();

	// 핸들링 함수
	virtual bool OnConnectionRequest(SOCKADDR_IN clientAddr);
	virtual void OnAccept(DWORD64 sessionID, SOCKADDR_IN addr);
	virtual void OnRelease(DWORD64 sessionID);
	virtual void OnMessage(DWORD64 sessionID, SerializePacketPtr pPacket);
	virtual void OnError(int errorCode, WCHAR* errorComment);

private:
	void PacketProc(DWORD64 sessionID, SerializePacketPtr pPacket);

private:
	void PacketProc_Login(DWORD64 sessionID, SerializePacketPtr pPacket);
	bool AuthorizeToken(const char* token);
	bool SaveTokenToRedis(INT64 accountNo, const char* sessionKey);
	void SendPacket_RES_LOGIN(INT64 accountNo, DWORD64 sessionID);

private:
	HANDLE hThread_Monitoring;
	static void MonitorThreadRun(LPVOID* lParam);
	void MonitorThread();

private:
	HANDLE hEvent_Quit;

private:
	procademy::MemoryPool_TLS<Player> mp{ 0,false };

	mutex _playeLock;
	unordered_map<DWORD64, int> sessionIDToIdx;
	vector<Player*> _playerVec;

private:
	void ConvertToUTF16();
	WCHAR _chattingServerIpStr1[16];
	WCHAR _chattingServerIpStr2[16];
	WCHAR _dummyIpStr1[16];
	WCHAR _dummyIpStr2[16];

	in_addr _dummyIpAddr1;
	in_addr _dummyIpAddr2;
};

