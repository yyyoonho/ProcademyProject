#include "pch.h"

#include "SectorManager.h"
#include "CharacterManager.h"
#include "Network.h"
#include "Protocol.h"
#include "LogManager.h"

#include "Game.h"

using namespace std;

#define FRAME 50
#define MSPERFRAME (1000/FRAME)

bool FrameControl()
{
	static int oldTime = timeGetTime();

	int diffTime = timeGetTime() - oldTime;

	if (diffTime < MSPERFRAME)
	{
		//Sleep(MSPERFRAME - diffTime); 
		return false;
	}
	else
	{
		oldTime += MSPERFRAME;
		return true;
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
	if (y < dfRANGE_MOVE_TOP || y >= dfRANGE_MOVE_RIGHT || x < dfRANGE_MOVE_LEFT || x >= dfRANGE_MOVE_BOTTOM)
		return false;

	return true;
}

void GameUpdate()
{
	if (!FrameControl())
		return;

	static DWORD oldTime = GetTickCount();
	DWORD deltaTime = GetTickCount() - oldTime;

	unordered_map<DWORD, stCharacter*>::iterator iter;
	for (iter = characterMap.begin(); iter != characterMap.end(); ++iter)
	{
		stCharacter* pCharacter = iter->second;

		// hp가 0이하면 종료처리.
		if (pCharacter->chHP <= 0)
		{
			PushSessionToDestroyQ(pCharacter->pSession);
			continue;
		}

		// 일정시간동안 수신이 없으면 종료처리.
		if (GetTickCount() - pCharacter->pSession->dwLastRecvTime > dfNETWORK_PACKET_RECV_TIMEOUT)
		{
			PushSessionToDestroyQ(pCharacter->pSession);

			_LOG(dfLOG_LEVEL_DEBUG, L"# Heartbeat TIMEOUT... # SessionID:%d\n", pCharacter->dwSessionID);

			continue;
		}

		double deltaTimeSec = (double)deltaTime / 1000;
		switch (pCharacter->dwAction)
		{
		case dfPACKET_MOVE_DIR_LL:
		{
			double dNextY = pCharacter->dY;
			double dNextX = pCharacter->dX + ((dfSPEED_PLAYER_X * 25) * deltaTimeSec) * -1;

			short shNextY = (short)(dNextY);
			short shNextX = (short)(dNextX);

			if (!CanGo(shNextY, shNextX))
				break;

			pCharacter->dY = dNextY;
			pCharacter->dX = dNextX;

			pCharacter->shY = shNextY;
			pCharacter->shX = shNextX;

			_LOG(dfLOG_LEVEL_DEBUG, L"# Moving... # SessionID:%d / Action:%d / X:%d / Y:%d\n",
				pCharacter->dwSessionID, pCharacter->dwAction, pCharacter->shX, pCharacter->shY);
		}
			break;

		case dfPACKET_MOVE_DIR_LU:
		{
			double dNextY = pCharacter->dY + ((dfSPEED_PLAYER_Y * 25) * deltaTimeSec) * -1;
			double dNextX = pCharacter->dX + ((dfSPEED_PLAYER_X * 25) * deltaTimeSec) * -1;

			short shNextY = (short)(dNextY);
			short shNextX = (short)(dNextX);

			if (!CanGo(shNextY, shNextX))
				break;

			pCharacter->dY = dNextY;
			pCharacter->dX = dNextX;

			pCharacter->shY = shNextY;
			pCharacter->shX = shNextX;

			_LOG(dfLOG_LEVEL_DEBUG, L"# Moving... # SessionID:%d / Action:%d / X:%d / Y:%d\n",
				pCharacter->dwSessionID, pCharacter->dwAction, pCharacter->shX, pCharacter->shY);
		}
			break;

		case dfPACKET_MOVE_DIR_UU:
		{
			double dNextY = pCharacter->dY + ((dfSPEED_PLAYER_Y * 25) * deltaTimeSec) * -1;
			double dNextX = pCharacter->dX;

			short shNextY = (short)(dNextY);
			short shNextX = (short)(dNextX);

			if (!CanGo(shNextY, shNextX))
				break;

			pCharacter->dY = dNextY;
			pCharacter->dX = dNextX;

			pCharacter->shY = shNextY;
			pCharacter->shX = shNextX;

			_LOG(dfLOG_LEVEL_DEBUG, L"# Moving... # SessionID:%d / Action:%d / X:%d / Y:%d\n",
				pCharacter->dwSessionID, pCharacter->dwAction, pCharacter->shX, pCharacter->shY);
		}
			break;

		case dfPACKET_MOVE_DIR_RU:
		{
			double dNextY = pCharacter->dY + ((dfSPEED_PLAYER_Y * 25) * deltaTimeSec) * -1;
			double dNextX = pCharacter->dX + ((dfSPEED_PLAYER_X * 25) * deltaTimeSec) * 1;

			short shNextY = (short)(dNextY);
			short shNextX = (short)(dNextX);

			if (!CanGo(shNextY, shNextX))
				break;

			pCharacter->dY = dNextY;
			pCharacter->dX = dNextX;

			pCharacter->shY = shNextY;
			pCharacter->shX = shNextX;

			_LOG(dfLOG_LEVEL_DEBUG, L"# Moving... # SessionID:%d / Action:%d / X:%d / Y:%d\n",
				pCharacter->dwSessionID, pCharacter->dwAction, pCharacter->shX, pCharacter->shY);
		}
			break;

		case dfPACKET_MOVE_DIR_RR:
		{
			double dNextY = pCharacter->dY;
			double dNextX = pCharacter->dX + ((dfSPEED_PLAYER_X * 25) * deltaTimeSec) * 1;

			short shNextY = (short)(dNextY);
			short shNextX = (short)(dNextX);

			if (!CanGo(shNextY, shNextX))
				break;

			pCharacter->dY = dNextY;
			pCharacter->dX = dNextX;

			pCharacter->shY = shNextY;
			pCharacter->shX = shNextX;

			_LOG(dfLOG_LEVEL_DEBUG, L"# Moving... # SessionID:%d / Action:%d / X:%d / Y:%d\n",
				pCharacter->dwSessionID, pCharacter->dwAction, pCharacter->shX, pCharacter->shY);
		}
			break;

		case dfPACKET_MOVE_DIR_RD:
		{
			double dNextY = pCharacter->dY + ((dfSPEED_PLAYER_Y * 25) * deltaTimeSec) * 1;
			double dNextX = pCharacter->dX + ((dfSPEED_PLAYER_X * 25) * deltaTimeSec) * 1;

			short shNextY = (short)(dNextY);
			short shNextX = (short)(dNextX);

			if (!CanGo(shNextY, shNextX))
				break;

			pCharacter->dY = dNextY;
			pCharacter->dX = dNextX;

			pCharacter->shY = shNextY;
			pCharacter->shX = shNextX;

			_LOG(dfLOG_LEVEL_DEBUG, L"# Moving... # SessionID:%d / Action:%d / X:%d / Y:%d\n",
				pCharacter->dwSessionID, pCharacter->dwAction, pCharacter->shX, pCharacter->shY);
		}
			break;

		case dfPACKET_MOVE_DIR_DD:
		{
			double dNextY = pCharacter->dY + ((dfSPEED_PLAYER_Y * 25) * deltaTimeSec) * 1;
			double dNextX = pCharacter->dX;

			short shNextY = (short)(dNextY);
			short shNextX = (short)(dNextX);

			if (!CanGo(shNextY, shNextX))
				break;

			pCharacter->dY = dNextY;
			pCharacter->dX = dNextX;

			pCharacter->shY = shNextY;
			pCharacter->shX = shNextX;

			_LOG(dfLOG_LEVEL_DEBUG, L"# Moving... # SessionID:%d / Action:%d / X:%d / Y:%d\n",
				pCharacter->dwSessionID, pCharacter->dwAction, pCharacter->shX, pCharacter->shY);
		}
			break;

		case dfPACKET_MOVE_DIR_LD:
		{
			double dNextY = pCharacter->dY = ((dfSPEED_PLAYER_Y * 25) * deltaTimeSec) * 1;
			double dNextX = pCharacter->dX + ((dfSPEED_PLAYER_X * 25) * deltaTimeSec) * -1;

			short shNextY = (short)(dNextY);
			short shNextX = (short)(dNextX);

			if (!CanGo(shNextY, shNextX))
				break;

			pCharacter->dY = dNextY;
			pCharacter->dX = dNextX;

			pCharacter->shY = shNextY;
			pCharacter->shX = shNextX;

			_LOG(dfLOG_LEVEL_DEBUG, L"# Moving... # SessionID:%d / Action:%d / X:%d / Y:%d\n",
				pCharacter->dwSessionID, pCharacter->dwAction, pCharacter->shX, pCharacter->shY);
		}
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