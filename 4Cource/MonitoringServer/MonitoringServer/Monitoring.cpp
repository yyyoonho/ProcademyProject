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

	InterlockedExchange(&_monitoringArr[(int)::MonitorType::RecvMessageLoginTPS], 0);
	InterlockedExchange(&_monitoringArr[(int)::MonitorType::RecvMessageMoveTPS], 0);
	InterlockedExchange(&_monitoringArr[(int)::MonitorType::RecvMessageChatTPS], 0);

	_monitoringArr[(int)MonitorType::PacketPool_FULL] = procademy::MemoryPool_TLS<Net_SerializePacket>::fullChunkStackCount;
	_monitoringArr[(int)MonitorType::PacketPool_EMPTY] = procademy::MemoryPool_TLS<Net_SerializePacket>::emptyChunkStackCount;

	_monitoringArr[(int)MonitorType::RCBPool_FULL] = procademy::MemoryPool_TLS<RefCountBlock>::fullChunkStackCount;	
	_monitoringArr[(int)MonitorType::RCBPool_EMPTY] = procademy::MemoryPool_TLS<RefCountBlock>::emptyChunkStackCount;

	_monitoringArr[(int)MonitorType::lockfreeQ_FULL] = LockFreeQueue<RawPtr>::mp.fullChunkStackCount;
	_monitoringArr[(int)MonitorType::lockfreeQ_EMPTY] = LockFreeQueue<RawPtr>::mp.emptyChunkStackCount;	
}

void Monitoring::PrintMonitoring()
{
	constexpr int NAME_WIDTH = sizeof("Disconnect From Server:") - 1;

	cout << "===============================================================================\n";

	cout << right << setw(NAME_WIDTH) << "PacketPool_fullChunk:" << " "
		<< _monitoringArr[(int)MonitorType::PacketPool_FULL] << "\n";
	cout << right << setw(NAME_WIDTH) << "PacketPool_emptyChunk:" << " "
		<< _monitoringArr[(int)MonitorType::PacketPool_EMPTY] << "\n\n";

	cout << right << setw(NAME_WIDTH) << "RCBPool_fullChunk:" << " "
		<< _monitoringArr[(int)MonitorType::RCBPool_FULL] << "\n";
	cout << right << setw(NAME_WIDTH) << "RCBPool_emptyChunk:" << " "
		<< _monitoringArr[(int)MonitorType::RCBPool_EMPTY] << "\n\n";

	cout << right << setw(NAME_WIDTH) << "LockFreeQ_fullChunk:" << " "
		<< _monitoringArr[(int)MonitorType::lockfreeQ_FULL] << "\n";
	cout << right << setw(NAME_WIDTH) << "LockFreeQ_emptyChunk:" << " "
		<< _monitoringArr[(int)MonitorType::lockfreeQ_EMPTY] << "\n\n";

	cout << right << setw(NAME_WIDTH) << "PacketUseSize:" << " "
		<< _monitoringArr[(int)MonitorType::PacketUseCount] << "\n";

	cout << "-------------------------------------------------------------------------------\n\n";

	cout << right << setw(NAME_WIDTH) << "SessionNum:" << " "
		<< _monitoringArr[(int)MonitorType::SessionNum] << "\n";
	cout << right << setw(NAME_WIDTH) << "PlayerCount:" << " "
		<< _monitoringArr[(int)MonitorType::PlayerCount] << "\n\n";

	cout << right << setw(NAME_WIDTH) << "AcceptTotal:" << " "
		<< _monitoringArr[(int)MonitorType::AcceptTotal] << "\n";
	cout << right << setw(NAME_WIDTH) << "AcceptTPS:" << " "
		<< _monitoringArr[(int)MonitorType::AcceptTPS] << "\n";
	cout << right << setw(NAME_WIDTH) << "UpdateTPS:" << " "
		<< _monitoringArr[(int)MonitorType::UpdateTPS] << "\n\n";

	cout << right << setw(NAME_WIDTH) << "RecvMessageTPS:" << " "
		<< _monitoringArr[(int)MonitorType::RecvMessageTPS] << "\n";
	cout << right << setw(NAME_WIDTH) << "SendMessageTPS:" << " "
		<< _monitoringArr[(int)MonitorType::SendMessageTPS] << "\n\n";

	cout << right << setw(NAME_WIDTH) << "RecvMessageLogin:" << " "
		<< _monitoringArr[(int)MonitorType::RecvMessageLoginTPS] << "\n";
	cout << right << setw(NAME_WIDTH) << "RecvMessageMove:" << " "
		<< _monitoringArr[(int)MonitorType::RecvMessageMoveTPS] << "\n";
	cout << right << setw(NAME_WIDTH) << "RecvMessageChat:" << " "
		<< _monitoringArr[(int)MonitorType::RecvMessageChatTPS] << "\n\n";

	cout << "-------------------------------------------------------------------------------\n";

	cout << right << setw(NAME_WIDTH) << "tmpPlayerArrSize:" << " "
		<< _monitoringArr[(int)MonitorType::tmpPlayerArrSize] << "\n";
	cout << right << setw(NAME_WIDTH) << "PlayerArrSize:" << " "
		<< _monitoringArr[(int)MonitorType::PlayerArrSize] << "\n";

	cout << "-------------------------------------------------------------------------------\n";

	cout << right << setw(NAME_WIDTH) << "Disconnect From Server:" << " "
		<< (_monitoringArr[(int)MonitorType::TokenFailed] +
			_monitoringArr[(int)MonitorType::DisconnectTotal_alreadyLogin]) << "\n";
	cout << right << setw(NAME_WIDTH) << "Invalid Token:" << " "
		<< _monitoringArr[(int)MonitorType::TokenFailed] << "\n";
	cout << right << setw(NAME_WIDTH) << "Duplicate Login:" << " "
		<< _monitoringArr[(int)MonitorType::DisconnectTotal_alreadyLogin] << "\n";

	cout << "===============================================================================\n\n";

}
