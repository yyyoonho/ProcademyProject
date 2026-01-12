#pragma once

#include <Pdh.h>
class CCpuUsage;
class MonitoringServer;

struct localClient
{
	// 데이터
	bool			activeData[en_PACKET_SS_MONITOR_DATA_UPDATE::COUNT];

	int				count[en_PACKET_SS_MONITOR_DATA_UPDATE::COUNT];
	int				avgData[en_PACKET_SS_MONITOR_DATA_UPDATE::COUNT];
	int				minData[en_PACKET_SS_MONITOR_DATA_UPDATE::COUNT];
	int				maxData[en_PACKET_SS_MONITOR_DATA_UPDATE::COUNT];

	void			ResetData(int i);
};


class LocalMonitoring
{
public:
	struct MonitoringSnapshot
	{
		int serverNo;
		int dataType;
		int avg;
		int min;
		int max;
	};

public:
	LocalMonitoring();
	~LocalMonitoring();

public:
	void Start();
	void Stop();
	void RegisterNetServer(MonitoringServer* pNetMonitoringServer);

private:
	HANDLE hEvent_Quit;
	HANDLE hThread_LocalMonitoring;
	HANDLE hThread_DB;

	static void ServerMonitoringRun(LPVOID* lParam);
	void ServerMonitoringThread();

	static void DBRun(LPVOID* lParam);
	void DBThread();

private:
	// 서버 모니터링 클라이언트
	int _serverNo = 7;

	mutex			_clientLock;
	localClient*	_localClient;

	// CPU 측정용
	CCpuUsage* _cpu;

	// PDH 쿼리 핸들 생성
	PDH_HQUERY _pdhQuery;

	PDH_HCOUNTER _nonpagedMemory;
	PDH_HCOUNTER _receivedPerSec;
	PDH_HCOUNTER _sentPerSec;
	PDH_HCOUNTER _availableMemory;

private:
	MonitoringServer* pNetMonitoringServer;
};

