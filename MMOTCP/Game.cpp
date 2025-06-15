#include "stdafx.h"

#include "SectorManager.h"
#include "CharacterManager.h"
#include "Network.h"
#include "Protocol.h"
#include "LogManager.h"
#include "MakePacket.h"
#include "SendPacket.h"

#include "Game.h"

using namespace std;

#define FRAME 25
#define MSPERFRAME (1000/FRAME)



void FrameControl()
{
	static int oldTime = timeGetTime();

	int diffTime = timeGetTime() - oldTime;

	if (diffTime < MSPERFRAME)
	{
		Sleep(MSPERFRAME - diffTime); 
		oldTime += MSPERFRAME;
		return;
	}
	else
	{
		oldTime += MSPERFRAME;
		return;
	}
}

void ShowFrame()
{
	static int oldTime = timeGetTime();
	static int count = 0;
	count++;

	int diffTime = timeGetTime() - oldTime;
	if (diffTime >= 1000)
	{
		printf("Frame: %d\n", count);
		count = 0;

		oldTime += 1000;
	}
}

bool CanGo(int y, int x)
{
	if (y < dfRANGE_MOVE_TOP || y >= dfRANGE_MOVE_BOTTOM || x < dfRANGE_MOVE_LEFT || x >= dfRANGE_MOVE_RIGHT)
		return false;

	return true;
}

void Move(DWORD deltaTime, stCharacter* pCharacter, BYTE dir)
{
	int dy[8] = { 0, -1, -1, -1, 0, 1, 1, 1 };
	int dx[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };

	double deltaTimeSec = (double)deltaTime / 1000.f;

	double tmpX = (double(dfSPEED_PLAYER_X * FRAME) * deltaTimeSec) * dx[dir];
	double tmpY = (double(dfSPEED_PLAYER_Y * FRAME) * deltaTimeSec) * dy[dir];

	short nextX = pCharacter->dX + tmpX;
	short nextY = pCharacter->dY + tmpY;

	if (!CanGo(nextY, nextX))
		return;

	pCharacter->dX = pCharacter->dX + tmpX;
	pCharacter->dY = pCharacter->dY + tmpY;

	pCharacter->shX = nextX;
	pCharacter->shY = nextY;

	_LOG(dfLOG_LEVEL_DEBUG, L"# Moving... # SessionID:%d / Action:%d / X:%d / Y:%d\n",
		pCharacter->dwSessionID, pCharacter->byMoveDirection, pCharacter->shX, pCharacter->shY);

	return;
}

void GameUpdate()
{
	ShowFrame();
	FrameControl();

	static DWORD oldTime = GetTickCount();
	DWORD deltaTime = GetTickCount() - oldTime;

	unordered_map<DWORD, stCharacter*>::iterator iter;
	for (iter = characterMap.begin(); iter != characterMap.end(); ++iter)
	{
		stCharacter* pCharacter = iter->second;

		// hp가 0이하면 종료처리.
		if (pCharacter->chHP <= 0)
		{
			{
				SerializePacket sPacket;
				mpDeleteCharacter(&sPacket, pCharacter->dwSessionID);

				SendPacket_Around(pCharacter->pSession, &sPacket, false);
			}
			_LOG(dfLOG_LEVEL_DEBUG, L"# HP ZERO... # SessionID:%d\n", pCharacter->dwSessionID);

			//TEST
			_LOG(dfLOG_LEVEL_SYSTEM, L"# Hp 0 # SessionID:%d\n", pCharacter->dwSessionID);
			PushQuitQ(pCharacter->pSession);

			
			continue;
		}

		// 일정시간동안 수신이 없으면 종료처리.
		DWORD a = GetTickCount();
		DWORD diffTime = (a - pCharacter->pSession->dwLastRecvTime);
		if ( diffTime > dfNETWORK_PACKET_RECV_TIMEOUT)
		{
			DebugBreak();

			{
				SerializePacket sPacket;
				mpDeleteCharacter(&sPacket, pCharacter->dwSessionID);

				SendPacket_Around(pCharacter->pSession, &sPacket, false);
			}

			_LOG(dfLOG_LEVEL_DEBUG, L"# Heartbeat TIMEOUT... # SessionID:%d\n", pCharacter->dwSessionID);

			//TEST
			_LOG(dfLOG_LEVEL_SYSTEM, L"# Heartbeat # SessionID:%d\n", pCharacter->dwSessionID);
			PushQuitQ(pCharacter->pSession);

			continue;
		}

		switch (pCharacter->byMoveDirection)
		{
		case dfPACKET_MOVE_DIR_LL:
			Move(deltaTime, pCharacter, pCharacter->byMoveDirection);
			break;
		case dfPACKET_MOVE_DIR_LU:
			Move(deltaTime, pCharacter, pCharacter->byMoveDirection);
			break;
		case dfPACKET_MOVE_DIR_UU:
			Move(deltaTime, pCharacter, pCharacter->byMoveDirection);
			break;
		case dfPACKET_MOVE_DIR_RU:
			Move(deltaTime, pCharacter, pCharacter->byMoveDirection);
			break;
		case dfPACKET_MOVE_DIR_RR:
			Move(deltaTime, pCharacter, pCharacter->byMoveDirection);
			break;
		case dfPACKET_MOVE_DIR_RD:
			Move(deltaTime, pCharacter, pCharacter->byMoveDirection);
			break;
		case dfPACKET_MOVE_DIR_DD:
			Move(deltaTime, pCharacter, pCharacter->byMoveDirection);
			break;
		case dfPACKET_MOVE_DIR_LD:
			Move(deltaTime, pCharacter, pCharacter->byMoveDirection);
			break;
		}


		// 섹터 변경 체크
		if (pCharacter->dwAction >= dfPACKET_MOVE_DIR_LL && pCharacter->dwAction <= dfPACKET_MOVE_DIR_LD)
		{
			if (UpdateSector(pCharacter))
			{
				CharacterSectorUpdatePacket(pCharacter);
			}
		}
	}

	oldTime = GetTickCount();
}