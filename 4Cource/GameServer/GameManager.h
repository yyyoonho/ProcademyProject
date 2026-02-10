#pragma once

// 사용법
// 1. GameManager 객체를 만든다.
// 2. start를 호출한다.
// 3. Auth 쓰레드를 등록한다.
// 4. field 쓰레드들을 등록한다.

#include "FieldStruct.h"

class Field;
class NetClient_Monitoring;
class Player;

class GameManager : public CNetServer
{
private:
	struct FieldBundle
	{
		FieldName fieldName;
		Field* field;

		RingBuffer threadQ_Create;
		mutex threadQ_Create_Lock;

		RingBuffer threadQ_Join;
		mutex threadQ_Join_Lock;

		// 이 쓰레드에서 관리되고 있는 session*
		vector<Session*> sessionVec;

		// release된(아직 재사용은 안된) session*
		vector<Session*> releaseVec;
	};

public:
	GameManager();
	~GameManager();

public:
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

public:
	void RegistField(FieldName fieldName, Field* pField);
	void RegistMonitoring(NetClient_Monitoring* pNetClient);

public:
	bool OnConnectionRequest(SOCKADDR_IN clientAddr) override;
	void OnError(int errorCode, WCHAR* errorComment) override;

	// 이 아래 3개는 필요없음.
	void OnAccept(DWORD64 sessionID) override;
	void OnRelease(DWORD64 sessionID) override;
	void OnMessage(DWORD64 sessionID, SerializePacketPtr pPacket) override;
	
	// 이 핸들러함수가 찐.
	virtual void OnAccept_GameManager(DWORD64 sessionID, Session* pSession) override;
	virtual void OnMessage_GameManager(DWORD64 sessionID, Session* pSession, SerializePacketPtr pPacket) override;
	virtual void OnRelease_GameManager(DWORD64 sessionID, Session* pSession) override;

	__forceinline Player*		GetPlayerFromSID(DWORD64 sessionID);
	void						SetPlayerToSession(DWORD64 sessionID, Player* pPlayer);

	__forceinline INT64			GetAccountNoFromSID(DWORD64 sessionID);
	void						SetAccountNoToSession(DWORD64 sessionID, INT64 accountNo);
	

private:
	void FieldThreadFunc(void* param, int id);
	void FrameControl();
	void ShowFPS(int id);

private:
	HANDLE									hEvent_Quit;

private:
	NetClient_Monitoring*					pNetClient;

private:
	std::thread								monitoringThread;
	void									MonitorThread();
	void									TossMonitoringData();

private:
	vector<std::thread>						threads;

	unordered_map<FieldName, FieldBundle*>	fieldBundleMap;
	mutex									fieldBundleMapLock;

public:
	// 테스트
	vector<RingBuffer*>						sendPacketQ;
	vector<std::thread>						sendThreads;
	void									SendPacketJobThread(int id);
};

__forceinline Player* GameManager::GetPlayerFromSID(DWORD64 sessionID)
{
	int idx = GetIdxFromSessionID(sessionID);
	Player* pPlayer = _sessionArray[idx].pPlayer;

	return pPlayer;
}


__forceinline INT64 GameManager::GetAccountNoFromSID(DWORD64 sessionID)
{
	int idx = GetIdxFromSessionID(sessionID);

	// 틈새
	_sessionArray[idx].pPlayer->heartbeat = GetTickCount64();

	return _sessionArray[idx].accountNo;
}