#pragma comment(lib, "Net_SerializeBuffer.lib")
#pragma comment(lib, "SerializeBufferPtr.lib")

#include <Windows.h>
#include <iostream>

#include "TLS_MemoryPool.h"
#include "Net_SerializeBuffer.h"
#include "SerializeBufferPtr.h"
#include "NetCodec.h"

using namespace std;

BYTE fixedKey = 0xa9;

void EncodingPacket(SerializePacketPtr sPacketPtr)
{
	// TODO: 랜덤키 생성
	BYTE randomKey = 0x31;


	// TODO: 체크섬 생성
	BYTE checkSum = 0;
	char* payloadStart = sPacketPtr.GetBufferPtr();
	for (int i = 0; i < sPacketPtr.GetDataSize(); i++)
	{
		checkSum += (unsigned char)*(payloadStart + i);
	}
	checkSum = checkSum;


	// TODO: Push Header (Code(1) + Len(2) + RK(1) + CheckSum(1))
	stNetHeader header;
	unsigned short payloadLen = sPacketPtr.GetDataSize();
	if (payloadLen <= 0)
	{
		DebugBreak();
	}

	header.code = 0xbb;
	memcpy_s(&header.len, sizeof(BYTE) * 2, (char*)&payloadLen, sizeof(BYTE) * 2);
	header.randomKey = randomKey;
	header.checkSum = checkSum;

	sPacketPtr.PushHeader((char*)&header, sizeof(stNetHeader));
	

	// TODO: 체크섬 + 페이로드 => 암호화진행 (1단계)
	unsigned char* target = (unsigned char*)(sPacketPtr.GetBufferPtr());
	target += sizeof(stNetHeader);

	unsigned char P = 0;
	for (int i = 0; i < payloadLen; i++)
	{
		unsigned char D = (*(target + i)) ^ (P + randomKey + i + 1);
		P = D;

		*(target + i) = D;
	}


	// TODO: 2단계
	unsigned char E = 0;
	for (int i = 0; i < payloadLen; i++)
	{
		unsigned char D = (*(target + i)) ^ (E + fixedKey + i + 1);
		E = D;

		*(target + i) = D;
	}

}

int main()
{
	unsigned char* tmp = new unsigned char[55];
	strcpy_s((char*)tmp, 55, "aaaaaaaaaabbbbbbbbbbcccccccccc1234567890abcdefghijklmn");
	//strcpy_s((char*)tmp, 5, "abcd");

	SerializePacketPtr p1 = SerializePacketPtr::MakeSerializePacket();
	p1.Clear();
	p1.Putdata((char*)tmp, 55);

	EncodingPacket(p1);

	int b = 3;

	return 0;
}