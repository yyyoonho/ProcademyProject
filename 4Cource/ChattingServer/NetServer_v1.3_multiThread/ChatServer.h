#pragma once

class PlayerManager;
class SectorManager;

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

		DWORD heartbeat;

		PLAYER_STATE state;

		SRWLOCK playerLock;
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
		bool codecOnOff) override;

	virtual void Stop() override;

public:
	virtual bool OnConnectionRequest(SOCKADDR_IN clientAddr) override;
	virtual void OnAccept(DWORD64 sessionID) override;
	virtual void OnRelease(DWORD64 sessionID) override;
	virtual void OnMessage(DWORD64 sessionID, SerializePacketPtr pPacket) override;
	virtual void OnError(int errorCode, WCHAR* errorComment) override;

private:
	bool _bShutdown = false;

	HANDLE _hThread;
	HANDLE _hMonitoringhread;

	SerializePacketPtr_RingBuffer _contentMSGQueue;
	HANDLE _hEventMsg;
	SRWLOCK _contentMSGLock;

private:
	void PacketProc(DWORD64 sessionID, SerializePacketPtr pPacket);
	void PacketProc_Login(DWORD64 sessionID, SerializePacketPtr pPacket);
	bool CheckDuplicateLogin(INT64 accountNo);
	void PacketProc_SectorMove(DWORD64 sessionID, SerializePacketPtr pPacket);
	void PacketProc_Message(DWORD64 sessionID, SerializePacketPtr pPacket);
	void PacketProc_Heartbeat(DWORD64 sessionID);

	void UpdateHeartbeat(DWORD64 sessionID);

	void DisconnectUnresponsivePlayers();

	void LockAroundSector_Shared(WORD sectorY, WORD sectorX);
	void UnLockAroundSector_Shared(WORD sectorY, WORD sectorX);

	static void HeartbeatThreadRun(LPVOID* lParam);
	void HeartbeatThread();

	static void MonitorThreadRun(LPVOID* lParam);
	void MonitorThread();

public:
	// accept 전용
	LockFreeStack<int>				tmpIdx_LockFreeStack;

	unordered_map<DWORD64, int>		tmpSessionIdToIndex;
	SRWLOCK							tmpSessionIdToIndex_Lock;

	WaitPlayer						tmpPlayerArr[30000];
	

	// play 전용
	LockFreeStack<int>				Idx_LockFreeStack;

	unordered_map<DWORD64, INT64>	sessionIdToAccountNo;
	SRWLOCK							sessionIdToAccountNo_Lock;

	unordered_map<INT64, int>		accountToIndex;
	SRWLOCK							accountToIndex_Lock;

	unordered_set<INT64>			onlineAccounts;			// 온라인 여부만 확인
	SRWLOCK							onlineAccounts_Lock;

	Player							playerArr[20000];

	// 섹터
	vector<DWORD64>					sector[MAX_SECTOR_Y][MAX_SECTOR_X];
	SRWLOCK							sector_Lock[MAX_SECTOR_Y][MAX_SECTOR_X];
};

