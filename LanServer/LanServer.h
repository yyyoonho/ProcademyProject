#pragma once

class LanServer
{
public:
	LanServer();
	~LanServer();

public:
	bool Start();
	void Stop();

	int GetSessionCount();

	bool Disconnect(DWORD64 sessionID);
	bool SendPacket(DWORD64 sessionID, SerializePacket* sPacket);

	// 이벤트 함수
	virtual bool OnConnectionRequest() = 0;

	virtual void OnAccept() = 0;
	virtual void OnRelease() = 0;

	virtual void OnMessage() = 0;

	virtual void OnError() = 0;

	// 모니터링 함수
	int GetAcceptTPS();
	int GetRecvMessageTPS();
	int GetSendMessageTPS();

};
