#include "stdafx.h"

#pragma comment(lib, "winmm.lib")

#include "DB_Profiler.h"

using namespace std;



LONG g_TlsIndex = -1;

list<stDBProfile*> profileList;
SRWLOCK listLock;

mutex fileOpenLock;

HANDLE hEventQuit;
HANDLE hThreadRecord;

void  DBProfilerRecord();

void DBProfileBegin(const char* targetName)
{
	if (g_TlsIndex == -1)
	{
		if (InterlockedCompareExchange(&g_TlsIndex, TlsAlloc(), -1) == -1)
		{
			if (g_TlsIndex == TLS_OUT_OF_INDEXES)
			{
				printf("Error: TlsAlloc() %d\n", GetLastError());
				return;
			}

			InitializeSRWLock(&listLock);
			hEventQuit = CreateEvent(NULL, TRUE, FALSE, NULL);
			hThreadRecord = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&DBProfilerRecord, NULL, NULL, NULL);
		}
	}

	stDBProfile* profilePtr = (stDBProfile*)TlsGetValue(g_TlsIndex);

	if (profilePtr == NULL)
	{
		profilePtr = new stDBProfile[10];
		TlsSetValue(g_TlsIndex, (LPVOID)profilePtr);

		AcquireSRWLockExclusive(&listLock);
		profileList.push_back(profilePtr);
		ReleaseSRWLockExclusive(&listLock);
	}

	int idx = -1;
	for (int i = 0; i < profilePtr[0].profileArrSize; i++)
	{
		if (strcmp(profilePtr[i].tagName, targetName) != 0)
			continue;

		idx = i;

		if (profilePtr[idx].state == DB_PROFILESTATE::DB_PROFILE_RESET)
		{
			break;
		}
		if (profilePtr[idx].state == DB_PROFILESTATE::DB_PROFILE_ERROR)
		{
			return;
		}
		if (profilePtr[idx].state == DB_PROFILESTATE::DB_PROFILE_BEGIN)
		{
			// Begin - Begin 시, call에 -1 등록.
			profilePtr[idx].call = -1;
			profilePtr[idx].state = DB_PROFILESTATE::DB_PROFILE_ERROR;
			return;
		}

		break;
	}

	if (idx == -1)
	{
		idx = profilePtr[0].profileArrSize;

		strcpy_s(profilePtr[idx].tagName, 64, targetName);
		profilePtr[idx].max = 0;
		profilePtr[idx].min = INT32_MAX;
		profilePtr[idx].call = 0;
		profilePtr[idx].total = 0;

		profilePtr[idx].threadID = GetCurrentThreadId();

		profilePtr[0].profileArrSize++;
	}

	//QueryPerformanceCounter(&profilePtr[idx].start);
	profilePtr[idx].start = timeGetTime();
	profilePtr[idx].state = DB_PROFILESTATE::DB_PROFILE_BEGIN;
}

void DBProfileEnd(const char* targetName)
{
	stDBProfile* profilePtr = (stDBProfile*)TlsGetValue(g_TlsIndex);
	if (profilePtr == NULL)
		return;

	int idx = -1;
	for (int i = 0; i < profilePtr[0].profileArrSize; i++)
	{
		if (strcmp(profilePtr[i].tagName, targetName) != 0)
			continue;

		idx = i;

		if (profilePtr[idx].state == DB_PROFILESTATE::DB_PROFILE_RESET)
		{
			return;
		}
		if (profilePtr[idx].state == DB_PROFILESTATE::DB_PROFILE_ERROR)
		{
			return;
		}
		if (profilePtr[idx].state == DB_PROFILESTATE::DB_PROFILE_END)
		{
			// End - End 시, call에 -2 등록.
			profilePtr[idx].call = -2;
			profilePtr[idx].state = DB_PROFILESTATE::DB_PROFILE_ERROR;

			return;
		}

		break;
	}

	if (idx < 0)
	{
		// TODO: 크래쉬를 발생시킬까...
		return;
	}

	//LARGE_INTEGER end;
	//QueryPerformanceCounter(&end);

	DWORD end;
	end = timeGetTime();

	profilePtr[idx].call++;

	DWORD timeDiff = end - profilePtr[idx].start;

	profilePtr[idx].total += timeDiff;

	if (profilePtr[idx].max < timeDiff)
		profilePtr[idx].max = timeDiff;

	if (profilePtr[idx].min > timeDiff)
		profilePtr[idx].min = timeDiff;

	profilePtr[idx].state = DB_PROFILESTATE::DB_PROFILE_END;
}

void ProfileDataOutText(char* szFileName)
{
	FILE* fp;
	errno_t errChk = fopen_s(&fp, szFileName, "wt");
	if (errChk != 0)
	{
		printf("ERROR: fopen_s\n");
		return;
	}

	fprintf(fp,
		"=========================================================================================\n");
	fprintf(fp,
		"%-12s | %-20s | %15s | %15s | %15s | %10s\n",
		"ThreadID", "Name", "Average (ms)", "Min (ms)", "Max (ms)", "Call");
	fprintf(fp,
		"----------------------------------------------------------------------------------------------------------------------------------\n");

	AcquireSRWLockExclusive(&listLock);

	list<stDBProfile*>::iterator iter;
	for (iter = profileList.begin(); iter != profileList.end(); ++iter)
	{
		stDBProfile* profilePtr = *iter;

		int profileArrSize = profilePtr[0].profileArrSize;

		for (int i = 0; i < profileArrSize; i++)
		{
			if (profilePtr[i].call == -1 || profilePtr[i].call == -2)
			{
				fprintf(fp, "%-12u | %-20s | %15s | %15s | %15s | %10llu\n",
					profilePtr[i].threadID,
					profilePtr[i].tagName,
					"-", "-", "-", profilePtr[i].call);
				continue;
			}

			double average = 0;
			DWORD64 maxSum = (DWORD64)profilePtr[i].max;
			DWORD64 minSum = (DWORD64)profilePtr[i].min;
			DWORD64 call = profilePtr[i].call;

			if (call > 2)
			{
				call = profilePtr[i].call - 2;
				average = (double)(profilePtr[i].total - (maxSum + minSum)) / call;
			}
			else if (call > 0)
			{
				average = (double)(profilePtr[i].total) / call;
			}
			else
			{
				average = 0;
			}


			fprintf(fp, "%-12u | %-20s | %15.4f | %15.4u | %15.1u | %10llu\n",
				profilePtr[i].threadID,
				profilePtr[i].tagName,
				average,
				profilePtr[i].min,
				profilePtr[i].max,
				call);
		}
		fprintf(fp,
			"----------------------------------------------------------------------------------------------------------------------------------\n");

	}

	ReleaseSRWLockExclusive(&listLock);

	fprintf(fp,
		"=========================================================================================\n");

	fclose(fp);
}

void DBProfileReset()
{
	AcquireSRWLockExclusive(&listLock);

	list<stDBProfile*>::iterator iter;
	for (iter = profileList.begin(); iter != profileList.end(); ++iter)
	{
		stDBProfile* tmp = *iter;
		int profileArrSize = tmp[0].profileArrSize;

		for (int i = 0; i < profileArrSize; i++)
		{
			//tmp[i].start.QuadPart = 0;
			tmp[i].start = 0;

			tmp[i].call = 0;
			tmp[i].total = 0;
			tmp[i].max = 0;
			tmp[i].min = INT32_MAX;

			tmp[i].state = DB_PROFILESTATE::DB_PROFILE_RESET;
		}
	}

	ReleaseSRWLockExclusive(&listLock);
}

void  DBProfilerRecord()
{
	DWORD recordTime = RECORDTIME;
	char fileName[64] = "DB_ProfilerResult.txt";

	while (1)
	{
		DWORD ret = WaitForSingleObject(hEventQuit, recordTime);
		if (ret == WAIT_OBJECT_0)
		{
			break;
		}

		lock_guard<mutex> lock(fileOpenLock);

		ProfileDataOutText(fileName);
	}
}

void DBProfilerRecordStop()
{
	SetEvent(hEventQuit);

	if (hThreadRecord)
	{
		WaitForSingleObject(hThreadRecord, INFINITE);
	}
}

stDBProfile::stDBProfile()
{
	tagName[0] = NULL;
	//start.QuadPart = 0;
	start = 0;

	call = 0;
	total = 0;
	max = 0;
	min = INT32_MAX;

	state = DB_PROFILESTATE::DB_PROFILE_RESET;

	profileArrSize = 0;
	threadID = 0;
}

stDBProfile::~stDBProfile()
{
}
