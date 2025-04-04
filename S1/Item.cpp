#include "Data.h"

#include "Item.h"
#include "Player.h"
#include "BufferRender.h"

stItem stageItems[MAX_ITEM];

void ItemLogic()
{
	for (int i = 0; i < MAX_ITEM; i++)
	{
		if (stageItems[i].visible == false) continue;

		if (stageItems[i].x == player.x && stageItems[i].y == player.y)
		{
			stageItems[i].visible = false;

			getItems[stageItems[i].itemNum] = true;
		}
	}
}

void ItemRender()
{
	for (int i = 0; i < MAX_ITEM; i++)
	{
		if (stageItems[i].visible == false) continue;

		Sprite_Draw(stageItems[i].x, stageItems[i].y, '$');
	}
}

void ItemReset()
{
	for (int i = 0; i < MAX_ITEM; i++)
	{
		stageItems[i].visible = false;
	}
}
