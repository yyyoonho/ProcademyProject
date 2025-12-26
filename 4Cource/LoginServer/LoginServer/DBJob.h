#pragma once
#include <Windows.h>
#include <unordered_map>

using DBRow = std::unordered_map<std::string, std::string>;
using DBResult = std::vector<DBRow>;

/**************************************************************************/
// INTERFACE

class IDBJob_Write
{
public:
	virtual ~IDBJob_Write() {}
	virtual void Exec(MYSQL* connection) = 0;
};
class IDBJob_Read
{
public:
	virtual ~IDBJob_Read() {}
	virtual void Exec(MYSQL* connection, DBResult& queryResult) = 0;
};

/**************************************************************************/
// READ

class DBCheckAccountInfo :public IDBJob_Read
{
public:
	void Exec(MYSQL* connection, DBResult& queryResult) override;

	INT64 _accountNo;
};

/**************************************************************************/
// WRITE

class DBLevelUP :public IDBJob_Write
{
public:
	void Exec(MYSQL* connection) override;

	INT64 _accountNo;
	int _level;
};

class DBQuestComplete :public IDBJob_Write
{
public:
	void Exec(MYSQL* connection) override;

	INT64 _accountNo;
	int _questID;
};