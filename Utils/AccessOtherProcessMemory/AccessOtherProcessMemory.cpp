#include <Windows.h>
#include <iostream>
#pragma comment(lib, "winmm.lib")

using namespace std;


int main()
{
	DWORD ProcessID = 0;
	printf("접근할 프로세스의 PID를 입력하세요: ");
	scanf_s("%d", &ProcessID);
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessID);

	int targetWord = 0;
	printf("찾을 값을 입력하세요: ");
	scanf_s("%d", &targetWord);

	SYSTEM_INFO SystemInfo;
	GetSystemInfo(&SystemInfo);
	LPVOID pMemory = SystemInfo.lpMinimumApplicationAddress;
	
	while (1)
	{
		if (pMemory > SystemInfo.lpMaximumApplicationAddress)
			break;

		MEMORY_BASIC_INFORMATION MemoryBasicInfo;
		SIZE_T size = VirtualQueryEx(hProcess, pMemory, &MemoryBasicInfo, sizeof(MemoryBasicInfo));

		if (MemoryBasicInfo.State == MEM_COMMIT && MemoryBasicInfo.Type == MEM_PRIVATE)
		{
			BYTE* buf = new BYTE[MemoryBasicInfo.RegionSize];

			bool check = ReadProcessMemory(hProcess, pMemory, (void*)buf, MemoryBasicInfo.RegionSize, NULL);
			if (check == false)
			{
				printf("fail: ReadProcessMemory()\n");
			}

			for (int i = 0; i < MemoryBasicInfo.RegionSize; i += 4)
			{
				int* tmp = (int*)(buf+i);

				if (*tmp == targetWord)
				{
					int changeWord = 0;
					printf("찾은 값을 입력한 값으로 변경합니다: ");
					scanf_s("%d", &changeWord);

					WriteProcessMemory(hProcess, (BYTE*)pMemory + i, &changeWord, 4, NULL);
				}
			}

			delete[] buf;
		}
		
		pMemory = (BYTE*)pMemory + MemoryBasicInfo.RegionSize;
	}

}
