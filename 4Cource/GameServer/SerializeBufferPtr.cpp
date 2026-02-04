#include "stdafx.h"

#include "Monitoring.h"

#include "TLS_MemoryPool.h"
#include "Net_SerializeBuffer.h"
#include "SerializeBufferPtr.h"

using namespace std;

//void RawPtr::IncreaseRefCount()
//{
//	InterlockedIncrement(&_RCBPtr->count);
//}
//
//void RawPtr::DecreaseRefCount()
//{
//    if (InterlockedDecrement(&_RCBPtr->count) == 0)
//    {
//        Monitoring::GetInstance()->DecreaseInterlocked(MonitorType::PacketUseCount);
//
//
//        Net_SerializePacket::SPacketMP.Free(_ptr);
//        SerializePacketPtr::RcbMP.Free(_RCBPtr);
//
//        _ptr = NULL;
//        _RCBPtr = NULL;
//    }
//}

SerializePacketPtr::SerializePacketPtr()
{
    _ptr = NULL;
    _RCBPtr = NULL;
}

SerializePacketPtr::SerializePacketPtr(Net_SerializePacket* ptr)
{
    if (ptr != NULL)
    {
        _ptr = ptr;
        //_RCBPtr = new RefCountBlock;
        _RCBPtr = SerializePacketPtr::RcbMP.Alloc();

        InterlockedIncrement(&_RCBPtr->count);
    }
}

SerializePacketPtr::SerializePacketPtr(const SerializePacketPtr& other)
{
    InterlockedIncrement(&other._RCBPtr->count);

    _ptr = other._ptr;
    _RCBPtr = other._RCBPtr;
}

SerializePacketPtr::SerializePacketPtr(RawPtr rawPtr)
{
    rawPtr.IncreaseRefCount();

    _ptr = rawPtr._ptr;
    _RCBPtr = rawPtr._RCBPtr;
}

SerializePacketPtr& SerializePacketPtr::operator=(const SerializePacketPtr& other)
{
    if (this != &other)
    {
        DecreaseRefCount();

        InterlockedIncrement(&other._RCBPtr->count);
        _ptr = other._ptr;
        _RCBPtr = other._RCBPtr;
    }

    return *this;
}

SerializePacketPtr& SerializePacketPtr::operator=(std::nullptr_t)
{
    DecreaseRefCount();

    _ptr = NULL;
    _RCBPtr = NULL;

    return *this;
}

SerializePacketPtr::~SerializePacketPtr()
{
    DecreaseRefCount();
}

void SerializePacketPtr::GetRawPtr(OUT RawPtr* pRaw)
{
    pRaw->_ptr = _ptr;
    pRaw->_RCBPtr = _RCBPtr;
}

bool SerializePacketPtr::IsValid()
{
    return _ptr!= NULL;
}

int SerializePacketPtr::GetRefCount()
{
    if (_RCBPtr != NULL)
    {
        return _RCBPtr->count;
    }

    return -1;
}

Net_SerializePacket* SerializePacketPtr::MakeSerializePacket()
{
    Monitoring::GetInstance()->IncreaseInterlocked(MonitorType::PacketUseCount);

    return Net_SerializePacket::SPacketMP.Alloc();
}


void SerializePacketPtr::DecreaseRefCount()
{
    if (_ptr != NULL)
    {
        if (InterlockedDecrement(&_RCBPtr->count) == 0)
        {
            Monitoring::GetInstance()->DecreaseInterlocked(MonitorType::PacketUseCount);

            Net_SerializePacket::SPacketMP.Free(_ptr);
            //delete _RCBPtr;
            SerializePacketPtr::RcbMP.Free(_RCBPtr);

            _ptr = NULL;
            _RCBPtr = NULL;
        }
    }
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

void SerializePacketPtr::PushExtraBuffer(char* header, int headerSize)
{
    if (_ptr == NULL)
    {
        return;
    }

    _ptr->PushExtraBuffer(header, headerSize);

    return;
}

void SerializePacketPtr::MarkEncoded()
{
    _ptr->MarkEncoded();
}

bool SerializePacketPtr::IsEncoded()
{
    return _ptr->IsEncoded();
}
