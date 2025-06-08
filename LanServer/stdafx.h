#pragma once

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "RingBuffer.lib")
#pragma comment(lib, "SerializeBuffer.lib")
#pragma comment(lib, "MemoryPool.lib")
#pragma comment(lib, "CustomProfiler.lib")

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <iostream>
#include <thread>
#include <conio.h>
#include <vector>
#include <list>
#include <unordered_map>
#include <algorithm>
#include <stack>

#include "RingBuffer.h"
#include "SerializeBuffer.h"
#include "MemoryPool.h"
#include "CustomProfiler.h"