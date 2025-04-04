#pragma once

extern SCENE _nowScene;
extern SCENE _waitingScene;

extern int logicFPS;
extern int RenderFPS;

int main();

bool RenderSkipCheck();

void ShowFPS();