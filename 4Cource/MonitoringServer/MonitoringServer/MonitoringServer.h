#pragma once

struct NetClient
{
	DWORD64			sessionID;
	SOCKADDR_IN		addr;
};

class MonitoringServer : public CNetServer
{
public:	
	MonitoringServer();
	~MonitoringServer();

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

	// 핸들링 함수
	virtual bool OnConnectionRequest(SOCKADDR_IN clientAddr);
	virtual void OnAccept(DWORD64 sessionID, SOCKADDR_IN addr);
	virtual void OnRelease(DWORD64 sessionID);
	virtual void OnMessage(DWORD64 sessionID, SerializePacketPtr pPacket);
	virtual void OnError(int errorCode, WCHAR* errorComment);

	// 자체 핸들링 함수
	virtual void OnSendJob();

public:
	void EnqueueSendJob(stSendJob sendJob);

private:
	LockFreeQueue<stSendJob> sendJobQ;

private:
	void PacketProc(DWORD64 sessionID, SerializePacketPtr pPacket);
	void PacketProc_MonitorToolLogin(DWORD64 sessionID, SerializePacketPtr pPacket);

private:
	bool ReleaseTmpclient(DWORD64 sessionID);
	bool ReleaseToolclient(DWORD64 sessionID);

private:
	procademy::MemoryPool_TLS<NetClient> mp{ 0,false };

	std::mutex tmpClientMapLock;
	std::unordered_map<DWORD64, NetClient*> tmpClientMap;

	std::mutex monitoringToolArrLock;
	std::vector<NetClient*> monitoringToolArr;

	const char* loginSessionKey = "ajfw@!cv980dSZ[fje#@fdj123948djf";

};