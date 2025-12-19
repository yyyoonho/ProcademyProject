#pragma comment(lib, "libmysql.lib")
#pragma comment(lib, "winmm.lib")

#include <iostream>
#include <Windows.h>
#include <queue>
#include <string>
#include <mutex>

#include "mysql/include/mysql.h"
#include "mysql/include/errmsg.h"

#include "MyConfig.h"
#include "DBJob.h"
#include "DBConnector.h"

using namespace std;

DBConnector::DBConnector()
{
	// DB 정보세팅
	_ip = g_MyConfig.mySQLConfig.ip;
	_port = g_MyConfig.mySQLConfig.port;
	_user = g_MyConfig.mySQLConfig.user;
	_password = g_MyConfig.mySQLConfig.password;

    // DB연결
	connection = mysql_real_connect(&conn,
		_ip.c_str(),
		_user.c_str(),
		_password.c_str(),
		"mytest",
		_port,
		(char*)NULL,
		0);

	if (connection == NULL)
	{
		fprintf(stderr, "Mysql connection error : %s", mysql_error(&conn));
		return;
	}
}

DBConnector::~DBConnector()
{
	// DB 연결닫기
	mysql_close(connection);
}

void DBConnector::Query(IDBJob* job)
{
	job->Exec(connection);
}

void DBConnector::Query_Save()
{
	IDBJob* job = NULL;
	{
		lock_guard<mutex> lock(_queueLock);
		job = _DBJobQueue.front();
		_DBJobQueue.pop();
	}

	if (job != NULL)
	{
		job->Exec(connection);
		delete job;
	}
	
}

void DBConnector::PushQuery(IDBJob* job)
{
	lock_guard<mutex> lock(_queueLock);
	_DBJobQueue.push(job);
}


int main()
{
    std::cout << "Hello World!\n";
}
