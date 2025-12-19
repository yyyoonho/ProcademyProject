#pragma once
#include <Windows.h>
#include <string>

struct stMySQLConfig
{
	std::string ip;
	USHORT port = 0;

	std::string user;
	std::string password;
};

class MyConfig
{
public:
	stMySQLConfig mySQLConfig;

public:
	bool Load(const char* fileName);

private:
	void ApplyValue(const std::string& section, const std::string& key, const std::string& value);
	void Trim(std::string& s);
};

extern MyConfig g_MyConfig;