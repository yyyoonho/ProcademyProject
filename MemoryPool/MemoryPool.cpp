#include <iostream>
#include <Windows.h>

#include "MemoryPool.h"

using namespace std;



int main()
{
    procademy::MemoryPool<int> mp(0);
    int* a = mp.Alloc();

    *a = 3;
    cout << *a << endl;

    mp.Free(a);

    int* b = mp.Alloc();
    cout << *b << endl;
    *b = 5;

    cout << *b << endl;

    cout << mp.GetUseCount() << endl;
    cout << mp.GetCapacity() << endl;
}