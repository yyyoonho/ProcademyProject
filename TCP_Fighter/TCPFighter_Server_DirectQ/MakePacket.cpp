#include "Windows.h"
#include "PacketData.h"
#include "MakePacket.h"

void mpCreateMyCharacter(stPacketHeader* pHeader, stPACKET_SC_CREATE_MY_CHARACTER* pPacket, DWORD id, DWORD dir, short x, short y)
{
	pHeader->_code = 0x89;
	pHeader->_size = sizeof(stPACKET_SC_CREATE_MY_CHARACTER);
	pHeader->_type = PACKET_SC_CREATE_MY_CHARACTER;

	pPacket->_id = id;
	pPacket->_direction = dir;
	pPacket->_x = x;
	pPacket->_y = y;
	pPacket->_hp = 100;

	return;
}

void mpCreateOtherCharacter(stPacketHeader* pHeader, stPACKET_SC_CREATE_OTHER_CHARACTER* pPacket, DWORD id, DWORD dir, short x, short y, char hp)
{
	pHeader->_code = 0x89;
	pHeader->_size = sizeof(stPACKET_SC_CREATE_OTHER_CHARACTER);
	pHeader->_type = PACKET_SC_CREATE_OTHER_CHARACTER;

	pPacket->_id = id;
	pPacket->_direction = dir;
	pPacket->_x = x;
	pPacket->_y = y;
	pPacket->_hp = hp;

	return;
}

void mpDeleteCharacter(stPacketHeader* pHeader, stPACKET_SC_DELETE_CHARACTER* pPacket, DWORD id)
{
	pHeader->_code = 0x89;
	pHeader->_size = sizeof(stPACKET_SC_DELETE_CHARACTER);
	pHeader->_type = PACKET_SC_DELETE_CHARACTER;

	pPacket->_id = id;

	return;
}

void mpMoveStart(stPacketHeader* pHeader, stPACKET_SC_MOVE_START* pPacket, DWORD id, DWORD dir, short x, short y)
{
	pHeader->_code = 0x89;
	pHeader->_size = sizeof(stPACKET_SC_MOVE_START);
	pHeader->_type = PACKET_SC_MOVE_START;

	pPacket->_id = id;
	pPacket->_direction = dir;
	pPacket->_x = x;
	pPacket->_y = y;

	return;
}

void mpMoveStop(stPacketHeader* pHeader, stPACKET_SC_MOVE_STOP* pPacket, DWORD id, DWORD dir, short x, short y)
{
	pHeader->_code = 0x89;
	pHeader->_size = sizeof(stPACKET_SC_MOVE_STOP);
	pHeader->_type = PACKET_SC_MOVE_STOP;

	pPacket->_id = id;
	pPacket->_direction = dir;
	pPacket->_x = x;
	pPacket->_y = y;

	return;
}

void mpAttack1(stPacketHeader* pHeader, stPACKET_SC_ATTACK1* pPacket, DWORD id, DWORD dir, short x, short y)
{
	pHeader->_code = 0x89;
	pHeader->_size = sizeof(stPACKET_SC_ATTACK1);
	pHeader->_type = PACKET_SC_ATTACK1;

	pPacket->_id = id;
	pPacket->_direction = dir;
	pPacket->_x = x;
	pPacket->_y = y;

	return;
}

void mpAttack2(stPacketHeader* pHeader, stPACKET_SC_ATTACK2* pPacket, DWORD id, DWORD dir, short x, short y)
{
	pHeader->_code = 0x89;
	pHeader->_size = sizeof(stPACKET_SC_ATTACK2);
	pHeader->_type = PACKET_SC_ATTACK2;

	pPacket->_id = id;
	pPacket->_direction = dir;
	pPacket->_x = x;
	pPacket->_y = y;

	return;
}

void mpAttack3(stPacketHeader* pHeader, stPACKET_SC_ATTACK3* pPacket, DWORD id, DWORD dir, short x, short y)
{
	pHeader->_code = 0x89;
	pHeader->_size = sizeof(stPACKET_SC_ATTACK3);
	pHeader->_type = PACKET_SC_ATTACK3;

	pPacket->_id = id;
	pPacket->_direction = dir;
	pPacket->_x = x;
	pPacket->_y = y;

	return;
}

void mpDamage(stPacketHeader* pHeader, stPACKET_SC_DAMAGE* pPacket, DWORD attackId, DWORD damageId, char damageHp)
{
	pHeader->_code = 0x89;
	pHeader->_size = sizeof(stPACKET_SC_DAMAGE);
	pHeader->_type = PACKET_SC_DAMAGE;

	pPacket->_attackId = attackId;
	pPacket->_damageId = damageId;
	pPacket->_damageHp = damageHp;

	return;
}
