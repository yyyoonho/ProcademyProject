#include "pch.h"

#include "LogManager.h"

using namespace std;

int g_iLogLevel;
WCHAR g_szLogBuff[1024];

void Log(WCHAR* szString, int iLogLevel)
{
	char szFileName[32]; 
	sprintf_s(szFileName, "%s.txt", __DATE__);

	FILE* fp;
	fopen_s(&fp, szFileName, "at, ccs=UTF-16LE");

	if (fp == 0)
		return;

	fwrite(szString, wcslen(szString) * sizeof(WCHAR), 1, fp);

	fclose(fp);
}

//https://www.kernelpanic.kr/50