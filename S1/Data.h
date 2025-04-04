#pragma once

#define MSPERFRAME			30

#define dfSCREEN_WIDTH		81		// ÄÜ¼Ö °¡·Î 80Ä­ + NULL
#define dfSCREEN_HEIGHT		24		// ÄÜ¼Ö ¼¼·Î 24Ä­

#define GAMESCREEN_WIDTH	60

#define HPSTR_HEIGHT		24

#define MAX_PLAYER_Y		10
#define MAX_PLAYER_X		60

#define MAX_ENEMY			100
#define MAX_BULLET			100
#define MAX_ITEM			3
#define BULLET_FIRE_TERM	200

enum class SCENE
{
	TITLE,
	LOAD,
	GAME,
	GAMEOVER,
	ERROR_GAMEOUT,

	SCENECOUNT
};

struct stEnemy
{
	int y;
	int x;

	int hp;

	bool visible;
};

struct stBullet
{
	int y;
	int x;
	
	bool visible;
};

struct Player
{
	int y = 18;
	int x = 20;

	int hp = 3;
};

struct stItem
{
	int y;
	int x;

	int itemNum;

	bool visible;
};

enum class ITEM
{
	BULLETSPEEDUP=0,
	MACHINEGUN,

	ITEMCOUNT,
};