#pragma once

void ProfileBegin(const char* targetName);
void ProfileEnd(const char* targetName);
void ProfilerInput();

#define PROFILE

#ifdef PROFILE
#define PRO_BEGIN(TagName) ProfileBegin(TagName)
#define PRO_END(TagName) ProfileEnd(TagName)
#elif
#define PRO_BEGIN(TagName)  
#define PRO_END(TagName)  
#endif

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
	DWORD threadID;
};