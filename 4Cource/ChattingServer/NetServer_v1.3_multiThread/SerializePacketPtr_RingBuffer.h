#pragma once

class SerializePacketPtr;

class SerializePacketPtr_RingBuffer
{
public:
	SerializePacketPtr_RingBuffer();
	SerializePacketPtr_RingBuffer(int bufferSize);
	~SerializePacketPtr_RingBuffer();

	void Resize(int size);

	int GetBufferSize();

	int GetUseSize();
	int GetFreeSize();

	int Enqueue(const SerializePacketPtr& data);
	int Dequeue(SerializePacketPtr& dest);

	int Peek(SerializePacketPtr& dest);

	void ClearBuffer();

	int DirectEnqueueSize();
	int DirectDequeueSize();

	int MoveRear(int size);
	int MoveFront(int size);

private:
	SerializePacketPtr* _buf;

	int _capacity = 0;
	int _front = 0;
	int _rear = 0;
};