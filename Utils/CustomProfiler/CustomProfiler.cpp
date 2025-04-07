#include <iostream>
#include <Windows.h>
#include "CustomProfiler.h"

using namespace std;

struct stProfile
{
	char tagName[64];
	LARGE_INTEGER start;

	__int64 call;
	__int64 total;
	__int64 max;
	__int64 min;

	bool isUsing = false;
};

stProfile profileArr[20];
int profileArrSize = 0;

#define MICROSEC 1000000

void ProfileDataOutText(char* szFileName);
void ProfileReset();

void ProfileBegin(const char* targetName)
{
	int idx = -1;
	for (int i = 0; i < profileArrSize; i++)
	{
		if (strcmp(profileArr[i].tagName, targetName) != 0)
			continue;

		idx = i;
		if (profileArr[idx].isUsing == true)
		{
			// Begin - Begin 시, call에 -1 등록.
			profileArr[idx].call = -1;
			return;
		}
		if (profileArr[idx].call == -2)
		{
			return;
		}

		break;
	}

	if (idx == -1)
	{
		idx = profileArrSize;

		strcpy_s(profileArr[idx].tagName, 64, targetName);
		profileArr[idx].max = INT64_MIN;
		profileArr[idx].min = INT64_MAX;
		profileArrSize++;
	}

	QueryPerformanceCounter(&profileArr[idx].start);	

	profileArr[idx].isUsing = true;
}

void ProfileEnd(const char* targetName)
{
	int idx = -1;
	for (int i = 0; i < profileArrSize; i++)
	{
		if (strcmp(profileArr[i].tagName, targetName) != 0)
			continue;

		idx = i;
		if (profileArr[idx].isUsing == false)
		{
			// End - End 시, call에 -2 등록.
			profileArr[idx].call = -2;
			return;
		}
		if (profileArr[idx].call == -1)
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

	profileArr[idx].call++;

	__int64 timeDiff = (end.QuadPart - profileArr[idx].start.QuadPart);
	profileArr[idx].total += timeDiff;

	if (profileArr[idx].max < timeDiff)
		profileArr[idx].max = timeDiff;

	if (profileArr[idx].min > timeDiff)
		profileArr[idx].min = timeDiff;

	profileArr[idx].isUsing = false;
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

void ProfileDataOutText(char* szFileName)
{
	// TODO: 수정. 한번만 딱 얻어오는 방식으로 바꾸자.
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);

	FILE* fp;
	errno_t errChk = fopen_s(&fp, szFileName, "wt");
	if (errChk != 0)
	{
		printf("ERROR: fopen_s\n");
		return;
	}

	char buf[200];
	sprintf_s(buf, "%15s  |%15s   |%15s   |%15s   |%10s  |\n",
		"Name",
		"Average",
		"Min",
		"Max",
		"Call");
	fwrite(buf, strlen(buf), 1, fp);

	for (int i = 0; i < profileArrSize; i++)
	{
		if (profileArr[i].call == -1 || profileArr[i].call == -2)
		{
			sprintf_s(buf, "%15s  |%15.4d㎲ |%15.4d㎲ |%15.4d㎲ |%10d  |\n",
				profileArr[i].tagName,
				-1,
				-1,
				-1,
				profileArr[i].call);
			
			fwrite(buf, strlen(buf), 1, fp);
			continue;
		}

		double average = 0;
		__int64 maxSum = 0;
		__int64 minSum = 0;
		__int64 call = profileArr[i].call - 2;
		
		maxSum += profileArr[i].max;
		minSum += profileArr[i].min;

		average = (double)(profileArr[i].total - (maxSum + minSum)) / call;

		double minTime = (double)profileArr[i].min * MICROSEC;
		minTime = minTime / frequency.QuadPart;

		double maxTime = (double)profileArr[i].max * MICROSEC;
		maxTime = maxTime / frequency.QuadPart;

		sprintf_s(buf, "%15s  |%15.4f㎲ |%15.4f㎲ |%15.4f㎲ |%10lld  |\n",
			profileArr[i].tagName,
			average  * MICROSEC / frequency.QuadPart,
			minTime,
			maxTime,
			call);

		fwrite(buf, strlen(buf), 1, fp);
	}

	fclose(fp);
}

void ProfileReset()
{
	for (int i = 0; i < profileArrSize; i++)
	{
		profileArr[i].start.QuadPart = 0;
		profileArr[i].call = 0;
		profileArr[i].total = 0;
		profileArr[i].max = INT64_MIN;
		profileArr[i].min = INT64_MAX;
		profileArr[i].isUsing = false;
	}
}


void Input()
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

void Logic()
{
	PRO_BEGIN("TEST1");
	for (int i = 0; i < 1000; i++)
	{
		int a = 3;
	}
	PRO_END("TEST1");

	PRO_BEGIN("TEST2");
	for (int i = 0; i < 3000; i++)
	{
		
	}
	PRO_END("TEST2");
}

int main()
{
	while (1)
	{
		Input();

		Logic();
	};

	return 0;
}