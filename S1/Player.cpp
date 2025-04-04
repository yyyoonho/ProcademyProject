#include <Windows.h>
#include "Data.h"

#include "Enemy.h"
#include "Player.h"
#include "BufferRender.h"
#include "Load.h"

Player player;
stBullet playerBullets[MAX_BULLET];

bool getItems[(int)ITEM::ITEMCOUNT];

unsigned int bulletSpeed = 1;

void PlayerBulletMoving();
void PlayerBulletLifeCheck();
void PlayerItemLogic();
void SpawnBullet();

void PlayerInput()
{
	if (GetAsyncKeyState(VK_UP))
	{
		if (MAX_PLAYER_Y < player.y - 1)
		{
			player.y--;
		}
	}

	if (GetAsyncKeyState(VK_DOWN))
	{
		if (dfSCREEN_HEIGHT > player.y + 1)
		{
			player.y++;
		}
	}

	if (GetAsyncKeyState(VK_RIGHT))
	{
		if (MAX_PLAYER_X > player.x + 1)
		{
			player.x++;
		}
	}

	if (GetAsyncKeyState(VK_LEFT))
	{
		if (0 < player.x - 1)
		{
			player.x--;
		}
	}

	if (GetAsyncKeyState(VK_LCONTROL))
	{
		static DWORD t = timeGetTime();
		if (timeGetTime() - t < BULLET_FIRE_TERM / bulletSpeed)
		{
			return;
		}
		t = timeGetTime();

		SpawnBullet();
	}
}

void PlayerLogic()
{
	PlayerBulletMoving();

	PlayerBulletLifeCheck();

	PlayerItemLogic();
}

void PlayerRender()
{
	Sprite_Draw(player.x, player.y, 'A');

	for (int i = 0; i < MAX_BULLET; i++)
	{
		if (playerBullets[i].visible == false) continue;

		Sprite_Draw(playerBullets[i].x, playerBullets[i].y, '*');
	}

	const char* playerHpStr = "HP: ";
	unsigned int i = 0;
	for (; i < strlen(playerHpStr); i++)
	{
		Sprite_Draw(GAMESCREEN_WIDTH + i, HPSTR_HEIGHT-1, playerHpStr[i]);
	}
	Sprite_Draw(GAMESCREEN_WIDTH + i, HPSTR_HEIGHT - 1, (char)(player.hp + '0'));
}

void PlayerBulletMoving()
{
	for (int i = 0; i < MAX_BULLET; i++)
	{
		if (playerBullets[i].visible == false) continue;

		playerBullets[i].y--;
	}
}

void PlayerBulletLifeCheck()
{
	for (int i = 0; i < MAX_BULLET; i++)
	{
		if (playerBullets[i].visible == false) continue;

		if (playerBullets[i].y < 0)
		{
			playerBullets[i].visible = false;

			continue;
		}

		for (int j = 0; j < MAX_ENEMY; j++)
		{
			if (enemies[j].visible == false)
				continue;

			if (enemies[j].x == playerBullets[i].x && enemies[j].y == playerBullets[i].y)
			{
				playerBullets[i].visible = false;

				EnemyIsHit(j);
				break;
			}
		}
	}
}

void PlayerItemLogic()
{
	for (int i = 0; i < int(ITEM::ITEMCOUNT); i++)
	{
		if (getItems[i] == false) continue;

		switch (ITEM(i))
		{
		case ITEM::BULLETSPEEDUP:
			bulletSpeed = 2;
			getItems[i] = false;
			break;
		case ITEM::MACHINEGUN:
			// TODO: 아이템에 추가 로직이 있을경우 이곳에 추가
			break;
		}

	}
}

void SpawnBullet()
{
	if (getItems[(int)ITEM::MACHINEGUN] == true)
	{
		for (int i = 0; i < MAX_BULLET; i++)
		{
			if (playerBullets[i].visible == false)
			{
				playerBullets[i].visible = true;
				playerBullets[i].y = player.y - 1;
				playerBullets[i].x = player.x - 1;

				if (i + 1 < MAX_BULLET)
				{
					i++;
					playerBullets[i].visible = true;
					playerBullets[i].y = player.y - 1;
					playerBullets[i].x = player.x + 1;
				}

				return;
			}
		}
	}
	else
	{
		for (int i = 0; i < MAX_BULLET; i++)
		{
			if (playerBullets[i].visible == false)
			{
				playerBullets[i].visible = true;

				playerBullets[i].y = player.y - 1;
				playerBullets[i].x = player.x;

				return;
			}
		}
	}
}

void PlayerIsHit()
{
	player.hp--;

	if (player.hp <= 0)
	{
		ChangeScene(SCENE::GAMEOVER);
	}

	return;
}

void PlayerReset()
{
	for (int i = 0; i < MAX_BULLET; i++)
	{
		if (playerBullets[i].visible == true)
			playerBullets[i].visible = false;
	}

	for (int i = 0; i < (int)ITEM::ITEMCOUNT; i++)
	{
		getItems[i] = false;
	}

	bulletSpeed = 1;

	player.hp = 3;
}
