#include "stdafx.h"
#include "CommonProtocol.h"
#include "PacketProc.h"

using namespace std;

void PacketProc_Login(DWORD64 sessionID, SerializePacketPtr pPacket);
void PacketProc_SectorMove(DWORD64 sessionID, SerializePacketPtr pPacket);
void PacketProc_Message(DWORD64 sessionID, SerializePacketPtr pPacket);
void PacketProc_Heartbeat(DWORD64 sessionID, SerializePacketPtr pPacket);

void PacketProc(SerializePacketPtr pPacket)
{
	DWORD64 sessionID;
	pPacket.GetData((char*)&sessionID, sizeof(DWORD64));

	WORD msgType;
	pPacket >> msgType;

	// switch - case로 메시지별 처리 함수 찾아가기.
	switch (msgType)
	{
	en_PACKET_CS_CHAT_REQ_LOGIN:
		PacketProc_Login(sessionID, pPacket);
		break;

	en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
		PacketProc_SectorMove(sessionID, pPacket);
		break;

	en_PACKET_CS_CHAT_REQ_MESSAGE:
		PacketProc_Message(sessionID, pPacket);
		break;

	en_PACKET_CS_CHAT_REQ_HEARTBEAT:
		PacketProc_Heartbeat(sessionID, pPacket);
		break;
	}
}

