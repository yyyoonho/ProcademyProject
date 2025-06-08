#include "stdafx.h"
#include "LanServer.h"

#include "EchoServer.h"

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