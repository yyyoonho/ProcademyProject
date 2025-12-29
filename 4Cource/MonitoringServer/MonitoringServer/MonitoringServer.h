#pragma once

enum class SESSION_ROLE
{
	None = 0,
	Accept,
	MonitoringTool,

	ChatServer,			// ServerNo: 10~19
	LoginServer,		// ServerNo: 20~29
	GameServer,			// ServerNo: 30~39
};

struct Client
{
	DWORD64			sessionID;
	SOCKADDR_IN		addr;

	BYTE			serverNo;
	SESSION_ROLE	sessionRole;

	int				dataCount;
	DWORD64			sumData[en_PACKET_SS_MONITOR_DATA_UPDATE::COUNT];
	DWORD64			minData[en_PACKET_SS_MONITOR_DATA_UPDATE::COUNT];
	DWORD64			maxData[en_PACKET_SS_MONITOR_DATA_UPDATE::COUNT];
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
	void PacketProc_ServerLogin(DWORD64 sessionID, SerializePacketPtr pPacket);
	void PacketProc_DataUpdate(DWORD64 sessionID, SerializePacketPtr pPacket);
	void PacketProc_MonitorToolLogin(DWORD64 sessionID, SerializePacketPtr pPacket);

private:
	bool ReleaseTmpclient(DWORD64 sessionID);
	bool ReleaseOriginClient(DWORD64 sessionID);
	bool ReleaseToolclient(DWORD64 sessionID);

private:
	procademy::MemoryPool_TLS<Client> mp{ 0,false };

	std::mutex tmpClientMapLock;
	std::unordered_map<DWORD64, Client*> tmpClientMap;

	std::mutex clientMapLock;
	std::unordered_map<DWORD64, Client*> clientMap;

	std::mutex monitoringToolArrLock;
	std::vector<Client*> monitoringToolArr;

	const char* loginSessionKey = "ajfw@!cv980dSZ[fje#@fdj123948djf";

};