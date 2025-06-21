#pragma comment(lib,"SerializeBuffer.lib")

#include <iostream>
#include "Windows.h"
#include "MemoryPool.h"
#include "SerializeBuffer.h"
#include "SPacketPtr.h"

using namespace std;

void RawPtr::IncreseRefCount()
{
    InterlockedIncrement(&_RCBPtr->count);
}

void RawPtr::DecreseRefCount()
{
    if (InterlockedDecrement(&_RCBPtr->count) == 0)
    {
        SerializePacket::SPacketMP.Free(_ptr);
        delete _RCBPtr;

        _ptr = NULL;
        _RCBPtr = NULL;
    }
}

SerializePacket* SPacketPtr::MakeSerializePacket()
{
    return SerializePacket::SPacketMP.Alloc();
}

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
        // 찐 소멸
        SerializePacket::SPacketMP.Free(_ptr);
        delete _RCBPtr;
    }
}

void SPacketPtr::GetRawPtr(OUT RawPtr* pRaw)
{
    pRaw->_ptr = _ptr;
    pRaw->_RCBPtr = _RCBPtr;
}
