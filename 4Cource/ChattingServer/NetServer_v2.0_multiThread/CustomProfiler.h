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

enum class PROFILESTATE
{
	PROFILE_BEGIN,
	PROFILE_END,
	PROFILE_RESET,
	PROFILE_ERROR,
};

class stProfile
{
public:
	stProfile();
	~stProfile();
	
	char tagName[64];
	LARGE_INTEGER start;

	DWORD64 call;
	DWORD64 total;
	DWORD64 max;
	DWORD64 min;

	PROFILESTATE state;

	int profileArrSize;
	DWORD threadID;
};