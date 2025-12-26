#pragma comment(lib, "libmysql.lib")

#include "mysql/include/mysql.h"
#include "mysql/include/errmsg.h"

#include <iostream>
#include <Windows.h>
#include <queue>
#include <string>
#include <mutex>
#include <array>

#include "DBJob.h"
#include "DBConnector.h"
#include "MyConfig.h"

using namespace std;

MyConfig g_MyConfig;

int main()
{
	g_MyConfig.Load("MySQLconfig.ini");

	while (1)
	{
		Sleep(10);

		//DBConnector::GetInstance()->QueryWrite(0, new DBLevelUP);

		DBResult queryRes;
		DBConnector::GetInstance()->QueryRead(new DBCheckAccountInfo, queryRes);

		for (int i = 0; i < queryRes.size(); i++)
		{
			cout << queryRes[i]["accountNo"] << endl;
		}
	}

	return 0;
}