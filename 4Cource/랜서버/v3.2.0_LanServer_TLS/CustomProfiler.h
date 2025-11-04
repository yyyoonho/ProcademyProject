#pragma once

void ProfileBegin(const char* targetName);
void ProfileEnd(const char* targetName);
void ProfilerInput();

// TODO:
// 4. 객체생성소멸 이용을 위한 클래스 생성 (선택)
// 5. 파일저장시, 이름을 현재시간으로 만들기 

#define PROFILE

#ifdef PROFILE
#define PRO_BEGIN(TagName) ProfileBegin(TagName)
#define PRO_END(TagName) ProfileEnd(TagName)
#elif
#define PRO_BEGIN(TagName)  
#define PRO_END(TagName)  
#endif
