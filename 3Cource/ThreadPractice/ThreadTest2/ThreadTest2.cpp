#pragma comment(lib, "winmm.lib")

#include <iostream>
#include <Windows.h>
#include <thread>
#include <conio.h>
#include <list>

using namespace std;

// 전역변수
list<int> g_list;

// 동기화객체
SRWLOCK srwLock;

// 이벤트객체
HANDLE hSaveEvent;
HANDLE hQuitEvent;

// 쓰레드 핸들
HANDLE threadHandles[6];

void PrintThread()
{
    while (1)
    {
        DWORD ret = WaitForSingleObject(hQuitEvent, 1000);
        if (ret != WAIT_TIMEOUT)
        {
            return;
        }

        AcquireSRWLockShared(&srwLock);
        list<int>::iterator iter;
        for (iter = g_list.begin(); iter != g_list.end(); ++iter)
        {
            printf("%d - ", *iter);
        }
        ReleaseSRWLockShared(&srwLock);
        printf("\n");
    }
}

void DeleteThread()
{
    while (1)
    {
        DWORD ret = WaitForSingleObject(hQuitEvent, 333);
        if (ret != WAIT_TIMEOUT)
        {
            return;
        }

        AcquireSRWLockExclusive(&srwLock);
        if(!g_list.empty())
            g_list.pop_back();
        ReleaseSRWLockExclusive(&srwLock);
    }
}

void WorkerThread()
{
    srand(time(NULL) + GetCurrentThreadId());

    while (1)
    {
        DWORD ret = WaitForSingleObject(hQuitEvent, 1000);
        if (ret != WAIT_TIMEOUT)
        {
            return;
        }

        int randNum = rand();
        AcquireSRWLockExclusive(&srwLock);
        g_list.push_front(randNum);
        ReleaseSRWLockExclusive(&srwLock);
    }
}

void SaveThread()
{
    HANDLE eventHandles[2];

    eventHandles[0] = hSaveEvent;
    eventHandles[1] = hQuitEvent;

    while (1)
    {
        DWORD ret = WaitForMultipleObjects(2, eventHandles, FALSE, INFINITE);
        if (ret == WAIT_OBJECT_0 + 1)
        {
            return;
        }

        Sleep(1000);
        
        // Lock();

        // 잽싸게 파일에 쓸 데이터들을 지역변수로 저장.

        // UnLock();

        // 독점권 해제 후, 여유롭게 파일 쓰기 시작.
    }
}

int main()
{
    timeBeginPeriod(1);

    srand(time(NULL));
    InitializeSRWLock(&srwLock);

    hSaveEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    hQuitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (hSaveEvent == NULL)
        return 0;
    if (hQuitEvent == NULL)
        return 0;


    for (int i = 0; i < 10; i++)
    {
        g_list.push_back(rand());
    }

    threadHandles[0] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&PrintThread, NULL, NULL, NULL);
    threadHandles[1] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&DeleteThread, NULL, NULL, NULL);
    threadHandles[2] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&WorkerThread, NULL, NULL, NULL);
    threadHandles[3] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&WorkerThread, NULL, NULL, NULL);
    threadHandles[4] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&WorkerThread, NULL, NULL, NULL);
    threadHandles[5] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&SaveThread, NULL, NULL, NULL);

    while (1)
    {
        if (_kbhit())
        {
            int inputKey = _getch();
            
            if (inputKey == 'Q' || inputKey == 'q')
            {
                SetEvent(hQuitEvent);
                break;
            }

            else if (inputKey == 'S' || inputKey == 's')
            {
                SetEvent(hSaveEvent);
            }
        }
    }


    WaitForMultipleObjects(6, threadHandles, TRUE, INFINITE);

    return 0;
}