#pragma once

class ChatServer : public CNetServer
{
public:
	virtual bool Start(const WCHAR* ipAddress,
		unsigned short port,
		unsigned short workerThreadCount,
		unsigned short coreSkip,
		bool isNagle,
		unsigned int maximumSessionCount) override;

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
	HANDLE _hEventQuit;

	SerializePacketPtr_RingBuffer _contentMSGQueue;
	SRWLOCK _contentMSGLock;

	static void ContentThreadRun(LPVOID* lParam);
	void ContentThread();

private:
	void UnicastByAccountNo(INT64 accountNo, SerializePacketPtr sPacket);
	void UnicastBySessionID(DWORD64 sessionID, SerializePacketPtr sPacket);
	void Multicast(std::vector<INT64>& accountNoArr, SerializePacketPtr sPacket);

private:
	void AcceptProc(SerializePacketPtr pPacket);

private:
	void PacketProc(SerializePacketPtr pPacket);
	void PacketProc_Login(DWORD64 sessionID, SerializePacketPtr pPacket);
	void PacketProc_SectorMove(DWORD64 sessionID, SerializePacketPtr pPacket);
	void PacketProc_Message(DWORD64 sessionID, SerializePacketPtr pPacket);
	void PacketProc_Heartbeat(DWORD64 sessionID);
};

