#include <Windows.h>
#include <iostream>

#include "TLS_MemoryPool.h"
#include "Net_SerializeBuffer.h"

using namespace std;

Net_SerializePacket::Net_SerializePacket()
{
    _original = new char[sizeof(stExtraBuffer) + eBUFFER_DEFAULT];
    _buf = _original + sizeof(stExtraBuffer);
    _originalBuf = _buf;

    _writePos = 0;
    _readPos = 0;
    _size = 0;
    _usingExtraBuffer = 0;

    _capacity = sizeof(stExtraBuffer) + eBUFFER_DEFAULT;
}

Net_SerializePacket::Net_SerializePacket(int bufferSize)
{
    _original = new char[sizeof(stExtraBuffer) + bufferSize];
    _buf = _buf + sizeof(stExtraBuffer);
    _originalBuf = _buf;

    _writePos = 0;
    _readPos = 0;
    _size = 0;
    _usingExtraBuffer = 0;

    _capacity = sizeof(stExtraBuffer) + bufferSize;
}

Net_SerializePacket::~Net_SerializePacket()
{
    delete[] _original;
}

void Net_SerializePacket::Clear()
{
    _writePos = _readPos = 0;
    _size = 0;
    _usingExtraBuffer = 0;
    _buf = _originalBuf;

    _isHeaderPushed = false;
    _pushedHeaderSize = 0;
}

int Net_SerializePacket::GetBufferSize()
{
    return _capacity;
}

int Net_SerializePacket::GetDataSize()
{
    return _size;
}

char* Net_SerializePacket::GetBufferPtr()
{
    return _buf;
}

int Net_SerializePacket::MoveWritePos(int size)
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

int Net_SerializePacket::MoveReadPos(int size)
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

Net_SerializePacket& Net_SerializePacket::operator=(Net_SerializePacket& clSrcPacket)
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

Net_SerializePacket& Net_SerializePacket::operator<<(unsigned char byValue)
{
    if (_writePos + sizeof(byValue) > _capacity)
        return *this;

    *((unsigned char*)(_buf + _writePos)) = byValue;
    _writePos += sizeof(byValue);

    _size = _writePos - _readPos;

    return *this;
}

Net_SerializePacket& Net_SerializePacket::operator<<(char chValue)
{
    if (_writePos + sizeof(chValue) > _capacity)
        return *this;

    *((char*)(_buf + _writePos)) = chValue;
    _writePos += sizeof(chValue);

    _size = _writePos - _readPos;

    return *this;
}

Net_SerializePacket& Net_SerializePacket::operator<<(short shValue)
{
    if (_writePos + sizeof(shValue) > _capacity)
        return *this;

    *((short*)(_buf + _writePos)) = shValue;
    _writePos += sizeof(shValue);

    _size = _writePos - _readPos;

    return *this;
}

Net_SerializePacket& Net_SerializePacket::operator<<(unsigned short wValue)
{
    if (_writePos + sizeof(wValue) > _capacity)
        return *this;

    *((unsigned short*)(_buf + _writePos)) = wValue;
    _writePos += sizeof(wValue);

    _size = _writePos - _readPos;

    return *this;
}

Net_SerializePacket& Net_SerializePacket::operator<<(int iValue)
{
    if (_writePos + sizeof(iValue) > _capacity)
        return *this;

    *((int*)(_buf + _writePos)) = iValue;
    _writePos += sizeof(iValue);

    _size = _writePos - _readPos;

    return *this;
}

Net_SerializePacket& Net_SerializePacket::operator<<(long lValue)
{
    if (_writePos + sizeof(lValue) > _capacity)
        return *this;

    *((long*)(_buf + _writePos)) = lValue;
    _writePos += sizeof(lValue);

    _size = _writePos - _readPos;

    return *this;
}

Net_SerializePacket& Net_SerializePacket::operator<<(float fValue)
{
    if (_writePos + sizeof(fValue) > _capacity)
        return *this;

    *((float*)(_buf + _writePos)) = fValue;
    _writePos += sizeof(fValue);

    _size = _writePos - _readPos;

    return *this;
}

Net_SerializePacket& Net_SerializePacket::operator<<(DWORD dwValue)
{
    if (_writePos + sizeof(dwValue) > _capacity)
        return *this;

    *((DWORD*)(_buf + _writePos)) = dwValue;
    _writePos += sizeof(dwValue);

    _size = _writePos - _readPos;

    return *this;
}

Net_SerializePacket& Net_SerializePacket::operator<<(__int64 iValue)
{
    if (_writePos + sizeof(iValue) > _capacity)
        return *this;

    *((__int64*)(_buf + _writePos)) = iValue;
    _writePos += sizeof(iValue);

    _size = _writePos - _readPos;

    return *this;
}

Net_SerializePacket& Net_SerializePacket::operator<<(double dValue)
{
    if (_writePos + sizeof(dValue) > _capacity)
        return *this;

    *((double*)(_buf + _writePos)) = dValue;
    _writePos += sizeof(dValue);

    _size = _writePos - _readPos;

    return *this;
}

Net_SerializePacket& Net_SerializePacket::operator>>(BYTE& byValue)
{
    if (_size < sizeof(byValue))
        return *this;

    byValue = *((BYTE*)(_buf + _readPos));
    _readPos += sizeof(BYTE);

    _size = _writePos - _readPos;

    return *this;
}

Net_SerializePacket& Net_SerializePacket::operator>>(char& chValue)
{
    if (_size < sizeof(chValue))
        return *this;

    chValue = *((char*)(_buf + _readPos));
    _readPos += sizeof(char);

    _size = _writePos - _readPos;

    return *this;
}

Net_SerializePacket& Net_SerializePacket::operator>>(short& shValue)
{
    if (_size < sizeof(shValue))
        return *this;

    shValue = *((short*)(_buf + _readPos));
    _readPos += sizeof(short);

    _size = _writePos - _readPos;

    return *this;
}

Net_SerializePacket& Net_SerializePacket::operator>>(WORD& wValue)
{
    if (_size < sizeof(wValue))
        return *this;

    wValue = *((WORD*)(_buf + _readPos));
    _readPos += sizeof(WORD);

    _size = _writePos - _readPos;

    return *this;
}

Net_SerializePacket& Net_SerializePacket::operator>>(int& iValue)
{
    if (_size < sizeof(iValue))
        return *this;

    iValue = *((int*)(_buf + _readPos));
    _readPos += sizeof(int);

    _size = _writePos - _readPos;

    return *this;
}

Net_SerializePacket& Net_SerializePacket::operator>>(DWORD& dwValue)
{
    if (_size < sizeof(dwValue))
        return *this;

    dwValue = *((DWORD*)(_buf + _readPos));
    _readPos += sizeof(DWORD);

    _size = _writePos - _readPos;

    return *this;
}

Net_SerializePacket& Net_SerializePacket::operator>>(float& fValue)
{
    if (_size < sizeof(fValue))
        return *this;

    fValue = *((float*)(_buf + _readPos));
    _readPos += sizeof(float);

    _size = _writePos - _readPos;

    return *this;
}

Net_SerializePacket& Net_SerializePacket::operator>>(__int64& iValue)
{
    if (_size < sizeof(iValue))
        return *this;

    iValue = *((__int64*)(_buf + _readPos));
    _readPos += sizeof(__int64);

    _size = _writePos - _readPos;

    return *this;
}

Net_SerializePacket& Net_SerializePacket::operator>>(double& dValue)
{
    if (_size < sizeof(dValue))
        return *this;

    dValue = *((double*)(_buf + _readPos));
    _readPos += sizeof(double);

    _size = _writePos - _readPos;

    return *this;
}

int Net_SerializePacket::GetData(char* chpDest, int iSize)
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

int Net_SerializePacket::Putdata(char* chpSrc, int iSrcSize)
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


bool Net_SerializePacket::PushHeader(char* header, int headerSize)
{
    if (headerSize > sizeof(stExtraBuffer) - _usingExtraBuffer)
        return false;

    memcpy_s(_buf - headerSize, headerSize, header, headerSize);
    _size += headerSize;

    _isHeaderPushed = true;
    _pushedHeaderSize = headerSize;
    _usingExtraBuffer += headerSize;

    _buf = _buf - headerSize;
    _writePos = _writePos + headerSize;

    return true;
}

bool Net_SerializePacket::PushExtraBuffer(char* header, int headerSize)
{
    if (headerSize > sizeof(stExtraBuffer) - _usingExtraBuffer)
        return false;

    memcpy_s(_buf - headerSize, headerSize, header, headerSize);
    _size += headerSize;

    _isHeaderPushed = true;
    _pushedHeaderSize = headerSize;
    _usingExtraBuffer += headerSize;

    _buf = _buf - headerSize;
    _writePos = _writePos + headerSize;

    return true;
}
