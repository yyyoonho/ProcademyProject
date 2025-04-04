#pragma once

#define SERVERPORT 3000
#define SERVERIP L"192.168.219.105"

#define MAX_X 80
#define MAX_Y 23

#define MSGLEN 16 // 고정길이

#define FRAME 50
#define MSPERFRAME (1000/FRAME) 


struct Player
{
    int _id;
    int _x;
    int _y;
};