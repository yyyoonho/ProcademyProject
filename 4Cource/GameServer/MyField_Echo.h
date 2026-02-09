#pragma once
class MyField_Echo : public Field
{
public:
	MyField_Echo();
	virtual ~MyField_Echo();

	virtual void OnEnter(DWORD64 sessionID) override;
	virtual void OnEnterWithPlayer(DWORD64 sessionID, void* pPlayer) override;
	virtual void OnRecv(DWORD64 sessionID, SerializePacketPtr sPacket) override;
	virtual void OnUpdate() override;
	virtual void OnLeave(DWORD64 sessionID) override;
	virtual void OnFieldLeave(DWORD64 sessionID) override;

private:
	bool PacketProc_Echo(DWORD64 sessionID, SerializePacketPtr sPacket);
	bool PacketProc_HB(DWORD64 sessionID);

	bool CheckMessageRateLimit(Player* pPlayer);

private:
	//procademy::MemoryPool_TLS<Player> playerPool{ 0,false };

	vector<Player*>					PlayerArr;
	unordered_map<DWORD64, Player*>	SIDToPlayer;
	unordered_map<INT64, DWORD64>	accountNoToSID;
};

