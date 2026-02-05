#include "stdafx.h"
#include "DB_Profiler.h"
#include "DBJob.h"

using namespace std;

int g_number;

void DBLevelUP::Exec(MYSQL* connection)
{
	// 쿼리 생성 및 전송
	/*string query =
		"UPDATE account SET level = " + to_string(_level) +
		" WHERE accountNo = " + to_string(accountNo);

	if (mysql_query(connection, query.c_str()) != 0)
	{
		printf("DBLevelUP error: %s\n", mysql_error(connection));
		return;
	}*/

	// INSERT 테스트
	for (int i = 0; i < 3; i++)
	{
		g_number++;

		string query =
			"INSERT INTO `new_table` (`accountNo`) VALUES ('" + to_string(g_number) + "');";

		int query_stat;

		// Insert 쿼리문
		query_stat = mysql_query(connection, query.c_str());
		if (query_stat != 0)
		{
			printf("Mysql query error : %s", mysql_error(connection));
			return;
		}
	}
}

void DBQuestComplete::Exec(MYSQL* connection)
{
	// 쿼리 생성 및 전송
	string query =
		"INSERT INTO quest_log(accountNo, questID) VALUES (" +
		to_string(_accountNo) + ", " + to_string(_questID) + ")";

	if (mysql_query(connection, query.c_str()) != 0)
	{
		printf("DBQuestComplete error: %s\n", mysql_error(connection));
	}
}

void DBCheckAccountInfo::Exec(MYSQL* connection, DBResult& queryResult)
{
	// Select 쿼리문
	string query = "SELECT * FROM sessionKey WHERE accountNo = " + to_string(_accountNo);
	int query_stat = mysql_query(connection, query.c_str());
	if (query_stat != 0)
	{
		printf("Mysql query error : %s", mysql_error(connection));
		return;
	}

	// 결과출력
	MYSQL_RES* sql_result;
	MYSQL_ROW sql_row;

	sql_result = mysql_store_result(connection);		// 결과 전체를 미리 가져옴

	unsigned int colCount = mysql_num_fields(sql_result);
	MYSQL_FIELD* fields = mysql_fetch_fields(sql_result);

	while ((sql_row = mysql_fetch_row(sql_result)) != NULL)
	{
		DBRow oneRow;

		for (int i = 0; i < colCount; i++)
		{
			string colName = fields[i].name;
			string value;
			if (sql_row[i] == NULL)
			{
				value = "";
			}
			else
			{
				value = sql_row[i];
			}

			oneRow.emplace(colName, value);
		}

		queryResult.push_back(oneRow);
	}

	if (queryResult.size() <= 0)
	{
		int a = 3;
	}


	mysql_free_result(sql_result);
}

void DBMonitoringLog::Exec(MYSQL* connection)
{
	DB_PRO_BEGIN("DBMonitoringLog");

	/*string query =
		"INSERT INTO monitorlog (logtime, serverno, type, avg, min, max) VALUES ("
		"NOW(), " +
		to_string(_serverNo) + ", " +
		to_string(_dataType) + ", " +
		to_string(_avg) + ", " +
		to_string(_min) + ", " +
		to_string(_max) +
		")";

	if (mysql_query(connection, query.c_str()) != 0)
	{
		printf("DBQuestComplete error: %s\n", mysql_error(connection));
	}*/

	// -----------------------------
	// 1. 월별 테이블명 생성
	// -----------------------------
	time_t now = time(nullptr);
	tm local{};
	localtime_s(&local, &now);

	char tableName[64];
	sprintf_s(
		tableName,
		"monitorlog_%04d%02d",
		local.tm_year + 1900,
		local.tm_mon + 1
	);

	// -----------------------------
	// 2. INSERT 쿼리 생성
	// -----------------------------
	string query =
		"INSERT INTO " + string(tableName) +
		" (logtime, serverno, type, avg, min, max) VALUES ("
		"NOW(), " +
		std::to_string(_serverNo) + ", " +
		std::to_string(_dataType) + ", " +
		std::to_string(_avg) + ", " +
		std::to_string(_min) + ", " +
		std::to_string(_max) +
		")";

	// -----------------------------
	// 3. INSERT 시도
	// -----------------------------
	if (mysql_query(connection, query.c_str()) != 0)
	{
		unsigned int err = mysql_errno(connection);

		// 테이블 없음
		if (err == 1146)
		{
			string createQuery =
				"CREATE TABLE " + string(tableName) +
				" LIKE monitorlog";

			// 테이블 생성
			if (mysql_query(connection, createQuery.c_str()) != 0)
			{
				DebugBreak();
				return;
			}

			// 다시 INSERT
			if (mysql_query(connection, query.c_str()) != 0)
			{
				DebugBreak();
			}
		}
	}

	DB_PRO_END("DBMonitoringLog");
}
