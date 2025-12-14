#pragma comment(lib, "libmysql.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "RingBuffer.lib")

#include "mysql/include/mysql.h"
#include "mysql/include/errmsg.h"

#include <windows.h>
#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <conio.h>
#include <string>

#include "RingBuffer.h"

#include "DB_JOB.h"

using namespace std;

HANDLE hEVENT_QUIT;
HANDLE hEVENT_MSGQ;
HANDLE hThread_UpdateThread1;
HANDLE hThread_UpdateThread2;
HANDLE hThread_DBWriterThread;
HANDLE hThread_MonitoringThread;

RingBuffer msgQ;
mutex msgQ_Lock;

DWORD64 g_number = 0;

#define DEFAULTFRAME 50

DWORD64 msgTPS;

int g_Frame = DEFAULTFRAME;
LONG g_FrameCount = 0;
float g_UsePercent;

bool Frame()
{
	static thread_local DWORD oldTime = timeGetTime();

	DWORD diffTime = timeGetTime() - oldTime;
	if (diffTime >= (1000 / g_Frame))
	{
		oldTime += (1000 / g_Frame);
		//g_FrameCount++;
		InterlockedIncrement(&g_FrameCount);
		return true;
	}
	else
	{
		return false;
	}
}

void MonitoringThread()
{
	while (1)
	{
		DWORD ret = WaitForSingleObject(hEVENT_QUIT, 1000);
		if (ret == WAIT_OBJECT_0)
		{
			break;
		}

		int useSize = msgQ.GetUseSize();
		g_UsePercent = ((float)useSize / (float)msgQ._capacity) * 100;
		LONG frameCount = g_FrameCount;

		printf("----------------------------------------------------------\n");
		printf("UpdateThreads Frame: %d\n", frameCount/2);
		printf("DBWriterThread TPS: %lld\n", msgTPS);
		printf("\n");
		printf("RINGBUFFER USESIZE: %d / %d [ %f %%]\n", useSize, msgQ._capacity, g_UsePercent);
		printf("----------------------------------------------------------\n");

		InterlockedExchange(&g_FrameCount, 0);
		msgTPS = 0;

		if (g_UsePercent >= 80)
		{
			g_Frame = DEFAULTFRAME / 10;
		}
		if (g_UsePercent <= 30)
		{
			g_Frame = DEFAULTFRAME;
		}
	}
}

void UpdateThread()
{
	// TODO: 일감을 만들어 Q에 삽입합니다.
	srand(time(NULL) + GetCurrentThreadId());

	while (1)
	{
		if (!Frame())
			continue;

		DWORD ret = WaitForSingleObject(hEVENT_QUIT, 0);
		if (ret == WAIT_OBJECT_0)
		{
			break;
		}

		int type = rand() % 3;
		switch (type)
		{
		case 0:
		{
			st_DBQUERY_HEADER header;
			header.Type = df_DBQUERY_MSG_LEVELUP;

			st_DBQUERY_MSG_LEVELUP msg;
			msg.number = 0;

			{
				lock_guard<mutex> lock(msgQ_Lock);

				msgQ.Enqueue((char*)&header, sizeof(st_DBQUERY_HEADER));
				msgQ.Enqueue((char*)&msg, sizeof(st_DBQUERY_MSG_LEVELUP));
			}

			SetEvent(hEVENT_MSGQ);
		}
			break;
		case 1:
		{
			st_DBQUERY_HEADER header;
			header.Type = df_DBQUERY_MSG_MONEY_ADD;

			st_DBQUERY_MSG_MONEY_ADD msg;
			msg.number = 1;

			{
				lock_guard<mutex> lock(msgQ_Lock);

				msgQ.Enqueue((char*)&header, sizeof(st_DBQUERY_HEADER));
				msgQ.Enqueue((char*)&msg, sizeof(st_DBQUERY_MSG_MONEY_ADD));
			}

			SetEvent(hEVENT_MSGQ);
		}
			break;
		case 2:
		{
			st_DBQUERY_HEADER header;
			header.Type = df_DBQUERY_MSG_QUEST_COMPLETE;

			st_DBQUERY_MSG_QUEST_COMPLETE msg;
			msg.number = 2;

			{
				lock_guard<mutex> lock(msgQ_Lock);

				msgQ.Enqueue((char*)&header, sizeof(st_DBQUERY_HEADER));
				msgQ.Enqueue((char*)&msg, sizeof(st_DBQUERY_MSG_QUEST_COMPLETE));
			}

			SetEvent(hEVENT_MSGQ);
		}
			break;
		}

	}
}

void DBWriterThread()
{
	// TODO: 큐에서 메시지를 뽑아서 DB에 저장합니다.
	MYSQL conn;
	MYSQL* connection = NULL;

	// 초기화
	mysql_init(&conn);

	// DB 연결

	connection = mysql_real_connect(&conn, "127.0.0.1", "root", "dbsgh123!@", "mytest", 3306, (char*)NULL, 0);
	if (connection == NULL)
	{
		// mysql_errno(&_MySQL);
		fprintf(stderr, "Mysql connection error : %s", mysql_error(&conn));
		return;
	}

	//*************************************************************************************************************//

	HANDLE hThreads[2] = { hEVENT_QUIT, hEVENT_MSGQ };

	while (1)
	{
		DWORD ret = WaitForMultipleObjects(2, hThreads, FALSE, INFINITE);
		if (ret == WAIT_OBJECT_0)
		{
			break;
		}

		if (msgQ.GetUseSize() < sizeof(st_DBQUERY_HEADER))
		{
			continue;
		}

		st_DBQUERY_HEADER header;
		msgQ.Peek((char*)&header, sizeof(st_DBQUERY_HEADER));

		WORD type = header.Type;

		switch (type)
		{
		case df_DBQUERY_MSG_LEVELUP:
		{
			st_DBQUERY_MSG_LEVELUP msg;
			if (msgQ.GetUseSize() < sizeof(st_DBQUERY_HEADER) + sizeof(st_DBQUERY_MSG_LEVELUP))
			{
				break;
			}

			msgQ.MoveFront(sizeof(st_DBQUERY_HEADER));
			msgQ.Dequeue((char*)&msg, sizeof(st_DBQUERY_MSG_LEVELUP));

			if (msgQ.GetUseSize() > 0)
			{
				SetEvent(hEVENT_MSGQ);
			}

			// TODO: 쿼리 날리기
			for (int i = 0; i < 3; i++)
			{
				g_number++;

				string query =
					"INSERT INTO `mytest`.`new_table` (`accountNo`) VALUES ('" + to_string(g_number) +"');";

				int query_stat;

				// Insert 쿼리문
				query_stat = mysql_query(connection, query.c_str());
				if (query_stat != 0)
				{
					printf("Mysql query error : %s", mysql_error(&conn));
					return;
				}
			}
		}
			break;
		case df_DBQUERY_MSG_MONEY_ADD:
		{
			st_DBQUERY_MSG_MONEY_ADD msg;
			if (msgQ.GetUseSize() < sizeof(st_DBQUERY_HEADER) + sizeof(st_DBQUERY_MSG_MONEY_ADD))
			{
				break;
			}

			msgQ.MoveFront(sizeof(st_DBQUERY_HEADER));
			msgQ.Dequeue((char*)&msg, sizeof(st_DBQUERY_MSG_MONEY_ADD));

			if (msgQ.GetUseSize() > 0)
			{
				SetEvent(hEVENT_MSGQ);
			}

			// TODO: 쿼리 날리기
			for (int i = 0; i < 3; i++)
			{
				g_number++;

				string query =
					"INSERT INTO `mytest`.`new_table` (`accountNo`) VALUES ('" + to_string(g_number) + "');";

				int query_stat;

				// Insert 쿼리문
				query_stat = mysql_query(connection, query.c_str());
				if (query_stat != 0)
				{
					printf("Mysql query error : %s", mysql_error(&conn));
					return;
				}
			}
		}
			break;
		case df_DBQUERY_MSG_QUEST_COMPLETE:
		{
			st_DBQUERY_MSG_QUEST_COMPLETE msg;
			if (msgQ.GetUseSize() < sizeof(st_DBQUERY_HEADER) + sizeof(st_DBQUERY_MSG_QUEST_COMPLETE))
			{
				break;
			}

			msgQ.MoveFront(sizeof(st_DBQUERY_HEADER));
			msgQ.Dequeue((char*)&msg, sizeof(st_DBQUERY_MSG_QUEST_COMPLETE));

			if (msgQ.GetUseSize() > 0)
			{
				SetEvent(hEVENT_MSGQ);
			}

			// TODO: 쿼리 날리기
			for (int i = 0; i < 3; i++)
			{
				g_number++;

				string query =
					"INSERT INTO `mytest`.`new_table` (`accountNo`) VALUES ('" + to_string(g_number) + "');";

				int query_stat;

				// Insert 쿼리문
				query_stat = mysql_query(connection, query.c_str());
				if (query_stat != 0)
				{
					printf("Mysql query error : %s", mysql_error(&conn));
					return;
				}
			}
		}

			break;
		}

		msgTPS++;
	}

	// DB 연결닫기
	mysql_close(connection);

}

int main()
{
	timeBeginPeriod(1);

	msgQ.Resize(5000);

	hEVENT_QUIT = CreateEvent(NULL, TRUE, FALSE, NULL);
	hEVENT_MSGQ = CreateEvent(NULL, FALSE, FALSE, NULL);


	hThread_MonitoringThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&MonitoringThread, NULL, NULL, NULL);
	hThread_UpdateThread1 = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&UpdateThread, NULL, NULL, NULL);
	hThread_UpdateThread2 = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&UpdateThread, NULL, NULL, NULL);
	hThread_DBWriterThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&DBWriterThread, NULL, NULL, NULL);

	HANDLE hThreads[4] = { hThread_UpdateThread1 ,hThread_UpdateThread2,hThread_DBWriterThread, hThread_MonitoringThread };

	while (1)
	{
		char input = _getch();
		if (input =='Q' || input =='q')
		{
			SetEvent(hEVENT_QUIT);
		}
	}

	WaitForMultipleObjects(4, hThreads, TRUE, INFINITE);

	return 0;
}
