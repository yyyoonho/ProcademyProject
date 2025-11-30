#pragma once

/*
Code(1byte) - Len(2byte) - RandKey(1byte) - CheckSum(1byte) - Payload(Len byte)
@ 암호화 대상은 CheckSum + Payload 입니다.
@ checkSum은 페이로드를 1바이트씩 더한값 % 256.
@ payload는 인코딩/디코딩 함수 사용.
@ Len 과 RandKey 는 암호화 하지 않고 그대로 노출 합니다.
*/


BYTE GetRandomKey();

bool EncodingPacket(SerializePacketPtr sPacketPtr);
bool DecodingPacket(SerializePacketPtr sPacketPtr, stNetHeader netHeader);

bool JustPushHeader(SerializePacketPtr sPacketPtr);