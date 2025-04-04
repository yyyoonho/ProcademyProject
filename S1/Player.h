#pragma once

extern Player player;
extern bool getItems[(int)ITEM::ITEMCOUNT];

void PlayerInput();
void PlayerLogic();
void PlayerRender();

void PlayerIsHit();

void PlayerReset();