#pragma once

struct RefCountBlock
{
	LONG count = 0;
};

class SPacketPtr
{
public:
	SPacketPtr();
	SPacketPtr(SerializePacket* ptr);
	SPacketPtr(const SPacketPtr& other);

	SPacketPtr& operator=(const SPacketPtr& other);

	~SPacketPtr();

	static procademy::MemoryPool<SerializePacket> SPacketMP;

private:
	SerializePacket* _ptr;
	RefCountBlock* _RCBPtr;
};

procademy::MemoryPool<SerializePacket> SPacketPtr::SPacketMP(0, false);