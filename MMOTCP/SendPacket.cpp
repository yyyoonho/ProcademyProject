#include "stdafx.h"

#include "Network.h"
#include "SectorManager.h"
#include "CharacterManager.h"

#include "SendPacket.h"

using namespace std;

void SendPacket_SectorOne(int iSectorY, int isectorX, SerializePacket* sPacket, stSession* pExceptSeesion)
{
}

void SendPacket_Unicast(stSession* pTargetSession, SerializePacket* sPacket)
{
	pTargetSession->sendQ.Enqueue(sPacket->GetBufferPtr(), sPacket->GetDataSize());
}

void SendPacket_Around(stSession* pMySession, SerializePacket* sPacket, bool bSendMe)
{
}

void SendPacket_Broadcast(stSession* pSession, SerializePacket* sPacket)
{
	return;
}
