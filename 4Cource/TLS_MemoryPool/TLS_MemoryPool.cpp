#include <Windows.h>
#include <iostream>
#include <thread>
#include <queue>

#include "TLS_MemoryPool.h"

using namespace std;



procademy::MemoryPool<int> mp(0, false);
SRWLOCK srwLock;
queue<int*> Q;

void ThreadFuncA()
{
	while (1)
	{
		AcquireSRWLockExclusive(&srwLock);

		int* tmp = mp.Alloc();
		Q.push(tmp);

		ReleaseSRWLockExclusive(&srwLock);
	}
}

void ThreadFuncB()
{
	while (1)
	{
		/*AcquireSRWLockExclusive(&srwLock);

		int* tmp = NULL;

		if (!Q.empty())
		{
			tmp = Q.front();
			Q.pop();
		}

		if (tmp != NULL)
			mp.Free(tmp);

		ReleaseSRWLockExclusive(&srwLock);*/
	}
}


int main()
{
	InitializeSRWLock(&srwLock);

	int a = 3;

	for (int i = 0; i < 200; i++)
	{
		int* tmp = mp.Alloc();

		Q.push(tmp);
	}

	int b = 3;

	HANDLE hThreads[4];
	
	hThreads[1] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&ThreadFuncB, NULL, NULL, NULL);
	hThreads[2] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&ThreadFuncB, NULL, NULL, NULL);
	hThreads[0] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&ThreadFuncA, NULL, NULL, NULL);
	hThreads[3] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&ThreadFuncA, NULL, NULL, NULL);
	
	while (1)
	{
		DWORD ret = WaitForMultipleObjects(4, hThreads, TRUE, 500);
		if (ret != WAIT_TIMEOUT)
			break;

		// TODO: 디버깅 코드
		printf("---------------------------------\n");
		printf("fullChunkStackCount: %ld\n", mp.GetFullChunkStackCount());
		printf("emptyChunkStackCount: %ld\n", mp.GetEmptyChunkStackCount());
		printf("QSize: %d\n", Q.size());
		printf("---------------------------------\n");
	}

	return 0;
}