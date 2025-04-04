#pragma once

extern stEnemy enemies[MAX_ENEMY];
extern stBullet enemybullets[MAX_BULLET];
extern int enemyCount;

extern int movingPatternIdx;
extern char moving_FileName[100][64];
extern int moving_Pattern[100][64];
extern int moving_Count[100];

void EnemyInput();
void EnemyLogic();
void EnemyRender();

void EnemyIsHit(int idx);

void SetEnemy(int inputY, int inputX, int inputHp, int count);

void SetEnemySpeed(int inputSpeed);

void EnemyReset();