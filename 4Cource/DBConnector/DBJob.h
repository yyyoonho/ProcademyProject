#pragma once
#include <Windows.h>

class IDBJob
{
public:
	virtual void Exec(MYSQL* connection) = 0;
};

class DBCheckAccountInfo :public IDBJob
{
public:
	void Exec(MYSQL* connection) override;

	INT64 accountNo;
};

class DBLevelUP :public IDBJob
{
public:
	void Exec(MYSQL* connection) override;

	INT64 accountNo;
	int _level;
};

class DBQuestComplete :public IDBJob
{
public:
	void Exec(MYSQL* connection) override;

	INT64 accountNo;
	int questID;
};