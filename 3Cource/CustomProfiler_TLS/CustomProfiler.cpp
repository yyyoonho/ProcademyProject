#include <iostream>
#include <Windows.h>
#include <list>
#include "CustomProfiler.h"

using namespace std;

#define MICROSEC 1000000


LONG g_TlsIndex = -1;

list<stProfile*> profileList;
SRWLOCK listLock;

LARGE_INTEGER frequency;

void ProfileBegin(const char* targetName)
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

			QueryPerformanceFrequency(&frequency);
		}
	}

	stProfile* profilePtr = (stProfile*)TlsGetValue(g_TlsIndex);

	if (profilePtr == NULL)
	{
		profilePtr = new stProfile[10];
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

		if (profilePtr[idx].state == PROFILESTATE::PROFILE_RESET)
		{
			break;
		}
		if (profilePtr[idx].state == PROFILESTATE::PROFILE_ERROR)
		{
			return;
		}
		if (profilePtr[idx].state == PROFILESTATE::PROFILE_BEGIN)
		{
			// Begin - Begin 시, call에 -1 등록.
			profilePtr[idx].call = -1;
			profilePtr[idx].state = PROFILESTATE::PROFILE_ERROR;
			return;
		}

		break;
	}

	if (idx == -1)
	{
		idx = profilePtr[0].profileArrSize;

		AcquireSRWLockExclusive(&profilePtr[idx].profileResetLock);

		strcpy_s(profilePtr[idx].tagName, 64, targetName);
		profilePtr[idx].max = 0;
		profilePtr[idx].min = INT64_MAX;
		profilePtr[idx].call = 0;
		profilePtr[idx].total = 0;

		profilePtr[idx].threadID = GetCurrentThreadId();

		profilePtr[0].profileArrSize++;

		ReleaseSRWLockExclusive(&profilePtr[idx].profileResetLock);
	}

	AcquireSRWLockExclusive(&profilePtr[idx].profileResetLock);

	QueryPerformanceCounter(&profilePtr[idx].start);
	profilePtr[idx].state = PROFILESTATE::PROFILE_BEGIN;

	ReleaseSRWLockExclusive(&profilePtr[idx].profileResetLock);
}

void ProfileEnd(const char* targetName)
{
	stProfile* profilePtr = (stProfile*)TlsGetValue(g_TlsIndex);
	if (profilePtr == NULL)
		return;

	int idx = -1;
	for (int i = 0; i < profilePtr[0].profileArrSize; i++)
	{
		if (strcmp(profilePtr[i].tagName, targetName) != 0)
			continue;

		idx = i;

		AcquireSRWLockExclusive(&profilePtr[idx].profileResetLock);

		if (profilePtr[idx].state == PROFILESTATE::PROFILE_RESET)
		{
			ReleaseSRWLockExclusive(&profilePtr[idx].profileResetLock);
			return;
		}
		if (profilePtr[idx].state == PROFILESTATE::PROFILE_ERROR)
		{
			ReleaseSRWLockExclusive(&profilePtr[idx].profileResetLock);
			return;
		}
		if (profilePtr[idx].state == PROFILESTATE::PROFILE_END)
		{
			// End - End 시, call에 -2 등록.
			profilePtr[idx].call = -2;
			profilePtr[idx].state = PROFILESTATE::PROFILE_ERROR;

			ReleaseSRWLockExclusive(&profilePtr[idx].profileResetLock);
			return;
		}

		break;
	}

	if (idx < 0)
	{
		// TODO: 크래쉬를 발생시킬까...
		return;
	}

	LARGE_INTEGER end;
	QueryPerformanceCounter(&end);

	profilePtr[idx].call++;

	DWORD64 timeDiff = (end.QuadPart - profilePtr[idx].start.QuadPart);
	profilePtr[idx].total += timeDiff;

	if (profilePtr[idx].max < timeDiff)
		profilePtr[idx].max = timeDiff;

	if (profilePtr[idx].min > timeDiff)
		profilePtr[idx].min = timeDiff;

	profilePtr[idx].state = PROFILESTATE::PROFILE_END;

	ReleaseSRWLockExclusive(&profilePtr[idx].profileResetLock);
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
		"ThreadID", "Name", "Average (㎲)", "Min (㎲)", "Max (㎲)", "Call");
	fprintf(fp,
		"----------------------------------------------------------------------------------------------------------------------------------\n");

	AcquireSRWLockExclusive(&listLock);

	list<stProfile*>::iterator iter;
	for (iter = profileList.begin(); iter != profileList.end(); ++iter)
	{
		stProfile* profilePtr = *iter;

		int profileArrSize = profilePtr[0].profileArrSize;

		for (int i = 0; i < profileArrSize; i++)
		{
			if (profilePtr[i].call == -1 || profilePtr[i].call == -2)
			{
				fprintf(fp, "%-12u | %-20s | %15s | %15s | %15s | %10d\n",
					profilePtr[i].threadID,
					profilePtr[i].tagName,
					"-", "-", "-", profilePtr[i].call);
				continue;
			}

			double average = 0;
			DWORD64 maxSum = profilePtr[i].max;
			DWORD64 minSum = profilePtr[i].min;
			DWORD64 call = profilePtr[i].call;

			if (call == 0)
				return;

			if (call > 2)
			{
				call = profilePtr[i].call - 2;
				average = (double)(profilePtr[i].total - (maxSum + minSum)) / call;
			}
			else
			{
				average = (double)(profilePtr[i].total) / call;
			}

			double minTime = (double)profilePtr[i].min * MICROSEC / frequency.QuadPart;
			double maxTime = (double)profilePtr[i].max * MICROSEC / frequency.QuadPart;
			double avgTime = average * MICROSEC / frequency.QuadPart;

			fprintf(fp, "%-12u | %-20s | %15.4f | %15.4f | %15.1f | %10lld\n",
				profilePtr[i].threadID,
				profilePtr[i].tagName,
				avgTime,
				minTime,
				maxTime,
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

void ProfileReset()
{
	AcquireSRWLockExclusive(&listLock);

	list<stProfile*>::iterator iter;
	for (iter = profileList.begin(); iter != profileList.end(); ++iter)
	{
		stProfile* tmp = *iter;
		int profileArrSize = tmp[0].profileArrSize;

		for (int i = 0; i < profileArrSize; i++)
		{
			AcquireSRWLockExclusive(&tmp[i].profileResetLock);

			tmp[i].start.QuadPart = 0;

			tmp[i].call = 0;
			tmp[i].total = 0;
			tmp[i].max = 0;
			tmp[i].min = INT64_MAX;

			tmp[i].state = PROFILESTATE::PROFILE_RESET;

			ReleaseSRWLockExclusive(&tmp[i].profileResetLock);
		}
	}

	ReleaseSRWLockExclusive(&listLock);
}

void ProfilerInput()
{
	if (GetAsyncKeyState(VK_UP))
	{
		// TODO: 누르는 시간을 받아서 제목으로 만들까
		char fileName[64] = "ProfilerResult.txt";
		ProfileDataOutText(fileName);
	}

	if (GetAsyncKeyState(VK_DOWN))
	{
		ProfileReset();
	}
}

stProfile::stProfile()
{
	tagName[0] = NULL;
	start.QuadPart = 0;

	call = 0;
	total = 0;
	max = 0;
	min = INT64_MAX;

	state = PROFILESTATE::PROFILE_RESET;

	profileArrSize = 0;
	threadID = 0;

	InitializeSRWLock(&profileResetLock);
}

stProfile::~stProfile()
{
}
