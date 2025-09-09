#include <Windows.h>
#include <iostream>
#include <thread>

#include "MemoryPool.h"

using namespace std;

procademy::MemoryPool<int> mp(0, false);

void ThreadFunc()
{
    while (1)
    {
        int* arr[1000];

        for (int i = 0; i < 1000; i++)
        {
            arr[i] = mp.Alloc();
        }

        for (int i = 0; i < 1000; i++)
        {
            mp.Free(arr[i]);
        }
    }
}

void MonitorFunc()
{
    while (1)
    {
        cout << "Capacity: " << mp.GetCapacity() << endl;
        cout << "UseCount: " << mp.GetUseCount() << endl;
        cout << "-------------------------------------------" << endl;

        Sleep(1000);
    }
}

int main()
{


    HANDLE hThread[7];
    for (int i = 0; i < 6; i++)
    {
        hThread[i] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&ThreadFunc, NULL, NULL, NULL);
    }

    hThread[6] = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&MonitorFunc, NULL, NULL, NULL);

    WaitForMultipleObjects(7, hThread, TRUE, INFINITE);

    int a = 3;
    
    return 0;
}
