#pragma once

#ifndef __PROCADEMY_LIB_CRASH_DUMP__
#define __PROCADEMY_LIB_CRASH_DUMP__

#include <DbgHelp.h>
#include <minidumpapiset.h>

namespace procademy
{
	class CrashDump
	{
	public:
		CrashDump()
		{
			_DumpCount = 0;

			_invalid_parameter_handler oldHandler, newHandler;
			newHandler = myInvalidParameterHandler;

			oldHandler = _set_invalid_parameter_handler(newHandler);
			_CrtSetReportMode(_CRT_WARN, 0);
			_CrtSetReportMode(_CRT_ASSERT, 0);
			_CrtSetReportMode(_CRT_ERROR, 0);

			_CrtSetReportHook(_custom_Report_hook);

			_set_purecall_handler(myPurecallHandler);

			SetHandlerDump();
		}

		static void Crash()
		{
			int* p = nullptr;
			*p = 0;
		}

		static LONG WINAPI MyExceptionFilter(__in PEXCEPTION_POINTERS pExceptionPointer)
		{
			int iWorkingMemory = 0;
			SYSTEMTIME stNowTime;

			long DumpCount = InterlockedIncrement(&_DumpCount);

			//--------------------------------------
			// 현재 날짜와 시간을 알아온다.
			//--------------------------------------
			WCHAR filename[MAX_PATH];

			GetLocalTime(&stNowTime);
			wsprintf(filename, L"Dump_%d%02d%02d_%02d.%02d.%02d_%d.dmp",
				stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond);

			wprintf(L"\n\n\n !!! Crash Error !!! %d.%d.%d/ %d:%d:%d\n", stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond);
			wprintf(L"Now Save dump file...\n");

			HANDLE hDumpFile = ::CreateFile(
				filename,
				GENERIC_WRITE,
				FILE_SHARE_WRITE,
				NULL,
				CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				NULL
			);

			if (hDumpFile != INVALID_HANDLE_VALUE)
			{
				_MINIDUMP_EXCEPTION_INFORMATION MinidumpExceptionInformation;

				MinidumpExceptionInformation.ThreadId = ::GetCurrentProcessId();
				MinidumpExceptionInformation.ExceptionPointers = pExceptionPointer;
				MinidumpExceptionInformation.ClientPointers = TRUE;

				MiniDumpWriteDump(GetCurrentProcess(),
					GetCurrentProcessId(),
					hDumpFile,
					MiniDumpWithFullMemory,
					&MinidumpExceptionInformation,
					NULL,
					NULL
				);

				CloseHandle(hDumpFile);

				wprintf(L"CrashDump save Finish!");
			}

			return EXCEPTION_EXECUTE_HANDLER;
		}

		static void SetHandlerDump()
		{
			SetUnhandledExceptionFilter(MyExceptionFilter);
		}

		static void myInvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t pReserved)
		{
			Crash();
		}

		static int _custom_Report_hook(int ireposttype, char* message, int* returnvalue)
		{
			Crash();
			return true;
		}

		static void myPurecallHandler()
		{
			Crash();
		}

		static long _DumpCount;
	};

	long CrashDump::_DumpCount = 0;
}

#endif // !__PROCADEMY_LIB_CRASH_DUMP__