#pragma once

class CEchoServer : public CLanServer
{
public:
	// CLanServer을(를) 통해 상속됨
	virtual bool OnConnectionRequest(SOCKADDR_IN clientAddr) override;

	virtual void OnAccept(INT64 sessionID) override;

	virtual void OnRelease(INT64 sessionID) override;

	virtual void OnMessage(INT64 sessionID, SerializePacket* SPacket) override;

	virtual void OnError(int errorCode, WCHAR* errorComment) override;

};