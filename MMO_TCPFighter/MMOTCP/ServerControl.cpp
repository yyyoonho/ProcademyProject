#include "pch.h"

#include "LogManager.h"
#include "MMOTCP.h"
#include "ServerControl.h"

using namespace std;

/*
	L: 컨트롤 Lock
	U: 컨트롤 UnLock
	Q: 서버 종료

	0: 로그레벨 - 디버그
	1: 로그레벨 - 에러
	2: 로그레벨 - 시스템
*/

void ServerControl()
{
	static bool bControlMode = false;

	if (_kbhit())
	{
		WCHAR ControlKey = _getwch();

		if (L'U' == ControlKey || L'u' == ControlKey)
		{
			bControlMode = true;
		}

		if (L'L' == ControlKey || L'l' == ControlKey)
		{
			bControlMode = false;
		}

		if ((L'Q' == ControlKey || L'q' == ControlKey) && bControlMode)
		{
			g_bShutdown = true;
		}

		if ((L'0' == ControlKey) && bControlMode)
		{
			g_iLogLevel = 0;

			_LOG(dfLOG_LEVEL_DEBUG, L"%ls", L"테스트입니다.");
		}
		
		if ((L'1' == ControlKey) && bControlMode)
		{
			g_iLogLevel = 1;
		}
		if ((L'2' == ControlKey) && bControlMode)
		{
			g_iLogLevel = 2;
		}
	}

}