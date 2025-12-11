#pragma once

class PlayerManager;
class SectorManager;

class ChatServer : public CNetServer
{
public:
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
	HANDLE _hEventQuit;

	SerializePacketPtr_RingBuffer _contentMSGQueue;
	HANDLE _hEventMsg;
	SRWLOCK _contentMSGLock;

	static void ContentThreadRun(LPVOID* lParam);
	void ContentThread();

private:
	void AcceptProc(DWORD64 sessionID, SerializePacketPtr pPacket);

	void PacketProc(DWORD64 sessionID, SerializePacketPtr pPacket);
	void PacketProc_Login(DWORD64 sessionID, SerializePacketPtr pPacket);
	bool CheckDuplicateLogin(INT64 accountNo);
	void PacketProc_SectorMove(DWORD64 sessionID, SerializePacketPtr pPacket);
	void PacketProc_Message(DWORD64 sessionID, SerializePacketPtr pPacket);
	void PacketProc_Heartbeat(DWORD64 sessionID);

	void ReleaseProc(DWORD64 sessionID, SerializePacketPtr pPacket);

	void UpdateHeartbeat(DWORD64 sessionID);

	void DisconnectUnresponsivePlayers();

public:
	procademy::MemoryPool_TLS<Player> playerPool{ 0,false };

	vector<Player*>					tmpPlayerArr;

	vector<Player*>					playerArr;

	unordered_map<DWORD64, INT64>	sessionIDAccountNoMap;
	unordered_map<INT64, int>		accountToIndex;

	vector<DWORD64>					sector[MAX_SECTOR_Y][MAX_SECTOR_X];
};

