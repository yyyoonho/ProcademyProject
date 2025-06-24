#include "stdafx.h"
#include "LogManager.h"
#include "MMOTCP.h"

#include "ServerControlManager.h"
using namespace std;

void ServerControl()
{
	static bool bControlMode = false;

	if (_kbhit())
	{
		WCHAR ControlKey = _getwch();

		if (L'U' == ControlKey || L'u' == ControlKey)
		{
			printf("컨트롤 Unlock\n");
			bControlMode = true;
		}

		if (L'L' == ControlKey || L'l' == ControlKey)
		{
			printf("컨트롤 Lock\n");
			bControlMode = false;
		}

		if ((L'Q' == ControlKey || L'q' == ControlKey) && bControlMode)
		{
			printf("서버를 종료합니다...\n");
			_LOG(dfLOG_LEVEL_SYSTEM, L"ServerControl: Shutdown.\n");
			g_bShutdown = true;
		}

		if ((L'0' == ControlKey) && bControlMode)
		{
			printf("로그레벨: 디버그 \n");
			g_iLogLevel = dfLOG_LEVEL_DEBUG;
		}

		if ((L'1' == ControlKey) && bControlMode)
		{
			printf("로그레벨: 에러 \n");
			g_iLogLevel = dfLOG_LEVEL_ERROR;
		}
		if ((L'2' == ControlKey) && bControlMode)
		{
			printf("로그레벨: 시스템 \n");
			g_iLogLevel = dfLOG_LEVEL_SYSTEM;
		}
	}
}