#include <iostream>
#include <Windows.h>

#include "MemoryPool.h"

using namespace std;



int main()
{
    procademy::MemoryPool<int> mp(0);
    int* a = mp.Alloc();

    *a = 3;
    cout << *a;

    mp.Free(a);

    int* b = mp.Alloc();
    cout << *b;

    cout << mp.GetCapacity();
}