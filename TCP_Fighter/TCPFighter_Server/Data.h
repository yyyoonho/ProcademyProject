#pragma once

// FRMAE
#define FRAME 50

// 프레임당 움직임
#define XMOVE 3
#define YMOVE 2

// 화면 이동영역
#define RANGE_MOVE_TOP		50
#define RANGE_MOVE_LEFT		10
#define RANGE_MOVE_RIGHT	630
#define RANGE_MOVE_BOTTOM	470

// 이동 오류체크 범위
#define ERROR_RANGE			50

// 공격 범위
#define ATTACK1_RANGE_X		80
#define ATTACK1_RANGE_Y		10

#define ATTACK2_RANGE_X		90
#define ATTACK2_RANGE_Y		10

#define ATTACK3_RANGE_X		100
#define ATTACK3_RANGE_Y		20

// 방향
#define PACKET_MOVE_DIR_LL					0
#define PACKET_MOVE_DIR_LU					1
#define PACKET_MOVE_DIR_UU					2
#define PACKET_MOVE_DIR_RU					3
#define PACKET_MOVE_DIR_RR					4
#define PACKET_MOVE_DIR_RD					5
#define PACKET_MOVE_DIR_DD					6
#define PACKET_MOVE_DIR_LD					7

#define PACKET_STOP							8

int dy[8] = { 0, -1, -1, -1, 0, 1, 1, 1 };
int dx[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };
