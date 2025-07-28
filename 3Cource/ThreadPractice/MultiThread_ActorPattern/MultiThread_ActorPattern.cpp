#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "RingBuffer.lib")

#include <Windows.h>
#include <iostream>
#include <thread>
#include <list>
#include <string.h>
#include <algorithm>
#include <map>

#include "RingBuffer.h"
#include "Protocol.h"

using namespace std;

#define BUFSIZE 100
#define DEFAULTFRAME 50

//wstring str[5] = { L"HANHWA", L"LG", L"LOTTE", L"KIA", L"KT" };
wstring str[5] = { L"한화", L"엘지", L"롯데", L"기아", L"케티" };

//-----------------------------------------------
// 컨텐츠 부, 문자열 리스트
//-----------------------------------------------
list<wstring>		g_List;

//-----------------------------------------------
// 스레드 메시지 큐 (사이즈 넉넉하게 크게 4~5만 바이트)
//-----------------------------------------------
RingBuffer 			g_msgQ(50000);


SRWLOCK msgQ_Lock;
SRWLOCK list_Lock;
HANDLE hEventForMsgQ;
HANDLE hEventForQuitThread;

HANDLE hWorkerThread[3];

HANDLE hMonitorThread;

LONG TPS_ALLMSG;

LONG TPS_ADD;
LONG TPS_DEL;
LONG TPS_SORT;
LONG TPS_FIND;
LONG TPS_PRINT;

map<DWORD, LONG*> TPS_WORKER_map;
LONG TPS_WORKER[3];

int g_Frame = 50;
int g_FrameCount = 0;

float g_UsePercent;

void MonitorThread()
{
	while (1)
	{
		DWORD ret = WaitForSingleObject(hEventForQuitThread, 1000);
		if (ret != WAIT_TIMEOUT)
		{
			return;
		}

		LONG tpsALL = TPS_ALLMSG;

		LONG tpsADD = TPS_ADD;
		LONG tpsDEL = TPS_DEL;
		LONG tpsSORT = TPS_SORT;
		LONG tpsFIND = TPS_FIND;
		LONG tpsPRINT = TPS_PRINT;

		LONG tpsWorker0 = TPS_WORKER[0];
		LONG tpsWorker1 = TPS_WORKER[1];
		LONG tpsWorker2 = TPS_WORKER[2];

		int useSize = g_msgQ.GetUseSize();
		g_UsePercent = ((float)useSize / (float)g_msgQ._capacity) * 100;

		InterlockedExchange(&TPS_ALLMSG, 0);
		InterlockedExchange(&TPS_ADD, 0);
		InterlockedExchange(&TPS_DEL, 0);
		InterlockedExchange(&TPS_SORT, 0);
		InterlockedExchange(&TPS_FIND, 0);
		InterlockedExchange(&TPS_PRINT, 0);
		InterlockedExchange(&TPS_WORKER[0], 0);
		InterlockedExchange(&TPS_WORKER[1], 0);
		InterlockedExchange(&TPS_WORKER[2], 0);

		printf("----------------------------------------------------------\n");
		printf("Frame: %d\n", g_FrameCount);
		g_FrameCount = 0;
		printf("\n");

		printf("TPS_ALLMSG: %d\n", tpsALL);
		printf("\n");
		printf("TPS_ADD: %d\n", tpsADD);
		printf("TPS_DEL: %d\n", tpsDEL);
		printf("TPS_SORT: %d\n", tpsSORT);
		printf("TPS_FIND: %d\n", tpsFIND);
		printf("TPS_PRINT: %d\n", tpsPRINT);
		printf("\n");
		printf("TPS_WORKER0: %d\n", tpsWorker0);
		printf("TPS_WORKER1: %d\n", tpsWorker1);
		printf("TPS_WORKER2: %d\n", tpsWorker2);
		printf("\n");
		printf("RINGBUFFER USESIZE: %d / %d [ %f %%]\n", useSize, g_msgQ._capacity, g_UsePercent);
		printf("----------------------------------------------------------\n");
	}
}

void WorkerThread()
{
	while (1)
	{
		WaitForSingleObject(hEventForMsgQ, INFINITE);

		AcquireSRWLockExclusive(&msgQ_Lock);

		st_MSG_HEAD msgHeader;
		wstring strData;

		msgHeader.shPayloadLen = -1;
		msgHeader.shType = -1;

		if (g_msgQ.GetUseSize() >= sizeof(st_MSG_HEAD))
		{
			g_msgQ.Dequeue((char*)&msgHeader, sizeof(st_MSG_HEAD));
		}

		if (msgHeader.shType == dfJOB_ADD || msgHeader.shType == dfJOB_FIND || msgHeader.shType == dfJOB_DEL)
		{
			g_msgQ.Dequeue((char*)&strData, msgHeader.shPayloadLen);
		}

		if (g_msgQ.GetUseSize() >= sizeof(st_MSG_HEAD))
		{
			SetEvent(hEventForMsgQ);
		}

		ReleaseSRWLockExclusive(&msgQ_Lock);

		switch (msgHeader.shType)
		{
		case dfJOB_ADD:
		{
			AcquireSRWLockExclusive(&list_Lock);

			g_List.push_front(strData);

			ReleaseSRWLockExclusive(&list_Lock);

			InterlockedIncrement(&TPS_ADD);
		}
			break;
		case dfJOB_DEL:
		{
			AcquireSRWLockExclusive(&list_Lock);

			list<wstring>::iterator iter;
			iter = find(g_List.begin(), g_List.end(), strData);

			/*if (iter != g_List.end())
				g_List.erase(iter);*/

			g_List.clear();

			ReleaseSRWLockExclusive(&list_Lock);

			InterlockedIncrement(&TPS_DEL);
		}
			break;
		case dfJOB_SORT:
		{
			AcquireSRWLockExclusive(&list_Lock);

			g_List.sort();

			ReleaseSRWLockExclusive(&list_Lock);

			InterlockedIncrement(&TPS_SORT);
		}
			break;
		case dfJOB_FIND:
		{
			AcquireSRWLockShared(&list_Lock);

			list<wstring>::iterator iter;
			iter = find(g_List.begin(), g_List.end(), strData);

			ReleaseSRWLockShared(&list_Lock);

			InterlockedIncrement(&TPS_FIND);
		}
			break;
		case dfJOB_PRINT:
		{
			AcquireSRWLockShared(&list_Lock);

			wstring wStrArr[100];

			int cnt = 0;
			list<wstring>::iterator iter;
			for (iter = g_List.begin(); iter != g_List.end(); ++iter)
			{
				//wStrArr[cnt] = *iter;
				cnt++;
			}

			ReleaseSRWLockShared(&list_Lock);

			/*for (int i = 0; i < cnt; i++)
			{
				wprintf(L"%ls - ", wStrArr[i]);
			}
			cout << endl;*/

			// 출력을 1000으로 표현
			Sleep(1000);

			InterlockedIncrement(&TPS_PRINT);
		}
			break;
		case dfJOB_QUIT:
		{
			AcquireSRWLockShared(&list_Lock);

			g_msgQ.Enqueue((char*)&msgHeader, sizeof(msgHeader));
			SetEvent(hEventForMsgQ);

			ReleaseSRWLockShared(&list_Lock);

			return;
		}
			break;
		}

		InterlockedIncrement(&TPS_ALLMSG);

		InterlockedIncrement(TPS_WORKER_map[GetCurrentThreadId()]);
	}
}

bool Frame()
{
	static DWORD oldTime = timeGetTime();

	if (g_UsePercent >= 80)
	{
		g_Frame = DEFAULTFRAME / 10;
	}
	if (g_UsePercent <= 50)
	{
		g_Frame = DEFAULTFRAME;
	}


	DWORD diffTime = timeGetTime() - oldTime;
	if (diffTime >= (1000 / g_Frame))
	{
		oldTime += (1000 / g_Frame);
		g_FrameCount++;
		return true;
	}
	else
	{
		return false;
	}
}

int main()
{
	timeBeginPeriod(1);

	setlocale(LC_ALL, "korean");
	_wsetlocale(LC_ALL, L"korean");

	InitializeSRWLock(&msgQ_Lock);
	InitializeSRWLock(&list_Lock);
	hEventForMsgQ = CreateEvent(NULL, FALSE, FALSE, NULL);
	hEventForQuitThread = CreateEvent(NULL, FALSE, FALSE, NULL);
	
	srand(time(NULL));

	// 모니터링 쓰레드 생성
	hMonitorThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&MonitorThread, NULL, NULL, NULL);

	// 워커쓰레드 생성
	for (int i = 0; i < 3; i++)
	{
		hWorkerThread[i] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&WorkerThread, NULL, NULL, NULL);

		DWORD threadID = GetThreadId(hWorkerThread[i]);
		TPS_WORKER_map.insert(make_pair(threadID, &TPS_WORKER[i]));
	}

	while (1)
	{
		if (!Frame())
			continue;

		st_MSG_HEAD msgHeader;
		int msgType = rand() % 5; // 0 ~ 4
		int strType = rand() % 5;

		// 메시지 생성
		{
			msgHeader.shType = msgType;
			msgHeader.shPayloadLen = 0;

			if (msgType == dfJOB_ADD || msgType == dfJOB_FIND || msgType == dfJOB_DEL)
			{
				msgHeader.shPayloadLen = (str[strType].size() + 1) * sizeof(str[0]);
			}
		}

		// 메시지 인큐
		{
			AcquireSRWLockExclusive(&msgQ_Lock);

			g_msgQ.Enqueue((char*)&msgHeader, sizeof(msgHeader));
			if (msgHeader.shType == dfJOB_ADD || msgHeader.shType == dfJOB_FIND || msgHeader.shType == dfJOB_DEL)
			{
				g_msgQ.Enqueue((char*)&str[strType], msgHeader.shPayloadLen);
			}

			ReleaseSRWLockExclusive(&msgQ_Lock);

			SetEvent(hEventForMsgQ);
		}

		if (GetAsyncKeyState(VK_UP) & 0x8001)
		{
			AcquireSRWLockExclusive(&msgQ_Lock);

			msgHeader.shType = dfJOB_QUIT;
			msgHeader.shPayloadLen = -1;

			g_msgQ.Enqueue((char*)&msgHeader, sizeof(msgHeader));
			SetEvent(hEventForMsgQ);

			ReleaseSRWLockExclusive(&msgQ_Lock);
			break;
		}
	}
	
	WaitForMultipleObjects(3, hWorkerThread, TRUE, INFINITE);

	return 0;
}