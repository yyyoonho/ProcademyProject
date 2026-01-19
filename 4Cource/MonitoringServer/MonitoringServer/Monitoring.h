#pragma once
// 모니터링 항목
enum class MonitorType : int
{
	ConnectedMonitoringToolCount,

	ConnectedLoginServerCount,
	ConnectedChatServerCount,
	ConnectedGameServerCount,

	AcceptTotal,
	RecvMessageTPS,
	SendMessageTPS,

	SessionNum,

	SendJobQ,

	PacketUseCount,

	LoginProcessingCount,

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

public:
	LONG _monitoringArr[(int)MonitorType::COUNT];
	
	// TODO: 서버 전체 모니터링 
	// (CPU토탈/ 논페이지 메모리/ 네트워크 수신량/ 네트워크 송신량/ 사용가능 메모리)
};

