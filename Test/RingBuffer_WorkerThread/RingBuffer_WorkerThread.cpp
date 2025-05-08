#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "RingBuffer.lib")

#include <Windows.h>
#include <iostream>
#include <list>
#include <conio.h>

#include "RingBuffer.h"

using namespace std;

struct st_MSG_HEAD
{
    short shType;
    short shPayloadLen;
};

#define dfJOB_ADD	0
#define dfJOB_DEL	1
#define dfJOB_SORT	2
#define dfJOB_FIND	3
#define dfJOB_PRINT	4	// 출력 or 저장 / 읽기만 하는 느린 행동
#define dfJOB_QUIT	5	

//-----------------------------------------------
// 컨텐츠 부, 문자열 리스트
//-----------------------------------------------
list<wstring>		g_List;

//-----------------------------------------------
// 스레드 메시지 큐 (사이즈 넉넉하게 크게 4~5만 바이트)
//-----------------------------------------------
RingBuffer 		g_msgQ(40000);

void WorkerThread()
{

}

void MonitorThread()
{

}

void Frame()
{
    static DWORD oldTime = timeGetTime();

    DWORD diffTime = timeGetTime() - oldTime;

    if (diffTime < 50)
    {
        Sleep(50 - diffTime);
    }

    oldTime += 50;
    return;
}

int main()
{
    timeBeginPeriod(1);

    while (1)
    {
        Frame();

        // TODO: 메시지 생성

        // TODO: 메시지 인큐
        {

        }

        
        char key = _getch();
        if (key == 'Q' || key == 'q')
        {
            // TODO: JOB_QUIT 메시지 인큐
        }
    }

    timeEndPeriod(1);
}