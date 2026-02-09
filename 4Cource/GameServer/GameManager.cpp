#include "stdafx.h"

#include "CustomProfiler.h"

#include "Monitoring.h"
#include "Field.h"
#include "NetServer.h"
#include "SendJob.h"
#include "NetClient.h"
#include "NetClient_Monitoring.h"
#include "GameManager.h"


unsigned int GetIdxFromSessionID(DWORD64 sessionID);

GameManager::GameManager()
{
}

GameManager::~GameManager()
{
}

bool GameManager::Start(const WCHAR* ipAddress, unsigned short port, unsigned short workerThreadCount, unsigned short coreSkip, bool isNagle, unsigned int maximumSessionCount, bool codecOnOff, BYTE fixedKey, BYTE code)
{
	bool ret = CNetServer::Start(ipAddress, port, workerThreadCount, coreSkip, isNagle, maximumSessionCount, codecOnOff, fixedKey, code);
	if (ret == false)
	{
		return false;
	}

	hEvent_Quit = CreateEvent(NULL, TRUE, FALSE, NULL);
	monitoringThread = thread(&GameManager::MonitorThread, this);

	for (int i = 0; i < 3; i++)
	{
		RingBuffer* a = new RingBuffer;
		sendPacketQ.push_back(a);

		sendPacketQ[i]->Resize(10000000);

		sendThreads.emplace_back(
			&GameManager::SendPacketJobThread,
			this,
			i);
	}

	return true;
}

void GameManager::Stop()
{
	CNetServer::Stop();

	SetEvent(hEvent_Quit);
}

bool GameManager::OnConnectionRequest(SOCKADDR_IN clientAddr)
{
	return false;
}

void GameManager::OnAccept(DWORD64 sessionID)
{
	
}

void GameManager::OnRelease(DWORD64 sessionID)
{
	


}

void GameManager::OnMessage(DWORD64 sessionID, SerializePacketPtr pPacket)
{
	
}

void GameManager::OnError(int errorCode, WCHAR* errorComment)
{
}


void GameManager::OnAccept_GameManager(DWORD64 sessionID, Session* pSession)
{
	// OnAccept -> 첫 접속
	// AuthTh의 큐에 삽입.

	// 세션 수명관리 테스트
	IncreaseIO_Count(pSession);

	FieldBundle* pAuthFieldBundle = NULL;

	{
		lock_guard<mutex> lock(fieldBundleMapLock);

		auto iter = fieldBundleMap.find(FieldName::Auth);
		if (iter == fieldBundleMap.end())
		{
			DebugBreak();
		}

		pAuthFieldBundle = iter->second;
	}

	if (pAuthFieldBundle == NULL)
	{
		DebugBreak();
		return;
	}

	{
		lock_guard<mutex> lock(pAuthFieldBundle->threadQ_Create_Lock);
		pAuthFieldBundle->threadQ_Create.Enqueue((char*)&pSession, sizeof(Session*));
	}
}

void GameManager::OnMessage_GameManager(DWORD64 sessionID, Session* pSession, SerializePacketPtr pPacket)
{
	RawPtr rawPtr;
	pPacket.GetRawPtr(&rawPtr);
	rawPtr.IncreaseRefCount();

	pSession->contentMsgQ.Enqueue((char*)&rawPtr, sizeof(rawPtr));
}

void GameManager::OnRelease_GameManager(DWORD64 sessionID, Session* pSession)
{
	// 세션에 표시를 해두자.
	
	// 이후는 콘텐츠에 맡기자.
	// 콘텐츠가 할일: 여기서 표시해둔걸 OnRecv에서 발견-> Release용 벡터에 플레이어 저장.
	// OnLeave 가 실행될대 해당 벡터를 참조. Leave로직 진행.

	pSession->releaseWait = true;
}

void GameManager::FieldThreadFunc(void* param, int id)
{
	FieldBundle* pFieldBundle = (FieldBundle*)param;
	if (pFieldBundle == NULL)
	{
		DebugBreak();
	}

	while (1)
	{
		FrameControl();
		ShowFPS(id);

		//PRO_BEGIN("Total");

		// OnEnter
		// 1. threadQ_Create에서 디큐.
		// 2. session* 를 sessionVec에 등록
		// 3. field의 OnEnter호출 -> 인자로 sessionID넘겨주기.
		// TODO(콘텐츠):
		// 1. 넘어온 sessionID를 Key로 Player생성하기.
		// 2. newPlayer에게 줄 데이터들 sendPacket();
		{
			int useSize = pFieldBundle->threadQ_Create.GetUseSize();
			int size = (useSize / sizeof(Session*));

			for (int i = 0; i < size; i++)
			{
				Session* newSession = NULL;
				
				pFieldBundle->threadQ_Create.Dequeue((char*)&newSession, sizeof(Session*));
				if (newSession == NULL)
				{
					DebugBreak();
				}

				DWORD64 sid = newSession->sessionID;


				pFieldBundle->sessionVec.push_back(newSession);

				// TODO: 3 O
				pFieldBundle->field->OnEnter(sid);
			}
			
		}

		// OnEnterWithPlayer
		// 1. threadQ_Join에서 디큐.
		// 2. session의 contentMsgQ 비우기.
		// 3. session* 를 sessionVec에 등록
		// 4. field의 OnEnter호출 -> 인자로 sessionID넘겨주기.
		// TODO(콘텐츠):
		// 1. 넘어온 sessionID를 Key로 넘어온 Player 그대로 이양받기
		// 2. newPlayer에게 줄 데이터들 sendPacket();
		{
			int useSize = pFieldBundle->threadQ_Join.GetUseSize();
			int size = (useSize / sizeof(joinQContext));

			for (int i = 0; i < size; i++)
			{
				joinQContext joinQContext;

				pFieldBundle->threadQ_Join.Dequeue((char*)&joinQContext, sizeof(joinQContext));

				Session* newSession = joinQContext.pSession;
				void* pPlayer = joinQContext.pPlayer;

				// 새로운곳에 왔으니 세션 큐 비우기.
				int contentQUseSize = newSession->contentMsgQ.GetUseSize();
				while (1)
				{
					if (contentQUseSize <= 0)
						break;

					RawPtr r;
					newSession->contentMsgQ.Dequeue((char*)&r, sizeof(RawPtr));

					r.DecreaseRefCount();

					contentQUseSize--;
				}
				newSession->contentMsgQ.ClearBuffer();

				pFieldBundle->sessionVec.push_back(newSession);

				pFieldBundle->field->OnEnterWithPlayer(newSession->sessionID, pPlayer);
			}
		}


		// OnRecv
		// 1. sessionVec에 등록된 session을 순회
		// 2. 만약 세션에 releaseWait에 불이 들어와있다면->leaveVec에 삽입.
		// 3. 메시지를 빼기 (rawPtr형태입니다. 빼고나서 decrease를 잊지마세요)
		// 4. TODO: field의 OnRecv호출 -> 인자로 sessionID와 newPacket넘겨주기.
		// TODO(콘텐츠):
		// 1. OnRecv로 메시지가 들어왔다?
		// 2. 바로 메시지 처리하면된다.
		{
			int size = pFieldBundle->sessionVec.size();

			for (int i = 0; i < size; i++)
			{
				Session* pSession = pFieldBundle->sessionVec[i];
				DWORD64 sid = pSession->sessionID;

				if (pSession->releaseWait == true)
				{
					pFieldBundle->releaseVec.push_back(pSession);
					continue;
				}

				int msgCnt = (pSession->contentMsgQ.GetUseSize() / sizeof(RawPtr));
				while(msgCnt--)
				{
					RawPtr rawPtr;
					pSession->contentMsgQ.Dequeue((char*)&rawPtr, sizeof(RawPtr));

					SerializePacketPtr newPacket(rawPtr);

					rawPtr.DecreaseRefCount();

					// TODO: 4 O
					pFieldBundle->field->OnRecv(sid, newPacket);
				}
				
			}
		}

		// OnUpdate
		// 1. OnUpdate를 호출
		// TODO(콘텐츠):
		// 1. 채팅이라서 딱히 없다.
		// 2. 하트비트 정도만 체크하자.
		{
			pFieldBundle->field->OnUpdate();
		}


		// OnLeave
		// 1. OnRecv에서 넣어놓은 releaseVec을 확인.
		// 2. 내꺼에서 session 제거
		// 2. releaseVec을 돌면서 OnLeave를 호출
		// 3. releaseVec Clear
		
		// TODO(콘텐츠):
		// 1. OnLeave로 넘어온 sessionID를 확인하자.
		// 2. sessionID와 매칭되는 Player를 없애자.
		{
			int size = pFieldBundle->releaseVec.size();
			for (int i = 0; i < size; i++)
			{
				Session* pSession = pFieldBundle->releaseVec[i];
				DWORD64 sid = pSession->sessionID;

				int size2 = pFieldBundle->sessionVec.size();
				for (int j = 0; j < size2; j++)
				{
					if (pFieldBundle->sessionVec[j] != pSession)
						continue;

					pFieldBundle->sessionVec[j] = pFieldBundle->sessionVec.back();
					pFieldBundle->sessionVec.pop_back();
					break;
				}

				DecreaseIO_Count(pSession);
				pFieldBundle->field->OnLeave(sid);
			}

			pFieldBundle->releaseVec.clear();
		}


		// field의 movePackVec순회 및 처리
		// 1. 내꺼에서 session제거.
		// 2. 콘텐츠에 알리기.
		// 3. 목적지의 threadQ_Join에 인큐
		// 4. movePack clear.

		// TODO(콘텐츠):
		// 1. OnLeave로 넘어온 sessionID를 확인하자.
		// 2. sessionID와 매칭되는 Player를 없애자.
		{
			int size = pFieldBundle->field->movePackageVec.size();
			for (int i = 0; i < size; i++)
			{
				FieldName fieldName = pFieldBundle->field->movePackageVec[i].fieldName;
				DWORD64 sid = pFieldBundle->field->movePackageVec[i].sessionID;
				void* pPlayer = pFieldBundle->field->movePackageVec[i].pPlayer;

				// 세션 검색
				unsigned int idx = GetIdxFromSessionID(sid);
				Session* pSession = NULL;
				pSession = &_sessionArray[idx];

				if (pSession == NULL)
					DebugBreak();

				// 1.
				int size2 = pFieldBundle->sessionVec.size();
				for (int i = 0; i < size2; i++)
				{
					if (pFieldBundle->sessionVec[i] != pSession)
						continue;

					pFieldBundle->sessionVec[i] = pFieldBundle->sessionVec.back();
					pFieldBundle->sessionVec.pop_back();
					break;
				}

				// 2.
				pFieldBundle->field->OnFieldLeave(sid);

				// 3. 
				auto iter = fieldBundleMap.find(fieldName);
				if (iter == fieldBundleMap.end())
					DebugBreak();

				FieldBundle* pDestination = iter->second;

				joinQContext tmpJoinQContext;
				tmpJoinQContext.pSession = pSession;
				tmpJoinQContext.pPlayer = pPlayer;

				{
					lock_guard<mutex> lock(pDestination->threadQ_Join_Lock);
					pDestination->threadQ_Join.Enqueue((char*)&tmpJoinQContext, sizeof(joinQContext));
				}
			}

			// 4.
			pFieldBundle->field->movePackageVec.clear();
		}

		//PRO_END("Total");
	}
}

void GameManager::FrameControl()
{
	thread_local static DWORD oldTick = timeGetTime();

	DWORD nowTime = timeGetTime();
	DWORD diffTime = nowTime - oldTick;

	if (diffTime < 20)
	{
		Sleep(20 - diffTime);
	}

	oldTick = timeGetTime();

	return;
}


void GameManager::ShowFPS(int id)
{
	thread_local static int tick = timeGetTime();
	thread_local static int count = 0;
	count++;


	if (timeGetTime() - tick >= 1000)
	{
		if (id == 0)
			Monitoring::GetInstance()->SetAuthFPS(count);
		else if (id == 1)
			Monitoring::GetInstance()->SetEchoFPS(count);

		count = 0;
		tick += 1000;
	}

	return;
}

void GameManager::MonitorThread()
{
	while (1)
	{
		DWORD ret = WaitForSingleObject(hEvent_Quit, 1000);
		if (ret == WAIT_OBJECT_0)
		{
			break;
		}

		Monitoring::GetInstance()->UpdatePDHnCpuUsage();

		Monitoring::GetInstance()->CollectPerSecond();
		TossMonitoringData();

		Monitoring::GetInstance()->PrintMonitoring();
		Monitoring::GetInstance()->Clear();
	}
}


void GameManager::RegistField(FieldName fieldName, Field* pField)
{
	static int id = 0;

	pField->RegistGameManager(this);

	// 1. fieldBundle 만들기
	// 2. fieldBundleMap에 넣기.
	// 3. 쓰레드 생성하기 (인자로 fieldBundle 넣기)

	// 1.
	FieldBundle* fieldBundle = new FieldBundle;
	fieldBundle->fieldName = fieldName;
	fieldBundle->field = pField;
	fieldBundle->threadQ_Create.Resize(50000);
	fieldBundle->threadQ_Join.Resize(50000);

	// 2.
	{
		lock_guard<mutex> lock(fieldBundleMapLock);
		fieldBundleMap.insert({ fieldName, fieldBundle });
	}

	// 3.
	threads.emplace_back(
		&GameManager::FieldThreadFunc,
		this,
		fieldBundle,
		id
	);

	id++;
}

void GameManager::RegistMonitoring(NetClient_Monitoring* pNetClient)
{
	this->pNetClient = pNetClient;
}

void GameManager::SendPacketJobThread(int id)
{
	while (1)
	{
		if (sendPacketQ[id]->GetUseSize() < sizeof(RawPtr))
		{
			Sleep(0);
			continue;
		}
		
		SendPacketJob tmpJob;
		sendPacketQ[id]->Dequeue((char*)&tmpJob, sizeof(SendPacketJob));

		DWORD64 sid = tmpJob.sid;

		SerializePacketPtr sPacket(tmpJob.r);
		tmpJob.r.DecreaseRefCount();
		
		SendPacket(sid, sPacket);
	}
}
