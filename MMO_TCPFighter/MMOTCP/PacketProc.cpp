#include <Windows.h>

#include "Protocol.h"
#include "SerializeBuffer.h"
#include "Network.h"

#include "PacketProc.h"

using namespace std;

bool netPacketProc_MoveStart(stSession* pSession, SerializePacket* sPacket);
bool netPacketProc_MoveStop(stSession* pSession, SerializePacket* sPacket);
bool netPacketProc_Attack1(stSession* pSession, SerializePacket* sPacket);
bool netPacketProc_Attack2(stSession* pSession, SerializePacket* sPacket);
bool netPacketProc_Attack3(stSession* pSession, SerializePacket* sPacket);
bool netPacketProc_Echo(stSession* pSession, SerializePacket* sPacket);

bool PacketProc(stSession* pSession, BYTE byPacketType, SerializePacket* sPacket)
{
	switch (byPacketType)
	{
	case dfPACKET_CS_MOVE_START:
		return netPacketProc_MoveStart(pSession, sPacket);
		break;

	case dfPACKET_CS_MOVE_STOP:
		return netPacketProc_MoveStop(pSession, sPacket);
		break;

	case dfPACKET_CS_ATTACK1:
		return netPacketProc_Attack1(pSession, sPacket);
		break;

	case dfPACKET_CS_ATTACK2:
		return netPacketProc_Attack2(pSession, sPacket);
		break;

	case dfPACKET_CS_ATTACK3:
		return netPacketProc_Attack3(pSession, sPacket);
		break;

	case dfPACKET_CS_ECHO:
		return netPacketProc_Echo(pSession, sPacket);
		break;
	}

	return true;
}

bool netPacketProc_MoveStart(stSession* pSession, SerializePacket* sPacket)
{
	return false;
}

bool netPacketProc_MoveStop(stSession* pSession, SerializePacket* sPacket)
{
	return false;
}

bool netPacketProc_Attack1(stSession* pSession, SerializePacket* sPacket)
{
	return false;
}

bool netPacketProc_Attack2(stSession* pSession, SerializePacket* sPacket)
{
	return false;
}

bool netPacketProc_Attack3(stSession* pSession, SerializePacket* sPacket)
{
	return false;
}

bool netPacketProc_Echo(stSession* pSession, SerializePacket* sPacket)
{
	return false;
}