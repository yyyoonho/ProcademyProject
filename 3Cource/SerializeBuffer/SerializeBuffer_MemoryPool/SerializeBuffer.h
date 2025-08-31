#pragma once

struct stTCPHeader
{
	BYTE _header[5];
};

class SerializePacket
{
public:
	enum en_PACKET
	{
		eBUFFER_DEFAULT = 1400 // 패킷의 기본 버퍼 사이즈
	};

	SerializePacket();
	SerializePacket(int bufferSize);
	virtual ~SerializePacket();

	void Clear();
	int GetBufferSize();
	int GetDataSize();

	char* GetBufferPtr();
	
	int MoveWritePos(int size);
	int MoveReadPos(int size);


	/* 연산자 오버로딩 */
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

	bool PushHeader(char* header, int headerSize);

protected:
	char* _original;
	char* _buf;

	int _writePos;
	int _readPos;

	int _capacity;
	int _size;

	bool _isHeaderPushed = false;
	int _pushedHeaderSize = 0;

};