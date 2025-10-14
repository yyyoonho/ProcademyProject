#pragma once

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "RingBuffer.lib")
#pragma comment(lib, "CustomProfiler.lib")
#pragma comment(lib, "SerializeBuffer.lib")
#pragma comment(lib, "SerializeBufferPtr.lib")

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <stdio.h>
#include <iostream>
#include <thread>
#include <list>
#include <vector>
#include <map>
#include <unordered_map>
#include <stack>
#include <conio.h>
#include <queue>

#include "RingBuffer.h"
#include "CustomProfiler.h"

#include "MemoryPool.h"
#include "SerializeBuffer.h"
#include "SerializeBufferPtr.h"

#include "Struct.h"

#include "Protocol.h"