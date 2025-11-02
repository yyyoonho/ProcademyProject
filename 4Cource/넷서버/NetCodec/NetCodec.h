#pragma once

/*
Code(1byte) - Len(2byte) - RandKey(1byte) - CheckSum(1byte) - Payload(Len byte)
@ 암호화 대상은 CheckSum + Payload 입니다.  
@ Len 과 RandKey 는 암호화 하지 않고 그대로 노출 합니다.
*/


void EncodingPacket(SerializePacketPtr sPacketPtr, int size);