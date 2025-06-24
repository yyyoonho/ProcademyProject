#include "stdafx.h"

#include "LogManager.h"
#include "SectorManager.h"
#include "CharacterManager.h"
#include "Network.h"
#include "MMOTCP.h"

#include "MonitorManager.h"

using namespace std;

DWORD minTime = 99999999;
DWORD maxTime = 0;
DWORD sumTime;
int deltaTimeCount;

void LogDeltaTime();

void Monitor()
{
	static DWORD oldTime = timeGetTime();

	int diffTime = timeGetTime() - oldTime;
	if (diffTime < 1000)
		return;

	printf("---------------------------------------------------------\n");

	// 현재 접속한 접속자 (세션 수 | 캐릭터 수)
	printf("SessionCount: %d | CharacterCount: %d\n", GetSessionSize(), GetCharacterSize());

	// 게임 로직 프레임
	printf("Frame: %d\n", g_FrameCount);
	g_FrameCount = 0;

	LogDeltaTime();

	oldTime += 1000;

	printf("---------------------------------------------------------\n\n");
}

void PushDeltaTime(DWORD deltaTime)
{
	deltaTimeCount++;

	if (minTime > deltaTime)
		minTime = deltaTime;

	if (maxTime < deltaTime)
		maxTime = deltaTime;

	sumTime += deltaTime;
}

void LogDeltaTime()
{
	if (sumTime == 0 || deltaTimeCount == 0)
		return;

	DWORD avgTime = sumTime / deltaTimeCount;

	_LOG(dfLOG_LEVEL_DEBUG, L"DeltaTime| Min: %d | Max: %d | Avg: %d\n", minTime, maxTime, avgTime);

	minTime = 99999999;
	maxTime = 0;
	deltaTimeCount = 0;
	sumTime = 0;
}