#include <Windows.h>
#include "ND1.h"
#include <iostream>

#undef new

using namespace std;

/* 메모리 할당 정보 구조체*/
struct AllocInfo
{
    void* ptr;
    int size;
    char fileName[64];
    int line;
    bool array;
};

enum class LOGTYPE
{
    NOALLOC,
    ARRAY,
    LEAK,
};

class ND_Helper
{
public:
    ND_Helper();
    ~ND_Helper();

public:
    void CreateAllocInfo(void* ptr, int size, const char* File, int Line, bool array);
    bool SearchPtrInAllocInfoArr(void* ptr, OUT int* idx);
    void RemoveAllocInfo(int idx);
    bool IsArray(int idx);

    void WriteLogToFile(LOGTYPE logType, int idx, void* p);

private:
    void CreateLogFileName();

private:
    int _count;
    AllocInfo _allocInfoArr[100];
    char _logFileName[100];
};

ND_Helper gND_Helper;

ND_Helper::ND_Helper() : _count(0)
{
    for (int i = 0; i < 100; i++)
    {
        _allocInfoArr[i].ptr = nullptr;
    }
    
    CreateLogFileName();
}

ND_Helper::~ND_Helper()
{
    for (int i = 0; i < 100; i++)
    {
        if (_allocInfoArr[i].ptr == nullptr)
            continue;

        WriteLogToFile(LOGTYPE::LEAK, i, _allocInfoArr[i].ptr);
    }
}

void ND_Helper::CreateAllocInfo(void* ptr, int size, const char* File, int Line, bool array)
{
    _allocInfoArr[_count].ptr = ptr;
    _allocInfoArr[_count].size = size;
    strcpy_s(_allocInfoArr[_count].fileName, 64, File);
    _allocInfoArr[_count].line = Line;
    _allocInfoArr[_count].array = array;

    _count++;
}

bool ND_Helper::SearchPtrInAllocInfoArr(void* ptr, OUT int* idx)
{
    for (int i = 0; i < _count; i++)
    {
        if (_allocInfoArr[i].ptr == nullptr)
            continue;

        if (_allocInfoArr[i].ptr == ptr)
        {
            *idx = i;
            return true;
        }
    }

    return false;
}

void ND_Helper::RemoveAllocInfo(int idx)
{
    _allocInfoArr[idx].ptr = nullptr;
}

bool ND_Helper::IsArray(int idx)
{
    return _allocInfoArr[idx].array;
}

void ND_Helper::WriteLogToFile(LOGTYPE logType, int idx, void* p)
{
    FILE* fp;
    errno_t errChk = fopen_s(&fp, _logFileName, "at");
    if (errChk != 0)
    {
        printf("ERROR: fopen_s\n");
        return;
    }

    char logBuf[200];

    switch (logType)
    {
    case LOGTYPE::ARRAY:
        sprintf_s(logBuf, 200, "ARRAY [0x%p] [%8d] ", p, _allocInfoArr[idx].size);
        strcat_s(logBuf, 200, _allocInfoArr[idx].fileName);
        strcat_s(logBuf, 200, " : ");
        sprintf_s(logBuf + strlen(logBuf), 200, "%d\n", _allocInfoArr[idx].line);

        fwrite(logBuf, strlen(logBuf), 1, fp);
        break;

    case LOGTYPE::NOALLOC:
        sprintf_s(logBuf, 200, "NOALLOC [0x%p] \n", p);
        fwrite(logBuf, strlen(logBuf), 1, fp);
        break;

    case LOGTYPE::LEAK:
        sprintf_s(logBuf, 200, "LEAK [0x%p] [%8d] ", p, _allocInfoArr[idx].size);
        strcat_s(logBuf, 200, _allocInfoArr[idx].fileName);
        strcat_s(logBuf, 200, " : ");
        sprintf_s(logBuf + strlen(logBuf), 200, "%d\n", _allocInfoArr[idx].line);

        fwrite(logBuf, strlen(logBuf), 1, fp);
        break;
    }

    fclose(fp);
}

void ND_Helper::CreateLogFileName()
{
    struct tm date;
    time_t now = time(NULL);
    localtime_s(&date, &now);

    sprintf_s(_logFileName, 100, "Alloc_%d%02d%02d_%02d%02d%02d.txt",
        date.tm_year + 1900,
        date.tm_mon + 1,
        date.tm_mday,
        date.tm_hour,
        date.tm_min,
        date.tm_sec
    );
}

void* operator new(size_t size, const char* File, int Line)
{
    void* tmpPtr = malloc(size);

    gND_Helper.CreateAllocInfo(tmpPtr, size, File, Line, false);

    return tmpPtr;
}

void* operator new[](size_t size, const char* File, int Line)
{
    void* tmpPtr = malloc(size + 4);

    gND_Helper.CreateAllocInfo(tmpPtr, size, File, Line, true);

    return tmpPtr;
}

void operator delete(void* p, char* File, int Line)
{
}

void operator delete[](void* p, char* File, int Line)
{
}

void operator delete(void* p)
{
    int idx = -1;
    bool searchPtr = gND_Helper.SearchPtrInAllocInfoArr(p, &idx);
  
    if (searchPtr == true)
    {
        // 소멸자 X, Array O
        if (gND_Helper.IsArray(idx))
        {
            gND_Helper.WriteLogToFile(LOGTYPE::ARRAY, idx, p);
        }
        // 소멸자 X, Array X
        else
        {
            gND_Helper.RemoveAllocInfo(idx);
            free(p);
        }
        
        return;
    }
    
    else
    {
        void* tmpPtr = (BYTE*)p - 4;
        searchPtr = gND_Helper.SearchPtrInAllocInfoArr(tmpPtr, &idx);

        // 소멸자 O, Array O
        if (searchPtr == true)
        {
            gND_Helper.WriteLogToFile(LOGTYPE::ARRAY, idx, p);
        }
        // NoInfo
        else
        {
            gND_Helper.WriteLogToFile(LOGTYPE::NOALLOC, idx, p);
        }
    }
}

void operator delete[](void* p)
{
    int idx = -1;
    bool searchPtr = gND_Helper.SearchPtrInAllocInfoArr(p, &idx);
    
    if (searchPtr == true)
    {
        if (gND_Helper.IsArray(idx))
        {
            gND_Helper.RemoveAllocInfo(idx);
            free(p);
        }
        else
        {
            gND_Helper.WriteLogToFile(LOGTYPE::ARRAY, idx, p);
        }
        return;
    }
    else
    {
        gND_Helper.WriteLogToFile(LOGTYPE::NOALLOC, idx, p);
    }

}

int main()
{

    return 0;
}