#pragma once

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "RingBuffer.lib")
#pragma comment(lib, "CustomProfiler.lib")
#pragma comment(lib, "Net_SerializeBuffer.lib")
#pragma comment(lib, "SerializeBufferPtr.lib")
#pragma comment(lib, "SerializePacketPtr_RingBuffer.lib")

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <stdio.h>
#include <iostream>
#include <thread>
#include <list>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <conio.h>
#include <queue>

#include "RingBuffer.h"
#include "CustomProfiler.h"

#include "MemoryPool.h"
#include "TLS_MemoryPool.h"


#include "Net_SerializeBuffer.h"
#include "SerializeBufferPtr.h"
#include "SerializePacketPtr_RingBuffer.h"

#include "LockFree_Queue.h"
#include "LockFreeStack.h"

#include "Struct.h"

#include "Protocol.h"