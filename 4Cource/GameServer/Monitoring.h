#pragma once
#include <Pdh.h>
// 모니터링 항목
class CCpuUsage;

enum class MonitorType : int
{
	PacketPool_FULL,		// packet 메모리풀_full					O
	PacketPool_EMPTY,		// packet 메모리풀_empty					O

	PacketUseCount,			//										O
		
	SessionNum,				// NetServer 의 세션수					O	

	AuthPlayerCount,		//										O
	GamePlayerCount,		//										O

	AcceptTotal,			// Accept 전체 카운트 (accept 리턴시 +1)	O
	AcceptTPS,				// Accept 처리 횟수						O	

	RecvMessageTPS,			// 초당 받은 메시지 수					O	
	SendMessageTPS,			// 초당 보낸 메시지 수					O

	RecvLoginTPS,			// 초당 받은 메시지 수 (login)			O
	RecvEchoTPS,			// 초당 받은 메시지 수 (echo)				O
	RecvHeartbeatTPS,		// 초당 받은 메시지 수 (heartbeat)		O

	AuthFieldFrame,			//										O
	EchoFieldFrame,			//										O

	SendJobQ,				//										O

	ActiveWorkerTh,

	SendPacketQ_0,
	SendPacketQ_1,
	SendPacketQ_2,
	SendPacketQ_3,
	SendPacketQ_4,

	TotalSendPacketCount,

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
	LONG IncreaseInterlocked(MonitorType type);
	void Decrease(MonitorType type);
	void DecreaseInterlocked(MonitorType type);
	void PrintMonitoring();
	void Clear();

	LONG GetInterlocked(MonitorType type);

	void SetAuthFPS(int fps);
	void SetEchoFPS(int fps);

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

