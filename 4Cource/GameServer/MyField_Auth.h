#pragma once
class MyField_Auth : public Field
{
public:
	MyField_Auth();
	virtual ~MyField_Auth();

	virtual void OnEnter(DWORD64 sessionID) override;
	virtual void OnEnterWithPlayer(DWORD64 sessionID, void* pPlayer) override;
	virtual void OnRecv(DWORD64 sessionID, SerializePacketPtr sPacket) override;
	virtual void OnUpdate() override;
	virtual void OnLeave(DWORD64 sessionID) override;
	virtual void OnFieldLeave(DWORD64 sessionID) override;

private:
	bool PacketProc_Login(DWORD64 sessionID, SerializePacketPtr sPacket);
	bool PacketProc_HB(DWORD64 sessionID);

	bool CheckMessageRateLimit(DWORD64 sessionID);

private:
	vector<Player*>						loginWaitPlayerArr;
	unordered_map<DWORD64, Player*>		SIDToPlayer;
};

