#pragma once

/* New/Delete 오버로딩 */
void* operator new (size_t size, const char* File, int Line);

void* operator new[](size_t size, const char* File, int Line);

void operator delete (void* p, char* File, int Line);

void operator delete[](void* p, char* File, int Line);

void operator delete (void* p);

void operator delete[](void* p);

/* 매크로 */
#define new new(__FILE__,__LINE__)