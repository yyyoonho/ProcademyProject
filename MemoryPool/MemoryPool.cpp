#include <iostream>
#include <Windows.h>

using namespace std;

#include "MemoryPool.h"

class CTest
{
public:
    CTest()
    {
        cout << "CTEST 생성자 호출" << endl;
        _x = 0xff;
    }
    ~CTest()
    {
        cout << "CTEST 소멸자 호출" << endl;
        _x = 0;
    }

    int _x;
    //char _y;
};

int main()
{
    procademy::MemoryPool<CTest> mp(1, true);

    /*CTest* a = mp.Alloc();
    mp.Free(a);*/

    CTest* b = mp.Alloc();
    
    
    int bf2 = 3;
}