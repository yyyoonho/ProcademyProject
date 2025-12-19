#pragma once

class DBConnector
{
public:
	DBConnector();
	~DBConnector();

public:
	// TLS 전용 - SELECT
	void Query(IDBJob* job);

	// DBWriter 전용 - UPDATE/DELETE/INSERT
	void Query_Save();
	void PushQuery(IDBJob* job);

private:
	std::string		_ip;
	USHORT			_port;
	std::string		_user;
	std::string		_password;

	MYSQL conn;
	MYSQL* connection = NULL;

	std::mutex _queueLock;
	std::queue<IDBJob*> _DBJobQueue;
};