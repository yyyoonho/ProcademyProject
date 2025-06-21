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

	void IncreseRefCount();
	void DecreseRefCount();
};

class SPacketPtr
{
public:
	SPacketPtr();
	SPacketPtr(SerializePacket* ptr);
	SPacketPtr(const SPacketPtr& other);

	SPacketPtr& operator=(const SPacketPtr& other);

	~SPacketPtr();

public:
	void GetRawPtr(OUT RawPtr* pRaw);

	static SerializePacket* MakeSerializePacket();

public:
	void Clear();
	int GetBufferSize();
	int GetDataSize();

	char* GetBufferPtr();

	int MoveWritePos(int size);
	int MoveReadPos(int size);

	SerializePacket& operator= (SerializePacket& clSrcPacket);

	SerializePacket& operator<< (unsigned char byValue);
	SerializePacket& operator<< (char chValue);

	SerializePacket& operator << (short shValue);
	SerializePacket& operator << (unsigned short wValue);

	SerializePacket& operator << (int iValue);
	SerializePacket& operator << (long lValue);
	SerializePacket& operator << (float fValue);
	SerializePacket& operator << (DWORD dwValue);

	SerializePacket& operator << (__int64 iValue);
	SerializePacket& operator << (double dValue);

	SerializePacket& operator >> (BYTE& byValue);
	SerializePacket& operator >> (char& chValue);

	SerializePacket& operator >> (short& shValue);
	SerializePacket& operator >> (WORD& wValue);

	SerializePacket& operator >> (int& iValue);
	SerializePacket& operator >> (DWORD& dwValue);
	SerializePacket& operator >> (float& fValue);

	SerializePacket& operator >> (__int64& iValue);
	SerializePacket& operator >> (double& dValue);


	int GetData(char* chpDest, int iSize);
	int Putdata(char* chpSrc, int iSrcSize);

	void PushHeader(char* header, int headerSize);

private:
	SerializePacket* _ptr;
	RefCountBlock* _RCBPtr;
};