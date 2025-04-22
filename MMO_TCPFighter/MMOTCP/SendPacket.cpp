#include <Windows.h>
#include <list>
#include <vector>

#include "RingBuffer.h"
#include "SerializeBuffer.h"
#include "Network.h"
#include "SectorManager.h"
#include "CharacterManager.h"

#include "SendPacket.h"

using namespace std;

void SendPacket_SectorOne(int iSectorY, int isectorX, SerializePacket* sPacket, stSession* pExceptSeesion)
{
	vector<stSession*> v;

	GetSessionsFromSector(iSectorY, isectorX, v);

	for (int j = 0; j < v.size(); j++)
	{
		if (pExceptSeesion == v[j] && pExceptSeesion != NULL)
			continue;

		v[j]->sendQ.Enqueue(sPacket->GetBufferPtr(), sPacket->GetDataSize());
	}

}

void SendPacket_Unicast(stSession* pTargetSession, SerializePacket* sPacket)
{
	pTargetSession->sendQ.Enqueue(sPacket->GetBufferPtr(), sPacket->GetDataSize());

	return;
}

void SendPacket_Around(stSession* pMySession, SerializePacket* sPacket, bool bSendMe)
{
	stSECTOR_POS curSectorPos;
	GetCurSectorPos(pMySession, &curSectorPos);

	stSECTOR_AROUND sectorAround;
	GetSectorAround(curSectorPos.iY, curSectorPos.iX, &sectorAround);

	for (int i = 0; i < sectorAround.iCount; i++) // 여기서 자꾸 icount가 0 이 나온다...
	{
		int sectorY = sectorAround.around[i].iY;
		int sectorX = sectorAround.around[i].iX;
		vector<stSession*> v;

		GetSessionsFromSector(sectorY, sectorX, v);

		for (int j = 0; j < v.size(); j++)
		{
			if (pMySession == v[j] && bSendMe == false)
				continue;

			v[j]->sendQ.Enqueue(sPacket->GetBufferPtr(), sPacket->GetDataSize());
		}

	}

	return;
}

void SendPacket_Broadcast(stSession* pSession, SerializePacket* sPacket)
{
	return;
}
