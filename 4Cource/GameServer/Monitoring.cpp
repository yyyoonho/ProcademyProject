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

Monitoring::Monitoring()
{
	for (int i = 0; i < (int)MonitorType::COUNT; i++)
	{
		_monitoringArr[i] = 0;
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
		_pMonitoring = new Monitoring;
	}

	return _pMonitoring;
}

void Monitoring::Increase(MonitorType type)
{
	_monitoringArr[(int)type]++;
}

LONG Monitoring::IncreaseInterlocked(MonitorType type)
{
	return InterlockedIncrement(&_monitoringArr[(int)type]);
}

void Monitoring::Decrease(MonitorType type)
{
	_monitoringArr[(int)type]--;
}

void Monitoring::DecreaseInterlocked(MonitorType type)
{
	InterlockedDecrement(&_monitoringArr[(int)type]);
}

void Monitoring::Clear()
{
	InterlockedExchange(&_monitoringArr[(int)::MonitorType::AcceptTPS], 0);

	InterlockedExchange(&_monitoringArr[(int)::MonitorType::RecvMessageTPS], 0);
	InterlockedExchange(&_monitoringArr[(int)::MonitorType::SendMessageTPS], 0);

	InterlockedExchange(&_monitoringArr[(int)::MonitorType::RecvLoginTPS], 0);
	InterlockedExchange(&_monitoringArr[(int)::MonitorType::RecvEchoTPS], 0);
	InterlockedExchange(&_monitoringArr[(int)::MonitorType::RecvHeartbeatTPS], 0);

	_monitoringArr[(int)MonitorType::PacketPool_FULL] = procademy::MemoryPool_TLS<Net_SerializePacket>::fullChunkStackCount;
	_monitoringArr[(int)MonitorType::PacketPool_EMPTY] = procademy::MemoryPool_TLS<Net_SerializePacket>::emptyChunkStackCount;
}

LONG Monitoring::GetInterlocked(MonitorType type)
{
	LONG ret = InterlockedAdd(&_monitoringArr[(int)type], 0);

	return ret;
}

void Monitoring::SetAuthFPS(int fps)
{
	_monitoringArr[(int)MonitorType::AuthFieldFrame] = fps;
}

void Monitoring::SetEchoFPS(int fps)
{
	_monitoringArr[(int)MonitorType::EchoFieldFrame] = fps;
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

void Monitoring::PrintMonitoring()
{
	constexpr int NAME_WIDTH = sizeof("Disconnect From Server:") - 1;

	cout << "===============================================================================\n";

	cout << right << setw(NAME_WIDTH) << "PacketPool_fullChunk:" << " "
		<< _monitoringArr[(int)MonitorType::PacketPool_FULL] << "\n";
	cout << right << setw(NAME_WIDTH) << "PacketPool_emptyChunk:" << " "
		<< _monitoringArr[(int)MonitorType::PacketPool_EMPTY] << "\n\n";

	cout << right << setw(NAME_WIDTH) << "PacketUseSize:" << " "
		<< _monitoringArr[(int)MonitorType::PacketUseCount] << "\n";

	cout << "-------------------------------------------------------------------------------\n\n";

	cout << right << setw(NAME_WIDTH) << "SessionNum:" << " "
		<< _monitoringArr[(int)MonitorType::SessionNum] << "\n";
	cout << right << setw(NAME_WIDTH) << "AuthPlayerCount:" << " "
		<< _monitoringArr[(int)MonitorType::AuthPlayerCount] << "\n";
	cout << right << setw(NAME_WIDTH) << "GamePlayerCount:" << " "
		<< _monitoringArr[(int)MonitorType::GamePlayerCount] << "\n\n";

	cout << right << setw(NAME_WIDTH) << "AcceptTotal:" << " "
		<< _monitoringArr[(int)MonitorType::AcceptTotal] << "\n";
	cout << right << setw(NAME_WIDTH) << "AcceptTPS:" << " "
		<< _monitoringArr[(int)MonitorType::AcceptTPS] << "\n\n";

	cout << right << setw(NAME_WIDTH) << "RecvMessageTPS:" << " "
		<< _monitoringArr[(int)MonitorType::RecvMessageTPS] << "\n";
	cout << right << setw(NAME_WIDTH) << "SendMessageTPS:" << " "
		<< _monitoringArr[(int)MonitorType::SendMessageTPS] << "\n\n";

	cout << right << setw(NAME_WIDTH) << "RecvLoginTPS:" << " "
		<< _monitoringArr[(int)MonitorType::RecvLoginTPS] << "\n";
	cout << right << setw(NAME_WIDTH) << "RecvEchoTPS:" << " "
		<< _monitoringArr[(int)MonitorType::RecvEchoTPS] << "\n";
	cout << right << setw(NAME_WIDTH) << "RecvHeartbeatTPS:" << " "
		<< _monitoringArr[(int)MonitorType::RecvHeartbeatTPS] << "\n\n";

	cout << right << setw(NAME_WIDTH) << "AuthFieldFrame:" << " "
		<< _monitoringArr[(int)MonitorType::AuthFieldFrame] << "\n";
	cout << right << setw(NAME_WIDTH) << "EchoFieldFrame:" << " "
		<< _monitoringArr[(int)MonitorType::EchoFieldFrame] << "\n\n";

	//cout << "-------------------------------------------------------------------------------\n";
	//
	//cout << right << setw(NAME_WIDTH) << "Disconnect From Server:" << " "
	//	<< (_monitoringArr[(int)MonitorType::TokenFailed] +
	//		_monitoringArr[(int)MonitorType::DisconnectTotal_alreadyLogin]) << "\n";
	//cout << right << setw(NAME_WIDTH) << "Invalid Token:" << " "
	//	<< _monitoringArr[(int)MonitorType::TokenFailed] << "\n";
	//cout << right << setw(NAME_WIDTH) << "Duplicate Login:" << " "
	//	<< _monitoringArr[(int)MonitorType::DisconnectTotal_alreadyLogin] << "\n";
	//
	cout << "-------------------------------------------------------------------------------\n";

	cout << right << setw(NAME_WIDTH) << "SendJobQ Size:" << " "
		<< _monitoringArr[(int)MonitorType::SendJobQ] << "\n";

	cout << "===============================================================================\n\n";

}
