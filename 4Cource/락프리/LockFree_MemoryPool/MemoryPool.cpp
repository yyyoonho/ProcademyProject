#include <Windows.h>
#include <iostream>
#include <thread>

#include "MemoryPool.h"

using namespace std;

class CTest
{
public:
    CTest()
    {
        a = 0;
    }

public:
    int a;
};

procademy::MemoryPool<CTest> mp(0, TRUE);

void ThreadFunc()
{
    while (1)
    {
        CTest* arr[10000];

        for (int i = 0; i < 10000; i++)
        {
            arr[i] = mp.Alloc();

            LONG ret = InterlockedIncrement((LONG*)&arr[i]->a);
            if (ret == 2)
            {
                int a = 3;
            }
        }

        for (int i = 0; i < 10000; i++)
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

    int b = 3;

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
