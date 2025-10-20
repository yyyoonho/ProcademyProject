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
		int* arr[100];

		int a = 3;

		for (int i = 0; i < 100; i++)
		{
			int* newData = mp.Alloc();

			arr[i] = newData;
		}

		int b = 3;

		for (int i = 0; i < 100; i++)
		{
			mp.Free(arr[i]);
		}
	}
}


int main()
{
	InitializeSRWLock(&srwLock);

	HANDLE hThread[1];
	hThread[0] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&ThreadFuncA, NULL, NULL, NULL);
	
	while (1)
	{
		DWORD ret = WaitForMultipleObjects(1, hThread, TRUE, 500);
		if (ret != WAIT_TIMEOUT)
			break;

		cout << mp.GetFullChunkUseCount() << endl;

	}

	return 0;
}