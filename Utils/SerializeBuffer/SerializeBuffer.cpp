#include <Windows.h>
#include <iostream>

#include "MemoryPool.h"
#include "SerializeBuffer.h"

using namespace std;

procademy::MemoryPool<SerializePacket> SerializePacket::SPacketMP(0, false);

SerializePacket::SerializePacket()
{
    _original = new char[sizeof(stTCPHeader) + eBUFFER_DEFAULT];
    _buf = _original + sizeof(stTCPHeader);
    
    _writePos = 0;
    _readPos = 0;
    _size = 0;

    _capacity = sizeof(stTCPHeader) + eBUFFER_DEFAULT;

    InitializeCriticalSection(&cs);
}

SerializePacket::SerializePacket(int bufferSize)
{
    _original = new char[sizeof(stTCPHeader) + bufferSize];
    _buf = _buf + sizeof(stTCPHeader);

    _writePos = 0;
    _readPos = 0;
    _size = 0;

    _capacity = sizeof(stTCPHeader) + bufferSize;
}

SerializePacket::~SerializePacket()
{
    delete[] _original;
}

void SerializePacket::Clear()
{
    EnterCriticalSection(&cs);

    _writePos = _readPos = 0;
    _size = 0;

    LeaveCriticalSection(&cs);
}

int SerializePacket::GetBufferSize()
{
    return _capacity;
}

int SerializePacket::GetDataSize()
{
    return _size;
}

char* SerializePacket::GetBufferPtr()
{
    if (isHeaderPushed)
    {
        return _buf - pushedHeaderSize;
    }
    else
    {
        return _buf;
    }
}

int SerializePacket::MoveWritePos(int size)
{
    int tmpWritePos = _writePos + size;
    if (tmpWritePos > _capacity)
    {
        _writePos = _capacity;
        
        _size = _writePos - _readPos;

        return (_capacity - _writePos);
    }
    else
    {
        _writePos = tmpWritePos;
        
        _size = _writePos - _readPos;

        return size;
    }
    
}

int SerializePacket::MoveReadPos(int size)
{
    int tmpReadPos = _readPos + size;
    if (tmpReadPos > _writePos)
    {
        _readPos = _writePos;
        _size = _writePos - _readPos;
        return (_writePos - _readPos);
    }
    else
    {
        _readPos = tmpReadPos;
        _size = _writePos - _readPos;
        return size;
    }
}

SerializePacket& SerializePacket::operator=(SerializePacket& clSrcPacket)
{
    this->_writePos = clSrcPacket._writePos;
    this->_readPos = clSrcPacket._readPos;

    this->_capacity = clSrcPacket._capacity;
    this->_size = clSrcPacket._size;

    _buf = new char[this->_capacity];

    memcpy_s(&_buf[_writePos], _capacity - _writePos - 1,
        &clSrcPacket._buf[clSrcPacket._writePos], clSrcPacket._size);

    return*this;
}

SerializePacket& SerializePacket::operator<<(unsigned char byValue)
{
    if (_writePos + sizeof(byValue) > _capacity)
        return *this;

    *((unsigned char*)(_buf + _writePos)) = byValue;
    _writePos += sizeof(byValue);

    _size = _writePos - _readPos;

    return *this;
}

SerializePacket& SerializePacket::operator<<(char chValue)
{
    if (_writePos + sizeof(chValue) > _capacity)
        return *this;

    *((char*)(_buf + _writePos)) = chValue;
    _writePos += sizeof(chValue);

    _size = _writePos - _readPos;

    return *this;
}

SerializePacket& SerializePacket::operator<<(short shValue)
{
    if (_writePos + sizeof(shValue) > _capacity)
        return *this;

    *((short*)(_buf + _writePos)) = shValue;
    _writePos += sizeof(shValue);

    _size = _writePos - _readPos;

    return *this;
}

SerializePacket& SerializePacket::operator<<(unsigned short wValue)
{
    if (_writePos + sizeof(wValue) > _capacity)
        return *this;

    *((unsigned short*)(_buf + _writePos)) = wValue;
    _writePos += sizeof(wValue);

    _size = _writePos - _readPos;

    return *this;
}

SerializePacket& SerializePacket::operator<<(int iValue)
{
    if (_writePos + sizeof(iValue) > _capacity)
        return *this;

    *((int*)(_buf + _writePos)) = iValue;
    _writePos += sizeof(iValue);

    _size = _writePos - _readPos;

    return *this;
}

SerializePacket& SerializePacket::operator<<(long lValue)
{
    if (_writePos + sizeof(lValue) > _capacity)
        return *this;

    *((long*)(_buf + _writePos)) = lValue;
    _writePos += sizeof(lValue);

    _size = _writePos - _readPos;

    return *this;
}

SerializePacket& SerializePacket::operator<<(float fValue)
{
    if (_writePos + sizeof(fValue) > _capacity)
        return *this;

    *((float*)(_buf + _writePos)) = fValue;
    _writePos += sizeof(fValue);

    _size = _writePos - _readPos;

    return *this;
}

SerializePacket& SerializePacket::operator<<(DWORD dwValue)
{
    if (_writePos + sizeof(dwValue) > _capacity)
        return *this;

    *((DWORD*)(_buf + _writePos)) = dwValue;
    _writePos += sizeof(dwValue);

    _size = _writePos - _readPos;

    return *this;
}

SerializePacket& SerializePacket::operator<<(__int64 iValue)
{
    if (_writePos + sizeof(iValue) > _capacity)
        return *this;

    *((__int64*)(_buf + _writePos)) = iValue;
    _writePos += sizeof(iValue);

    _size = _writePos - _readPos;

    return *this;
}

SerializePacket& SerializePacket::operator<<(double dValue)
{
    if (_writePos + sizeof(dValue) > _capacity)
        return *this;

    *((double*)(_buf + _writePos)) = dValue;
    _writePos += sizeof(dValue);

    _size = _writePos - _readPos;

    return *this;
}

SerializePacket& SerializePacket::operator>>(BYTE& byValue)
{
    if (_size < sizeof(byValue))
        return *this;

    byValue = *((BYTE*)(_buf + _readPos));
    _readPos += sizeof(BYTE);

    _size = _writePos - _readPos;

    return *this;
}

SerializePacket& SerializePacket::operator>>(char& chValue)
{
    if (_size < sizeof(chValue))
        return *this;

    chValue = *((char*)(_buf + _readPos));
    _readPos += sizeof(char);

    _size = _writePos - _readPos;

    return *this;
}

SerializePacket& SerializePacket::operator>>(short& shValue)
{
    if (_size < sizeof(shValue))
        return *this;

    shValue = *((short*)(_buf + _readPos));
    _readPos += sizeof(short);

    _size = _writePos - _readPos;

    return *this;
}

SerializePacket& SerializePacket::operator>>(WORD& wValue)
{
    if (_size < sizeof(wValue))
        return *this;

    wValue = *((WORD*)(_buf + _readPos));
    _readPos += sizeof(WORD);

    _size = _writePos - _readPos;

    return *this;
}

SerializePacket& SerializePacket::operator>>(int& iValue)
{
    if (_size < sizeof(iValue))
        return *this;

    iValue = *((int*)(_buf + _readPos));
    _readPos += sizeof(int);

    _size = _writePos - _readPos;

    return *this;
}

SerializePacket& SerializePacket::operator>>(DWORD& dwValue)
{
    if (_size < sizeof(dwValue))
        return *this;

    dwValue = *((DWORD*)(_buf + _readPos));
    _readPos += sizeof(DWORD);

    _size = _writePos - _readPos;

    return *this;
}

SerializePacket& SerializePacket::operator>>(float& fValue)
{
    if (_size < sizeof(fValue))
        return *this;

    fValue = *((float*)(_buf + _readPos));
    _readPos += sizeof(float);

    _size = _writePos - _readPos;

    return *this;
}

SerializePacket& SerializePacket::operator>>(__int64& iValue)
{
    if (_size < sizeof(iValue))
        return *this;

    iValue = *((__int64*)(_buf + _readPos));
    _readPos += sizeof(__int64);

    _size = _writePos - _readPos;

    return *this;
}

SerializePacket& SerializePacket::operator>>(double& dValue)
{
    if (_size < sizeof(dValue))
        return *this;

    dValue = *((double*)(_buf + _readPos));
    _readPos += sizeof(double);

    _size = _writePos - _readPos;

    return *this;
}

int SerializePacket::GetData(char* chpDest, int iSize)
{
    if (_size < iSize)
        return 0;

    int ret = memcpy_s(chpDest, iSize, _buf + _readPos, iSize);
    if (ret != 0)
        return 0;

    _readPos += iSize;
    _size = _writePos - _readPos;

    return iSize;
}

int SerializePacket::Putdata(char* chpSrc, int iSrcSize)
{
    if (_writePos + iSrcSize > _capacity)
        return 0;

    int ret = memcpy_s(_buf + _writePos, _capacity - _size, chpSrc, iSrcSize);
    if (ret != 0)
        return 0;

    _writePos += iSrcSize;
    _size = _writePos - _readPos;

    return iSrcSize;
}

void SerializePacket::PushHeader(char* header, int headerSize)
{
    memcpy_s(_buf - headerSize, sizeof(stTCPHeader), header, headerSize);
    _size += headerSize;

    isHeaderPushed = true;
    pushedHeaderSize = headerSize;
}

/*
int main()
{
    SerializePacket sp;

    struct stTest
    {
        int _type = 1;
        int _size = 2;
    };

    int a = 3;
    char b = 'b';
    double c = 3.14;
    stTest s1;

    sp << a;
    sp << b;
    sp << c;
    sp.Putdata((char*)&s1, sizeof(stTest));

   

    int d;
    char e;
    double f;
    stTest t2;

    sp >> d;
    sp >> e;
    sp >> f;
    sp.GetData((char*)&t2, sizeof(stTest));

    cout << d << endl;
    cout << e << endl;
    cout << f << endl;

    cout << t2._size << endl;
    cout << t2._type << endl;
}
*/