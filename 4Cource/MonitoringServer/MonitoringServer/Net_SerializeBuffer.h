#pragma once

union stExtraBuffer
{
	BYTE raw[16]; // 전체를 한 번에 접근하기 위한 배열
};

class Net_SerializePacket
{
	friend procademy::MemoryPool_TLS<Net_SerializePacket>;

public:
	inline static procademy::MemoryPool_TLS<Net_SerializePacket> SPacketMP{ 0,false };

public:
	enum en_PACKET
	{
		eBUFFER_DEFAULT = 1400 // 패킷의 기본 버퍼 사이즈
	};

private:
	Net_SerializePacket();
	Net_SerializePacket(int bufferSize);
	virtual ~Net_SerializePacket();

public:
	void Clear();
	int GetBufferSize();
	int GetDataSize();

	char* GetBufferPtr();

	int MoveWritePos(int size);
	int MoveReadPos(int size);


	/* 연산자 오버로딩 */
	Net_SerializePacket& operator= (Net_SerializePacket& clSrcPacket);

	Net_SerializePacket& operator<< (unsigned char byValue);
	Net_SerializePacket& operator<< (char chValue);

	Net_SerializePacket& operator << (short shValue);
	Net_SerializePacket& operator << (unsigned short wValue);

	Net_SerializePacket& operator << (int iValue);
	Net_SerializePacket& operator << (long lValue);
	Net_SerializePacket& operator << (float fValue);
	Net_SerializePacket& operator << (DWORD dwValue);

	Net_SerializePacket& operator << (__int64 iValue);
	Net_SerializePacket& operator << (double dValue);

	Net_SerializePacket& operator >> (BYTE& byValue);
	Net_SerializePacket& operator >> (char& chValue);

	Net_SerializePacket& operator >> (short& shValue);
	Net_SerializePacket& operator >> (WORD& wValue);

	Net_SerializePacket& operator >> (int& iValue);
	Net_SerializePacket& operator >> (DWORD& dwValue);
	Net_SerializePacket& operator >> (float& fValue);

	Net_SerializePacket& operator >> (__int64& iValue);
	Net_SerializePacket& operator >> (double& dValue);


	int GetData(char* chpDest, int iSize);
	int Putdata(char* chpSrc, int iSrcSize);

	bool PushHeader(char* header, int headerSize);
	bool PushExtraBuffer(char* header, int headerSize);

	bool IsEncoded();
	void MarkEncoded();

protected:
	char* _original;
	char* _originalBuf;
	char* _buf;

	int _writePos;
	int _readPos;

	int _capacity;
	int _size;
	int _usingExtraBuffer;

	bool _isHeaderPushed = false;
	int _pushedHeaderSize = 0;

	bool _bEncoded = false;
};