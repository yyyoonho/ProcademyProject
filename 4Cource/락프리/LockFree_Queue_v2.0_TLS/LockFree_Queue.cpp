#include <Windows.h>
#include <thread>
#include <iostream>

#include "TLS_MemoryPool.h"
#include "LockFree_Queue.h"

using namespace std;

LockFreeQueue<int> myQueue;

void Thread()
{
	while (1)
	{
		for (int i = 0; i < 1000; i++)
		{
			bool ret = myQueue.Enqueue(i);
			if (ret == false)
			{
				DebugBreak();
			}
		}

		for (int i = 0; i < 1000; i++)
		{
			int tmp;
			bool ret = myQueue.Dequeue(&tmp);
			if (ret == false)
			{
				DebugBreak();
			}
		}
	}
}

int main()
{
	HANDLE hThreads[6];
	for (int i = 0; i < 6; i++)
	{
		hThreads[i] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&Thread, NULL, NULL, NULL);
		Sleep(10);
	}

	WaitForMultipleObjects(6, hThreads, TRUE, INFINITE);

}