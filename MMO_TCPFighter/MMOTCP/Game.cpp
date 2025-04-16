#include <Windows.h>
#include <iostream>
#include <list>
#include <unordered_map>

#include "MemoryPool.h"
#include "RingBuffer.h"

#include "Struct.h"
#include "Game.h"

using namespace std;

procademy::MemoryPool<stCharacter> characterMP(0, false);
unordered_map<DWORD, stCharacter* > characterMap;

void GameUpdate()
{
}

void CreateCharacter(stSession* pSession, DWORD dwSessionID)
{
	return;
}