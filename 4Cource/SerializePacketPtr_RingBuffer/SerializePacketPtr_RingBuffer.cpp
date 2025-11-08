#pragma comment(lib,"Net_SerializeBuffer.lib")
#pragma comment(lib,"SerializeBufferPtr.lib")

#include <Windows.h>
#include <iostream>

#include "TLS_MemoryPool.h"
#include "Net_SerializeBuffer.h"
#include "SerializeBufferPtr.h"

#include "SerializePacketPtr_RingBuffer.h"

using namespace std;

#define DEFAULT_BUFSIZE 7000

SerializePacketPtr_RingBuffer::SerializePacketPtr_RingBuffer()
{
    _capacity = DEFAULT_BUFSIZE;
    _buf = new SerializePacketPtr[_capacity];
}

SerializePacketPtr_RingBuffer::SerializePacketPtr_RingBuffer(int bufferSize)
{
    _capacity = bufferSize;
    _buf = new SerializePacketPtr[_capacity];
}

SerializePacketPtr_RingBuffer::~SerializePacketPtr_RingBuffer()
{
    delete[] _buf;
}

void SerializePacketPtr_RingBuffer::Resize(int size)
{
    SerializePacketPtr* tmpBuf = new SerializePacketPtr[size];
    memcpy_s(tmpBuf, size * sizeof(SerializePacketPtr), _buf, _capacity);

    delete[] _buf;

    _buf = tmpBuf;
    _capacity = size;
}

int SerializePacketPtr_RingBuffer::GetBufferSize()
{
    return _capacity;
}

int SerializePacketPtr_RingBuffer::GetUseSize()
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

int SerializePacketPtr_RingBuffer::GetFreeSize()
{
    return (_capacity - GetUseSize()) - 1;
}

int SerializePacketPtr_RingBuffer::Enqueue(const SerializePacketPtr& data)
{
    int freeSize = GetFreeSize();
    if (freeSize == 0)
        return 0;

    _buf[_rear] = data;

    _rear = (_rear + 1) % _capacity;

    return 1;
}

int SerializePacketPtr_RingBuffer::Dequeue(SerializePacketPtr& data)
{
    int useSize = GetUseSize();
    if (useSize == 0)
        return 0;

    data = _buf[_front];
    _buf[_front] = nullptr;

    _front = (_front + 1) % _capacity;

    return 1;
}

int SerializePacketPtr_RingBuffer::Peek(SerializePacketPtr& dest)
{
    int useSize = GetUseSize();
    if (useSize == 0)
        return 0;

    dest = _buf[_front];

    return 1;
}

void SerializePacketPtr_RingBuffer::ClearBuffer()
{
    _front = _rear = 0;

    return;
}

int SerializePacketPtr_RingBuffer::DirectEnqueueSize()
{
    if (_front > _rear)
    {
        return GetFreeSize();
    }
    else
    {
        if (_front == 0)
        {
            return (_capacity - _rear) - 1;
        }
        else
        {
            return (_capacity - _rear);
        }
    }
}

int SerializePacketPtr_RingBuffer::DirectDequeueSize()
{
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

int SerializePacketPtr_RingBuffer::MoveRear(int size)
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

int SerializePacketPtr_RingBuffer::MoveFront(int size)
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