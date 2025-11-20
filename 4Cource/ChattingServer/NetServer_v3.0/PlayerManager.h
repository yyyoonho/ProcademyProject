#pragma once

#define NONESECTOR 65535

class ChatServer;


class PlayerManager
{
public:
	PlayerManager(ChatServer* owner);
	~PlayerManager();

public:
	struct stMsgCountInfo
	{
		DWORD firstTime;
		int count;
	};

	struct stPlayerInfo
	{
		INT64 accountNo;
		DWORD64 sessionID;

		WCHAR ID[20];				// null 포함
		WCHAR nickname[20];			// null 포함
		char sessionKey[64];

		DWORD heartbeat;

		PLAYER_STATE state;

		std::unordered_map<WORD, stMsgCountInfo> msgCooltime;
		bool yellowCard;
	};

public:
	void			CreatePlayerInfo(DWORD64 sessionID);
	
	bool			LoginPlayer(DWORD64 sessionID , INT64 accountNo, WCHAR* id, WCHAR* nickName, char* sessionKey);
	bool			IsLoggedIn(INT64 accountNo);
	
	bool			SerializeIDnNickname(INT64 accountNo, SerializePacketPtr sPacket);
	DWORD64			GetSessionID(INT64 accountNo);

	bool			UpdateHeartbeat(DWORD64 sessionID);
	void			CheckAcceptTime(DWORD nowTime);
	void			CheckHeartbeat(DWORD nowTime);

	INT64			ReleaseProc(DWORD64 sessionID);

	bool			IsActive(DWORD64 sessionID);

	bool			CheckMessageRateLimit(DWORD64 sessionID, WORD msgType);
	

private:
	ChatServer*											_owner;

private:
	procademy::MemoryPool_TLS<stPlayerInfo>				_playerInfoMemoryPool{ 0,false };

private:
	std::unordered_map<DWORD64, stPlayerInfo*>			_waitMap;			// sessionID - stPlayerInfo*

	std::unordered_map<INT64, stPlayerInfo*>			_playerMap;			// accountNo - stPlayerInfo*
	std::unordered_map<DWORD64, INT64>					_IdAccountNoMap;	// sessionID - accountNo
};