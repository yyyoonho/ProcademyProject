#include "stdafx.h"
#include "NetServer.h"
#include "ChatServer.h"
#include "PlayerManager.h"

using namespace std;


PlayerManager::PlayerManager(ChatServer* owner)
{
	_owner = owner;
}

PlayerManager::~PlayerManager()
{
}

void PlayerManager::CreatePlayerInfo(DWORD64 sessionID)
{
	stPlayerInfo* tmpPlayerInfo = _playerInfoMemoryPool.Alloc();

	tmpPlayerInfo->sessionID = sessionID;
	tmpPlayerInfo->state = PLAYER_STATE::WAIT_LOGIN;
	tmpPlayerInfo->heartbeat = timeGetTime();

	_waitMap.insert({ sessionID, tmpPlayerInfo });
}

bool PlayerManager::LoginPlayer(DWORD64 sessionID, INT64 accountNo, WCHAR* id, WCHAR* nickName, char* sessionKey)
{
	unordered_map<DWORD64, stPlayerInfo*>::iterator iter = _waitMap.find(sessionID);
	if (iter == _waitMap.end())
	{
		cout << "#Error: LoginPlayer - NotFound" << endl;
		return false;
	}

	stPlayerInfo* pPlayer = iter->second;
	_waitMap.erase(iter);

	unordered_map<INT64, stPlayerInfo*>::iterator iter2 = _playerMap.find(accountNo);
	if (iter2 != _playerMap.end())
	{
		// 중복로그인!
		stPlayerInfo* pOriginPlayer = iter2->second;
		
		cout << "#Error: 중복로그인! acccountNo:" << pOriginPlayer->accountNo << endl;

		// 둘다 disconnect!
		_owner->Disconnect(pPlayer->sessionID);
		_owner->Disconnect(pOriginPlayer->sessionID);

		return false;
	}

	pPlayer->accountNo = accountNo;
	//wcscpy_s(pPlayer->ID, 20, id);
	//wcscpy_s(pPlayer->nickname, 20, nickName);
	//strcpy_s(pPlayer->sessionKey, sessionKey);

	wcsncpy_s(pPlayer->ID, 20, id, _TRUNCATE);
	wcsncpy_s(pPlayer->nickname, 20, nickName, _TRUNCATE);
	strncpy_s(pPlayer->sessionKey, 64, sessionKey, _TRUNCATE);

	pPlayer->heartbeat = timeGetTime();
	pPlayer->state = PLAYER_STATE::LOGGED_IN;

	_playerMap.insert({ accountNo, pPlayer });
	_IdAccountNoMap.insert({ sessionID, accountNo });

	return true;
}

bool PlayerManager::IsLoggedIn(INT64 accountNo)
{
	unordered_map<INT64, stPlayerInfo*>::iterator iter = _playerMap.find(accountNo);
	if (iter == _playerMap.end())
	{
		return false;
	}
		
	if (iter->second->state == PLAYER_STATE::LOGGED_IN)
	{
		return true;
	}
	else
		return false;
}

bool PlayerManager::SerializeIDnNickname(INT64 accountNo, SerializePacketPtr sPacket)
{
	unordered_map<INT64, stPlayerInfo*>::iterator iter = _playerMap.find(accountNo);
	if (iter == _playerMap.end())
	{
		cout << "#Error: SerializeIDnNickname - _playerMap" << endl;
		return false;
	}

	WCHAR* id = iter->second->ID;
	WCHAR* nickname = iter->second->nickname;

	sPacket.Putdata((char*)id, sizeof(WCHAR) * 20);
	sPacket.Putdata((char*)nickname, sizeof(WCHAR) * 20);

	return true;
}

DWORD64 PlayerManager::GetSessionID(INT64 accountNo)
{
	unordered_map<INT64, stPlayerInfo*>::iterator iter = _playerMap.find(accountNo);
	if (iter == _playerMap.end())
	{
		cout << "#Error: GetSessionID - _playerMap" << endl;
		return -1;
	}

	return iter->second->sessionID;
}

bool PlayerManager::UpdateHeartbeat(DWORD64 sessionID)
{
	unordered_map<DWORD64, INT64>::iterator iter = _IdAccountNoMap.find(sessionID);
	if (iter == _IdAccountNoMap.end())
	{
		cout << "#Error: UpdateHeartbeat - _IdAccountNoMap" << endl;
		return false;
	}

	INT64 accountNo = iter->second;

	unordered_map<INT64, stPlayerInfo*>::iterator iter2 = _playerMap.find(accountNo);
	if (iter2 == _playerMap.end())
	{
		cout << "#Error: UpdateHeartbeat - _playerMap" << endl;
		return false;
	}

	iter2->second->heartbeat = timeGetTime();

	return true;
}

void PlayerManager::CheckAcceptTime(DWORD nowTime)
{
	unordered_map<DWORD64, stPlayerInfo*>::iterator iter = _waitMap.begin();
	for (; iter != _waitMap.end(); ++iter)
	{
		stPlayerInfo* target = iter->second;

		if (nowTime - target->heartbeat >= 4000)
		{
			cout << "#Error: accept후 로그인패킷 반응없음! acccountNo:" << target->accountNo << endl;
			_owner->Disconnect(target->sessionID);
		}
	}

}

void PlayerManager::CheckHeartbeat(DWORD nowTime)
{
	unordered_map<INT64, stPlayerInfo*>::iterator iter = _playerMap.begin();
	for (; iter != _playerMap.end(); ++iter)
	{
		stPlayerInfo* target = iter->second;

		if (nowTime - target->heartbeat >= 30000)
		{
			cout << "#Error: 하트비트 초과! acccountNo:" << target->accountNo << endl;

			_owner->Disconnect(target->sessionID);
		}
	}
}

INT64 PlayerManager::ReleaseProc(DWORD64 sessionID)
{
	unordered_map<DWORD64, stPlayerInfo*>::iterator iter = _waitMap.find(sessionID);
	if (iter != _waitMap.end())
	{
		stPlayerInfo* tmp = iter->second;
		_waitMap.erase(iter);

		_playerInfoMemoryPool.Free(tmp);

		return -1;
	}

	INT64 accountNo = -1;

	unordered_map<DWORD64, INT64>::iterator iter2 = _IdAccountNoMap.find(sessionID);
	if (iter2 != _IdAccountNoMap.end())
	{
		accountNo = iter2->second;
		_IdAccountNoMap.erase(iter2);
	}

	unordered_map<INT64, stPlayerInfo*>::iterator iter3 = _playerMap.find(accountNo);
	if (iter3 != _playerMap.end())
	{
		stPlayerInfo* tmp = iter3->second;
		_playerMap.erase(iter3);

		_playerInfoMemoryPool.Free(tmp);
	}
	
	return accountNo;
}

INT64 PlayerManager::GetAccountNo(DWORD64 sessionID)
{
	unordered_map<DWORD64, INT64>::iterator iter = _IdAccountNoMap.find(sessionID);
	if (iter == _IdAccountNoMap.end())
		return -1;

	return iter->second;
}
