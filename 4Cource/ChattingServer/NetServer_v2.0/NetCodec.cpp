#include "stdafx.h"
#include "NetCodec.h"

using namespace std;

BYTE fixedKey = 0xa9;

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

	char* t = sPacketPtr.GetBufferPtr();

	return true;
}

bool DecodingPacket(SerializePacketPtr sPacketPtr, stNetHeader netHeader)
{
	unsigned char* target = (unsigned char*)sPacketPtr.GetBufferPtr();

	BYTE randomKey = netHeader.randomKey;

	// 1단계 디코딩
	unsigned char E = 0;
	for (int i = 0; i < sPacketPtr.GetDataSize(); i++)
	{
		unsigned char tmpE = *(target + i);

		unsigned char P = (*(target + i)) ^ (E + fixedKey + (i + 1));
		*(target + i) = P;

		E = tmpE;
	}

	// 2단계 디코딩
	unsigned char P = 0;
	for (int i = 0; i < sPacketPtr.GetDataSize(); i++)
	{
		unsigned char tmpP = *(target + i);

		unsigned char D = (*(target + i)) ^ (P + randomKey + (i + 1));
		*(target + i) = D;

		P = tmpP;
	}

	// 체크섬 확인
	BYTE originCheckSum = netHeader.checkSum;
	BYTE checkSum = 0;
	for (int i = 0; i < sPacketPtr.GetDataSize(); i++)
	{
		checkSum += *(target + i);
	}

	if (originCheckSum == checkSum)
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
