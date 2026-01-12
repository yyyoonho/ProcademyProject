#include "stdafx.h"
#include <Pdh.h>
#include <pdhmsg.h>
#pragma comment(lib,"Pdh.lib")
#include "CpuUsage.h"

#include "CommonProtocol.h"
#include "SendJob.h"
#include "DBJob.h"
#include "DBConnector.h"

#include "NetServer.h"
#include "SendJob.h"
#include "MonitoringServer.h"

#include "LocalMonitoring.h"

int GetTotalKBytesPerSec(PDH_HCOUNTER counter);

LocalMonitoring::LocalMonitoring()
{
	_localClient = new localClient;
	for (int i = 0; i < en_PACKET_SS_MONITOR_DATA_UPDATE::COUNT; i++)
	{
		_localClient->activeData[i] = false;

		_localClient->count[i] = 0;
		_localClient->avgData[i] = 0;
		_localClient->minData[i] = INT32_MAX;
		_localClient->maxData[i] = 0;
	}

	_cpu = new CCpuUsage;

	PdhOpenQuery(NULL, NULL, &_pdhQuery);

	// PDH 카운터 경로 생성
	PdhAddCounter(_pdhQuery, L"\\Memory\\Pool Nonpaged Bytes", NULL, &_nonpagedMemory);
	PdhAddCounter(_pdhQuery, L"\\Network Interface(*)\\Bytes Received/sec", NULL, &_receivedPerSec);
	PdhAddCounter(_pdhQuery, L"\\Network Interface(*)\\Bytes Sent/sec", NULL, &_sentPerSec);
	PdhAddCounter(_pdhQuery, L"\\Memory\\Available MBytes", NULL, &_availableMemory);

	// 첫 갱신
	PdhCollectQueryData(_pdhQuery);
}

LocalMonitoring::~LocalMonitoring()
{
	delete _cpu;
}

void LocalMonitoring::Start()
{
	hEvent_Quit = CreateEvent(NULL, TRUE, FALSE, NULL);
	hThread_LocalMonitoring = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&ServerMonitoringRun, this, NULL, NULL);
	hThread_DB = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&DBRun, this, NULL, NULL);
	
}

void LocalMonitoring::Stop()
{
	SetEvent(hEvent_Quit);
}

void LocalMonitoring::RegisterNetServer(MonitoringServer* pNetMonitoringServer)
{
	this->pNetMonitoringServer = pNetMonitoringServer;
}

void LocalMonitoring::ServerMonitoringRun(LPVOID* lParam)
{
	LocalMonitoring* self = (LocalMonitoring*)lParam;
	self->ServerMonitoringThread();
}

void LocalMonitoring::ServerMonitoringThread()
{
	while (1)
	{
		DWORD ret = WaitForSingleObject(hEvent_Quit, 1000);
		if (ret == WAIT_OBJECT_0)
			break;

		int timeStamp = (int)time_t();

		// 갱신
		_cpu->UpdateCpuTime();
		PdhCollectQueryData(_pdhQuery);

		int cpuProcessorTotal;
		int npMBytes;
		int recvedKBytes;
		int sentKBytes;
		int availableMByes;

		lock_guard<mutex> lock(_clientLock);

		// CPU
		{
			cpuProcessorTotal = (int)_cpu->ProcessorTotal();

			stSendJob job;
			job.dataType = dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL;
			job.dataValue = cpuProcessorTotal;
			job.timeStamp = timeStamp;

			pNetMonitoringServer->EnqueueSendJob(job);

			_localClient->count[job.dataType]++;
			_localClient->activeData[job.dataType] = true;
			_localClient->avgData[job.dataType] += (job.dataValue - _localClient->avgData[job.dataType]) / _localClient->count[job.dataType];
			_localClient->minData[job.dataType] = min(_localClient->minData[job.dataType], job.dataValue);
			_localClient->maxData[job.dataType] = max(_localClient->maxData[job.dataType], job.dataValue);
		}

		// 논페이지드메모리
		{
			PDH_FMT_COUNTERVALUE counterVal;
			PdhGetFormattedCounterValue(_nonpagedMemory, PDH_FMT_DOUBLE, NULL, &counterVal);
			npMBytes = (int)(counterVal.doubleValue / (1024 * 1024));

			stSendJob job;
			job.dataType = dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY;
			job.dataValue = npMBytes;
			job.timeStamp = timeStamp;

			pNetMonitoringServer->EnqueueSendJob(job);

			_localClient->count[job.dataType]++;
			_localClient->activeData[job.dataType] = true;
			_localClient->avgData[job.dataType] += (job.dataValue - _localClient->avgData[job.dataType]) / _localClient->count[job.dataType];
			_localClient->minData[job.dataType] = min(_localClient->minData[job.dataType], job.dataValue);
			_localClient->maxData[job.dataType] = max(_localClient->maxData[job.dataType], job.dataValue);
		}

		// Received Network
		{
			PDH_FMT_COUNTERVALUE counterVal;
			PdhGetFormattedCounterValue(_receivedPerSec, PDH_FMT_DOUBLE, NULL, &counterVal);

			// 테스트
			//recvedKBytes = (int)(counterVal.doubleValue / (1024));
			recvedKBytes = GetTotalKBytesPerSec(_receivedPerSec);

			stSendJob job;
			job.dataType = dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV;
			job.dataValue = recvedKBytes;
			job.timeStamp = timeStamp;

			pNetMonitoringServer->EnqueueSendJob(job);

			_localClient->count[job.dataType]++;
			_localClient->activeData[job.dataType] = true;
			_localClient->avgData[job.dataType] += (job.dataValue - _localClient->avgData[job.dataType]) / _localClient->count[job.dataType];
			_localClient->minData[job.dataType] = min(_localClient->minData[job.dataType], job.dataValue);
			_localClient->maxData[job.dataType] = max(_localClient->maxData[job.dataType], job.dataValue);
		}

		// Sent Network
		{
			PDH_FMT_COUNTERVALUE counterVal;
			PdhGetFormattedCounterValue(_sentPerSec, PDH_FMT_DOUBLE, NULL, &counterVal);

			// 테스트
			//sentKBytes = (int)(counterVal.doubleValue / (1024));
			sentKBytes = GetTotalKBytesPerSec(_sentPerSec);

			stSendJob job;
			job.dataType = dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND;
			job.dataValue = sentKBytes;
			job.timeStamp = timeStamp;

			pNetMonitoringServer->EnqueueSendJob(job);

			_localClient->count[job.dataType]++;
			_localClient->activeData[job.dataType] = true;
			_localClient->avgData[job.dataType] += (job.dataValue - _localClient->avgData[job.dataType]) / _localClient->count[job.dataType];
			_localClient->minData[job.dataType] = min(_localClient->minData[job.dataType], job.dataValue);
			_localClient->maxData[job.dataType] = max(_localClient->maxData[job.dataType], job.dataValue);
		}

		// available 메모리
		{
			PDH_FMT_COUNTERVALUE counterVal;
			PdhGetFormattedCounterValue(_availableMemory, PDH_FMT_DOUBLE, NULL, &counterVal);
			availableMByes = (int)(counterVal.doubleValue);

			stSendJob job;
			job.dataType = dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY;
			job.dataValue = availableMByes;
			job.timeStamp = timeStamp;

			pNetMonitoringServer->EnqueueSendJob(job);

			_localClient->count[job.dataType]++;
			_localClient->activeData[job.dataType] = true;
			_localClient->avgData[job.dataType] += (job.dataValue - _localClient->avgData[job.dataType]) / _localClient->count[job.dataType];
			_localClient->minData[job.dataType] = min(_localClient->minData[job.dataType], job.dataValue);
			_localClient->maxData[job.dataType] = max(_localClient->maxData[job.dataType], job.dataValue);
		}

	}
}

void LocalMonitoring::DBRun(LPVOID* lParam)
{
	LocalMonitoring* self = (LocalMonitoring*)lParam;
	self->DBThread();
}

void LocalMonitoring::DBThread()
{
	while (1)
	{
		DWORD ret = WaitForSingleObject(hEvent_Quit, 1000 * 60 * 10);
		if (ret == WAIT_OBJECT_0)
		{
			break;
		}

		// DB 저장할 Job 만들어서 DBWorker에 토스.
		vector<MonitoringSnapshot> snapShots;

		{
			lock_guard<mutex> lock(_clientLock);

			for (int i = 0; i < en_PACKET_SS_MONITOR_DATA_UPDATE::COUNT; i++)
			{
				if (_localClient->activeData[i] != true)
					continue;

				snapShots.push_back({
						_serverNo,
						i,
						_localClient->avgData[i],
						_localClient->minData[i],
						_localClient->maxData[i]
					});

				_localClient->ResetData(i);
			}	
		}

		for (int i = 0; i < snapShots.size(); i++)
		{
			auto job = new DBMonitoringLog;

			job->_serverNo = snapShots[i].serverNo;
			job->_dataType = snapShots[i].dataType;
			job->_avg = snapShots[i].avg;
			job->_min = snapShots[i].min;
			job->_max = snapShots[i].max;

			DBConnector::GetInstance()->QueryWrite(0, job);
		}
	}
}

void localClient::ResetData(int i)
{
	count[i] = 0;
	activeData[i] = false;
	avgData[i] = 0;
	minData[i] = INT32_MAX;
	maxData[i] = 0;
}


int GetTotalKBytesPerSec(PDH_HCOUNTER counter)
{
	DWORD bufferSize = 0;
	DWORD itemCount = 0;

	PDH_STATUS status = PdhGetFormattedCounterArray(counter, PDH_FMT_DOUBLE, &bufferSize, &itemCount, nullptr);
	
	vector<BYTE> buffer(bufferSize);
	auto items = (PDH_FMT_COUNTERVALUE_ITEM*)(buffer.data());

	status = PdhGetFormattedCounterArray(counter, PDH_FMT_DOUBLE, &bufferSize, &itemCount, items);
	if (status != ERROR_SUCCESS)
		return 0;

	double totalBytes = 0.0;
	for (DWORD i = 0; i < itemCount; i++)
	{
		if (items[i].FmtValue.CStatus == PDH_CSTATUS_VALID_DATA)
		{
			totalBytes += items[i].FmtValue.doubleValue; // bytes/sec
		}
	}

	double totalKBytes = totalBytes / 1024.0;

	return (int)totalKBytes;
}