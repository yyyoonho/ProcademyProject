#pragma once

class RingBuffer
{
public:
	RingBuffer();
	RingBuffer(int bufferSize);
	~RingBuffer();

	void Resize(int size);

	int GetBufferSize();

	int GetUseSize();
	int GetFreeSize();

	int Enqueue(const char* data, int size);
	int Dequeue(char* dest, int size);

	int Peek(char* dest, int size);

	void ClearBuffer();

	int DirectEnqueueSize();
	int DirectDequeueSize();

	int MoveRear(int size);
	int MoveFront(int size);

	char* GetFrontBufferPtr();
	char* GetRearBufferPtr();

public:
	char* _buf;

	int _capacity = 0;
	int _front = 0;
	int _rear = 0;
};