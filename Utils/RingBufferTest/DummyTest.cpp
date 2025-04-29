#include <iostream>
#include <Windows.h>
#include <thread>
#include "RingBuffer.h"

using namespace std;

const char* str = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
char Buf[200];
RingBuffer serverRingBuffer(50);
RingBuffer clientRingBuffer(50);

int serverIdx = 0;
int clientIdx = 0;

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
	while (1)
	{
		cout << "Enqueue 시작" << endl;
		int cntNum = (rand() % 3) + 1;

		for (int i = 0; i < cntNum; i++)
		{
			int cutOffNum = rand() % 20;

			if (serverIdx + cutOffNum > strlen(str))
				cutOffNum = strlen(str) - serverIdx;
			int ret = serverRingBuffer.Enqueue(str + serverIdx, cutOffNum);
			serverIdx = (serverIdx + ret) % strlen(str);

			PrintRingBuffer();

		}
		cout << "Enqueue 끝" << endl << endl;
	}
}

void DequeueThread()
{
	while (1)
	{
		cout << "Dequeue 시작" << endl;
		int cntNum2 = (rand() % 3) + 1;

		for (int i = 0; i < cntNum2; i++)
		{
			int cutOffNum2 = rand() % 20;

			int ret = serverRingBuffer.Dequeue(Buf, cutOffNum2);
			PrintRingBuffer();

			cout << "Buf : "<< Buf << endl;

			char tmp1[70];
			char tmp2[70];
			if (clientIdx + ret > strlen(str))
			{
				memset(tmp1, 0, 70);
				memset(tmp2, 0, 70);

				int oneStep = strlen(str) - clientIdx;
				int twoStep = ret - oneStep;

				memcpy_s(tmp1, 70, str + clientIdx, oneStep);
				memcpy_s(tmp2, 70, str, twoStep);
				strcat_s(tmp1, tmp2);
			}
			else
			{
				memcpy_s(tmp1, 70, str + clientIdx, ret);
			}

			if (strncmp(Buf, tmp1, ret) != 0)
			{
				DebugBreak();
			}

			clientIdx = (clientIdx + ret) % strlen(str);

			memset(Buf, 0, 200);
		}

		cout << "Dequeue 끝" << endl << endl;
	}
}

int main()
{
	srand(time(NULL));

	HANDLE threadHandles[2];

	threadHandles[0] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&EnqueueThread, NULL, NULL, NULL);
	threadHandles[1] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&DequeueThread, NULL, NULL, NULL);

	WaitForMultipleObjects(2, threadHandles, true, INFINITE);

	return 0;
}