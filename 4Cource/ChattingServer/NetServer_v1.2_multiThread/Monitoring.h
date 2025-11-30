#pragma once
// 모니터링 항목
enum class MonitorType : int
{
	SessionNum,				// NetServer 의 세션수					(O) -> 초기화 x
	PacketPool,				// Packet 풀 할당량 (아직)				()	-> 초기화 x

	UpdateMessageQueue,		// ContentThread 큐 남은개수				(O) -> 초기화 x

	PlayerDataPool,			// Player 구조체 할당량 (아직)			()	-> 초기화 x
	PlayerCount,			// Contents 파트 Player 수				(O)	-> 초기화 x

	AcceptTotal,			// Accept 전체 카운트 (accept 리턴시 +1)	(O)	-> 초기화 x
	AcceptTPS,				// Accept 처리 횟수						(O)
	UpdateTPS,				// Update 처리 초당 횟수					(O)

	RecvMessageTPS,			// 초당 받은 메시지 수					(O)
	SendMessageTPS,			// 초당 보낸 메시지 수					(O)

	RecvMessageLoginTPS,	// 초당 받은 메시지 수 (login)			(O)
	RecvMessageMoveTPS,		// 초당 받은 메시지 수 (move)				(O)
	RecvMessageChatTPS,		// 초당 받은 메시지 수 (chat)				(O)

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

