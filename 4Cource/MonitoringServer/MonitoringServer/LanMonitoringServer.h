#pragma once
#include <Pdh.h>
class CCpuUsage;
class MonitoringServer;

struct LanClient
{
	DWORD64			sessionID;
	SOCKADDR_IN		addr;

	//SESSION_ROLE	sessionRole;
	int				serverNo;

	// 데이터
	std::mutex		LanClientLock;
	int				count[en_PACKET_SS_MONITOR_DATA_UPDATE::COUNT];
	bool			activeData[en_PACKET_SS_MONITOR_DATA_UPDATE::COUNT];
	int				avgData[en_PACKET_SS_MONITOR_DATA_UPDATE::COUNT];
	int				minData[en_PACKET_SS_MONITOR_DATA_UPDATE::COUNT];
	int				maxData[en_PACKET_SS_MONITOR_DATA_UPDATE::COUNT];

	void			ResetData(int i);
};


class LanMonitoringServer : public CNetServer
{
public:
	struct MonitoringSnapshot
	{
		int serverNo;
		int dataType;
		int avg;
		int min;
		int max;
	};

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

private:
	HANDLE hThread_ServerMonitoring;
	HANDLE hThread_DB;
	HANDLE hEvent_Quit;

	static void DBContentRun(LPVOID* lParam);
	void DBContentThread();

};

