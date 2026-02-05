#include "stdafx.h"

#include "RingBuffer.h"

using namespace std;

#define DEFAULT_BUFSIZE 1000

RingBuffer::RingBuffer()
{
    _capacity = DEFAULT_BUFSIZE;
    _buf = new char[_capacity];
}

RingBuffer::RingBuffer(int bufferSize)
{
    _capacity = bufferSize;
    _buf = new char[_capacity];
}

RingBuffer::~RingBuffer()
{
    delete[] _buf;
}

void RingBuffer::Resize(int size)
{
    // 아직 논의중
    char* tmpBuf = new char[size];
    memcpy_s(tmpBuf, size, _buf, _capacity);

    delete[] _buf;

    _buf = tmpBuf;
    _capacity = size;
}

int RingBuffer::GetBufferSize()
{
    return _capacity;
}

int RingBuffer::GetFreeSize()
{
    return (_capacity - GetUseSize()) - 1;
}

void RingBuffer::ClearBuffer()
{
    _front = _rear = 0;

    return;
}