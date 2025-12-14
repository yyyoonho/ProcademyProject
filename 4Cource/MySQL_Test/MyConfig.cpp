#include <cstdio>
#include <cstring>
#include <string>
#include <iostream>
#include <Windows.h>

#include "MyConfig.h"

using namespace std;

bool MyConfig::Load(const char* fileName)
{
	FILE* fp;
	errno_t checkOpen = fopen_s(&fp, fileName, "r");
	if (checkOpen != 0)
	{
		printf("ERROR: fopen_s()");
		return false;
	}

	char buffer[512];
	string line;
	string currentSection;

    while (fgets(buffer, sizeof(buffer), fp))
    {
        line = buffer;
        Trim(line);

        // 빈 줄
        if (line.empty())
            continue;

        // 주석
        if (line.find("//") == 0)
            continue;

        // 섹션
        if (line[0] == ':')
        {
            currentSection = line.substr(1);
            Trim(currentSection);
            continue;
        }

        // 블록 시작/끝
        if (line == "{" || line == "}")
            continue;

        // KEY = VALUE
        size_t pos = line.find('=');
        if (pos == string::npos)
            continue;

        string key = line.substr(0, pos);
        string value = line.substr(pos + 1);

        Trim(key);
        Trim(value);

        // 문자열 따옴표 제거
        if (!value.empty() && value.front() == '"')
            value = value.substr(1, value.size() - 2);

        ApplyValue(currentSection, key, value);
    }

    fclose(fp);
    return true;
}

void MyConfig::ApplyValue(const string& section, const string& key, const string& value)
{
    if (section == "MySQL")
    {
        if (key == "IP")
        {
            mySQLConfig.ip = value;
        }
        else if (key == "PORT")
        {
            mySQLConfig.port = stoi(value);
        }
        else if (key == "USER")
        {
            mySQLConfig.user = value;
        }
        else if (key == "PASSWORD")
        {
            mySQLConfig.password = value;
        }
    }
}

void MyConfig::Trim(string& s)
{
    // 앞 공백 제거
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c) {
        return !std::isspace(c);
        }));

    // 뒤 공백 제거
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c) {
        return !std::isspace(c);
        }).base(), s.end());
}
