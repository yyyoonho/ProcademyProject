#pragma once

#define SERVERPORT 3000

#define MAX_X 80
#define MAX_Y 23

struct Player
{
    SOCKET clientSocket;
    SOCKADDR_IN addr;

    int _id;
    int _x;
    int _y;

    bool _destory = false;
};