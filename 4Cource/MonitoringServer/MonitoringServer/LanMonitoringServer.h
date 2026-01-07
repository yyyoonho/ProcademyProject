#pragma once

class MonitoringServer;

struct LanClient
{
	DWORD64			sessionID;
	SOCKADDR_IN		addr;

	//SESSION_ROLE	sessionRole;
	BYTE			serverNo;

	// 데이터
	std::mutex		LanClientLock;
	int				count;
	int				sumData[en_PACKET_SS_MONITOR_DATA_UPDATE::COUNT];
	int				minData[en_PACKET_SS_MONITOR_DATA_UPDATE::COUNT];
	int				maxData[en_PACKET_SS_MONITOR_DATA_UPDATE::COUNT];
};

class LanMonitoringServer : public CNetServer
{
public:
	LanMonitoringServer();
	~LanMonitoringServer();

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
	virtual void OnSendJob();

	void RegisterNetServer(MonitoringServer* pNetMonitoringServer);

private:
	void PacketProc(DWORD64 sessionID, SerializePacketPtr pPacket);
	void PakcetProc_Login(DWORD64 sessionID, SerializePacketPtr pPacket);
	void PakcetProc_DataUpdate(DWORD64 sessionID, SerializePacketPtr pPacket);

private:
	bool ReleaseTmpclient(DWORD64 sessionID);
	bool ReleaseOriginclient(DWORD64 sessionID);

private:
	procademy::MemoryPool_TLS<LanClient> mp{ 0,false };

	std::mutex tmpClientMapLock;
	std::unordered_map<DWORD64, LanClient*> tmpClientMap;

	std::mutex clientMapLock;
	std::unordered_map<DWORD64, LanClient*> clientMap;

private:
	MonitoringServer* pNetMonitoringServer;
};

