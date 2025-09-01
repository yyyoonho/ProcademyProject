#include <iostream>
#include <Windows.h>
#include <list>
#include "CustomProfiler.h"

using namespace std;

#define MICROSEC 1000000

struct stProfile
{
	char tagName[64];
	LARGE_INTEGER start;

	__int64 call;
	__int64 total;
	__int64 max;
	__int64 min;

	bool isUsing = false;

	int profileArrSize = 0;
};


int tlsIndex = -1;

list<stProfile*> profileList;
SRWLOCK listLock;

LARGE_INTEGER frequency;

void ProfileBegin(const char* targetName)
{
	if (tlsIndex == -1)
	{
		tlsIndex = TlsAlloc();
		if (tlsIndex == TLS_OUT_OF_INDEXES)
		{
			printf("Error: TlsAlloc() %d\n", GetLastError());
			return;
		}

		InitializeSRWLock(&listLock);

		QueryPerformanceFrequency(&frequency);
	}

	stProfile* profilePtr = (stProfile*)TlsGetValue(tlsIndex);

	if (profilePtr == NULL)
	{
		profilePtr = new stProfile[10];
		TlsSetValue(tlsIndex, (LPVOID)profilePtr);

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
		if (profilePtr[idx].isUsing == true)
		{
			// Begin - Begin 시, call에 -1 등록.
			profilePtr[idx].call = -1;
			return;
		}
		if (profilePtr[idx].call == -2)
		{
			return;
		}

		break;
	}

	if (idx == -1)
	{
		idx = profilePtr[0].profileArrSize;

		strcpy_s(profilePtr[idx].tagName, 64, targetName);
		profilePtr[idx].max = INT64_MIN;
		profilePtr[idx].min = INT64_MAX;
		profilePtr[0].profileArrSize++;
	}

	QueryPerformanceCounter(&profilePtr[idx].start);

	profilePtr[idx].isUsing = true;
}

void ProfileEnd(const char* targetName)
{
	stProfile* profilePtr = (stProfile*)TlsGetValue(tlsIndex);

	int idx = -1;
	for (int i = 0; i < profilePtr[0].profileArrSize; i++)
	{
		if (strcmp(profilePtr[i].tagName, targetName) != 0)
			continue;

		idx = i;
		if (profilePtr[idx].isUsing == false)
		{
			// End - End 시, call에 -2 등록.
			profilePtr[idx].call = -2;
			return;
		}
		if (profilePtr[idx].call == -1)
		{
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

	__int64 timeDiff = (end.QuadPart - profilePtr[idx].start.QuadPart);
	profilePtr[idx].total += timeDiff;

	if (profilePtr[idx].max < timeDiff)
		profilePtr[idx].max = timeDiff;

	if (profilePtr[idx].min > timeDiff)
		profilePtr[idx].min = timeDiff;

	profilePtr[idx].isUsing = false;
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
		"-----------------------------------------------------------------------------------------\n");

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
					GetCurrentThreadId(),
					profilePtr[i].tagName,
					"-", "-", "-", profilePtr[i].call);
				continue;
			}

			double average = 0;
			__int64 maxSum = profilePtr[i].max;
			__int64 minSum = profilePtr[i].min;
			__int64 call = profilePtr[i].call;

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

			fprintf(fp, "%-12u | %-20s | %15.4f | %15.4f | %15.4f | %10lld\n",
				GetCurrentThreadId(),
				profilePtr[i].tagName,
				avgTime,
				minTime,
				maxTime,
				call);
		}

	}

	ReleaseSRWLockExclusive(&listLock);

	fprintf(fp,
		"=========================================================================================\n");

	fclose(fp);
}

void ProfileReset()
{
	list<stProfile*>::iterator iter;
	for (iter = profileList.begin(); iter != profileList.end(); ++iter)
	{
		stProfile* tmp = *iter;

		int profileArrSize = tmp[0].profileArrSize;

		for (int i = 0; i < profileArrSize; i++)
		{
			tmp[i].start.QuadPart = 0;
			tmp[i].call = 0;
			tmp[i].total = 0;
			tmp[i].max = INT64_MIN;
			tmp[i].min = INT64_MAX;
			tmp[i].isUsing = false;
		}

	}
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
