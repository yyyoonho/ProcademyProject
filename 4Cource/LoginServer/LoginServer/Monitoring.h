#pragma once
// 모니터링 항목

#include <Pdh.h>
// 모니터링 항목
class CCpuUsage;

enum class MonitorType : int
{
	PacketPool_FULL,		// packet 메모리풀_full					()	-> 초기화 x
	PacketPool_EMPTY,		// packet 메모리풀_empty					()	-> 초기화 x

	RCBPool_FULL,			// RCB 메모리풀_full						()	-> 초기화 x	
	RCBPool_EMPTY,			// RCB 메모리풀_empty					()	-> 초기화 x

	lockfreeQ_FULL,			// 락프리큐 노드 메모리풀_full			()	-> 초기화 x
	lockfreeQ_EMPTY,		// 락프리큐 노드 메모리풀_empty			()	-> 초기화 x

	PacketUseCount,

	SessionNum,				// NetServer 의 세션수					(O) -> 초기화 x
	PlayerCount,			// Contents 파트 Player 수				(O)	-> 초기화 x

	AcceptTotal,			// Accept 전체 카운트 (accept 리턴시 +1)	(O)	-> 초기화 x
	AcceptTPS,				// Accept 처리 횟수						(O)
	UpdateTPS,				// Update 처리 초당 횟수					(O)

	RecvMessageTPS,			// 초당 받은 메시지 수					(O)
	SendMessageTPS,			// 초당 보낸 메시지 수					(O)

	HB_Kick,				// 하트비트 초과로 연결끊음
	DuplicateLogin,				// 하트비트 초과로 연결끊음

	SendJobQ,

	AuthTPS,

	COUNT,
};

class Monitoring
{
private:
	static Monitoring* _pMonitoring;

private:
	Monitoring();
	~Monitoring();

public:
	static Monitoring* GetInstance();
	void Increase(MonitorType type);
	void IncreaseInterlocked(MonitorType type);
	void Decrease(MonitorType type);
	void DecreaseInterlocked(MonitorType type);
	void PrintMonitoring();
	void Clear();

public:
	void UpdatePDHnCpuUsage();

	int GetCpuUsage();
	int GetPrivateBytes();


public:
	LONG _monitoringArr[(int)MonitorType::COUNT];
	CCpuUsage* _cpu;

	// PDH 쿼리 핸들 생성
	PDH_HQUERY _cpuQuery;

	// PDH 리소스 카운터 생성 (여러개 수집시 이를 여러개 생성)
	PDH_HCOUNTER _privateBytes;
};