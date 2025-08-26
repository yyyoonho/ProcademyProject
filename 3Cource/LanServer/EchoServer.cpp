#include "stdafx.h"
#include "LanServer.h"
#include "EchoServer.h"

using namespace std;

bool CEchoServer::OnConnectionRequest(SOCKADDR_IN clientAddr)
{
	return true;
}

void CEchoServer::OnAccept(DWORD64 sessionID)
{
}

void CEchoServer::OnRelease(DWORD64 sessionID)
{
}

void CEchoServer::OnMessage(DWORD64 sessionID, SerializePacket* pSPacket)
{
	// 데이터 추출
	stMessage msg;
	pSPacket->GetData((char*)&msg, sizeof(stMessage));

	// 직렬화버퍼에 데이터 삽입
	SerializePacket newSPacket;
	newSPacket.Putdata((char*)&msg, sizeof(stMessage));

	SendPacket(sessionID, &newSPacket);
}

void CEchoServer::OnError(int errorCode, WCHAR* errorComment)
{
}
