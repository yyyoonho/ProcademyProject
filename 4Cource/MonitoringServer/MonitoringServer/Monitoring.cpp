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
	InterlockedExchange(&_monitoringArr[(int)MonitorType::RecvMessageTPS], 0);
	InterlockedExchange(&_monitoringArr[(int)MonitorType::SendMessageTPS], 0);
}

void Monitoring::PrintMonitoring()
{
	constexpr int NAME_WIDTH = sizeof("Disconnect From Server:") - 1;

	cout << "===============================================================================\n";

	cout << right << setw(NAME_WIDTH) << "ConnectedMonitoringToolCount:" << " "
		<< _monitoringArr[(int)MonitorType::ConnectedMonitoringToolCount] << "\n\n";

	cout << right << setw(NAME_WIDTH) << "ConnectedLoginServerCount:" << " "
		<< _monitoringArr[(int)MonitorType::ConnectedLoginServerCount] << "\n";
	cout << right << setw(NAME_WIDTH) << "ConnectedChatServerCount:" << " "
		<< _monitoringArr[(int)MonitorType::ConnectedChatServerCount] << "\n";
	cout << right << setw(NAME_WIDTH) << "ConnectedGameServerCount:" << " "
		<< _monitoringArr[(int)MonitorType::ConnectedGameServerCount] << "\n\n";

	cout << right << setw(NAME_WIDTH) << "AcceptTotal:" << " "
		<< _monitoringArr[(int)MonitorType::AcceptTotal] << "\n";
	cout << right << setw(NAME_WIDTH) << "RecvMessageTPS:" << " "
		<< _monitoringArr[(int)MonitorType::RecvMessageTPS] << "\n";
	cout << right << setw(NAME_WIDTH) << "SendMessageTPS:" << " "
		<< _monitoringArr[(int)MonitorType::SendMessageTPS] << "\n\n";

	cout << right << setw(NAME_WIDTH) << "SendJobQ:" << " "
		<< _monitoringArr[(int)MonitorType::SendJobQ] << "\n";
	
	cout << "===============================================================================\n\n";

}
