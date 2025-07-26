#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "RingBuffer.lib")

#include <Windows.h>
#include <iostream>
#include <thread>
#include <list>
#include <string.h>
#include <algorithm>

#include "RingBuffer.h"
#include "Protocol.h"

using namespace std;

#define BUFSIZE 100

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

HANDLE hWorkerThread[3];


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
		}
			break;
		case dfJOB_DEL:
		{
			AcquireSRWLockExclusive(&list_Lock);

			list<wstring>::iterator iter;
			iter = find(g_List.begin(), g_List.end(), strData);

			if (iter != g_List.end())
				g_List.erase(iter);

			ReleaseSRWLockExclusive(&list_Lock);
		}
			break;
		case dfJOB_SORT:
		{
			AcquireSRWLockExclusive(&list_Lock);

			g_List.sort();

			ReleaseSRWLockExclusive(&list_Lock);
		}
			break;
		case dfJOB_FIND:
		{
			AcquireSRWLockShared(&list_Lock);

			list<wstring>::iterator iter;
			iter = find(g_List.begin(), g_List.end(), strData);

			ReleaseSRWLockShared(&list_Lock);
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
				wStrArr[cnt] = *iter;
				cnt++;
			}

			ReleaseSRWLockShared(&list_Lock);

			for (int i = 0; i < cnt; i++)
			{
				wprintf(L"%ls - ", wStrArr[i]);
			}
			cout << endl;

		}
			break;
		case dfJOB_QUIT:
		{
			AcquireSRWLockShared(&list_Lock);



			ReleaseSRWLockShared(&list_Lock);

			return;
		}
			break;
		}

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
	
	srand(time(NULL) + GetCurrentThreadId());

	// 워커쓰레드 생성
	for (int i = 0; i < 3; i++)
	{
		hWorkerThread[i] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&WorkerThread, NULL, NULL, NULL);
	}

	while (1)
	{
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

		Sleep(50);
	}
	
	WaitForMultipleObjects(3, hWorkerThread, TRUE, INFINITE);

	return 0;
}