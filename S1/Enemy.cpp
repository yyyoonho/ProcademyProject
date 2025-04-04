#include <Windows.h>
#include "Data.h"

#include "Enemy.h"
#include "BufferRender.h"
#include "Player.h"

int enemyCount;

stEnemy enemies[MAX_ENEMY];
stBullet enemyBullets[MAX_BULLET];

int enemySpeed;

char moving_FileName[100][64];

int moving_Count[100];
int moving_Pattern[100][64];
int movingPatternIdx;
int sprite;

int dy[4] = { 0,-1,0,1 };
int dx[4] = { -1,0,1,0 };

void EnemyBulletMoving();
void EnemyBulletLifeCheck();

void EnemyMove();
void EnemyShot();

void EnemyInput()
{
	EnemyMove();
	EnemyShot();
}

void EnemyLogic()
{
	EnemyBulletMoving();
	EnemyBulletLifeCheck();
}

void EnemyRender()
{
	for (int i = 0; i < MAX_ENEMY; i++)
	{
		if (enemies[i].visible == false) continue;

		Sprite_Draw(enemies[i].x, enemies[i].y, (char)(enemies[i].hp + '0'));
	}

	for (int i = 0; i < MAX_BULLET; i++)
	{
		if (enemyBullets[i].visible == false) continue;

		Sprite_Draw(enemyBullets[i].x, enemyBullets[i].y, 'v');
	}
}

void EnemyIsHit(int idx)
{
	enemies[idx].hp--;

	if (enemies[idx].hp <= 0)
	{
		enemies[idx].visible = false;

		enemyCount--;
	}

	return;
}

void SetEnemy(int inputY, int inputX, int inputHp, int count)
{
	enemies[count].y = inputY;
	enemies[count].x = inputX;
	enemies[count].hp = inputHp;
	enemies[count].visible = true;

	enemyCount = count;
}

void EnemyBulletMoving()
{
	for (int i = 0; i < MAX_BULLET; i++)
	{
		if (enemyBullets[i].visible == false) continue;

		enemyBullets[i].y++;
	}
}

void EnemyBulletLifeCheck()
{
	for (int i = 0; i < MAX_BULLET; i++)
	{
		if (enemyBullets[i].visible == false) continue;

		if (enemyBullets[i].y > dfSCREEN_HEIGHT)
		{
			enemyBullets[i].visible = false;

			continue;
		}

		if (player.x == enemyBullets[i].x && player.y == enemyBullets[i].y)
		{
			enemyBullets[i].visible = false;

			PlayerIsHit();

			continue;
		}
	}
}

void SetEnemySpeed(int inputSpeed)
{
	enemySpeed = inputSpeed;
}

void EnemyReset()
{
	for (int i = 0; i < MAX_ENEMY; i++)
	{
		enemies[i].visible = false;
	}

	for (int i = 0; i < MAX_BULLET; i++)
	{
		enemyBullets[i].visible = false;
	}

	sprite = 0;
}

void EnemyMove()
{
	static DWORD time = timeGetTime();

	if (timeGetTime() - time < 500)
		return;

	for (int i = 0; i < MAX_ENEMY; i++)
	{
		if (enemies[i].visible == false) continue;

		int nextY = enemies[i].y + dy[moving_Pattern[movingPatternIdx][sprite]];
		int nextX = enemies[i].x + dx[moving_Pattern[movingPatternIdx][sprite]];

		enemies[i].y = nextY;
		enemies[i].x = nextX;
	}

	sprite = (sprite + 1) % moving_Count[movingPatternIdx];

	time = timeGetTime();
}

void EnemyShot()
{
	static DWORD t = timeGetTime();
	if ((timeGetTime() - t) < (BULLET_FIRE_TERM * 5) / (unsigned long)enemySpeed)
	{
		return;
	}
	t = timeGetTime();

	int aliveEnemyIdx[MAX_ENEMY];
	int aliveEnemyCnt = 0;
	for (int i = 0; i < MAX_ENEMY; i++)
	{
		if (enemies[i].visible == false) continue;

		aliveEnemyIdx[aliveEnemyCnt] = i;
		aliveEnemyCnt++;	
	}

	if (aliveEnemyCnt == 0) return;

	int shotingEnemyIdx = aliveEnemyIdx[rand() % aliveEnemyCnt];

	for (int i = 0; i < MAX_BULLET; i++)
	{
		if (enemyBullets[i].visible == false)
		{
			enemyBullets[i].visible = true;
			enemyBullets[i].y = enemies[shotingEnemyIdx].y - 1;
			enemyBullets[i].x = enemies[shotingEnemyIdx].x;

			return;
		}
	}
}