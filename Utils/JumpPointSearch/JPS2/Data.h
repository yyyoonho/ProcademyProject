#pragma once

#define GRID_SIZE 16
#define GRID_WIDTH 100
#define GRID_HEIGHT 50


enum
{
    NONE,
    OPENLIST,

    OBSTACLE,
    CLOSELIST,

    START,
    END,
};

enum
{
    LL = 0x00000001,
    LU = 0x00000010,
    UU = 0x00000100,
    RU = 0x00001000,
    RR = 0x00010000,
    RD = 0x00100000,
    DD = 0x01000000,
    LD = 0x10000000,
};

struct Node
{
    int _y;
    int _x;

    double _G;
    double _H;
    double _F;

    unsigned int _dir = 0;

    Node* parent = NULL;
};