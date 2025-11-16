#include "stdafx.h"
#include "SectorManager.h"

using namespace std;

WORD dy[9] = { -1,-1,-1,0,0,0,1,1,1 };
WORD dx[9] = { -1,0,1,-1,0,1,-1,0,1 };

SectorManager::SectorManager(ChatServer* owner)
{
	_owner = owner;
}

SectorManager::~SectorManager()
{
}

bool SectorManager::MovePlayer(INT64 accountNo, WORD newSectorY, WORD newSectorX)
{
	unordered_map<INT64, stSectorInfo*>::iterator iter = _sectorInfo.find(accountNo);
	if (iter == _sectorInfo.end())
	{
		stSectorInfo* newSectorInfo = _sectorInfoMemoryPool.Alloc();
		newSectorInfo->sectorY = newSectorY;
		newSectorInfo->sectorX = newSectorX;

		_sectorInfo.insert({ accountNo, newSectorInfo });
		_sector[newSectorY][newSectorX].push_back(accountNo);
	}
	else
	{
		// old
		WORD oldSectorY = iter->second->sectorY;
		WORD oldSectorX = iter->second->sectorX;

		list<INT64>::iterator iter2 = _sector[oldSectorY][oldSectorX].begin();
		for (; iter2 != _sector[oldSectorY][oldSectorX].end(); ++iter2)
		{
			if (*iter2 != accountNo)
				continue;

			_sector[oldSectorY][oldSectorX].erase(iter2);
			break;
		}

		// new
		iter->second->sectorY = newSectorY;
		iter->second->sectorX = newSectorX;

		_sector[newSectorY][newSectorX].push_back(accountNo);
	}

	return true;
}

bool SectorManager::GetAroundAccountNo(INT64 accountNo, std::vector<INT64>& accountNoArr)
{
	unordered_map<INT64, stSectorInfo*>::iterator iter = _sectorInfo.find(accountNo);
	if (iter == _sectorInfo.end())
	{
		cout << "#Error: GetAroundAccountNo - _sectorInfo" << endl;
		return false;
	}

	WORD sectorY = iter->second->sectorY;
	WORD sectorX = iter->second->sectorX;

	for (int i = 0; i < 9; i++)
	{
		WORD nextY = sectorY + dy[i];
		WORD nextX = sectorX + dx[i];

		if (nextY < 0 || nextY >= MAX_SECTOR_Y || nextX < 0 || nextX >= MAX_SECTOR_X)
			continue;

		list<INT64>::iterator iter2 = _sector[nextY][nextX].begin();
		for (; iter2 != _sector[nextY][nextX].end(); ++iter2)
		{
			accountNoArr.push_back(*iter2);
		}
	}
}

bool SectorManager::ReleaseProc(INT64 accountNo)
{
	unordered_map<INT64, stSectorInfo*>::iterator iter = _sectorInfo.find(accountNo);
	if (iter == _sectorInfo.end())
	{
		return false;
	}
	
	stSectorInfo* targetInfo = iter->second;
	
	WORD sectorY = targetInfo->sectorY;
	WORD sectorX = targetInfo->sectorX;

	_sectorInfo.erase(iter);
	_sectorInfoMemoryPool.Free(targetInfo);

	list<INT64>::iterator iter2 = _sector[sectorY][sectorX].begin();
	for (; iter2 != _sector[sectorY][sectorX].end(); ++iter2)
	{
		if (*iter2 == accountNo)
		{
			_sector[sectorY][sectorX].erase(iter2);
			break;
		}
	}

	return true;
}
