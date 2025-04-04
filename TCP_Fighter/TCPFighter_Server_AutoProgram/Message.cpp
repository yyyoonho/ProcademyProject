#include "Windows.h"
#include "SerializeBuffer.h"
#include "Message.h"
#include "PacketData.h"
 
void mpCreateMyCharacter(SerializePacket* sPacket, DWORD id, BYTE dir, short x, short y, char hp)
{
	stPacketHeader header;
	header._code = 0x89;
	header._size = 10;
	header._type = 0;
	(*sPacket).Putdata((char*)&header, sizeof(stPacketHeader));
	(*sPacket) <<  id;
	(*sPacket) <<  dir;
	(*sPacket) <<  x;
	(*sPacket) <<  y;
	(*sPacket) <<  hp;
}
 
void mpCreateOtherCharacter(SerializePacket* sPacket, DWORD id, BYTE dir, short x, short y, char hp)
{
	stPacketHeader header;
	header._code = 0x89;
	header._size = 10;
	header._type = 1;
	(*sPacket).Putdata((char*)&header, sizeof(stPacketHeader));
	(*sPacket) <<  id;
	(*sPacket) <<  dir;
	(*sPacket) <<  x;
	(*sPacket) <<  y;
	(*sPacket) <<  hp;
}
 
void mpDeleteCharacter(SerializePacket* pPacket, DWORD id)
{
	stPacketHeader header;
	header._code = 0x89;
	header._size = 4;
	header._type = 2;
	(*pPacket).Putdata((char*)&header, sizeof(stPacketHeader));
	(*pPacket) <<  id;
}
 
void mpMoveStart(SerializePacket* sPacket, DWORD id, BYTE dir, short x, short y)
{
	stPacketHeader header;
	header._code = 0x89;
	header._size = 9;
	header._type = 11;
	(*sPacket).Putdata((char*)&header, sizeof(stPacketHeader));
	(*sPacket) <<  id;
	(*sPacket) <<  dir;
	(*sPacket) <<  x;
	(*sPacket) <<  y;
}
 
void mpMoveStop(SerializePacket* sPacket, DWORD id, BYTE dir, short x, short y)
{
	stPacketHeader header;
	header._code = 0x89;
	header._size = 9;
	header._type = 13;
	(*sPacket).Putdata((char*)&header, sizeof(stPacketHeader));
	(*sPacket) <<  id;
	(*sPacket) <<  dir;
	(*sPacket) <<  x;
	(*sPacket) <<  y;
}
 
void mpAttack1(SerializePacket* sPacket, DWORD id, BYTE dir, short x, short y)
{
	stPacketHeader header;
	header._code = 0x89;
	header._size = 9;
	header._type = 21;
	(*sPacket).Putdata((char*)&header, sizeof(stPacketHeader));
	(*sPacket) <<  id;
	(*sPacket) <<  dir;
	(*sPacket) <<  x;
	(*sPacket) <<  y;
}
 
void mpAttack2(SerializePacket* sPacket, DWORD id, BYTE dir, short x, short y)
{
	stPacketHeader header;
	header._code = 0x89;
	header._size = 9;
	header._type = 23;
	(*sPacket).Putdata((char*)&header, sizeof(stPacketHeader));
	(*sPacket) <<  id;
	(*sPacket) <<  dir;
	(*sPacket) <<  x;
	(*sPacket) <<  y;
}
 
void mpAttack3(SerializePacket* sPacket, DWORD id, BYTE dir, short x, short y)
{
	stPacketHeader header;
	header._code = 0x89;
	header._size = 9;
	header._type = 25;
	(*sPacket).Putdata((char*)&header, sizeof(stPacketHeader));
	(*sPacket) <<  id;
	(*sPacket) <<  dir;
	(*sPacket) <<  x;
	(*sPacket) <<  y;
}
 
void mpDamage(SerializePacket* sPacket, DWORD attackId, DWORD damageId, char damageHp)
{
	stPacketHeader header;
	header._code = 0x89;
	header._size = 9;
	header._type = 30;
	(*sPacket).Putdata((char*)&header, sizeof(stPacketHeader));
	(*sPacket) <<  attackId;
	(*sPacket) <<  damageId;
	(*sPacket) <<  damageHp;
}
 
