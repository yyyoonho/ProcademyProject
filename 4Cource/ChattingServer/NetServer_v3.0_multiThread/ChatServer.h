#pragma once

class PlayerManager;
class SectorManager;
class NetClient_Monitoring;

class ChatServer : public CNetServer
{
public:
	struct WaitPlayer
	{
		DWORD64 sessionID;
		DWORD waitLoginTime;
		SRWLOCK playerLock;

		PLAYER_STATE state;
	};

	struct Player
	{
		DWORD64 sessionID;
		INT64 accountNo;

		WCHAR ID[20];
		WCHAR nickName[20];
		char sessionKey[64];

		WORD sectorY;
		WORD sectorX;

		DWORD64 heartbeat;

		PLAYER_STATE state;

		SRWLOCK playerLock;
		bool IsInitLock = false;
	};

	struct Heartbeat
	{
		DWORD heartbeatTime;

		DWORD64 sessionID;
		INT64 accountNo;

		bool operator<(const Heartbeat& b)const
		{
			return heartbeatTime > b.heartbeatTime;
		}
	};

public:
	virtual bool Start(const WCHAR* ipAddress,
		unsigned short port,
		unsigned short workerThreadCount,
		unsigned short coreSkip,
		bool isNagle,
		unsigned int maximumSessionCount, 
		bool codecOnOff,
		BYTE fixedKey,
		BYTE code) override;

	virtual void Stop() override;

public:
	virtual bool OnConnectionRequest(SOCKADDR_IN clientAddr) override;
	virtual void OnAccept(DWORD64 sessionID) override;
	virtual void OnRelease(DWORD64 sessionID) override;
	virtual void OnMessage(DWORD64 sessionID, SerializePacketPtr pPacket) override;
	virtual void OnError(int errorCode, WCHAR* errorComment) override;

private:
	bool _bShutdown = false;

	HANDLE hEvent_Quit;

	HANDLE _hMonitoringThread;	// 자체 콘솔 모니터링을 위한 쓰레드
	HANDLE _hMonitoringThread2;	// 모니터링서버로 보내기위한 정보수집 쓰레드
	HANDLE _hThread_Heartbeat;	

	HANDLE _hEventMsg;
	SRWLOCK _contentMSGLock;

private:
	void PacketProc(DWORD64 sessionID, SerializePacketPtr pPacket);
	bool PacketProc_Login(DWORD64 sessionID, SerializePacketPtr pPacket);
	bool IsTokenValid(INT64 accountNo, const char* sessionKey);

	bool PacketProc_SectorMove(DWORD64 sessionID, SerializePacketPtr pPacket);
	bool PacketProc_Message(DWORD64 sessionID, SerializePacketPtr pPacket);
	bool PacketProc_Heartbeat(DWORD64 sessionID);

	void UpdateHeartbeat(DWORD64 sessionID);

	bool ReleaseTmpPlayer(DWORD64 sessionID);
	bool ReleaseOriginPlayer(DWORD64 sessionID);

	void LockAroundSector_Shared(WORD sectorY, WORD sectorX);
	void UnLockAroundSector_Shared(WORD sectorY, WORD sectorX);

	static void HeartbeatThreadRun(LPVOID* lParam);
	void HeartbeatThread();

	static void MonitorThreadRun(LPVOID* lParam);
	void MonitorThread();
	void TossMonitoringData();


public:
	procademy::MemoryPool_TLS<Player> playerPool{ 0,false };

	// accept 전용
	vector<Player*>					tmpPlayerArr;
	SRWLOCK							tmpPlayerArrLock;

	unordered_map<DWORD64, Player*>	tmpSIDToPlayer;
	SRWLOCK							tmpSIDToPlayerLock;

	// play 전용
	vector<Player*>					playerArr;
	SRWLOCK							playerArrLock;

	unordered_map<DWORD64, Player*>	SIDToPlayer;
	SRWLOCK							SIDToPlayerLock;

	unordered_map<INT64, DWORD64>	accountNoToSID;
	SRWLOCK							accountNoToSIDLock;

	// 섹터
	vector<DWORD64>					sector[MAX_SECTOR_Y][MAX_SECTOR_X];
	SRWLOCK							sector_Lock[MAX_SECTOR_Y][MAX_SECTOR_X];

private:
	NetClient_Monitoring*			pNetClient;
public:
	void RegisterNetServer(NetClient_Monitoring* pNetClient);

private:
	unsigned int					_maxPlayerCount;
};

