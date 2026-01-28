#pragma once

// 사용 방법
// DB_PRO_BEGIN 호출 시 프로파일링이 자동으로 시작됩니다.
// Record 수집은 별도 호출 없이 자동으로 수행됩니다.
// Record 주기는 하단의 RECORDTIME 값을 수정하여 조정할 수 있습니다.
// 프로젝트 종료 전에 반드시 DBProfilerRecordStop()을 호출해야 합니다.


void DBProfileBegin(const char* targetName);
void DBProfileEnd(const char* targetName);
void DBProfilerRecordStop();

#define DB_PROFILE
#define RECORDTIME 1000 * 60 * 10

#ifdef DB_PROFILE
#define DB_PRO_BEGIN(TagName) DBProfileBegin(TagName)
#define DB_PRO_END(TagName) DBProfileEnd(TagName)
#elif
#define DB_PRO_BEGIN(TagName)  
#define DB_PRO_END(TagName)  
#endif

enum class DB_PROFILESTATE
{
	DB_PROFILE_BEGIN,
	DB_PROFILE_END,
	DB_PROFILE_RESET,
	DB_PROFILE_ERROR,
};

class stDBProfile
{
public:
	stDBProfile();
	~stDBProfile();

	char tagName[64];
	//LARGE_INTEGER start;
	DWORD start;

	DWORD64 call;
	DWORD64 total;
	DWORD max;
	DWORD min;

	DB_PROFILESTATE state;

	int profileArrSize;
	DWORD threadID;
};