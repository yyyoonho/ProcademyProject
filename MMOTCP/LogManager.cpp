#include "stdafx.h"

#include "LogManager.h"

using namespace std;

int g_iLogLevel = dfLOG_LEVEL_SYSTEM;
WCHAR g_szLogBuff[1024];
time_t now;

void Log(WCHAR* szString, int iLogLevel)
{
	char szFileName[101];
	struct tm date;
	localtime_s(&date, &now);

	sprintf_s(szFileName, 100, ".\\Log\\LOG_%d%02d%02d_%02d%02d%02d.txt",
		date.tm_year + 1900,
		date.tm_mon + 1,
		date.tm_mday,
		date.tm_hour,
		date.tm_min,
		date.tm_sec
	);

	FILE* fp;
	fopen_s(&fp, szFileName, "at, ccs=UTF-16LE");

	if (fp == 0)
		return;

	fwrite(szString, wcslen(szString) * sizeof(WCHAR), 1, fp);

	fclose(fp);
}

void InitLog()
{
	now = time(NULL);
}