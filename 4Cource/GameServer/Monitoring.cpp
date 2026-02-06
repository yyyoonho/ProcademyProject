#include "stdafx.h"
#include <stdio.h>
#include <iomanip>
#include <Pdh.h>
#include <psapi.h>
#include <tchar.h>

#pragma comment(lib,"Pdh.lib")
#pragma comment(lib, "psapi.lib")

#include "CpuUsage.h"
#include "Monitoring.h"

Monitoring* Monitoring::_pMonitoring = NULL;
mutex initLock;

thread_local MonitoringTLS* _tlsCount;

Monitoring::Monitoring()
{
	for (int i = 0; i < (int)MonitorType::COUNT; i++)
	{
		_globalCounter[i] = 0;
	}

	_cpu = new CCpuUsage();

	PdhOpenQuery(NULL, NULL, &_cpuQuery);

	WCHAR processName[MAX_PATH] = { 0 };
	GetModuleBaseName(GetCurrentProcess(), NULL, processName, MAX_PATH);

	// ".exe" 확장자 제거
	std::wstring baseName(processName);
	size_t pos = baseName.find(L".exe");
	if (pos != std::wstring::npos)
		baseName = baseName.substr(0, pos);

	// PDH 카운터 경로 생성
	std::wstring counterPath = L"\\Process(" + baseName + L")\\Private Bytes";

	PdhAddCounter(_cpuQuery, counterPath.c_str(), NULL, &_privateBytes);

	PdhCollectQueryData(_cpuQuery);
}

Monitoring::~Monitoring()
{
	delete _cpu;
}

Monitoring* Monitoring::GetInstance()
{
	if (_pMonitoring == NULL)
	{
		lock_guard<mutex> lock(initLock);
		if (_pMonitoring == NULL)
		{
			_pMonitoring = new Monitoring;
		}
	}

	return _pMonitoring;
}

void Monitoring::Increase(MonitorType type)
{
	InitTls();

	int idx = _tlsCount->activeIdx;
	++_tlsCount->counter[idx][(int)type];
}

LONG Monitoring::IncreaseInterlocked(MonitorType type)
{
	return InterlockedIncrement(&_globalCounter[(int)type]);
}

void Monitoring::Decrease(MonitorType type)
{
	InitTls();

	int idx = _tlsCount->activeIdx;
	--_tlsCount->counter[idx][(int)type];
}

void Monitoring::DecreaseInterlocked(MonitorType type)
{
	InterlockedDecrement(&_globalCounter[(int)type]);
}

void Monitoring::CollectPerSecond()
{
	vector<MonitoringTLS*> snapshot;
	{
		lock_guard<mutex> lock(_counterArrLock);
		snapshot = _counterArr;
	}

	for (MonitoringTLS* tls : snapshot)
	{
		if (tls == nullptr) 
			continue;

		// old = 기존 activeIdx, new = 1-old
		LONG oldIdx = tls->activeIdx;
		LONG newIdx = 1 - oldIdx;

		// activeIdx 변경 (이 순간 이후 쓰레드들은 newIdx로만 증가)
		tls->activeIdx = newIdx;


		LONG* oldBuf = tls->counter[oldIdx];

		for (int i = 0; i < (int)MonitorType::COUNT; i++)
		{
			LONG v = oldBuf[i];
			if (v == 0)
				continue;

			_globalCounter[i] += v;
			oldBuf[i] = 0;
		}
	}
}

void Monitoring::Clear()
{
	// Reset
	InterlockedExchange(&_globalCounter[(int)MonitorType::AcceptTPS], 0);

	InterlockedExchange(&_globalCounter[(int)MonitorType::RecvMessageTPS], 0);
	InterlockedExchange(&_globalCounter[(int)MonitorType::SendMessageTPS], 0);

	InterlockedExchange(&_globalCounter[(int)MonitorType::RecvLoginTPS], 0);
	InterlockedExchange(&_globalCounter[(int)MonitorType::RecvEchoTPS], 0);
	InterlockedExchange(&_globalCounter[(int)MonitorType::RecvHeartbeatTPS], 0);

	InterlockedExchange(&_globalCounter[(int)MonitorType::TotalSendPacketCount], 0);

	_globalCounter[(int)MonitorType::PacketPool_FULL] =
		procademy::MemoryPool_TLS<Net_SerializePacket>::fullChunkStackCount;

	_globalCounter[(int)MonitorType::PacketPool_EMPTY] =
		procademy::MemoryPool_TLS<Net_SerializePacket>::emptyChunkStackCount;
}

LONG Monitoring::GetInterlocked(MonitorType type)
{
	LONG ret = InterlockedAdd(&_globalCounter[(int)type], 0);

	return ret;
}

void Monitoring::SetAuthFPS(int fps)
{
	_globalCounter[(int)MonitorType::AuthFieldFrame] = fps;
}

void Monitoring::SetEchoFPS(int fps)
{
	_globalCounter[(int)MonitorType::EchoFieldFrame] = fps;
}

void Monitoring::UpdatePDHnCpuUsage()
{
	_cpu->UpdateCpuTime();
	PdhCollectQueryData(_cpuQuery);
}

int Monitoring::GetCpuUsage()
{
	//float a = _cpu->ProcessUser();
	float a = _cpu->ProcessTotal();
	return (int)a;
}

int Monitoring::GetPrivateBytes()
{
	// 갱신 데이터 얻음
	PDH_FMT_COUNTERVALUE counterVal;
	PdhGetFormattedCounterValue(_privateBytes, PDH_FMT_DOUBLE, NULL, &counterVal);

	int mBytes = (int)(counterVal.doubleValue / (1024 * 1024));

	return mBytes;
}

void Monitoring::InitTls()
{
	if (_tlsCount == NULL)
	{
		_tlsCount = new MonitoringTLS;
		_tlsCount->activeIdx = 0;

		for (int i = 0; i < (int)MonitorType::COUNT; i++)
		{
			_tlsCount->counter[0][i] = 0;
			_tlsCount->counter[1][i] = 0;
		}

		lock_guard<mutex> lock(_counterArrLock);
		_counterArr.push_back(_tlsCount);
	}
}

void Monitoring::PrintMonitoring()
{
	constexpr int NAME_WIDTH = sizeof("Disconnect From Server:") - 1;

	cout << "===============================================================================\n";

	cout << right << setw(NAME_WIDTH) << "PacketPool_fullChunk:" << " "
		<< _globalCounter[(int)MonitorType::PacketPool_FULL] << "\n";
	cout << right << setw(NAME_WIDTH) << "PacketPool_emptyChunk:" << " "
		<< _globalCounter[(int)MonitorType::PacketPool_EMPTY] << "\n\n";

	cout << right << setw(NAME_WIDTH) << "PacketUseSize:" << " "
		<< _globalCounter[(int)MonitorType::PacketUseCount] << "\n";

	cout << "-------------------------------------------------------------------------------\n\n";

	cout << right << setw(NAME_WIDTH) << "SessionNum:" << " "
		<< _globalCounter[(int)MonitorType::SessionNum] << "\n";
	cout << right << setw(NAME_WIDTH) << "AuthPlayerCount:" << " "
		<< _globalCounter[(int)MonitorType::AuthPlayerCount] << "\n";
	cout << right << setw(NAME_WIDTH) << "GamePlayerCount:" << " "
		<< _globalCounter[(int)MonitorType::GamePlayerCount] << "\n\n";

	cout << right << setw(NAME_WIDTH) << "AcceptTotal:" << " "
		<< _globalCounter[(int)MonitorType::AcceptTotal] << "\n";
	cout << right << setw(NAME_WIDTH) << "AcceptTPS:" << " "
		<< _globalCounter[(int)MonitorType::AcceptTPS] << "\n\n";

	cout << right << setw(NAME_WIDTH) << "RecvMessageTPS:" << " "
		<< _globalCounter[(int)MonitorType::RecvMessageTPS] << "\n";
	cout << right << setw(NAME_WIDTH) << "SendMessageTPS:" << " "
		<< _globalCounter[(int)MonitorType::SendMessageTPS] << "\n\n";

	cout << right << setw(NAME_WIDTH) << "RecvLoginTPS:" << " "
		<< _globalCounter[(int)MonitorType::RecvLoginTPS] << "\n";
	cout << right << setw(NAME_WIDTH) << "RecvEchoTPS:" << " "
		<< _globalCounter[(int)MonitorType::RecvEchoTPS] << "\n";
	cout << right << setw(NAME_WIDTH) << "RecvHeartbeatTPS:" << " "
		<< _globalCounter[(int)MonitorType::RecvHeartbeatTPS] << "\n\n";

	cout << right << setw(NAME_WIDTH) << "AuthFieldFrame:" << " "
		<< _globalCounter[(int)MonitorType::AuthFieldFrame] << "\n";
	cout << right << setw(NAME_WIDTH) << "EchoFieldFrame:" << " "
		<< _globalCounter[(int)MonitorType::EchoFieldFrame] << "\n\n";


	cout << "-------------------------------------------------------------------------------\n";

	cout << right << setw(NAME_WIDTH) << "SendJobQ Size:" << " "
		<< _globalCounter[(int)MonitorType::SendJobQ] << "\n";
	cout << right << setw(NAME_WIDTH) << "ActiveWorkerTh Count:" << " "
		<< _globalCounter[(int)MonitorType::ActiveWorkerTh] << "\n\n";

	cout << right << setw(NAME_WIDTH) << "TotalSendPacketCount:" << " "
		<< _globalCounter[(int)MonitorType::TotalSendPacketCount] << "\n";

	cout << "===============================================================================\n\n";

}
