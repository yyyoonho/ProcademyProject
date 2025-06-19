#pragma comment(lib,"SerializeBuffer.lib")

#include <iostream>
#include "Windows.h"
#include "SerializeBuffer.h"
#include "MemoryPool.h"

#include "SPacketPtr.h"

using namespace std;


SPacketPtr::SPacketPtr()
{
    _ptr = NULL;
    _RCBPtr = NULL;
}

SPacketPtr::SPacketPtr(SerializePacket* ptr) : _ptr(ptr)
{
    _RCBPtr = new RefCountBlock;
    InterlockedIncrement(&_RCBPtr->count);
}

SPacketPtr::SPacketPtr(const SPacketPtr& other) : _ptr(other._ptr), _RCBPtr(other._RCBPtr)
{
    InterlockedIncrement(&_RCBPtr->count);
}

SPacketPtr& SPacketPtr::operator=(const SPacketPtr& other)
{
    InterlockedIncrement(&other._RCBPtr->count);
    
    _ptr = other._ptr;
    _RCBPtr = other._RCBPtr;

    return *this;
}

SPacketPtr::~SPacketPtr()
{
    if (InterlockedDecrement(&_RCBPtr->count) == 0)
    {
        SPacketPtr::SPacketMP.Free(_ptr);
        delete _RCBPtr;

        cout << "찐 소멸" << endl;
    }
}


void Func(SPacketPtr tmp)
{
    return;
}

int main()
{
    SPacketPtr a(SPacketPtr::SPacketMP.Alloc());

    SPacketPtr b = a;

    Func(a);
   

}