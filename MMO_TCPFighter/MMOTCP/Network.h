#pragma once
#include "RingBuffer.h"

struct stSession
{
	SOCKET socket;			// 현 접속자의 TCP소켓
	DWORD dwSessionID;		// 접속자의 고유 세션 ID

	SOCKADDR_IN clientAddr;

	RingBuffer recvQ;		// 수신 링버퍼
	RingBuffer sendQ;		// 송신 링버퍼

	DWORD dwLastRecvTime;	// 메시지 수신 체크를 위한 시간 (타임아웃용)
};


void NetInit();
void NetCleanUp();

void NetworkUpdate();
void PushSessionToDestroyQ(stSession* pSession);