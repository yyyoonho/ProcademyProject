#include "stdafx.h"

#include "LogManager.h"
#include "SectorManager.h"
#include "CharacterManager.h"
#include "Network.h"

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

	if (timeGetTime() - oldTime < 1000)
		return;

	// 모니터링
	printf("SessionCount: %d | CharacterCount: %d\n", GetSessionSize(), GetCharacterSize());

	LogDeltaTime();

	oldTime += 1000;
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
	DWORD avgTime = sumTime / deltaTimeCount;

	_LOG(dfLOG_LEVEL_SYSTEM, L"DeltaTime| Min: %d | Max: %d | Avg: %d\n", minTime, maxTime, avgTime);

	minTime = 99999999;
	maxTime = 0;
	deltaTimeCount = 0;
	sumTime = 0;
}