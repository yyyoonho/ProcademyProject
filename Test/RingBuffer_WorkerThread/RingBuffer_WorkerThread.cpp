#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "RingBuffer.lib")

#include <Windows.h>
#include <iostream>
#include <list>
#include <conio.h>
#include <thread>
#include <algorithm>

#include "RingBuffer.h"

using namespace std;

struct st_MSG_HEAD
{
    short shType;
    short shPayloadLen;
};

wstring strings[10] = {L"나의사랑한글날", L"맥도날드", L"샴푸", L"츄파츕스", L"WORLD", L"RING",L"스타워즈",L"아이폰12",L"Holdem", L"건담"};

#define dfJOB_ADD	0
#define dfJOB_DEL	1
#define dfJOB_SORT	2
#define dfJOB_FIND	3
#define dfJOB_PRINT	4	// 출력 or 저장 / 읽기만 하는 느린 행동
#define dfJOB_QUIT	5	

//-----------------------------------------------
// 컨텐츠 부, 문자열 리스트
//-----------------------------------------------
list<wstring>		g_List;

//-----------------------------------------------
// 스레드 메시지 큐 (사이즈 넉넉하게 크게 4~5만 바이트)
//-----------------------------------------------
RingBuffer 		g_msgQ(40000);

SRWLOCK srwLock_msgQ;
SRWLOCK srwLock_list;
HANDLE workEvent;

LONG g_Total_TPS; // 초당 메시지 전체 처리수
LONG g_Thread0_TPS; // 초당 쓰레드 별 처리수
LONG g_Thread1_TPS; // 초당 쓰레드 별 처리수
LONG g_Thread2_TPS; // 초당 쓰레드 별 처리수
LONG g_Msg0_TPS;     // 초당 메시지별 처리수
LONG g_Msg1_TPS;     // 초당 메시지별 처리수
LONG g_Msg2_TPS;     // 초당 메시지별 처리수
LONG g_Msg3_TPS;     // 초당 메시지별 처리수
LONG g_Msg4_TPS;     // 초당 메시지별 처리수
LONG g_Msg5_TPS;     // 초당 메시지별 처리수


void WorkerThread()
{
    while (1)
    {
        WaitForSingleObject(workEvent, INFINITE);

        AcquireSRWLockExclusive(&srwLock_msgQ);

        st_MSG_HEAD tmpHead;
        wstring tmpString;
        if (g_msgQ.GetUseSize() >= sizeof(st_MSG_HEAD))
        {
            g_msgQ.Dequeue((char*)&tmpHead, sizeof(st_MSG_HEAD));

            _InterlockedIncrement(&g_Total_TPS);
        }

        if (tmpHead.shType == dfJOB_ADD || tmpHead.shType == dfJOB_FIND)
        {
            if (g_msgQ.GetUseSize() >= tmpHead.shPayloadLen)
            {
                g_msgQ.Dequeue((char*)&tmpString, tmpHead.shPayloadLen);
            }
        }

        if (g_msgQ.GetUseSize() > 0)
        {
            SetEvent(&workEvent);
        }

        ReleaseSRWLockExclusive(&srwLock_msgQ);

        switch (tmpHead.shType)
        {
        case dfJOB_ADD:
        {
            AcquireSRWLockExclusive(&srwLock_list);

            g_List.push_back(tmpString);
            _InterlockedIncrement(&g_Msg0_TPS);

            ReleaseSRWLockExclusive(&srwLock_list);
        }
            break;
        case dfJOB_DEL:
        {
            AcquireSRWLockExclusive(&srwLock_list);

            list<wstring>::iterator iter;
            iter = find(g_List.begin(), g_List.end(), tmpString);

            if (iter != g_List.end())
                g_List.erase(iter);

            _InterlockedIncrement(&g_Msg1_TPS);

            ReleaseSRWLockExclusive(&srwLock_list);
        }
            break;
        case dfJOB_SORT:
        {
            AcquireSRWLockExclusive(&srwLock_list);

            //sort(g_List.begin(), g_List.end());

            _InterlockedIncrement(&g_Msg2_TPS);

            ReleaseSRWLockExclusive(&srwLock_list);
        }
            break;
        case dfJOB_FIND:
        {
            AcquireSRWLockShared(&srwLock_list);

            list<wstring>::iterator iter;
            iter = find(g_List.begin(), g_List.end(), tmpString);

            _InterlockedIncrement(&g_Msg3_TPS);

            ReleaseSRWLockShared(&srwLock_list);
        }
            break;
        case dfJOB_PRINT:
        {
            AcquireSRWLockShared(&srwLock_list);

            // TODO: 느린작업
            printf("HEllOHEllOHEllOHEllO\n");

            _InterlockedIncrement(&g_Msg4_TPS);

            ReleaseSRWLockShared(&srwLock_list);
        }
            break;
        case dfJOB_QUIT:
        {
            AcquireSRWLockExclusive(&srwLock_msgQ);

            g_msgQ.Enqueue((char*)&tmpHead, sizeof(st_MSG_HEAD));
            SetEvent(workEvent);

            _InterlockedIncrement(&g_Msg5_TPS);

            ReleaseSRWLockExclusive(&srwLock_msgQ);
            return;
        }
            break;
        }
    }
}

void MonitorThread()
{
    while (1)
    {
        Sleep(1000);

        AcquireSRWLockShared(&srwLock_msgQ);

        int useSize = g_msgQ.GetUseSize();

        ReleaseSRWLockShared(&srwLock_msgQ);

        printf("메시지 큐 의 사이즈 (사용량): %d\n", useSize);
        printf("초당 메시지 전체 처리 수: %d\n", _InterlockedExchange(&g_Total_TPS,0));
        printf("초당 0번 메시지 처리 수: %d\n", _InterlockedExchange(&g_Msg0_TPS,0));
        printf("초당 1번 메시지 처리 수: %d\n", _InterlockedExchange(&g_Msg1_TPS,0));
        printf("초당 2번 메시지 처리 수: %d\n", _InterlockedExchange(&g_Msg2_TPS,0));
        printf("초당 3번 메시지 처리 수: %d\n", _InterlockedExchange(&g_Msg3_TPS,0));
        printf("초당 4번 메시지 처리 수: %d\n", _InterlockedExchange(&g_Msg4_TPS,0));
        printf("초당 5번 메시지 처리 수: %d\n", _InterlockedExchange(&g_Msg5_TPS,0));
    }
}

void Frame()
{
    static DWORD oldTime = timeGetTime();

    DWORD diffTime = timeGetTime() - oldTime;

    if (diffTime < 50)
    {
        Sleep(50 - diffTime);
        oldTime += 50;
        return;
    }
    else
    {
        oldTime += 50;
        return;
    }

}

int main()
{
    timeBeginPeriod(1);
    srand(time(NULL));

    InitializeSRWLock(&srwLock_msgQ);
    InitializeSRWLock(&srwLock_list);
    workEvent = CreateEvent(NULL, false, true, NULL);

    HANDLE threadHandles[3];
    threadHandles[0] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&WorkerThread, NULL, NULL, NULL);
    threadHandles[1] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&WorkerThread, NULL, NULL, NULL);
    threadHandles[2] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&WorkerThread, NULL, NULL, NULL);

    HANDLE monitorThreadHandle;
    monitorThreadHandle = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&MonitorThread, NULL, NULL, NULL);

    while (1)
    {
        Frame();

        // TODO: 메시지 생성
        st_MSG_HEAD msgHead;
        short msgType = rand() % 5;
        short msgNum = rand() % 10;
        msgHead.shType = msgType;
        msgHead.shPayloadLen = 0;
        if (msgType == dfJOB_ADD || msgType == dfJOB_FIND)
        {
            
            msgHead.shPayloadLen = strings[msgNum].length() + 1;
        }

        // TODO: 메시지 인큐
        {
            AcquireSRWLockExclusive(&srwLock_msgQ);

            g_msgQ.Enqueue((char*)&msgHead, sizeof(msgHead));
            g_msgQ.Enqueue((char*)&strings[msgNum], msgHead.shPayloadLen);

            ReleaseSRWLockExclusive(&srwLock_msgQ);

            SetEvent(workEvent);
        }

        char key;
        if (_kbhit())
        {
            key = _getch();
            if (key == 'Q' || key == 'q')
            {
                // TODO: JOB_QUIT 메시지 인큐
                AcquireSRWLockExclusive(&srwLock_msgQ);

                msgHead.shType = dfJOB_QUIT;
                msgHead.shPayloadLen = 0;
                g_msgQ.Enqueue((char*)&msgHead, sizeof(msgHead));

                ReleaseSRWLockExclusive(&srwLock_msgQ);

                SetEvent(workEvent);

                break;
            }
        }
    }


    WaitForMultipleObjects(3, threadHandles, true, INFINITE);

    timeEndPeriod(1);
}