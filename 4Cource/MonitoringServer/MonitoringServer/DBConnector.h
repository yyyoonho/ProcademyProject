#pragma once

/*
»ç¿ë¹ý

// Read
	DBResult queryRes;
	DBConnector::GetInstance()->QueryRead(new DBCheckAccountInfo, queryRes);
	for (int i = 0; i < queryRes.size(); i++)
	{
		cout << queryRes[i]["accountNo"] << endl;
	}

// Write
	DBConnector::GetInstance().QueryWrite(mapId, new DBSavePlayer(playerData);

*/

enum class DB_FIELD
{
	FIELD_0 = 0,
	FIELD_1,
	FIELD_2,

	DB_FIELD_COUNT,
};

class DBConnector
{
public:
	static DBConnector* _pDBConnector;
	static DBConnector* GetInstance();

private:
	DBConnector();
	~DBConnector();

public:
	void Start();
	void Stop();

public:
	void QueryRead(IDBJob_Read* job, DBResult& queryResult);
	void QueryWrite(int fieldID, IDBJob_Write* job);

	/**************************************************/

private:
	std::string		_ip;
	USHORT			_port;
	std::string		_user;
	std::string		_password;

private:
	struct DBConnTLS
	{
		MYSQL readConn;
		MYSQL* readConnection = nullptr;
	};

	static thread_local DBConnTLS conn_tls;

	/**************************************************/

private:
	class WriteWorker
	{
	public:
		void Start(std::string ip, USHORT port, std::string user, std::string password);
		void Stop();
		void Push(IDBJob_Write* job);

	private:
		static void WorkerThreadRunProc(LPVOID* lParam);
		void Run();

	private:
		std::string		_ip;
		USHORT			_port;
		std::string		_user;
		std::string		_password;

		MYSQL _writeConn;
		MYSQL* _writeConnection;

	private:
		std::queue<IDBJob_Write*> _JobQueue;
		std::mutex _queueLock;
		HANDLE _hWriteThread;
		HANDLE _hEventJobQueue;
		HANDLE _hEventQuit;
	};

	/**************************************************/
private:
	WriteWorker _writeWorkers[(int)DB_FIELD::DB_FIELD_COUNT];
};
