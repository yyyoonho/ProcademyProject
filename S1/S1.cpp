#include <Windows.h>
#include <iostream>
#include "Data.h"
#include "S1.h"

#include "Console.h"
#include "Load.h"
#include "Title.h"
#include "Game.h"
#include "BufferRender.h"
#include "GameOver.h"

#pragma comment(lib, "winmm.lib")

SCENE _nowScene;
SCENE _waitingScene;

int logicFPS;
int RenderFPS;

void Init();

int main()
{
    timeBeginPeriod(1);

    Init();

    bool isNormal = true;

    while (isNormal)
    {
        switch (_nowScene)
        {
        case SCENE::TITLE:
            TitleUpdate();
            break;

        case SCENE::LOAD:
            Loading();
            break;

        case SCENE::GAME:
            GameUpdate();
            break;

        case SCENE::GAMEOVER:
            GameOverUpdate();
            break;
            
        case SCENE::ERROR_GAMEOUT:
            isNormal = false;
            break;
        }
    }

    timeEndPeriod(1);

    return 0;
}

void Init()
{
    cs_Initial();

    srand((unsigned int)time(NULL));

    _nowScene = SCENE::LOAD;
    _waitingScene = SCENE::TITLE;
}

bool RenderSkipCheck()
{
    static int oldTick = timeGetTime();

    int t = timeGetTime() - oldTick;

    if (t < MSPERFRAME)
    {
        Sleep(MSPERFRAME - t);
        oldTick += MSPERFRAME;
        return false;
    }
    else if (t >= MSPERFRAME * 2)
    {
        oldTick += MSPERFRAME;
        return true;
    }
    else
    {
        oldTick += MSPERFRAME;
        return false;
    }
}

void ShowFPS()
{
    static int static_Logicfps = 0;
    static int static_Renderfps = 0;

    static DWORD t = timeGetTime();
    if (timeGetTime() - t >= 1000)
    {
        static_Logicfps = logicFPS;
        logicFPS = 0;

        static_Renderfps = RenderFPS;
        RenderFPS = 0;

        t += 1000;
    }

    int startPointY = 0;

    const char* fpsStr[2] = { "Logic fps: ", "Render fps: " };

    for (int i = 0; i < 2; i++)
    {
        int startPointX = GAMESCREEN_WIDTH;

        for (unsigned int j = 0; j < strlen(fpsStr[i]); j++)
        {
            Sprite_Draw(startPointX++, startPointY, fpsStr[i][j]);
        }

        if (i == 0)
        {
            Sprite_Draw(startPointX++, startPointY, (char)((static_Logicfps / 10) + '0'));
            Sprite_Draw(startPointX++, startPointY, (char)((static_Logicfps % 10) + '0'));
        }
        else
        {
            Sprite_Draw(startPointX++, startPointY, (char)((static_Renderfps / 10) + '0'));
            Sprite_Draw(startPointX++, startPointY, (char)((static_Renderfps % 10) + '0'));
        }

        startPointY++;
    }
}