#include "stdafx.h"
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

	_monitoringArr[(int)MonitorType::PacketPool] = procademy::MemoryPool_TLS<Net_SerializePacket>::Get_Net_SerializePacket_fullChunkStackCount();
	_monitoringArr[(int)MonitorType::RCBPool] = procademy::MemoryPool_TLS<RefCountBlock>::Get_RefCountBlock_fullChunkStackCount();
}

void Monitoring::PrintMonitoring()
{
	cout << "===============================================================================\n";
	cout << "SessionNum: " << _monitoringArr[(int)MonitorType::SessionNum] << "\n";
	cout << "PacketPool: " << _monitoringArr[(int)MonitorType::PacketPool] << "\n";
	cout << "RCBPool: " << _monitoringArr[(int)MonitorType::RCBPool] << "\n";
	cout << "\n";
	cout << "PlayerCount: " << _monitoringArr[(int)MonitorType::PlayerCount] << "\n";
	cout << "\n";
	cout << "AcceptTotal: " << _monitoringArr[(int)MonitorType::AcceptTotal] << "\n";
	cout << "AcceptTPS: " << _monitoringArr[(int)MonitorType::AcceptTPS] << "\n";
	cout << "UpdateTPS: " << _monitoringArr[(int)MonitorType::UpdateTPS] << "\n";
	cout << "\n";
	cout << "RecvMessageTPS: " << _monitoringArr[(int)MonitorType::RecvMessageTPS] << "\n";
	cout << "SendMessageTPS: " << _monitoringArr[(int)MonitorType::SendMessageTPS] << "\n";
	cout << "\n";
	cout << "RecvMessageLogin: " << _monitoringArr[(int)MonitorType::RecvMessageLoginTPS] << "\n";
	cout << "RecvMessageMove: " << _monitoringArr[(int)MonitorType::RecvMessageMoveTPS] << "\n";
	cout << "RecvMessageChat: " << _monitoringArr[(int)MonitorType::RecvMessageChatTPS] << "\n";
	cout << "\n";
	cout << "Disconnect(중복로그인): " << _monitoringArr[(int)MonitorType::DisconnectTotal_alreadyLogin] << "\n";
	cout << "===============================================================================\n";
	cout << "\n";

}
