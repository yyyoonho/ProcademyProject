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
	LONG _monitoringArr[(int)MonitorType::COUNT];
	
};

