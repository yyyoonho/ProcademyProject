#pragma once

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

