#pragma comment(lib, "SerializeBuffer.lib")

#include "Windows.h"
#include "SerializeBuffer.h"
#include "PacketData.h"
#include "MakePacket.h"

void mpCreateMyCharacter(SerializePacket* sPacket, DWORD id, BYTE dir, short x, short y)
{
	stPacketHeader header;
	header._code = 0x89;
	header._size = 10;
	header._type = PACKET_SC_CREATE_MY_CHARACTER;
	
	(*sPacket).Putdata((char*)&header, sizeof(stPacketHeader));


	//pPacket->_id = id;
	//pPacket->_direction = dir;
	//pPacket->_x = x;
	//pPacket->_y = y;
	//pPacket->_hp = 100;

	char hp = 100;

	(*sPacket) << id;
	(*sPacket) << dir;
	(*sPacket) << x;
	(*sPacket) << y;
	(*sPacket) << hp;

	int a = (*sPacket).GetDataSize();
	int b = 3;

	return;
}

void mpCreateOtherCharacter(SerializePacket* sPacket, DWORD id, BYTE dir, short x, short y, char hp)
{
	stPacketHeader header;
	header._code = 0x89;
	header._size = 10;
	header._type = PACKET_SC_CREATE_OTHER_CHARACTER;

	(*sPacket).Putdata((char*)&header, sizeof(stPacketHeader));

	//pPacket->_id = id;
	//pPacket->_direction = dir;
	//pPacket->_x = x;
	//pPacket->_y = y;
	//pPacket->_hp = hp;

	(*sPacket) << id;
	(*sPacket) << dir;
	(*sPacket) << x;
	(*sPacket) << y;
	(*sPacket) << hp;

	return;
}

void mpDeleteCharacter(SerializePacket* sPacket, DWORD id)
{
	stPacketHeader header;
	header._code = 0x89;
	header._size = 4;
	header._type = PACKET_SC_DELETE_CHARACTER;

	(*sPacket).Putdata((char*)&header, sizeof(stPacketHeader));

	//pPacket->_id = id;

	(*sPacket) << id;

	return;
}

void mpMoveStart(SerializePacket* sPacket, DWORD id, BYTE dir, short x, short y)
{
	stPacketHeader header;
	header._code = 0x89;
	header._size = 9;
	header._type = PACKET_SC_MOVE_START;

	(*sPacket).Putdata((char*)&header, sizeof(stPacketHeader));

	//pPacket->_id = id;
	//pPacket->_direction = dir;
	//pPacket->_x = x;
	//pPacket->_y = y;

	(*sPacket) << id;
	(*sPacket) << dir;
	(*sPacket) << x;
	(*sPacket) << y;

	return;
}

void mpMoveStop(SerializePacket* sPacket, DWORD id, BYTE dir, short x, short y)
{
	stPacketHeader header;
	header._code = 0x89;
	header._size = 9;
	header._type = PACKET_SC_MOVE_STOP;

	(*sPacket).Putdata((char*)&header, sizeof(stPacketHeader));

	//pPacket->_id = id;
	//pPacket->_direction = dir;
	//pPacket->_x = x;
	//pPacket->_y = y;

	(*sPacket) << id;
	(*sPacket) << dir;
	(*sPacket) << x;
	(*sPacket) << y;

	return;
}

void mpAttack1(SerializePacket* sPacket, DWORD id, BYTE dir, short x, short y)
{
	stPacketHeader header;
	header._code = 0x89;
	header._size = 9;
	header._type = PACKET_SC_ATTACK1;

	(*sPacket).Putdata((char*)&header, sizeof(stPacketHeader));

	//pPacket->_id = id;
	//pPacket->_direction = dir;
	//pPacket->_x = x;
	//pPacket->_y = y;

	(*sPacket) << id;
	(*sPacket) << dir;
	(*sPacket) << x;
	(*sPacket) << y;

	return;
}

void mpAttack2(SerializePacket* sPacket, DWORD id, BYTE dir, short x, short y)
{
	stPacketHeader header;
	header._code = 0x89;
	header._size = 9;
	header._type = PACKET_SC_ATTACK2;

	(*sPacket).Putdata((char*)&header, sizeof(stPacketHeader));

	//pPacket->_id = id;
	//pPacket->_direction = dir;
	//pPacket->_x = x;
	//pPacket->_y = y;

	(*sPacket) << id;
	(*sPacket) << dir;
	(*sPacket) << x;
	(*sPacket) << y;

	return;
}

void mpAttack3(SerializePacket* sPacket, DWORD id, BYTE dir, short x, short y)
{
	stPacketHeader header;
	header._code = 0x89;
	header._size = 9;
	header._type = PACKET_SC_ATTACK3;

	(*sPacket).Putdata((char*)&header, sizeof(stPacketHeader));

	//pPacket->_id = id;
	//pPacket->_direction = dir;
	//pPacket->_x = x;
	//pPacket->_y = y;

	(*sPacket) << id;
	(*sPacket) << dir;
	(*sPacket) << x;
	(*sPacket) << y;

	return;
}

void mpDamage(SerializePacket* sPacket, DWORD attackId, DWORD damageId, char damageHp)
{
	stPacketHeader header;
	header._code = 0x89;
	header._size = 9;
	header._type = PACKET_SC_DAMAGE;

	(*sPacket).Putdata((char*)&header, sizeof(stPacketHeader));

	//pPacket->_attackId = attackId;
	//pPacket->_damageId = damageId;
	//pPacket->_damageHp = damageHp;

	(*sPacket) << attackId;
	(*sPacket) << damageId;
	(*sPacket) << damageHp;

	return;
}
