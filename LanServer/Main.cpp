#include "stdafx.h"
#include "LanServerProtocol.h"

#include "LanServer.h"

using namespace std;

class EchoServer : public LanServer
{
public:
	EchoServer();
	~EchoServer();

	virtual bool OnConnectionRequest(in_addr ipAddress, USHORT port);

	virtual void OnAccept(in_addr ipAddress, USHORT port, DWORD64 sessionID);
	virtual void OnRelease(DWORD64 sessionID);

	virtual void OnMessage(DWORD64 sessionID, SerializePacket* sPacket);

	virtual void OnError(int errorcode, WCHAR*);
};


EchoServer::EchoServer()
{
}

EchoServer::~EchoServer()
{
}

bool EchoServer::OnConnectionRequest(in_addr ipAddress, USHORT port)
{
	return true;
}

void EchoServer::OnAccept(in_addr ipAddress, USHORT port, DWORD64 sessionID)
{
	return;
}

void EchoServer::OnRelease(DWORD64 sessionID)
{
	return;
}

void EchoServer::OnMessage(DWORD64 sessionID, SerializePacket* sPacket)
{

	/* 에코를 위한 작업
	* 일부러 직렬화버퍼를 따로 또 만들어서 sendPacket을 해보자.
	*/
	{
		INT64 payload;
		*sPacket >> payload;

		SerializePacket sPacket2;
		sPacket2 << payload;

		SendPacket(sessionID, &sPacket2);
	}

	return;
}

void EchoServer::OnError(int errorcode, WCHAR*)
{
	return;
}


int main()
{
	timeBeginPeriod(1);

	EchoServer server1;
	server1.Start();

	while (1)
	{
		_int64 id = -1;
		if(id!=-1)
			server1.Disconnect(id);

		if (_kbhit())
		{
			char input = _getch();

			if (input == 'Q' || input == 'q')
			{
				// TODO: 종료 유도.
				server1.Stop();

				break;
			}
		}
	}

	return 0;
}