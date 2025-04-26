#pragma once

/*	사용법
*	
*	로그 레벨은 상황에 맞게 알아서 지정하세요.
*	_LOG(dfLOG_LEVEL_DEBUG, L"%ls %ls\n", L"테스트입니다.", L"리얼입니다.");
* 
*/

extern int g_iLogLevel;
extern WCHAR g_szLogBuff[1024];

#define dfLOG_LEVEL_DEBUG	0
#define dfLOG_LEVEL_ERROR	1
#define dfLOG_LEVEL_SYSTEM	2

#define _LOG(LogLevel, fmt, ...)					\
do{													\
	if (g_iLogLevel <= LogLevel)					\
	{												\
		wsprintf(g_szLogBuff, fmt, ##__VA_ARGS__);	\
		Log(g_szLogBuff, LogLevel);					\
	}												\
} while (0)			


void Log(WCHAR * szString, int iLogLevel);

void InitLog();