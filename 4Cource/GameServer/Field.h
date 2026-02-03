#pragma once
class GameManager;

class Field
{
public:
	Field();
	virtual ~Field();

public:
	virtual void OnEnter(DWORD64 sessionID) = 0;
	virtual void OnEnterWithPlayer(DWORD64 sessionID, void* pPlayer) = 0;
	virtual void OnRecv(DWORD64 sessionID, SerializePacketPtr sPacket) = 0;
	virtual void OnUpdate() = 0;
	virtual void OnLeave(DWORD64 sessionID) = 0;
	virtual void OnFieldLeave(DWORD64 sessionID) = 0;

	// 콘텐츠쪽에서 moveToOtherField()를 호출한 상황
	// 1. 만약 player가 다른곳으로 가고싶어한다.
	// 2. moveToOtherField(fieldName, sessionID) 호출
	// 3. A쓰레드에서는 sessionID로 해당 session을 찾는다.
	// 4. 찾아서 moveVec에 넣어둔다.
	void MoveToOtherField(FieldName fieldName, DWORD64 sessionID, void* pPlayer);
	void SendPacket(DWORD64 sessionID, SerializePacketPtr sPacket);
	void Disconnect(DWORD64 sessionID);

	void RegistGameManager(GameManager* pGM);

public:
	vector<FieldMovePack> movePackageVec;

private:
	GameManager* pGameManager;
};

