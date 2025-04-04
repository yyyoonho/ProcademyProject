#include <Windows.h>
#include <iostream>
#include <string.h>
#include "Data.h"

#include "S1.h"
#include "Load.h"
#include "Title.h"
#include "Game.h"
#include "Enemy.h"
#include "Item.h"
#include "Player.h"

#define STAGEDATACOUNT 4

bool loadStageInfoFileCheck;
bool loadMovingFileCheck;
const char* stageName;

void LoadTitle();
void LoadGame();

void MovingInfoLoad();

void ResetAll();

void Loading()
{
	switch (_waitingScene)
	{
	case SCENE::TITLE:
		LoadTitle();
		break;

	case SCENE::GAME:
		LoadGame();
		MovingInfoLoad();
		startTime = timeGetTime();
		break;

	case SCENE::GAMEOVER:
		break;
	}

	_nowScene = _waitingScene;
}

void ChangeScene(SCENE targetScene)
{
	_nowScene = SCENE::LOAD;
	_waitingScene = targetScene;
}

void LoadTitle()
{
	if (loadStageInfoFileCheck == true)
	{
		ResetAll();
		return;
	}
	loadStageInfoFileCheck = true;

	FILE* fpStageInfo;

	errno_t errorCheck = fopen_s(&fpStageInfo, "stageInfo.txt", "rt");
	if (errorCheck != 0)
	{
		printf("ERROR: fopen_s()\n");
		ChangeScene(SCENE::ERROR_GAMEOUT);
		return;
	}

	char buf[64];
	int stageCount;

	fgets(buf, 64, fpStageInfo);
	stageCount = atoi(buf);

	for (int i = 0; i < stageCount; i++)
	{
		fgets(buf, 64, fpStageInfo);
		
		strcpy_s(&title_fileName[i][0], 63, buf);
		int nameLen = strlen(&title_fileName[i][0]);
		title_fileName[i][nameLen - 1] = '\0';
	}
	
	title_fileCount = stageCount;
}

void LoadGame()
{
	// Stage load
	FILE* fpStage;
	errno_t checkOpen = fopen_s(&fpStage, stageName, "rt");
	if (checkOpen != 0)
	{
		printf("ERROR: fopen_s()");
		ChangeScene(SCENE::ERROR_GAMEOUT);
		return;
	}

	fseek(fpStage, 0, SEEK_END);
	int fileSize = ftell(fpStage);
	if (fileSize == -1)
		return;
	
	rewind(fpStage);

	char* buf = (char*)malloc(fileSize + 1);
	if (buf == nullptr)
	{
		printf("ERROR: malloc()");
		ChangeScene(SCENE::ERROR_GAMEOUT);
		return;
	}

	size_t readFileSize = fread(buf, 1, fileSize, fpStage);
	if (buf == nullptr)
	{
		printf("ERROR: fread()");
		ChangeScene(SCENE::ERROR_GAMEOUT);
		return;
	}
	buf[readFileSize] = '\0';

	fclose(fpStage);
	
	const char* parsingData4Game[STAGEDATACOUNT] = {
		"ITEM=",
		"ENEMYBULLETSPEED=",
		"MOVEPATTERN=",
		"MAP=" };

	for (int i = 0; i < STAGEDATACOUNT; i++)
	{
		char* targetP = strstr(buf, parsingData4Game[i]);
		int len = strlen(parsingData4Game[i]);
		char* dataP = targetP + len;
		if (dataP == nullptr)
		{
			printf("ERROR: nullptr 에러");
			ChangeScene(SCENE::ERROR_GAMEOUT);
			return;
		}

		if (i == 0)
		{
			int itemCount = 0;
			int idx = 0;

			while (1)
			{
				if (dataP[idx] == '\n')
					break;

				if (dataP[idx] != ',')
				{
					stageItems[itemCount].itemNum = atoi(&dataP[idx]);
					stageItems[itemCount].x = rand() % GAMESCREEN_WIDTH;
					stageItems[itemCount].y = rand() % (dfSCREEN_HEIGHT - MAX_PLAYER_Y - 1) + MAX_PLAYER_Y + 1;
					stageItems[itemCount].visible = true;
					itemCount++;
				}

				idx++;
			}
		}
		else if (i == 1)
		{
			SetEnemySpeed(atoi(dataP));
		}
		else if (i == 2)
		{
			movingPatternIdx = atoi(dataP);
		}
		else if (i == 3)
		{
			dataP += 1;
			int y = 0;
			int x = 0;
			int idx = 0;

			int enemyCount = 0;
			while (1)
			{
				if (dataP[idx] == '\n')
				{
					x = 0;
					y++;
				}
				else if (dataP[idx] == ' ')
				{
					x++;
				}
				else if (dataP[idx] >= '1' && dataP[idx] <= '5')
				{
					const char* tmp = &dataP[idx];

					SetEnemy(y, x, atoi(tmp), enemyCount);
					enemyCount++;
					x++;
				}

				if (dataP[idx + 1] == '\0')
				{
					break;
				}
				else
				{
					idx++;
				}
			}
		}
	}

	free(buf);
}


void PassStageName(const char* inputStageName)
{
	stageName = inputStageName;
}

void MovingInfoLoad()
{
	if (loadMovingFileCheck == true) return;
	loadMovingFileCheck = true;

	FILE* fp;
	errno_t openCheck = fopen_s(&fp, "movingInfo.txt", "rt");
	if (openCheck != 0)
	{
		printf("ERROR: fopen_s()");
		ChangeScene(SCENE::ERROR_GAMEOUT);
		return;
	}

	char buf[64];
	fgets(buf, 64, fp);

	int moving_FileCount = atoi(buf);
	
	for (int i = 0; i < moving_FileCount; i++)
	{
		fgets(buf, 64, fp);

		strcpy_s(&moving_FileName[i][0], 63, buf);
		int nameLen = strlen(&moving_FileName[i][0]);
		moving_FileName[i][nameLen - 1] = '\0';
	}

	fclose(fp);


	for (int i = 0; i < moving_FileCount; i++)
	{
		FILE* fp;
		errno_t openCheck = fopen_s(&fp, &moving_FileName[i][0], "rt");
		if (openCheck != 0)
		{
			printf("ERROR: fopen_s()");
			ChangeScene(SCENE::ERROR_GAMEOUT);
			return;
		}

		// 무브 파일 읽기
		fseek(fp, 0, SEEK_END);
		int fileSize = ftell(fp);
		if (fileSize == -1)
		{
			printf("ERROR: ftell()");
			ChangeScene(SCENE::ERROR_GAMEOUT);
			return;
		}
		rewind(fp);

		char buf[64];
		int readFileSize = fread(buf, 1, fileSize, fp);

		int cnt = 0;
		// enemy에 전달
		for (int j = 0; j < readFileSize; j++)
		{
			bool flag = false;
			int val = 0;
			
			if (buf[j] == 'L')
			{
				val = 0;
				flag = true;
			}
			else if (buf[j] == 'U')
			{
				val = 1;
				flag = true;
			}
			else if (buf[j] == 'R')
			{
				val = 2;
				flag = true;
			}
			else if (buf[j] == 'D')
			{
				val = 3;
				flag = true;
			}
				
			if (flag == true)
			{
				moving_Pattern[i + 1][j] = val;
				cnt++;
			}
		}
		moving_Count[i + 1] = cnt;

		fclose(fp);
	}
}

void ResetAll()
{
	PlayerReset();
	ItemReset();
	EnemyReset();
	GameReset();
}
