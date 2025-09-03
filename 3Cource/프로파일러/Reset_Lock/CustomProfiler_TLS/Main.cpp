#include <Windows.h>
#include <thread>
#include <conio.h>

#include "CustomProfiler.h"

using namespace std;

void ThreadFunc()
{
	int count = 0;

	while (1)
	{
		PRO_BEGIN("TEST");

		count++;
		for (int i = 0; i < 1000; i++)
		{
			
		}
		
		PRO_END("TEST");
	}
}

int main()
{
	HANDLE hThreadHandle[5];

	for (int i = 0; i < 5; i++)
	{
		hThreadHandle[i] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&ThreadFunc, NULL, NULL, NULL);
	}

	while (1)
	{
		ProfilerInput();
	}
}