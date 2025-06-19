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
	{
		INT64 payload;
		*sPacket >> payload;

		//SerializePacket* sPacket2 = new SerializePacket;
		SerializePacket* sPacket2 = sPacketMP.Alloc();
		sPacket2->Clear();
		(*sPacket2) << payload;

		SendPacket(sessionID, sPacket2);
	}

	return;
}

void EchoServer::OnError(int errorcode, WCHAR*)
{
	return;
}