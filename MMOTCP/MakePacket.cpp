#include "stdafx.h"

#include "MakePacket.h"
#include "LogManager.h"

using namespace std;

void mpCreateMyCharacter(SerializePacket* sPacket, DWORD id, BYTE dir, short x, short y)
{
	_LOG(dfLOG_LEVEL_DEBUG, L"# mpCreateMyCharacter # SessionID:%d\n", id);

	st_PACKET_HEADER header;
	header.byCode = dfPACKET_CODE;
	header.bySize = 10;
	header.byType = dfPACKET_SC_CREATE_MY_CHARACTER;

	(*sPacket).Putdata((char*)&header, sizeof(st_PACKET_HEADER));

	char hp = 100;
	(*sPacket) << id;
	(*sPacket) << dir;
	(*sPacket) << x;
	(*sPacket) << y;
	(*sPacket) << hp;

	return;
}

void mpCreateOtherCharacter(SerializePacket* sPacket, DWORD id, BYTE dir, short x, short y, char hp)
{
	_LOG(dfLOG_LEVEL_DEBUG, L"# mpCreateOtherCharacter # SessionID:%d\n", id);

	st_PACKET_HEADER header;
	header.byCode = dfPACKET_CODE;
	header.bySize = 10;
	header.byType = dfPACKET_SC_CREATE_OTHER_CHARACTER;

	(*sPacket).Putdata((char*)&header, sizeof(st_PACKET_HEADER));

	(*sPacket) << id;
	(*sPacket) << dir;
	(*sPacket) << x;
	(*sPacket) << y;
	(*sPacket) << hp;

	return;
}

void mpDeleteCharacter(SerializePacket* sPacket, DWORD id)
{
	_LOG(dfLOG_LEVEL_DEBUG, L"# mpDeleteCharacter # SessionID:%d\n", id);

	st_PACKET_HEADER header;
	header.byCode = dfPACKET_CODE;
	header.bySize = 4;
	header.byType = dfPACKET_SC_DELETE_CHARACTER;

	(*sPacket).Putdata((char*)&header, sizeof(st_PACKET_HEADER));

	(*sPacket) << id;

	return;
}

void mpMoveStart(SerializePacket* sPacket, DWORD id, BYTE dir, short x, short y)
{
	_LOG(dfLOG_LEVEL_DEBUG, L"# mpMoveStart # SessionID:%d\n", id);

	st_PACKET_HEADER header;
	header.byCode = dfPACKET_CODE;
	header.bySize = 9;
	header.byType = dfPACKET_SC_MOVE_START;

	(*sPacket).Putdata((char*)&header, sizeof(st_PACKET_HEADER));

	(*sPacket) << id;
	(*sPacket) << dir;
	(*sPacket) << x;
	(*sPacket) << y;

	return;
}

void mpMoveStop(SerializePacket* sPacket, DWORD id, BYTE dir, short x, short y)
{
	_LOG(dfLOG_LEVEL_DEBUG, L"# mpMoveStop # SessionID:%d\n", id);

	st_PACKET_HEADER header;
	header.byCode = dfPACKET_CODE;
	header.bySize = 9;
	header.byType = dfPACKET_SC_MOVE_STOP;

	(*sPacket).Putdata((char*)&header, sizeof(st_PACKET_HEADER));

	(*sPacket) << id;
	(*sPacket) << dir;
	(*sPacket) << x;
	(*sPacket) << y;

	return;
}

void mpAttack1(SerializePacket* sPacket, DWORD id, BYTE dir, short x, short y)
{
	_LOG(dfLOG_LEVEL_DEBUG, L"# mpAttack1 # SessionID:%d\n", id);

	st_PACKET_HEADER header;
	header.byCode = dfPACKET_CODE;
	header.bySize = 9;
	header.byType = dfPACKET_SC_ATTACK1;

	(*sPacket).Putdata((char*)&header, sizeof(st_PACKET_HEADER));

	(*sPacket) << id;
	(*sPacket) << dir;
	(*sPacket) << x;
	(*sPacket) << y;

	return;
}

void mpAttack2(SerializePacket* sPacket, DWORD id, BYTE dir, short x, short y)
{
	_LOG(dfLOG_LEVEL_DEBUG, L"# mpAttack2 # SessionID:%d\n", id);

	st_PACKET_HEADER header;
	header.byCode = dfPACKET_CODE;
	header.bySize = 9;
	header.byType = dfPACKET_SC_ATTACK2;

	(*sPacket).Putdata((char*)&header, sizeof(st_PACKET_HEADER));

	(*sPacket) << id;
	(*sPacket) << dir;
	(*sPacket) << x;
	(*sPacket) << y;

	return;
}

void mpAttack3(SerializePacket* sPacket, DWORD id, BYTE dir, short x, short y)
{
	_LOG(dfLOG_LEVEL_DEBUG, L"# mpAttack3 # SessionID:%d\n", id);

	st_PACKET_HEADER header;
	header.byCode = dfPACKET_CODE;
	header.bySize = 9;
	header.byType = dfPACKET_SC_ATTACK3;

	(*sPacket).Putdata((char*)&header, sizeof(st_PACKET_HEADER));

	(*sPacket) << id;
	(*sPacket) << dir;
	(*sPacket) << x;
	(*sPacket) << y;

	return;
}

void mpDamage(SerializePacket* sPacket, DWORD attackId, DWORD damageId, char damageHp)
{
	_LOG(dfLOG_LEVEL_DEBUG, L"# mpDamage # SessionID:%d\n", attackId);

	st_PACKET_HEADER header;
	header.byCode = dfPACKET_CODE;
	header.bySize = 9;
	header.byType = dfPACKET_SC_DAMAGE;

	(*sPacket).Putdata((char*)&header, sizeof(st_PACKET_HEADER));

	(*sPacket) << attackId;
	(*sPacket) << damageId;
	(*sPacket) << damageHp;

	return;
}

void mpSync(SerializePacket* sPacket, DWORD id, short x, short y)
{
	_LOG(dfLOG_LEVEL_DEBUG, L"# mpSync # SessionID:%d\n", id);

	st_PACKET_HEADER header;
	header.byCode = dfPACKET_CODE;
	header.bySize = 8;
	header.byType = dfPACKET_SC_SYNC;

	(*sPacket).Putdata((char*)&header, sizeof(st_PACKET_HEADER));

	(*sPacket) << id;
	(*sPacket) << x;
	(*sPacket) << y;

	return;
}

void mpEcho(SerializePacket* sPacket, DWORD time)
{
	st_PACKET_HEADER header;
	header.byCode = dfPACKET_CODE;
	header.bySize = 4;
	header.byType = dfPACKET_SC_ECHO;

	(*sPacket).Putdata((char*)&header, sizeof(st_PACKET_HEADER));

	(*sPacket) << time;

	return;
}
