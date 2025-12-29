#include "stdafx.h"

#include "MyConfig.h"
#include "DBJob.h"
#include "DBConnector.h"

using namespace std;

thread_local DBConnector::DBConnTLS DBConnector::conn_tls;
DBConnector* DBConnector::_pDBConnector = NULL;
mutex initLock;

DBConnector* DBConnector::GetInstance()
{
	if (_pDBConnector == NULL)
	{
		{
			lock_guard<mutex> lock(initLock);

			if (_pDBConnector == NULL)
			{
				_pDBConnector = new DBConnector;
			}
		}
	}

	return _pDBConnector;
}

DBConnector::DBConnector()
{
	Start();
}

DBConnector::~DBConnector()
{
	// DB 연결닫기

}

void DBConnector::Start()
{
	// DB 정보세팅
	_ip = myConfig.mySQLConfig.ip;
	_port = myConfig.mySQLConfig.port;
	_user = myConfig.mySQLConfig.user;
	_password = myConfig.mySQLConfig.password;

	//mysql_init(&conn_tls.readConn);

	for (int i = 0; i < (int)DB_FIELD::DB_FIELD_COUNT; i++)
	{
		_writeWorkers[i].Start(_ip, _port, _user, _password);
	}
}

void DBConnector::Stop()
{
	for (int i = 0; i < (int)DB_FIELD::DB_FIELD_COUNT; i++)
	{
		_writeWorkers[i].Stop();
	}
}

void DBConnector::QueryRead(IDBJob_Read* job, DBResult& queryResult)
{
	if (conn_tls.readConnection == nullptr)
	{
		mysql_init(&conn_tls.readConn);

		conn_tls.readConnection = mysql_real_connect(
			&conn_tls.readConn,
			_ip.c_str(),
			_user.c_str(),
			_password.c_str(),
			"logdb",
			_port,
			(char*)NULL,
			0);

		if (conn_tls.readConnection == NULL)
		{
			fprintf(stderr, "Mysql connection error : %s", mysql_error(&conn_tls.readConn));
			delete job;
			return;
		}
	}

	job->Exec(conn_tls.readConnection, queryResult);
	delete job;
}

void DBConnector::QueryWrite(int fieldID, IDBJob_Write* job)
{
	if (fieldID < 0 || fieldID >= (int)DB_FIELD::DB_FIELD_COUNT)
		return;

	_writeWorkers[fieldID].Push(job);
}

void DBConnector::WriteWorker::Start(std::string ip, USHORT port, std::string user, std::string password)
{
	_ip = ip;
	_port = port;
	_user = user;
	_password = password;

	_writeConnection = NULL;

	mysql_init(&_writeConn);

	_hEventJobQueue = CreateEvent(NULL, FALSE, FALSE, NULL);
	_hEventQuit = CreateEvent(NULL, TRUE, FALSE, NULL);
	_hWriteThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&WorkerThreadRunProc, this, NULL, NULL);
}

void DBConnector::WriteWorker::Stop()
{
	SetEvent(_hEventQuit);
}

void DBConnector::WriteWorker::Push(IDBJob_Write* job)
{
	{
		lock_guard<mutex> lock(_queueLock);
		_JobQueue.push(job);
	}

	SetEvent(_hEventJobQueue);
}

void DBConnector::WriteWorker::WorkerThreadRunProc(LPVOID* lParam)
{
	WriteWorker* self = (WriteWorker*)lParam;
	self->Run();
}

void DBConnector::WriteWorker::Run()
{
	if (_writeConnection == nullptr)
	{
		//mysql_init(&_writeConn);

		_writeConnection = mysql_real_connect(
			&_writeConn,
			_ip.c_str(),
			_user.c_str(),
			_password.c_str(),
			"logdb",
			_port,
			(char*)NULL,
			0);
		if (_writeConnection == NULL)
		{
			fprintf(stderr, "Mysql connection error : %s", mysql_error(&_writeConn));
			return;
		}
	}

	HANDLE hEvents[2] = { _hEventQuit, _hEventJobQueue };
	while (1)
	{
		DWORD ret = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
		if (ret == WAIT_OBJECT_0)
		{
			break;
		}

		while (1)
		{
			IDBJob_Write* job = NULL;

			{
				lock_guard<mutex> lock(_queueLock);

				if (_JobQueue.empty())
					break;

				job = _JobQueue.front();
				_JobQueue.pop();
			}

			job->Exec(_writeConnection);
			delete job;
		}

		{
			lock_guard<mutex> lock(_queueLock);
			if (!_JobQueue.empty())
				SetEvent(_hEventJobQueue);
		}
	}
}
