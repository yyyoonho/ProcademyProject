#pragma once

// ************************************
// 샘플 코드
// 
// SPacketPtr a(SPacketPtr::SPacketMP.Alloc());
// -> 이때, 직렬화버퍼 할당 + RCB 할당이 발생합니다.
// SPacketPtr b = a;
// SPacketPtr c;
// c = a;
// 
// ************************************


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