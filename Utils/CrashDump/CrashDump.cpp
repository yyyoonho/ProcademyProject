#include <Windows.h>
#include <iostream>

#include "CrashDump.h"

using namespace std;

int main()
{
	procademy::CCrashDump dump;

	int a = 0;
	int* p = &a;
	p = NULL;

	*p = 10;

	return 0;
}
