#include "RingBuffer.h"
#include <iostream>

using namespace std;

#define DEFAULT_BUFSIZE 5000

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

int RingBuffer::GetUseSize()
{
    if (_front == _rear)
        return 0;

    if (_front > _rear)
    {
        return _capacity - (_front - _rear);
    }
    else
    {
        return _rear - _front;
    }
}

int RingBuffer::GetFreeSize()
{
    return (_capacity - GetUseSize()) - 1;
}

int RingBuffer::Enqueue(const char* data, int size)
{
    int freeSize = GetFreeSize();
    if (freeSize == 0 || size == 0)
        return 0;

    int writeSize = size;
    if (writeSize > freeSize)
    {
        writeSize = freeSize;
    }

    if (_rear + writeSize > _capacity)
    {
        int oneStep = _capacity - _rear;
        memcpy_s(&_buf[_rear], oneStep, data, oneStep);
        int twoStep = writeSize - oneStep;
        memcpy_s(_buf, twoStep, &data[oneStep], twoStep);
    }
    else
    {
        memcpy_s(&_buf[_rear], writeSize, data, writeSize);
    }

    _rear = (_rear + writeSize) % _capacity;

    return writeSize;
}

int RingBuffer::Dequeue(char* dest, int size)
{
    int useSize = GetUseSize();
    if (useSize == 0 || size == 0)
        return 0;

    int readSize = size;
    if (useSize < readSize)
    {
        readSize = useSize;
    }

    if (_front + readSize > _capacity)
    {
        int oneStep = _capacity - _front;
        memcpy_s(dest, oneStep, &_buf[_front], oneStep);
        int twoStep = readSize - oneStep;
        memcpy_s(&dest[oneStep], twoStep, _buf, twoStep);
    }
    else
    {
        memcpy_s(dest, readSize, &_buf[_front], readSize);
    }

    _front = (_front + readSize) % _capacity;

    return readSize;
}

int RingBuffer::Peek(char* dest, int size)
{
    // TODO: TEST

    int useSize = GetUseSize();
    if (useSize == 0 || size == 0)
        return 0;

    int readSize = size;
    if (useSize < readSize)
    {
        readSize = useSize;
    }

    if (_front + readSize > _capacity)
    {
        int oneStep = _capacity - _front;
        memcpy_s(dest, oneStep, &_buf[_front], oneStep);

        int twoStep = readSize - oneStep;
        memcpy_s(&dest[oneStep], twoStep, _buf, twoStep);
    }
    else
    {
        memcpy_s(dest, readSize, &_buf[_front], readSize);
    }

    return readSize;
}

void RingBuffer::ClearBuffer()
{
    _front = _rear = 0;

    return;
}

int RingBuffer::DirectEnqueueSize()
{
    // TODO: TEST
    if (_front > _rear)
    {
        return GetFreeSize();
    }
    else
    {
        if (_front == 0)
        {
            return (_capacity - _rear) -1;
        }
        else
        {
            return (_capacity - _rear);
        }
    }
}

int RingBuffer::DirectDequeueSize()
{
    // TODO: TEST
    if (_front == _rear)
    {
        return 0;
    }

    if (_front > _rear)
    {
        return _capacity - _front;
    }
    else
    {
        return _rear - _front;
    }
}

int RingBuffer::MoveRear(int size)
{
    // TODO: TEST

    int freeSize = GetFreeSize();
    if (size > freeSize)
    {
        size = freeSize;
    }
    _rear = (_rear + size) % _capacity;

    return size;
}

int RingBuffer::MoveFront(int size)
{
    // TODO: TEST

    int useSize = GetUseSize();
    if (size > useSize)
    {
        size = useSize;
    }

    _front = (_front + size) % _capacity;

    return size;
}

char* RingBuffer::GetFrontBufferPtr()
{
    return &_buf[_front];
}

char* RingBuffer::GetRearBufferPtr()
{
    return &_buf[_rear];
}
