#pragma once
#include <Pdh.h>
// 모니터링 항목
class CCpuUsage;

enum class MonitorType : int
{
	PacketPool_FULL,		// packet 메모리풀_full					O
	PacketPool_EMPTY,		// packet 메모리풀_empty					O

	PacketPool2_FULL,		// 
	PacketPool2_EMPTY,		// 

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


	TotalSendPacketCount,

	COUNT,
};

struct MonitoringTLS
{
	alignas(64) LONG counter[2][(int)MonitorType::COUNT];
	alignas(64) LONG activeIdx; // 0 or 1
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
	void CollectPerSecond();
	void Clear();

	LONG GetInterlocked(MonitorType type);

	void SetAuthFPS(int fps);
	void SetEchoFPS(int fps);

public:
	void UpdatePDHnCpuUsage();

	int GetCpuUsage();
	int GetPrivateBytes();

public:
	CCpuUsage* _cpu;

	// PDH 쿼리 핸들 생성
	PDH_HQUERY _cpuQuery;

	// PDH 리소스 카운터 생성 (여러개 수집시 이를 여러개 생성)
	PDH_HCOUNTER _privateBytes;

private:
	void InitTls();
	mutex						_counterArrLock;
	vector<MonitoringTLS*>		_counterArr;

public:
	LONG						_globalCounter[(int)MonitorType::COUNT];
};

