#pragma once

// 특정 섹터 1개에 있는 클라이언트들에게 메시지 보내기
// 모든 캐릭터 이동 시에 섹터의 변경 여부를 확인하여 이대 대한 처리
void SendPacket_SectorOne(int iSectorY, int isectorX, SerializePacket* sPacket, stSession* pExceptSession);

// 특정 1명의 클라이언트에게 메시지 보내기
// 하트비트, echo, create(To me)
void SendPacket_Unicast(stSession* pTargetSession, SerializePacket* sPacket);

// 클라이언트 기준 주변 섹터에 메시지 보내기(최대 9개영역)
// attack, moveStart, moveStop, create(To Other)
void SendPacket_Around(stSession* pMySession, SerializePacket* sPacket, bool bSendMe = false);

// 진정 브로드 캐스팅 (시스템적인 메시지 외에는 사용하지 않음)
void SendPacket_Broadcast(stSession* pSession, SerializePacket* sPacket);