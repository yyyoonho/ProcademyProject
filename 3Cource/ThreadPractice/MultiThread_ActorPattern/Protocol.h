#pragma once

#define dfJOB_ADD	0
#define dfJOB_DEL	1
#define dfJOB_SORT	2
#define dfJOB_FIND	3
#define dfJOB_PRINT	4	// 출력 or 저장 / 읽기만 하는 느린 행동
#define dfJOB_QUIT	5	

struct st_MSG_HEAD
{
	short shType;
	short shPayloadLen;
};