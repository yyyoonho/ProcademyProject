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
    procademy::MemoryPool<CTest> mp(10, true);

    CTest* a = mp.Alloc(); // CTEST 생성자 -> Node 생성자
    mp.Free(a); // CTest 소멸자

    CTest* b = mp.Alloc(); // CTEST 생성자
    //mp.Free(b); // CTest 소멸자
    
    int bf2 = 3;
}
