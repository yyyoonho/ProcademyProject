#pragma once
// 모니터링 항목
enum class MonitorType : int
{
	PacketPool_FULL,		// Packet 풀 할당량 (아직)				()	-> 초기화 x
	RCBPool_FULL,			// RCB 구조체 할당량 (아직)				()	-> 초기화 x
	PacketPool_EMPTY,		// Packet 풀 할당량 (아직)				()	-> 초기화 x
	RCBPool_EMPTY,			// RCB 구조체 할당량 (아직)				()	-> 초기화 x

	PacketUseCount,

	SessionNum,				// NetServer 의 세션수					(O) -> 초기화 x

	AcceptTotal,			// Accept 전체 카운트 (accept 리턴시 +1)	(O)	-> 초기화 x
	AcceptTPS,				// Accept 처리 횟수						(O)
	UpdateTPS,				// Update 처리 초당 횟수					(O)

	RecvMessageTPS,			// 초당 받은 메시지 수					(O)
	SendMessageTPS,			// 초당 보낸 메시지 수					(O)

	DisconnectTotal_alreadyLogin,

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

private:
	LONG _monitoringArr[(int)MonitorType::COUNT];
	
};

