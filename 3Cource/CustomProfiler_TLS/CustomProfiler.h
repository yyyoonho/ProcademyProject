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
