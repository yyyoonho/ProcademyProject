#pragma once

// 헤더와 패킷을 만드는 함수
void mpCreateMyCharacter(SerializePacket* sPacket, DWORD id, BYTE dir, short x, short y);
void mpCreateOtherCharacter(SerializePacket* sPacket, DWORD id, BYTE dir, short x, short y, char hp);
void mpDeleteCharacter(SerializePacket* sPacket, DWORD id);

void mpMoveStart(SerializePacket* sPacket, DWORD id, BYTE dir, short x, short y);
void mpMoveStop(SerializePacket* sPacket, DWORD id, BYTE dir, short x, short y);

void mpAttack1(SerializePacket* sPacket, DWORD id, BYTE dir, short x, short y);
void mpAttack2(SerializePacket* sPacket, DWORD id, BYTE dir, short x, short y);
void mpAttack3(SerializePacket* sPacket, DWORD id, BYTE dir, short x, short y);

void mpDamage(SerializePacket* sPacket, DWORD attackId, DWORD damageId, char damageHp);

void mpSync(SerializePacket* sPacket, DWORD id, short x, short y);

void mpEcho(SerializePacket* sPacket, DWORD time);