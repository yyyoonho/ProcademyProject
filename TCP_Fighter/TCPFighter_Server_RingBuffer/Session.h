#pragma once

struct Session
{
	/* 세션 파트 */
	SOCKET _socket;
	SOCKADDR_IN _clientAddr;
	DWORD _id;
	RingBuffer recvQ;
	RingBuffer sendQ;

	/* 플레이어 파트*/
	DWORD _action; // STATE 역할
	BYTE  _characterDirection; // 캐릭터의 방향 (2방)
	short _x;
	short _y;
	char _hp;

	double _dX;
	double _dY;
};