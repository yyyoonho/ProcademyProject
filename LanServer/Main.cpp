#include "stdafx.h"
#include "LanServer.h"

using namespace std;

class TestServer : public LanServer
{
public:
	TestServer();
	~TestServer();

	virtual bool OnConnectionRequest(in_addr ipAddress, USHORT port);

	virtual void OnAccept(in_addr ipAddress, USHORT port, DWORD64 sessionID);
	virtual void OnRelease(DWORD64 sessionID);

	virtual void OnMessage(DWORD64 sessionID, SerializePacket* sPacket);

	virtual void OnError(int errorcode, WCHAR*);
};


TestServer::TestServer()
{
}

TestServer::~TestServer()
{
}

bool TestServer::OnConnectionRequest(in_addr ipAddress, USHORT port)
{
	return true;
}

void TestServer::OnAccept(in_addr ipAddress, USHORT port, DWORD64 sessionID)
{
	return;
}

void TestServer::OnRelease(DWORD64 sessionID)
{
	return;
}

void TestServer::OnMessage(DWORD64 sessionID, SerializePacket* sPacket)
{
	return;
}

void TestServer::OnError(int errorcode, WCHAR*)
{
	return;
}


int main()
{
	timeBeginPeriod(1);

	TestServer server1;
	server1.Start();

	while (1)
	{
		int a = 3;
	}

	return 0;
}