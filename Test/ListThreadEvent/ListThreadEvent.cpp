#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "CustomProfiler.lib")

#include <Windows.h>
#include <iostream>
#include <thread>
#include <list>
#include <conio.h>

#include "CustomProfiler.h"

using namespace std;

list<int> g_List;

HANDLE hEvent_Quit;
HANDLE hEvent_Save;

SRWLOCK srwLock;

void PrintFunc()
{
    while (1)
    {
        DWORD ret = WaitForSingleObject(hEvent_Quit, 1000);
        if (ret != WAIT_TIMEOUT)
        {
            return;
        }

        PRO_BEGIN("PrintThread");

        AcquireSRWLockShared(&srwLock);

        list<int>::iterator iter;
        for (iter = g_List.begin(); iter != g_List.end(); ++iter)
        {
            printf("%d-", *iter);
        }
        printf("\n");

        ReleaseSRWLockShared(&srwLock);

        PRO_END("PrintThread");
    }
}

void DeleteFunc()
{
    while (1)
    {
        DWORD ret = WaitForSingleObject(hEvent_Quit, 333);
        if (ret != WAIT_TIMEOUT)
        {
            return;
        }

        PRO_BEGIN("DeleteThread");

        AcquireSRWLockExclusive(&srwLock);

        if (!g_List.empty())
            g_List.pop_back();

        ReleaseSRWLockExclusive(&srwLock);

        PRO_END("DeleteThread");
    }
}

void InputFunc(int* seed)
{
    srand(time(NULL) * (*seed));

    char proName[20];
    sprintf_s(proName, 20, "%d InputThread", *seed);

    while (1)
    {
        DWORD ret = WaitForSingleObject(hEvent_Quit, 1000);
        if (ret != WAIT_TIMEOUT)
        {
            return;
        }

        
        PRO_BEGIN(proName);

        AcquireSRWLockExclusive(&srwLock);

        g_List.push_front(rand() % 500);

        ReleaseSRWLockExclusive(&srwLock);

        PRO_END(proName);
    }
}

void SaveFunc()
{
    HANDLE threadHandles[2];
    threadHandles[0] = hEvent_Save;
    threadHandles[1] = hEvent_Quit;

    while (1)
    {
        DWORD ret = WaitForMultipleObjects(2, threadHandles, false, INFINITE);
        
        if (ret == WAIT_OBJECT_0 + 1)
        {
            return;
        }

        
        FILE* fp;
        fopen_s(&fp, "ListThreadSave.txt", "wt");
        if (fp == 0)
            return;

        // 파일저장
        char buf[500];
        buf[0] = '\0';

        PRO_BEGIN("SaveThread");
        AcquireSRWLockShared(&srwLock);

        list<int>::iterator iter;
        for (iter = g_List.begin(); iter != g_List.end(); ++iter)
        {
            sprintf_s(buf + strlen(buf), sizeof(buf), "%d-", *iter);
        }
        
        ReleaseSRWLockShared(&srwLock);
        PRO_END("SaveThread");

        fwrite(buf, 1, strlen(buf), fp);

        fclose(fp);
    }

}

int main()
{
    timeBeginPeriod(1);

    hEvent_Quit = CreateEvent(NULL, true, NULL, NULL);
    hEvent_Save = CreateEvent(NULL, false, NULL, NULL);

    // 리스트가 텅비어있으면 허전하니까 세팅
    for (int i = 0; i < 10; i++)
    {
        g_List.push_front(i);
    }

    InitializeSRWLock(&srwLock);

    int seed1 = 1;
    int seed2 = 2;
    int seed3 = 3;

    HANDLE threadHandles[6];
    threadHandles[0] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&PrintFunc, NULL, NULL, NULL);
    threadHandles[1] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&DeleteFunc, NULL, NULL, NULL);
    threadHandles[2] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&InputFunc, (void*)&seed1, NULL, NULL);
    threadHandles[3] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&InputFunc, (void*)&seed2, NULL, NULL);
    threadHandles[4] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&InputFunc, (void*)&seed3, NULL, NULL);
    threadHandles[5] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&SaveFunc, NULL, NULL, NULL);
    

    while (1)
    {
        char key = _getch();

        if (key == 'S' || key == 's')
        {
            SetEvent(hEvent_Save);
        }

        if (key == 'Q' || key == 'q')
        {
            SetEvent(hEvent_Quit);
            break;
        }

        ProfilerInput();
    }

    WaitForMultipleObjects(6, threadHandles, true, INFINITE);

    timeEndPeriod(1);
}
