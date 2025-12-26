#pragma once
#include <Windows.h>
#include <string>

struct stLoginServerConfig
{
	std::string chatServer_ip1;
	std::string chatServer_ip2;
	USHORT port = 0;

	std::string dummy_ip1;
	std::string dummy_ip2;
};

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
	stLoginServerConfig loginServerConfig;
	stMySQLConfig mySQLConfig;

public:
	bool Load(const char* fileName);

private:
	void ApplyValue(const std::string& section, const std::string& key, const std::string& value);
	void Trim(std::string& s);
};

extern MyConfig myConfig;