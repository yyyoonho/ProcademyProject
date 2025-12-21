#pragma comment(lib, "libmysql.lib")

#include "mysql/include/mysql.h"
#include "mysql/include/errmsg.h"

#include <iostream>
#include <Windows.h>
#include <queue>
#include <string>
#include <unordered_map>

#include "DBJob.h"


using namespace std;
using DBRow = unordered_map<string, string>;
using DBResult = vector<DBRow>;

void DBLevelUP::Exec(MYSQL* connection)
{
	// 쿼리 생성 및 전송
	string query =
		"UPDATE account SET level = " + to_string(_level) +
		" WHERE accountNo = " + to_string(accountNo);

	if (mysql_query(connection, query.c_str()) != 0)
	{
		printf("DBLevelUP error: %s\n", mysql_error(connection));
		return;
	}
}

void DBQuestComplete::Exec(MYSQL* connection)
{
	// 쿼리 생성 및 전송
	string query =
		"INSERT INTO quest_log(accountNo, questID) VALUES (" +
		to_string(accountNo) + ", " + to_string(questID) + ")";

	if (mysql_query(connection, query.c_str()) != 0)
	{
		printf("DBQuestComplete error: %s\n", mysql_error(connection));
	}
}

void DBCheckAccountInfo::Exec(MYSQL* connection)
{
	// Select 쿼리문
	string query = "SELECT * FROM account";	// From 다음 DB에 존재하는 테이블 명으로 수정하세요
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

	DBResult result;
	while ((sql_row = mysql_fetch_row(sql_result)) != NULL)
	{
		DBRow oneRow;

		for (int i = 0; i < colCount; i++)
		{
			string colName = fields[i].name;
			string value = sql_row[i];
			
			oneRow.emplace(colName, value);
		}
		
		result.push_back(oneRow);
	}

	mysql_free_result(sql_result);
}
