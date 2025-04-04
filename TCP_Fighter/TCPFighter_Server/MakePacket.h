#pragma once

// 헤더와 패킷을 만드는 함수
void mpCreateMyCharacter(stPacketHeader* pHeader, stPACKET_SC_CREATE_MY_CHARACTER* pPacket, DWORD id, DWORD dir, short x, short y);
void mpCreateOtherCharacter(stPacketHeader* pHeader, stPACKET_SC_CREATE_OTHER_CHARACTER* pPacket, DWORD id, DWORD dir, short x, short y, char hp);
void mpDeleteCharacter(stPacketHeader* pHeader, stPACKET_SC_DELETE_CHARACTER* pPacket, DWORD id);

void mpMoveStart(stPacketHeader* pHeader, stPACKET_SC_MOVE_START* pPacket, DWORD id, DWORD dir, short x, short y);
void mpMoveStop(stPacketHeader* pHeader, stPACKET_SC_MOVE_STOP* pPacket, DWORD id, DWORD dir, short x, short y);

void mpAttack1(stPacketHeader* pHeader, stPACKET_SC_ATTACK1* pPacket, DWORD id, DWORD dir, short x, short y);
void mpAttack2(stPacketHeader* pHeader, stPACKET_SC_ATTACK2* pPacket, DWORD id, DWORD dir, short x, short y);
void mpAttack3(stPacketHeader* pHeader, stPACKET_SC_ATTACK3* pPacket, DWORD id, DWORD dir, short x, short y);

void mpDamage(stPacketHeader* pHeader, stPACKET_SC_DAMAGE* pPacket, DWORD attackId, DWORD damageId, char damageHp);