#pragma comment(lib, "SerializeBuffer.lib")

#include <iostream>

#include "Windows.h"
#include "MemoryPool.h"
#include "SerializeBuffer.h"
#include "SerializeBufferPtr.h"

using namespace std;

void RawPtr::IncreaseRefCount()
{
	InterlockedIncrement(&_RCBPtr->count);
}

void RawPtr::DecreaseRefCount()
{
    if (InterlockedDecrement(&_RCBPtr->count) == 0)
    {
        SerializePacket::SPacketMP.Free(_ptr);
        delete _RCBPtr;

        _ptr = NULL;
        _RCBPtr = NULL;
    }
}

SerializePacketPtr::SerializePacketPtr()
{
    _ptr = NULL;
    _RCBPtr = NULL;
}

SerializePacketPtr::SerializePacketPtr(SerializePacket* ptr)
{
    _ptr = ptr;
    _RCBPtr = new RefCountBlock;
    InterlockedIncrement(&_RCBPtr->count);
}

SerializePacketPtr::SerializePacketPtr(const SerializePacketPtr& other)
{
    InterlockedIncrement(&other._RCBPtr->count);

    _ptr = other._ptr;
    _RCBPtr = other._RCBPtr;
}

SerializePacketPtr& SerializePacketPtr::operator=(const SerializePacketPtr& other)
{
    LONG ret = InterlockedDecrement(&_RCBPtr->count);
    if (ret == 0)
    {
        SerializePacket::SPacketMP.Free(_ptr);

        _ptr = NULL;
        _RCBPtr = NULL;
    }

    InterlockedIncrement(&other._RCBPtr->count);
    _ptr = other._ptr;
    _RCBPtr = other._RCBPtr;

    return *this;
}

SerializePacketPtr::~SerializePacketPtr()
{
    if (InterlockedDecrement(&_RCBPtr->count) == 0)
    {
        // 찐 소멸
        SerializePacket::SPacketMP.Free(_ptr);
        delete _RCBPtr;

        _ptr = NULL;
        _RCBPtr = NULL;
    }
}

void SerializePacketPtr::GetRawPtr(OUT RawPtr* pRaw)
{
    pRaw->_ptr = _ptr;
    pRaw->_RCBPtr = _RCBPtr;
}

SerializePacket* SerializePacketPtr::MakeSerializePacket()
{
    return SerializePacket::SPacketMP.Alloc();
}

void SerializePacketPtr::Clear()
{
    if (_ptr == NULL)
        return;

    _ptr->Clear();
}

int SerializePacketPtr::GetBufferSize()
{
    if (_ptr == NULL)
    {
        return -1;
    }

    return _ptr->GetBufferSize();
}

int SerializePacketPtr::GetDataSize()
{
    if (_ptr == NULL)
    {
        return -1;
    }

    return _ptr->GetDataSize();
}

char* SerializePacketPtr::GetBufferPtr()
{
    if (_ptr == NULL)
    {
        return NULL;
    }

    return _ptr->GetBufferPtr();
}

int SerializePacketPtr::MoveWritePos(int size)
{
    if (_ptr == NULL)
    {
        return -1;
    }

    return _ptr->MoveWritePos(size);
}

int SerializePacketPtr::MoveReadPos(int size)
{
    if (_ptr == NULL)
    {
        return -1;
    }

    return _ptr->MoveReadPos(size);
}

SerializePacketPtr& SerializePacketPtr::operator<<(unsigned char byValue)
{
    if (_ptr == NULL)
    {
        return *this;
    }

    *_ptr << byValue;

    return *this;
}

SerializePacketPtr& SerializePacketPtr::operator<<(char chValue)
{
    if (_ptr == NULL)
    {
        return *this;
    }

    *_ptr << chValue;

    return *this;
}

SerializePacketPtr& SerializePacketPtr::operator<<(short shValue)
{
    if (_ptr == NULL)
    {
        return *this;
    }

    *_ptr << shValue;

    return *this;
}

SerializePacketPtr& SerializePacketPtr::operator<<(unsigned short wValue)
{
    if (_ptr == NULL)
    {
        return *this;
    }

    *_ptr << wValue;

    return *this;
}

SerializePacketPtr& SerializePacketPtr::operator<<(int iValue)
{
    if (_ptr == NULL)
    {
        return *this;
    }

    *_ptr << iValue;

    return *this;
}

SerializePacketPtr& SerializePacketPtr::operator<<(long lValue)
{
    if (_ptr == NULL)
    {
        return *this;
    }

    *_ptr << lValue;

    return *this;
}

SerializePacketPtr& SerializePacketPtr::operator<<(float fValue)
{
    if (_ptr == NULL)
    {
        return *this;
    }

    *_ptr << fValue;

    return *this;
}

SerializePacketPtr& SerializePacketPtr::operator<<(DWORD dwValue)
{
    if (_ptr == NULL)
    {
        return *this;
    }

    *_ptr << dwValue;

    return *this;
}

SerializePacketPtr& SerializePacketPtr::operator<<(__int64 iValue)
{
    if (_ptr == NULL)
    {
        return *this;
    }

    *_ptr << iValue;

    return *this;
}

SerializePacketPtr& SerializePacketPtr::operator<<(double dValue)
{
    if (_ptr == NULL)
    {
        return *this;
    }

    *_ptr << dValue;

    return *this;
}

SerializePacketPtr& SerializePacketPtr::operator>>(BYTE& byValue)
{
    if (_ptr == NULL)
    {
        return *this;
    }

    *_ptr >> byValue;

    return *this;
}

SerializePacketPtr& SerializePacketPtr::operator>>(char& chValue)
{
    if (_ptr == NULL)
    {
        return *this;
    }

    *_ptr >> chValue;

    return *this;
}

SerializePacketPtr& SerializePacketPtr::operator>>(short& shValue)
{
    if (_ptr == NULL)
    {
        return *this;
    }

    *_ptr >> shValue;

    return *this;
}

SerializePacketPtr& SerializePacketPtr::operator>>(WORD& wValue)
{
    if (_ptr == NULL)
    {
        return *this;
    }

    *_ptr >> wValue;

    return *this;
}

SerializePacketPtr& SerializePacketPtr::operator>>(int& iValue)
{
    if (_ptr == NULL)
    {
        return *this;
    }

    *_ptr >> iValue;

    return *this;
}

SerializePacketPtr& SerializePacketPtr::operator>>(DWORD& dwValue)
{
    if (_ptr == NULL)
    {
        return *this;
    }

    *_ptr >> dwValue;

    return *this;
}

SerializePacketPtr& SerializePacketPtr::operator>>(float& fValue)
{
    if (_ptr == NULL)
    {
        return *this;
    }

    *_ptr >> fValue;

    return *this;
}

SerializePacketPtr& SerializePacketPtr::operator>>(__int64& iValue)
{
    if (_ptr == NULL)
    {
        return *this;
    }

    *_ptr >> iValue;

    return *this;
}

SerializePacketPtr& SerializePacketPtr::operator>>(double& dValue)
{
    if (_ptr == NULL)
    {
        return *this;
    }

    *_ptr >> dValue;

    return *this;
}


int SerializePacketPtr::GetData(char* chpDest, int iSize)
{
    if (_ptr == NULL)
    {
        return -1;
    }

    return _ptr->GetData(chpDest, iSize);
}

int SerializePacketPtr::Putdata(char* chpSrc, int iSrcSize)
{
    if (_ptr == NULL)
    {
        return -1;
    }

    return _ptr->Putdata(chpSrc, iSrcSize);
}

void SerializePacketPtr::PushHeader(char* header, int headerSize)
{
    if (_ptr == NULL)
    {
        return;
    }

    _ptr->PushHeader(header, headerSize);

    return;
}
