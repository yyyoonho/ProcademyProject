#pragma once;

class ChatServer;

class SectorManager
{
public:
	SectorManager(ChatServer* owner);
	~SectorManager();

public:
	struct stSectorInfo
	{
		WORD sectorY;
		WORD sectorX;
	};

public:
	bool MovePlayer(INT64 accountNo, WORD newSectorY, WORD newSectorX);
	bool GetAroundAccountNo(INT64 accountNo, std::vector<INT64>& accountNoArr);
	bool ReleaseProc(INT64 accountNo);

private:
	procademy::MemoryPool_TLS<stSectorInfo>			_sectorInfoMemoryPool{ 0,false };

private:
	std::list<INT64>								_sector[MAX_SECTOR_Y][MAX_SECTOR_X];
	std::unordered_map<INT64, stSectorInfo*>		_sectorInfo;

private:
	ChatServer*										_owner;
};