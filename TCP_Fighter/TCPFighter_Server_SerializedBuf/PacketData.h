#pragma once

enum PACKET_TYPE
{
	PACKET_SC_CREATE_MY_CHARACTER			=0,
	PACKET_SC_CREATE_OTHER_CHARACTER		=1,
	PACKET_SC_DELETE_CHARACTER				=2,

	PACKET_CS_MOVE_START					=10,
	PACKET_SC_MOVE_START					=11,
	PACKET_CS_MOVE_STOP						=12,
	PACKET_SC_MOVE_STOP						=13,

	PACKET_CS_ATTACK1						=20,
	PACKET_SC_ATTACK1						=21,
	PACKET_CS_ATTACK2						=22,
	PACKET_SC_ATTACK2						=23,
	PACKET_CS_ATTACK3						=24,
	PACKET_SC_ATTACK3						=25,

	PACKET_SC_DAMAGE						=30,

	PACKET_CS_SYNC							=250,
	PACKET_SC_SYNC							=251,
};

#pragma pack(push, 1)
struct stPacketHeader
{
	BYTE _code;
	BYTE _size; // 패킷사이즈
	BYTE _type;	// 패킷타입.
};

// TYPE=0

struct stPACKET_SC_CREATE_MY_CHARACTER
{
	DWORD _id;
	BYTE _direction;
	short _x;
	short _y;
	char _hp;
};

// TYPE=1
struct stPACKET_SC_CREATE_OTHER_CHARACTER
{
	DWORD _id;
	BYTE _direction;
	short _x;
	short _y;
	char _hp;
};


// TYPE=2
struct stPACKET_SC_DELETE_CHARACTER
{
	DWORD _id;
};

// TYPE=10
struct stPACKET_CS_MOVE_START
{
	BYTE _direction;
	short _x;
	short _y;
};

// TYPE=11
struct stPACKET_SC_MOVE_START
{
	DWORD _id;
	BYTE _direction;
	short _x;
	short _y;
};

// TYPE=12
struct stPACKET_CS_MOVE_STOP
{
	BYTE _direction;
	short _x;
	short _y;
};

// TYPE=13
struct stPACKET_SC_MOVE_STOP
{
	DWORD _id;
	BYTE _direction;
	short _x;
	short _y;
};

// TYPE=20
struct stPACKET_CS_ATTACK1
{
	BYTE _direction;
	short _x;
	short _y;
};

// TYPE=21
struct stPACKET_SC_ATTACK1
{
	DWORD _id;
	BYTE _direction;
	short _x;
	short _y;
};

// TYPE=22
struct stPACKET_CS_ATTACK2
{
	BYTE _direction;
	short _x;
	short _y;
};

// TYPE=23
struct stPACKET_SC_ATTACK2
{
	DWORD _id;
	BYTE _direction;
	short _x;
	short _y;
};

// TYPE=24
struct stPACKET_CS_ATTACK3
{
	BYTE _direction;
	short _x;
	short _y;
};

// TYPE=25
struct stPACKET_SC_ATTACK3
{
	DWORD _id;
	BYTE _direction;
	short _x;
	short _y;
};

// TYPE=30
struct stPACKET_SC_DAMAGE
{
	DWORD _attackId;		// 공격자 ID
	DWORD _damageId;		// 피해자 ID
	char _damageHp;	// 피해자 HP
};

#pragma pack(pop)