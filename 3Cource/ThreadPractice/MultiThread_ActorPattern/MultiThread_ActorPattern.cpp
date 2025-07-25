#pragma comment(lib, "RingBuffer.lib")

#include <Windows.h>
#include <iostream>
#include <thread>
#include <list>

#include "RingBuffer.h"
#include "Protocol.h"

using namespace std;

wstring str[5] = { L"HANHWA", L"LG", L"LOTTE", L"KIA", L"KT" };

//-----------------------------------------------
// 컨텐츠 부, 문자열 리스트
//-----------------------------------------------
list<wstring>		g_List;

//-----------------------------------------------
// 스레드 메시지 큐 (사이즈 넉넉하게 크게 4~5만 바이트)
//-----------------------------------------------
RingBuffer 			g_msgQ(50000);


int main()
{
	timeBeginPeriod(1);

	while (1)
	{
		// 메시지 생성

		// 메시지 인큐

	}


	return 0;
}