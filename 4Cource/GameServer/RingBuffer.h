#pragma once

class RingBuffer
{
public:
	RingBuffer();
	RingBuffer(int bufferSize);
	~RingBuffer();

	void Resize(int size);

	int GetBufferSize();

	__forceinline int GetUseSize();
	int GetFreeSize();

	__forceinline int Enqueue(const char* data, int size);
	__forceinline int Dequeue(char* dest, int size);

	__forceinline int Peek(char* dest, int size);

	void ClearBuffer();

	__forceinline int DirectEnqueueSize();
	__forceinline int DirectDequeueSize();

	__forceinline int MoveRear(int size);
	__forceinline int MoveFront(int size);

	__forceinline char* GetFrontBufferPtr();
	__forceinline char* GetRearBufferPtr();

public:
	char* _buf;

	int _capacity = 0;
	int _front = 0;
	int _rear = 0;
};

__forceinline int RingBuffer::GetUseSize()
{
	int a = _front;
	int b = _rear;

	if (a == b)
		return 0;

	if (a > b)
	{
		return _capacity - (a - b);
	}
	else
	{
		return b - a;
	}
}

__forceinline int RingBuffer::Enqueue(const char* data, int size)
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
__forceinline int RingBuffer::Dequeue(char* dest, int size)
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

__forceinline int RingBuffer::Peek(char* dest, int size)
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

	return readSize;
}

__forceinline int RingBuffer::DirectEnqueueSize()
{
	int a = _front;
	int b = _rear;

	if (a > b)
	{
		return GetFreeSize();
	}
	else
	{
		if (a == 0)
		{
			return (_capacity - b) - 1;
		}
		else
		{
			return (_capacity - b);
		}
	}
}
__forceinline int RingBuffer::DirectDequeueSize()
{
	int a = _front;
	int b = _rear;

	if (a == b)
	{
		return 0;
	}

	if (a > b)
	{
		return _capacity - a;
	}
	else
	{
		return b - a;
	}
}

__forceinline int RingBuffer::MoveRear(int size)
{
	int freeSize = GetFreeSize();
	if (size > freeSize)
	{
		size = freeSize;
	}
	_rear = (_rear + size) % _capacity;

	return size;
}
__forceinline int RingBuffer::MoveFront(int size)
{
	int useSize = GetUseSize();
	if (size > useSize)
	{
		size = useSize;
	}

	_front = (_front + size) % _capacity;

	return size;
}

__forceinline char* RingBuffer::GetFrontBufferPtr()
{
	return &_buf[_front];
}
__forceinline char* RingBuffer::GetRearBufferPtr()
{
	return &_buf[_rear];
}