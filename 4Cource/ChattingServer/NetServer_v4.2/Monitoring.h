#pragma once
// 모니터링 항목
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

	MSGQueueSize,

	AcceptTotal,			// Accept 전체 카운트 (accept 리턴시 +1)	(O)	-> 초기화 x
	AcceptTPS,				// Accept 처리 횟수						(O)
	UpdateTPS,				// Update 처리 초당 횟수					(O)

	RecvMessageTPS,			// 초당 받은 메시지 수					(O)
	SendMessageTPS,			// 초당 보낸 메시지 수					(O)

	RecvMessageLoginTPS,	// 초당 받은 메시지 수 (login)			(O)
	RecvMessageMoveTPS,		// 초당 받은 메시지 수 (move)				(O)
	RecvMessageChatTPS,		// 초당 받은 메시지 수 (chat)				(O)

	WaitDuration,			// 초당 블락되어있는 ms					(O)
	WorkDuration,			// 초당 작동되고있는 ms					(O)

	DuplicatedDisconnect_new,		// 중복로그인으로 인한 disconnect			
	DuplicatedDisconnect_old,		// 중복로그인으로 인한 disconnect			

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

	void AddWaitTime(DWORD waitTime);
	void AddWorkTime(DWORD workTime);

	void PrintMonitoring();
	void Clear();

private:
	LONG _monitoringArr[(int)MonitorType::COUNT];
	
};

