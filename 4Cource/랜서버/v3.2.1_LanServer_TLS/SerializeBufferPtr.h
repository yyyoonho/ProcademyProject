#pragma once

// ************************************
// 샘플 코드
// 
// SPacketPtr a(SPacketPtr::MakeSerializePacket());
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

struct RawPtr
{
	//**********************************************
	// 받았으면 그 즉시, Increse함수를 호출하도록 하자.
	//**********************************************

	SerializePacket* _ptr;
	RefCountBlock* _RCBPtr;

	void IncreaseRefCount();
	void DecreaseRefCount();
};

class SerializePacketPtr
{
public:
	inline static procademy::MemoryPool_TLS<RefCountBlock> RcbMP{ 0,false };

public:
	SerializePacketPtr();
	SerializePacketPtr(SerializePacket* ptr);
	SerializePacketPtr(const SerializePacketPtr& other);

	SerializePacketPtr& operator=(const SerializePacketPtr& other);
	SerializePacketPtr& operator=(std::nullptr_t);

	~SerializePacketPtr();

public:
	void GetRawPtr(OUT RawPtr* pRaw);
	bool IsValid();
	int GetRefCount();

public:
	static SerializePacket* MakeSerializePacket();

private:
	void DecreaseRefCount();

public:
	void Clear();
	int GetBufferSize();
	int GetDataSize();

	char* GetBufferPtr();

	int MoveWritePos(int size);
	int MoveReadPos(int size);

	SerializePacketPtr& operator<< (unsigned char byValue);
	SerializePacketPtr& operator<< (char chValue);

	SerializePacketPtr& operator << (short shValue);
	SerializePacketPtr& operator << (unsigned short wValue);

	SerializePacketPtr& operator << (int iValue);
	SerializePacketPtr& operator << (long lValue);
	SerializePacketPtr& operator << (float fValue);
	SerializePacketPtr& operator << (DWORD dwValue);

	SerializePacketPtr& operator << (__int64 iValue);
	SerializePacketPtr& operator << (double dValue);

	SerializePacketPtr& operator >> (BYTE& byValue);
	SerializePacketPtr& operator >> (char& chValue);

	SerializePacketPtr& operator >> (short& shValue);
	SerializePacketPtr& operator >> (WORD& wValue);

	SerializePacketPtr& operator >> (int& iValue);
	SerializePacketPtr& operator >> (DWORD& dwValue);
	SerializePacketPtr& operator >> (float& fValue);

	SerializePacketPtr& operator >> (__int64& iValue);
	SerializePacketPtr& operator >> (double& dValue);


	int GetData(char* chpDest, int iSize);
	int Putdata(char* chpSrc, int iSrcSize);

	void PushHeader(char* header, int headerSize);

private:
	SerializePacket* _ptr = NULL;
	RefCountBlock* _RCBPtr = NULL;
};