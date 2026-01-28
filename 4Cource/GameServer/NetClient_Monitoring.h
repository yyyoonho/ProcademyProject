#pragma once

class NetClient_Monitoring : public CNetClient
{
public:
	NetClient_Monitoring();
	~NetClient_Monitoring();

public:
	virtual bool Connect(
		const WCHAR* ipAddress,
		unsigned short port,
		unsigned short workerThreadCount,
		unsigned short coreSkip,
		bool isNagle,
		bool codecOnOff,
		BYTE fixedKey,
		BYTE code) override;

	void OnEnterJoin() override;
	void OnLeaveServer() override;
	void OnMessage(SerializePacketPtr pPacket) override;
	void OnError(int errorCode, WCHAR* errorComment) override;
	void OnSendJob() override;

public:
	void EnqueueSendJob(stSendJob sendJob);

private:
	int _serverNo = 1000;
	LockFreeQueue<stSendJob> sendJobQ;

private:
	bool connected = false;
};