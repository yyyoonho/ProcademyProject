#include "stdafx.h"

#include <iomanip>
#include "Monitoring.h"

Monitoring* Monitoring::_pMonitoring = NULL;

Monitoring::Monitoring()
{
	for (int i = 0; i < (int)MonitorType::COUNT; i++)
	{
		_monitoringArr[i] = 0;
	}
}

Monitoring::~Monitoring()
{
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

void Monitoring::IncreaseInterlocked(MonitorType type)
{
	InterlockedIncrement(&_monitoringArr[(int)type]);
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
	InterlockedExchange(&_monitoringArr[(int)::MonitorType::UpdateTPS], 0);
	InterlockedExchange(&_monitoringArr[(int)::MonitorType::RecvMessageTPS], 0);
	InterlockedExchange(&_monitoringArr[(int)::MonitorType::SendMessageTPS], 0);

	_monitoringArr[(int)MonitorType::PacketPool_FULL] = procademy::MemoryPool_TLS<Net_SerializePacket>::fullChunkStackCount;
	_monitoringArr[(int)MonitorType::PacketPool_EMPTY] = procademy::MemoryPool_TLS<Net_SerializePacket>::emptyChunkStackCount;

	_monitoringArr[(int)MonitorType::RCBPool_FULL] = procademy::MemoryPool_TLS<RefCountBlock>::fullChunkStackCount;
	_monitoringArr[(int)MonitorType::RCBPool_EMPTY] = procademy::MemoryPool_TLS<RefCountBlock>::emptyChunkStackCount;

	_monitoringArr[(int)MonitorType::lockfreeQ_FULL] = LockFreeQueue<RawPtr>::mp.fullChunkStackCount;
	_monitoringArr[(int)MonitorType::lockfreeQ_EMPTY] = LockFreeQueue<RawPtr>::mp.emptyChunkStackCount;
}

void Monitoring::PrintMonitoring()
{
	const int NAME_WIDTH = 24;

	cout << "<로그인 서버>=====================================================================\n";

	cout << right << setw(NAME_WIDTH) << "PacketPool_fullChunk:"
		<< " " << _monitoringArr[(int)MonitorType::PacketPool_FULL] << "\n";
	cout << right << setw(NAME_WIDTH) << "PacketPool_emptyChunk:"
		<< " " << _monitoringArr[(int)MonitorType::PacketPool_EMPTY] << "\n";

	cout << "\n";

	cout << right << setw(NAME_WIDTH) << "RCBPool_fullChunk:"
		<< " " << _monitoringArr[(int)MonitorType::RCBPool_FULL] << "\n";
	cout << right << setw(NAME_WIDTH) << "RCBPool_emptyChunk:"
		<< " " << _monitoringArr[(int)MonitorType::RCBPool_EMPTY] << "\n";

	cout << "\n";

	cout << right << setw(NAME_WIDTH) << "LockFreeQ_fullChunk:"
		<< " " << _monitoringArr[(int)MonitorType::lockfreeQ_FULL] << "\n";
	cout << right << setw(NAME_WIDTH) << "LockFreeQ_emptyChunk:"
		<< " " << _monitoringArr[(int)MonitorType::lockfreeQ_EMPTY] << "\n";

	cout << "\n";

	cout << right << setw(NAME_WIDTH) << "PacketUseSize:"
		<< " " << _monitoringArr[(int)MonitorType::PacketUseCount] << "\n";
	cout << "-------------------------------------------------------------------------------\n\n";
	cout << right << setw(NAME_WIDTH) << "SessionNum:"
		<< " " << _monitoringArr[(int)MonitorType::SessionNum] << "\n";
	cout << right << setw(NAME_WIDTH) << "PlayerCount:"
		<< " " << _monitoringArr[(int)MonitorType::PlayerCount] << "\n";
	cout << "\n";
	cout << right << setw(NAME_WIDTH) << "AcceptTotal:"
		<< " " << _monitoringArr[(int)MonitorType::AcceptTotal] << "\n";
	cout << right << setw(NAME_WIDTH) << "AcceptTPS:"
		<< " " << _monitoringArr[(int)MonitorType::AcceptTPS] << "\n";
	cout << right << setw(NAME_WIDTH) << "UpdateTPS:"
		<< " " << _monitoringArr[(int)MonitorType::UpdateTPS] << "\n";

	cout << right << setw(NAME_WIDTH) << "RecvMessageTPS:"
		<< " " << _monitoringArr[(int)MonitorType::RecvMessageTPS] << "\n";
	cout << right << setw(NAME_WIDTH) << "SendMessageTPS:"
		<< " " << _monitoringArr[(int)MonitorType::SendMessageTPS] << "\n";

	cout << "===============================================================================\n";

}
