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

class MyConfig
{
public:
	stLoginServerConfig loginServerConfig;

public:
	bool Load(const char* fileName);

private:
	void ApplyValue(const std::string& section, const std::string& key, const std::string& value);
	void Trim(std::string& s);
};