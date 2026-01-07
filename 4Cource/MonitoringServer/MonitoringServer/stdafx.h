#pragma once

#pragma comment(lib, "libmysql.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "RingBuffer.lib")
#pragma comment(lib, "CustomProfiler.lib")

//#pragma comment (lib, "cpp_redis.lib")
//#pragma comment (lib, "tacopie.lib")

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include "mysql/include/mysql.h"
#include "mysql/include/errmsg.h"
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
#include <mutex>
#include <string>
//#include <cpp_redis/cpp_redis>

#include "CrashDump.h"

#include "RingBuffer.h"
#include "CustomProfiler.h"

#include "MemoryPool.h"
#include "TLS_MemoryPool.h"


#include "Net_SerializeBuffer.h"
#include "SerializeBufferPtr.h"

#include "LockFree_Queue.h"
#include "LockFreeStack.h"

#include "Struct.h"


#include "Protocol.h"


#include "NetCodec.h"