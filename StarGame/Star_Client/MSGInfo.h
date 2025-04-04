#pragma once

// 메시지 타입
enum MSG_TYPE
{
    IDALLOC = 0,
    CREATESTAR = 1,
    DESTROYSTAR = 2,
    MOVESTAR = 3,

    COUNT
};

// 메시지 구조체
struct MSG_IDAlloc
{
    int _type;
    int _id;
    int dummy1;
    int dummy2;
};
struct MSG_CreateStar
{
    int _type;
    int _id;
    int _x;
    int _y;
};
struct MSG_DestroyStar
{
    int _type;
    int _id;
    int dummy1;
    int dummy2;
};
struct MSG_MoveStar
{
    int _type;
    int _id;
    int _x;
    int _y;
};