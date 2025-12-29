#include "stdafx.h"
#include "NetCodec.h"

using namespace std;

BYTE fixedKey = 0x32;

BYTE GetRandomKey()
{
	// TODO: 랜덤키 생성

	return 0;
}

bool EncodingPacket(SerializePacketPtr sPacketPtr)
{
	// TODO: 랜덤키 생성
	BYTE randomKey = 0x31;
	//BYTE randomKey = GetRandomKey();

	// 체크섬 생성
	BYTE checkSum = 0;
	unsigned char* payloadStart = (unsigned char*)sPacketPtr.GetBufferPtr();

	unsigned short payloadLen = sPacketPtr.GetDataSize();

	for (int i = 0; i < payloadLen; i++)
	{
		checkSum += (unsigned char)*(payloadStart + i);
	}
	checkSum = checkSum % 256;


	// Push Header (Code(1) + Len(2) + RK(1) + CheckSum(1))
	stNetHeader header;

	header.code = 0x77;
	memcpy_s(&header.len, sizeof(BYTE) * 2, (char*)&payloadLen, sizeof(BYTE) * 2);
	header.randomKey = randomKey;
	header.checkSum = checkSum;

	sPacketPtr.PushHeader((char*)&header, sizeof(stNetHeader));

	// 1단게 디코딩
	unsigned char* target = (unsigned char*)sPacketPtr.GetBufferPtr();
	target = target + 4;

	unsigned char P = 0;
	for (int i = 0; i < payloadLen + 1; i++)
	{
		unsigned char tmpP = *(target + i) ^ (P + randomKey + (i + 1));
		P = tmpP;

		*(target + i) = tmpP;
	}

	unsigned char E = 0;
	for (int i = 0; i < payloadLen + 1; i++)
	{
		unsigned char tmpE = *(target + i) ^ (E + fixedKey + (i + 1));
		E = tmpE;

		*(target + i) = tmpE;
	}

	char* t = sPacketPtr.GetBufferPtr();

	return true;
}

bool DecodingPacket(SerializePacketPtr sPacketPtr, stNetHeader netHeader)
{
	unsigned char* target = (unsigned char*)sPacketPtr.GetBufferPtr();

	BYTE randomKey = netHeader.randomKey;
	BYTE checkSum = netHeader.checkSum;

	int len = sPacketPtr.GetDataSize();
	

	// 1단계 디코딩
	unsigned char E = checkSum;
	checkSum = E ^ (fixedKey + 1);

	for (int i = 0; i < len; i++)
	{
		unsigned char tmpE = *(target + i);

		unsigned char P = (*(target + i)) ^ (E + fixedKey + (i + 2));
		*(target + i) = P;

		E = tmpE;
	}

	// 2단계 디코딩
	unsigned char P = checkSum;
	checkSum = P ^ (randomKey + 1);

	for (int i = 0; i < len; i++)
	{
		unsigned char tmpP = *(target + i);

		unsigned char D = (*(target + i)) ^ (P + randomKey + (i + 2));
		*(target + i) = D;

		P = tmpP;
	}

	// 체크섬 확인
	BYTE myCheckSum = 0;
	for (int i = 0; i < len; i++)
	{
		myCheckSum += (unsigned char)*(target + i);
	}
	myCheckSum = myCheckSum % 256;

	if (checkSum == myCheckSum)
		return true;
	else
		return false;
}

bool JustPushHeader(SerializePacketPtr sPacketPtr)
{
	// TODO: 랜덤키 생성
	BYTE randomKey = 0x31;
	//BYTE randomKey = GetRandomKey();

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
		return false;
	}

	header.code = 0xbb;
	memcpy_s(&header.len, sizeof(BYTE) * 2, (char*)&payloadLen, sizeof(BYTE) * 2);
	header.randomKey = randomKey;
	header.checkSum = checkSum;

	sPacketPtr.PushHeader((char*)&header, sizeof(stNetHeader));

	return true;
}
