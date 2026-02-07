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
	if (size <= 0)
		return 0;

	int front = _front;
	int rear = _rear;

	int useSize;
	if (front == rear)
	{
		useSize = 0;
	}
	else if (front > rear)
	{
		useSize = _capacity - (front - rear);
	}
	else
	{
		useSize = rear - front;
	}

	int freeSize;
	freeSize = _capacity - useSize - 1;
	if (freeSize <= 0)
		return 0;

	int writeSize = size;
	if (writeSize > freeSize)
	{
		writeSize = freeSize;
	}




	int newRear = rear + writeSize;

	if (newRear <= _capacity)
	{
		memcpy(&_buf[rear], data, writeSize);

		if (newRear == _capacity)
			newRear = 0;
	}
	else
	{
		int oneStep = _capacity - rear;
		memcpy(&_buf[rear], data, oneStep);

		int twoStep = writeSize - oneStep;
		memcpy(_buf, data + oneStep, twoStep);

		newRear = twoStep;
	}

	_rear = newRear;
	return writeSize;
}
__forceinline int RingBuffer::Dequeue(char* dest, int size)
{
	if (size <= 0)
		return 0;

	int front = _front;
	int rear = _rear;

	int useSize;
	if (rear >= front)
		useSize = rear - front;
	else
		useSize = _capacity - (front - rear);

	if (useSize <= 0)
		return 0;

	int readSize = size;
	if (readSize > useSize)
		readSize = useSize;


	int newFront = front + readSize;

	if (newFront <= _capacity)  
	{
		memcpy(dest, &_buf[front], readSize);

		if (newFront == _capacity)
			newFront = 0;
	}
	else
	{
		int oneStep = _capacity - front;
		memcpy(dest, &_buf[front], oneStep);

		int twoStep = readSize - oneStep;
		memcpy(dest + oneStep, _buf, twoStep);

		newFront = twoStep;
	}

	_front = newFront;
	return readSize;
}

__forceinline int RingBuffer::Peek(char* dest, int size)
{
	if (size <= 0)
		return 0;

	int front = _front;
	int rear = _rear;

	int useSize;
	if (rear >= front)
		useSize = rear - front;
	else
		useSize = _capacity - (front - rear);

	if (useSize <= 0)
		return 0;

	int readSize = size;
	if (readSize > useSize)
		readSize = useSize;


	int endPos = front + readSize;

	if (endPos <= _capacity)
	{
		memcpy(dest, &_buf[front], readSize); 
	}
	else
	{
		int oneStep = _capacity - front;
		memcpy(dest, &_buf[front], oneStep);  

		int twoStep = readSize - oneStep;
		memcpy(dest + oneStep, _buf, twoStep); 
	}

	return readSize;
}

__forceinline int RingBuffer::DirectEnqueueSize()
{
	int front = _front;
	int rear = _rear;

	if (front > rear)
	{
		return (front - rear) - 1;
	}

	if (front == 0)
	{
		return (_capacity - rear) - 1;
	}

	return (_capacity - rear);
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
	if (size <= 0)
		return 0;

	int front = _front;
	int rear = _rear;

	int useSize;
	if (rear >= front)
		useSize = rear - front;
	else
		useSize = _capacity - (front - rear);

	int freeSize = _capacity - useSize - 1;
	if (freeSize <= 0)
		return 0;

	if (size > freeSize)
		size = freeSize;

	int newRear = rear + size; 
	if (newRear >= _capacity)  
		newRear -= _capacity;

	_rear = newRear;

	return size;
}
__forceinline int RingBuffer::MoveFront(int size)
{
	if (size <= 0)
		return 0;

	int front = _front;
	int rear = _rear;


	int useSize;
	if (rear >= front)
		useSize = rear - front;
	else
		useSize = _capacity - (front - rear);

	if (useSize <= 0)
		return 0;

	if (size > useSize)
		size = useSize;


	int newFront = front + size;
	if (newFront >= _capacity)  
		newFront -= _capacity;

	_front = newFront;

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