#include <iostream>
#include <Windows.h>
#include <thread>
#include "RingBuffer.h"

using namespace std;

const char* str = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
char Buf[200];
RingBuffer serverRingBuffer(500);
RingBuffer clientRingBuffer(500);

int serverIdx = 0;
int clientIdx = 0;

CRITICAL_SECTION cs;

void PrintRingBuffer()
{
	int front = serverRingBuffer._front;
	int rear = serverRingBuffer._rear;

	printf("RingBuffer: ");
	while (1)
	{
		if (front == rear) break;

		printf("%c", serverRingBuffer._buf[front]);
		front = (front + 1) % serverRingBuffer._capacity;
	}

	printf("\n");
}

void EnqueueThread()
{
	srand(time(NULL));

	while (1)
	{
		//cout << "Enqueue 시작" << endl;
		int cntNum = (rand() % 3) + 1;

		for (int i = 0; i < cntNum; i++)
		{
			int cutOffNum = rand() % 20;

			int b = serverRingBuffer.DirectEnqueueSize();

			if (cutOffNum > b)
			{
				cutOffNum = b;
			}

			if (serverIdx + cutOffNum > strlen(str))
				cutOffNum = strlen(str) - serverIdx;


			//int ret = serverRingBuffer.Enqueue(str + serverIdx, cutOffNum);
			memcpy_s(serverRingBuffer.GetRearBufferPtr(), b, str + serverIdx, cutOffNum);

			serverIdx = (serverIdx + cutOffNum) % strlen(str);

			serverRingBuffer.MoveRear(cutOffNum);

			//PrintRingBuffer();

		}
	}
}

void DequeueThread()
{
	srand(time(NULL));

	int cnt = 0;

	while (1)
	{
		int cntNum2 = (rand() % 3) + 1;

		for (int i = 0; i < cntNum2; i++)
		{
			int cutOffNum2 = rand() % 20;

			if (serverRingBuffer.DirectDequeueSize() == 0)
				continue;

			int a = serverRingBuffer.DirectDequeueSize();

			if (cutOffNum2 > a)
			{
				cutOffNum2 = a;
			}

			//int ret = serverRingBuffer.Dequeue(Buf, cutOffNum2);
			memcpy_s(Buf, 200, serverRingBuffer.GetFrontBufferPtr(), cutOffNum2);
			serverRingBuffer.MoveFront(cutOffNum2);

			int ret = cutOffNum2;

			//PrintRingBuffer();

			cout << Buf;

			char tmp1[50];
			char tmp2[50];
			if (clientIdx + ret > strlen(str))
			{
				memset(tmp1, 0, 50);
				memset(tmp2, 0, 50);

				int oneStep = strlen(str) - clientIdx;
				int twoStep = ret - oneStep;

				memcpy_s(tmp1, 50, str + clientIdx, oneStep);
				memcpy_s(tmp2, 50, str, twoStep);
				strcat_s(tmp1, tmp2);
			}
			else
			{
				memcpy_s(tmp1, 50, str + clientIdx, ret);
			}

			if (strncmp(Buf, tmp1, ret) != 0)
			{
				DebugBreak();
			}

			clientIdx = (clientIdx + ret) % strlen(str);

			memset(Buf, 0, 200);
		}

		//cout << "Dequeue 끝" << endl << endl;
	}
}

void Single()
{
	srand(time(NULL));


	while (1)
	{
		//cout << "Enqueue 시작" << endl;
		int cntNum = (rand() % 3) + 1;

		for (int i = 0; i < cntNum; i++)
		{
			int cutOffNum = rand() % 20;

			if (serverIdx + cutOffNum > strlen(str))
				cutOffNum = strlen(str) - serverIdx;

			int ret = serverRingBuffer.Enqueue(str + serverIdx, cutOffNum);

			serverIdx = (serverIdx + ret) % strlen(str);

			//PrintRingBuffer();

		}
		//cout << "Enqueue 끝" << endl << endl;

		//cout << "Dequeue 시작" << endl;
		int cntNum2 = (rand() % 3) + 1;

		for (int i = 0; i < cntNum2; i++)
		{
			int cutOffNum2 = rand() % 20;

			int ret = serverRingBuffer.Dequeue(Buf, cutOffNum2);

			//PrintRingBuffer();
			//cout << "Buf : "<< Buf << endl;


			cout << Buf;

			char tmp1[50];
			char tmp2[50];
			if (clientIdx + ret > strlen(str))
			{
				memset(tmp1, 0, 50);
				memset(tmp2, 0, 50);

				int oneStep = strlen(str) - clientIdx;
				int twoStep = ret - oneStep;

				memcpy_s(tmp1, 50, str + clientIdx, oneStep);
				memcpy_s(tmp2, 50, str, twoStep);
				strcat_s(tmp1, tmp2);
			}
			else
			{
				memcpy_s(tmp1, 50, str + clientIdx, ret);
			}

			if (strncmp(Buf, tmp1, ret) != 0)
			{
				DebugBreak();
			}

			clientIdx = (clientIdx + ret) % strlen(str);

			memset(Buf, 0, 200);
		}

		//cout << "Dequeue 끝" << endl << endl;
	}
}

int main()
{
	InitializeCriticalSection(&cs);

	HANDLE threadHandles[2];

	threadHandles[0] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&EnqueueThread, NULL, NULL, NULL);
	threadHandles[1] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&DequeueThread, NULL, NULL, NULL);

	WaitForMultipleObjects(2, threadHandles, true, INFINITE);

	/*
	HANDLE threadHandle;
	threadHandle = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&Single, NULL, NULL, NULL);

	WaitForSingleObject(threadHandle, INFINITE);
	*/

	return 0;
}