#include <Windows.h>
#include <thread>
#include <iostream>

#include "MemoryPool.h"
#include "LockFree_Queue.h"

using namespace std;

LockFreeQueue<int> myQueue;

void Thread()
{
	while (1)
	{
		for (int i = 0; i < 3; i++)
		{
			myQueue.Enqueue(i);
		}

		for (int i = 0; i < 3; i++)
		{
			int tmp;
			myQueue.Dequeue(&tmp);
		}
	}
}

int main()
{
	HANDLE hThreads[3];
	for (int i = 0; i < 3; i++)
	{
		hThreads[i] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&Thread, NULL, NULL, NULL);
	}

	WaitForMultipleObjects(3, hThreads, TRUE, INFINITE);
}