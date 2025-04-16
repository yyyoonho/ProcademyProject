#pragma once

struct stSession
{
	SOCKET socket;			// 현 접속자의 TCP소켓
	DWORD dwSessionID;		// 접속자의 고유 세션 ID

	RingBuffer recvQ;		// 수신 링버퍼
	RingBuffer sendQ;		// 송신 링버퍼

	DWORD dwLastRecvTime;	// 메시지 수신 체크를 위한 시간 (타임아웃용)

	bool bDestroy = false;
};

struct stCharacter
{
	stSession* pSession;
	DWORD dwSessionID;

	DWORD dwAction;
	BYTE byDirection;
	BYTE byMoveDirection;

	short shX;
	short shY;

	//SECTOR_POS curSector;
	//SECTOR_POS oldSector;

	char chHP;

};