#include <Windows.h>
#include <iostream>
#include "CacheSimulator.h"

#pragma comment (lib, "winmm.lib")

using namespace std;

#define WAY 8

struct stCacheLine
{
    int _tag = -1;
    DWORD _time = MAXULONG32;
};

stCacheLine cacheLineArr[64][WAY];

bool SearchCacheLine(int idx, int tag, OUT int* way);

bool Cache(void* ptr, const char* paramName)
{
    unsigned int tmpPtr = (unsigned int)ptr;

    int idx = (tmpPtr & 0x00000fc0) >> 6;
    int tag = (tmpPtr & 0xfffff000) >> 12;

    int way;
    bool flag = SearchCacheLine(idx, tag, &way);

    if (flag == true)
    {
        cacheLineArr[idx][way]._time = timeGetTime();
        return true;
    }
    else
    {
        int LRUIdx = 0;
        DWORD LRUTime = cacheLineArr[idx][LRUIdx]._time;
        for (int i = 0; i < 8; i++)
        {
            if (LRUTime > cacheLineArr[idx][i]._time)
            {
                LRUIdx = i;
                LRUTime = cacheLineArr[idx][i]._time;
            }
        }

        cacheLineArr[idx][LRUIdx]._tag = tag;
        cacheLineArr[idx][LRUIdx]._time = timeGetTime();

        return false;
    }
}

bool SearchCacheLine(int idx, int tag, OUT int* way)
{
    for (int i = 0; i < WAY; i++)
    {
        if (cacheLineArr[idx][i]._tag != tag) continue;

        *way = i;
        return true;
    }

    *way = -1;
    return false;
}

int main()
{
    timeBeginPeriod(1);

    int a = 10;
    int b = 20;

    bool flag = Cache(&a, "a");
    bool flag2 = Cache(&b, "b");
    
    timeEndPeriod(1);
}
