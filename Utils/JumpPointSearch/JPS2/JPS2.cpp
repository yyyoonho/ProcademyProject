#pragma comment(lib, "winmm.lib")

#include <Windows.h>
#include <iostream>
#include <windowsx.h>
#include <list>

#include "Data.h"

using namespace std;

// 윈도우 메시지 처리함수
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void DrawParentLine(HDC hdc);

/* 펜, 브러쉬 */
HBRUSH g_hObstacleTileBrush;        // 장애물용 브러시
HBRUSH g_hStartTileBrush;           // 스타트포인트용 브러시
HBRUSH g_hEndTileBrush;             // 엔드포인트용 브러시
HBRUSH g_hOpenListTileBrush;        // 오픈리스트용 브러시
HBRUSH g_hCloseListTileBrush;       // 클로즈리스트용 브러시
HPEN   g_hParentPen;                // 부모노드 연결용 펜
HPEN   g_hCompleteRoutePen;         // 완성된 경로 연결용 펜

HPEN g_hGridPen;                    // 그리드용 펜

bool g_bErase = false;
bool g_bDrag = false;

/* 더블버퍼링용 전역 변수 */
HBITMAP g_hMemDCBitmap;
HBITMAP g_hMemDCBitmap_old;
HDC g_hMemDC;
RECT g_MemDCRect;
HWND hWnd2;

/* 길찾기에 필요한 변수 */
char g_Tile[GRID_HEIGHT][GRID_WIDTH];
bool TileMap[GRID_HEIGHT][GRID_WIDTH]; // True -> 갈 수 있다.False -> 갈 수 없다.
double F_Tile[GRID_HEIGHT][GRID_WIDTH];

list<Node*> openList;
list<Node*> closeList;
list<Node*> completeRouteList;
pair<int, int> startNodeYX = { -1,-1 };
pair<int, int> endNodeYX = { -1,-1 };

bool bFlag = false;

int dy[8] = { 0, -1, -1, -1, 0, 1, 1, 1 };
int dx[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };

double euclidVal[8] = { 1, 1.5, 1, 1.5, 1, 1.5, 1, 1.5 };

/* 함수 전방선언 */
void RenderGrid(HDC hdc);
void RenderNode(HDC hdc);
double GetEuclid(int nowY, int nowX, int targetY, int targetX);
int GetManhattan(int nowY, int nowX, int targetY, int targetX);
void FindPath(HWND hWnd);
bool CanGo(int y, int x);
bool IsCorner(unsigned int dir, int y, int x);
void Init();
void GetMinFNode();
void Reset();

int main()
{
    timeBeginPeriod(1);

    /**************** 윈도우 생성 *****************/

    // 윈도우 클래스 등록
    WNDCLASS wndclass;
    wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wndclass.lpfnWndProc = WndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = NULL;
    wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = L"MyWndClass";

    if (!RegisterClass(&wndclass))
        return 1;

    // 윈도우 생성
    HWND hWnd = CreateWindow(L"MyWndClass", L"A_STAR", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, NULL, NULL);
    if (hWnd == NULL)
        return 1;

    ShowWindow(hWnd, SW_SHOWNORMAL);
    UpdateWindow(hWnd);


    /**************** 윈도우 메시지 루프 *****************/
    MSG msg;
    while (GetMessage(&msg, 0, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    timeEndPeriod(1);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
    case WM_PAINT:
    {
        PatBlt(g_hMemDC, 0, 0, g_MemDCRect.right, g_MemDCRect.bottom, WHITENESS);

        RenderNode(g_hMemDC);
        RenderGrid(g_hMemDC);

        DrawParentLine(g_hMemDC);

        hdc = BeginPaint(hWnd, &ps);
        BitBlt(hdc, 0, 0, g_MemDCRect.right, g_MemDCRect.bottom, g_hMemDC, 0, 0, SRCCOPY);
        EndPaint(hWnd, &ps);
    }
    break;

    case WM_CREATE:
    {
        HDC hdc = GetDC(hWnd);
        GetClientRect(hWnd, &g_MemDCRect);
        g_hMemDCBitmap = CreateCompatibleBitmap(hdc, g_MemDCRect.right, g_MemDCRect.bottom);
        g_hMemDC = CreateCompatibleDC(hdc);
        ReleaseDC(hWnd, hdc);
        g_hMemDCBitmap_old = (HBITMAP)SelectObject(g_hMemDC, g_hMemDCBitmap);

        g_hGridPen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
        g_hParentPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
        g_hCompleteRoutePen = CreatePen(PS_SOLID, 2, RGB(255, 0, 0));

        g_hObstacleTileBrush = CreateSolidBrush(RGB(100, 100, 100));

        g_hStartTileBrush = CreateSolidBrush(RGB(0, 255, 0));
        g_hEndTileBrush = CreateSolidBrush(RGB(255, 0, 0));
        g_hOpenListTileBrush = CreateSolidBrush(RGB(0, 0, 255));
        g_hCloseListTileBrush = CreateSolidBrush(RGB(255, 255, 0));
    }
    break;

    case WM_LBUTTONUP:
        g_bDrag = false;
        break;

    case WM_LBUTTONDOWN:
        g_bDrag = true;
        {
            int xPos = GET_X_LPARAM(lParam);
            int yPos = GET_Y_LPARAM(lParam);
            int iTileX = xPos / GRID_SIZE;
            int iTileY = yPos / GRID_SIZE;

            if (g_Tile[iTileY][iTileX] == OBSTACLE)
                g_bErase = true;
            else
                g_bErase = false;
        }
        break;

    case WM_MOUSEMOVE:
        if (g_bDrag)
        {
            int xPos = GET_X_LPARAM(lParam);
            int yPos = GET_Y_LPARAM(lParam);
            int iTileX = xPos / GRID_SIZE;
            int iTileY = yPos / GRID_SIZE;

            if (g_bErase)
            {
                g_Tile[iTileY][iTileX] = NONE;
            }
            else
            {
                g_Tile[iTileY][iTileX] = OBSTACLE;
            }

            InvalidateRect(hWnd, NULL, false);
        }
        break;

    case WM_LBUTTONDBLCLK:
    {
        int xPos = GET_X_LPARAM(lParam);
        int yPos = GET_Y_LPARAM(lParam);
        int iTileX = xPos / GRID_SIZE;
        int iTileY = yPos / GRID_SIZE;

        //CreateStartNode(iTileX, iTileY);

        if (startNodeYX.first != -1)
            g_Tile[startNodeYX.first][startNodeYX.second] = NONE;

        startNodeYX.first = iTileY;
        startNodeYX.second = iTileX;
        g_Tile[startNodeYX.first][startNodeYX.second] = START;

        InvalidateRect(hWnd, NULL, false);
    }
    break;

    case WM_RBUTTONDBLCLK:
    {
        int xPos = GET_X_LPARAM(lParam);
        int yPos = GET_Y_LPARAM(lParam);
        int iTileX = xPos / GRID_SIZE;
        int iTileY = yPos / GRID_SIZE;

        //CreateEndNode(iTileX, iTileY);

        if (endNodeYX.first != -1)
            g_Tile[endNodeYX.first][endNodeYX.second] = NONE;

        endNodeYX.first = iTileY;
        endNodeYX.second = iTileX;
        g_Tile[endNodeYX.first][endNodeYX.second] = END;

        InvalidateRect(hWnd, NULL, false);
    }
    break;

    case WM_KEYDOWN:
    {
        if (wParam == 'T')
        {
            SetTimer(hWnd, 10, 30, NULL);

            Init();

            Node* startNode = new Node;
            startNode->_y = startNodeYX.first;
            startNode->_x = startNodeYX.second;

            startNode->_dir = LL | LU | UU | RU | RR | RD | DD | LD;

            startNode->_G = 0;
            startNode->_H = GetManhattan(startNode->_y, startNode->_x, endNodeYX.first, endNodeYX.second);
            startNode->_F = startNode->_G + startNode->_H;

            startNode->parent = NULL;

            openList.push_front(startNode);

            FindPath(hWnd);
        }
        if (wParam == 'Y')
        {
            KillTimer(hWnd, 10);

        }
        if (wParam == 'G')
        {
            /*if (bFlag == false)
            {
                Init();

                startNode->_G = 0;
                startNode->_H = GetManhattan(startNode->_y, startNode->_x, endNode->_y, endNode->_x);
                startNode->_F = 0 + startNode->_H;

                startNode->_dir = LL | LU | UU | RU | RR | RD | DD | LD;

                F_Tile[startNode->_y][startNode->_x] = startNode->_F;
                openList.push_front(startNode);

                bFlag = true;
            }

            FindPath(hWnd);

            InvalidateRect(hWnd, NULL, false);
            */
        }
        if (wParam == 'R')
        {
            Reset();

            InvalidateRect(hWnd, NULL, false);
        }
    }
    break;

    case WM_TIMER:
        switch (wParam)
        {
        case 10:
        {
            FindPath(hWnd);
            InvalidateRect(hWnd, NULL, false);
        }
        break;
        }
        break;

    case WM_DESTROY:

        DeleteObject(g_hGridPen);
        DeleteObject(g_hParentPen);
        DeleteObject(g_hCompleteRoutePen);
        DeleteObject(g_hObstacleTileBrush);
        DeleteObject(g_hStartTileBrush);
        DeleteObject(g_hEndTileBrush);
        DeleteObject(g_hOpenListTileBrush);
        DeleteObject(g_hCloseListTileBrush);

        SelectObject(g_hMemDC, g_hMemDCBitmap_old);
        DeleteObject(g_hMemDC);
        DeleteObject(g_hMemDCBitmap);

        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);

    }

    return 0;
}


void DrawParentLine(HDC hdc)
{
    /*HPEN hPenOld = (HPEN)SelectObject(hdc, g_hParentPen);

    list<Node*>::iterator iter;
    for (iter = openList.begin(); iter != openList.end(); ++iter)
    {
        Node* tmp = *iter;

        int oldX = tmp->_x;
        oldX = oldX * GRID_SIZE + (GRID_SIZE / 2);
        int oldY = tmp->_y;
        oldY = oldY * GRID_SIZE + (GRID_SIZE / 2);

        int curX = tmp->parent->_x;
        curX = curX * GRID_SIZE + (GRID_SIZE / 2);
        int curY = tmp->parent->_y;
        curY = curY * GRID_SIZE + (GRID_SIZE / 2);

        curX = curX + (oldX - curX) / 2;
        curY = curY + (oldY - curY) / 2;

        MoveToEx(hdc, oldX, oldY, NULL);
        LineTo(hdc, curX, curY);
    }

    for (iter = closeList.begin(); iter != closeList.end(); ++iter)
    {
        Node* tmp = *iter;

        if (tmp->parent == NULL)
            continue;

        int oldX = tmp->_x;
        oldX = oldX * GRID_SIZE + (GRID_SIZE / 2);
        int oldY = tmp->_y;
        oldY = oldY * GRID_SIZE + (GRID_SIZE / 2);

        int curX = tmp->parent->_x;
        curX = curX * GRID_SIZE + (GRID_SIZE / 2);
        int curY = tmp->parent->_y;
        curY = curY * GRID_SIZE + (GRID_SIZE / 2);

        curX = curX + (oldX - curX) / 2;
        curY = curY + (oldY - curY) / 2;

        MoveToEx(hdc, oldX, oldY, NULL);
        LineTo(hdc, curX, curY);
    }

    SelectObject(hdc, hPenOld);
    */

    HPEN hPenOld = (HPEN)SelectObject(hdc, g_hCompleteRoutePen);
    list<Node*>::iterator iter;
    for (iter = completeRouteList.begin(); iter != completeRouteList.end(); ++iter)
    {
        Node* tmp = *iter;

        if (tmp->parent == NULL)
            continue;

        int oldX = tmp->_x;
        oldX = oldX * GRID_SIZE + (GRID_SIZE / 2);
        int oldY = tmp->_y;
        oldY = oldY * GRID_SIZE + (GRID_SIZE / 2);

        int curX = tmp->parent->_x;
        curX = curX * GRID_SIZE + (GRID_SIZE / 2);
        int curY = tmp->parent->_y;
        curY = curY * GRID_SIZE + (GRID_SIZE / 2);

        MoveToEx(hdc, oldX, oldY, NULL);
        LineTo(hdc, curX, curY);
    }

    SelectObject(hdc, hPenOld);
}

/* 타일그리기용 함수 */
void RenderGrid(HDC hdc)
{
    int iX = 0;
    int iY = 0;

    HPEN hOldPen = (HPEN)SelectObject(hdc, g_hGridPen);

    for (int i = 0; i <= GRID_WIDTH; i++)
    {
        MoveToEx(hdc, iX, 0, NULL);
        LineTo(hdc, iX, GRID_HEIGHT * GRID_SIZE);
        iX += GRID_SIZE;
    }

    for (int i = 0; i <= GRID_HEIGHT; i++)
    {
        MoveToEx(hdc, 0, iY, NULL);
        LineTo(hdc, GRID_WIDTH * GRID_SIZE, iY);
        iY += GRID_SIZE;
    }

    SelectObject(hdc, hOldPen);
}

/* 노드 그리기용 함수 */
void RenderNode(HDC hdc)
{
    int iX = 0;
    int iY = 0;

    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, g_hObstacleTileBrush);
    SelectObject(hdc, GetStockObject(NULL_PEN));

    for (int iW = 0; iW < GRID_WIDTH; iW++)
    {
        for (int iH = 0; iH < GRID_HEIGHT; iH++)
        {
            if (g_Tile[iH][iW] == OBSTACLE)
            {
                iX = iW * GRID_SIZE;
                iY = iH * GRID_SIZE;

                hOldBrush = (HBRUSH)SelectObject(hdc, g_hObstacleTileBrush);

                Rectangle(hdc, iX, iY, iX + GRID_SIZE + 2, iY + GRID_SIZE + 2);
            }

            else if (g_Tile[iH][iW] == START)
            {
                iX = iW * GRID_SIZE;
                iY = iH * GRID_SIZE;

                hOldBrush = (HBRUSH)SelectObject(hdc, g_hStartTileBrush);

                Rectangle(hdc, iX, iY, iX + GRID_SIZE + 2, iY + GRID_SIZE + 2);
            }

            else if (g_Tile[iH][iW] == END)
            {
                iX = iW * GRID_SIZE;
                iY = iH * GRID_SIZE;

                hOldBrush = (HBRUSH)SelectObject(hdc, g_hEndTileBrush);

                Rectangle(hdc, iX, iY, iX + GRID_SIZE + 2, iY + GRID_SIZE + 2);
            }

            else if (g_Tile[iH][iW] == OPENLIST)
            {
                iX = iW * GRID_SIZE;
                iY = iH * GRID_SIZE;

                hOldBrush = (HBRUSH)SelectObject(hdc, g_hOpenListTileBrush);

                Rectangle(hdc, iX, iY, iX + GRID_SIZE + 2, iY + GRID_SIZE + 2);
            }

            else if (g_Tile[iH][iW] == CLOSELIST)
            {
                iX = iW * GRID_SIZE;
                iY = iH * GRID_SIZE;

                hOldBrush = (HBRUSH)SelectObject(hdc, g_hCloseListTileBrush);

                Rectangle(hdc, iX, iY, iX + GRID_SIZE + 2, iY + GRID_SIZE + 2);
            }
        }
    }

    SelectObject(hdc, hOldBrush);
}


double GetEuclid(int nowY, int nowX, int targetY, int targetX)
{
    return sqrt(pow(nowY - targetY, 2) + pow(nowX - targetX, 2));
}

int GetManhattan(int nowY, int nowX, int targetY, int targetX)
{
    return abs(nowY - targetY) + abs(nowX - targetX);
}

void FindPath(HWND hWnd)
{
    if (openList.empty())
        return;

    GetMinFNode();

    Node* nowNode = openList.front();
    openList.pop_front();

    if (nowNode->_x == endNodeYX.second && nowNode->_y == endNodeYX.first)
    {
        // TODO: 도착
        
        Node* tmpNode = nowNode;
        while (1)
        {
            if (tmpNode->parent == NULL)
                break;

            completeRouteList.push_front(tmpNode);

            tmpNode = tmpNode->parent;
        }

        KillTimer(hWnd, 10);
        return;
    }

    if (F_Tile[nowNode->_y][nowNode->_x] < nowNode->_F)
        return;

    closeList.push_front(nowNode);
    TileMap[nowNode->_y][nowNode->_x] = false;
    g_Tile[nowNode->_y][nowNode->_x] = CLOSELIST;

    unsigned int nowDir = nowNode->_dir;

    if ((nowDir & LL) == LL)
    {
        int nextY = nowNode->_y;
        int nextX = nowNode->_x;
        int cnt = 0;

        while (1)
        {
            cnt++;

            cout << cnt << endl;

            nextX = nextX - 1;

            if (CanGo(nextY, nextX) == false)
                break;

            if (IsCorner(LL, nextY, nextX) == false && (nextX == endNodeYX.second && nextY == endNodeYX.first) == false)
                continue;

            double g = nowNode->_G + 1 * cnt;
            double h = GetManhattan(nextY, nextX, endNodeYX.first, endNodeYX.second);
            double f = g + h;

            if (g_Tile[nextY][nextX] == OPENLIST)
            {
                if (f < F_Tile[nextY][nextX])
                {
                    list<Node*>::iterator iter;
                    for (iter = openList.begin(); iter != openList.end(); ++iter)
                    {
                        if ((*iter)->_x == nextX && (*iter)->_y == nextY)
                        {
                            (*iter)->_G = g;
                            (*iter)->_H = h;
                            (*iter)->_F = f;

                            // 기본 방향
                            (*iter)->_dir = 0;
                            (*iter)->_dir = (*iter)->_dir | LL;

                            // 옵션 방향
                            if (TileMap[(*iter)->_y + 1][(*iter)->_x] == false && TileMap[(*iter)->_y + 1][(*iter)->_x - 1] == true)
                                (*iter)->_dir = (*iter)->_dir | LU;
                            if (TileMap[(*iter)->_y - 1][(*iter)->_x] == false && TileMap[(*iter)->_y - 1][(*iter)->_x - 1] == true)
                                (*iter)->_dir = (*iter)->_dir | LD;

                            (*iter)->parent = nowNode;

                            F_Tile[nextY][nextX] = f;

                            break;
                        }
                    }
                }
            }
            else
            {
                Node* newNode = new Node();

                newNode->_y = nextY;
                newNode->_x = nextX;

                newNode->_G = g;
                newNode->_H = h;
                newNode->_F = f;

                // 기본 방향
                newNode->_dir = newNode->_dir | LL;

                // 옵션 방향
                if (TileMap[newNode->_y + 1][newNode->_x] == false && TileMap[newNode->_y + 1][newNode->_x - 1] == true)
                    newNode->_dir = newNode->_dir | LU;
                if (TileMap[newNode->_y - 1][newNode->_x] == false && TileMap[newNode->_y - 1][newNode->_x - 1] == true)
                    newNode->_dir = newNode->_dir | LD;

                newNode->parent = nowNode;

                F_Tile[nextY][nextX] = newNode->_F;

                openList.push_back(newNode);
                g_Tile[newNode->_y][newNode->_x] = OPENLIST;
            }

            break;
        }
    }
    /*if ((nowDir & LU) == LU)
    {
        int nextY = nowNode->_y;
        int nextX = nowNode->_x;
        int cnt = 0;

        while (1)
        {
            cnt++;

            nextY = nextY - 1;
            nextX = nextX - 1;

            if (CanGo(nextY, nextX) == false)
                break;

            if (IsCorner(LU, nextY, nextX) || (nextX == endNodeYX.second && nextY == endNodeYX.first))
            {
                double g = nowNode->_G + 1.4 * cnt;
                double h = GetManhattan(nextY, nextX, endNodeYX.first, endNodeYX.second);
                double f = g + h;

                if (g_Tile[nextY][nextX] == OPENLIST)
                {
                    if (f < F_Tile[nextY][nextX])
                    {
                        list<Node*>::iterator iter;
                        for (iter = openList.begin(); iter != openList.end(); ++iter)
                        {
                            if ((*iter)->_x == nextX && (*iter)->_y == nextY)
                            {
                                (*iter)->_G = g;
                                (*iter)->_H = h;
                                (*iter)->_F = f;

                                // TODO: 방향설정
                                // 기본 방향
                                (*iter)->_dir = 0;
                                (*iter)->_dir = (*iter)->_dir | LU | LL | UU;

                                // 옵션 방향
                                if (TileMap[nextY][nextX + 1] == false && TileMap[nextY - 1][nextX + 1] == true)
                                    (*iter)->_dir = (*iter)->_dir | RU;
                                if (TileMap[nextY + 1][nextX] == false && TileMap[nextY + 1][nextX - 1] == true)
                                    (*iter)->_dir = (*iter)->_dir | LD;

                                (*iter)->parent = nowNode;

                                F_Tile[nextY][nextX] = f;

                                break;
                            }
                        }
                    }
                }
                else
                {
                    Node* newNode = new Node();

                    newNode->_y = nextY;
                    newNode->_x = nextX;

                    newNode->_G = g;
                    newNode->_H = h;
                    newNode->_F = f;

                    // 기본 방향
                    newNode->_dir = newNode->_dir | LU | LL | UU;

                    // 옵션 방향
                    if (TileMap[newNode->_y][newNode->_x + 1] == false && TileMap[newNode->_y - 1][newNode->_x + 1] == true)
                        newNode->_dir = newNode->_dir | RU;
                    if (TileMap[newNode->_y + 1][newNode->_x] == false && TileMap[newNode->_y + 1][newNode->_x - 1] == true)
                        newNode->_dir = newNode->_dir | LD;

                    newNode->parent = nowNode;

                    F_Tile[nextY][nextX] = newNode->_F;

                    openList.push_back(newNode);
                    g_Tile[newNode->_y][newNode->_x] = OPENLIST;
                }

                break;
            }

            // 수평 쭉 체크
            int horizontal_nextX = nextX;
            int horizontal_cnt = 0;
            while (1)
            {
                horizontal_cnt++;
                horizontal_nextX = horizontal_nextX - 1;

                if (CanGo(nextY, horizontal_nextX) == false)
                    break;

                if (IsCorner(LL, nextY, horizontal_nextX) || (horizontal_nextX == endNodeYX.second && nextY == endNodeYX.first))
                {
                    double g = nowNode->_G + 1.4 * cnt;
                    double h = GetManhattan(nextY, nextX, endNodeYX.first, endNodeYX.second);
                    double f = g + h;

                    if (g_Tile[nextY][nextX] == OPENLIST)
                    {
                        if (f < F_Tile[nextY][nextX])
                        {
                            list<Node*>::iterator iter;
                            for (iter = openList.begin(); iter != openList.end(); ++iter)
                            {
                                if ((*iter)->_x == nextX && (*iter)->_y == nextY)
                                {
                                    (*iter)->_G = g;
                                    (*iter)->_H = h;
                                    (*iter)->_F = f;

                                    // TODO: 방향설정
                                    (*iter)->_dir = 0;
                                    (*iter)->_dir = (*iter)->_dir | LU | LL | UU;

                                    (*iter)->parent = nowNode;

                                    F_Tile[nextY][nextX] = f;

                                    break;
                                }
                            }
                        }
                    }
                    else
                    {
                        Node* newNode = new Node();

                        newNode->_y = nextY;
                        newNode->_x = nextX;

                        newNode->_G = g;
                        newNode->_H = h;
                        newNode->_F = f;

                        // 기본 방향
                        newNode->_dir = newNode->_dir | LU | LL | UU;

                        newNode->parent = nowNode;

                        F_Tile[nextY][nextX] = newNode->_F;

                        openList.push_back(newNode);
                        g_Tile[newNode->_y][newNode->_x] = OPENLIST;
                    }

                    break;
                }
            }

            // 수직 쭉 체크
            int vertical_nextY = nextY;
            int vertical_cnt = 0;
            while (1)
            {
                vertical_cnt++;
                vertical_nextY = vertical_nextY - 1;

                if (CanGo(nextY, horizontal_nextX) == false)
                    break;

                if (IsCorner(UU, nextY, nextX) || (nextX == endNodeYX.second && nextY == endNodeYX.first))
                {
                    double g = nowNode->_G + 1.4 * cnt;
                    double h = GetManhattan(nextY, nextX, endNodeYX.first, endNodeYX.second);
                    double f = g + h;

                    if (g_Tile[nextY][nextX] == OPENLIST)
                    {
                        if (f < F_Tile[nextY][nextX])
                        {
                            list<Node*>::iterator iter;
                            for (iter = openList.begin(); iter != openList.end(); ++iter)
                            {
                                if ((*iter)->_x == nextX && (*iter)->_y == nextY)
                                {
                                    (*iter)->_G = g;
                                    (*iter)->_H = h;
                                    (*iter)->_F = f;

                                    // TODO: 방향설정
                                    (*iter)->_dir = 0;
                                    (*iter)->_dir = (*iter)->_dir | LU | LL | UU;

                                    (*iter)->parent = nowNode;

                                    F_Tile[nextY][nextX] = f;

                                    break;
                                }
                            }
                        }
                    }
                    else
                    {
                        Node* newNode = new Node();

                        newNode->_y = nextY;
                        newNode->_x = nextX;

                        newNode->_G = g;
                        newNode->_H = h;
                        newNode->_F = f;

                        // 기본 방향
                        newNode->_dir = newNode->_dir | LU | LL | UU;

                        newNode->parent = nowNode;

                        F_Tile[nextY][nextX] = newNode->_F;

                        openList.push_back(newNode);
                        g_Tile[newNode->_y][newNode->_x] = OPENLIST;
                    }

                    break;
                }
            }

        }
    }
    if ((nowDir & UU) == UU)
    {
        int nextY = nowNode->_y;
        int nextX = nowNode->_x;
        int cnt = 0;

        while (1)
        {
            cnt++;
            nextY = nextY - 1;

            if (CanGo(nextY, nextX) == false)
                break;

            if (IsCorner(UU, nextY, nextX) == false && (nextX == endNodeYX.second && nextY == endNodeYX.first) == false)
                continue;

            double g = nowNode->_G + 1 * cnt;
            double h = GetManhattan(nextY, nextX, endNodeYX.first, endNodeYX.second);
            double f = g + h;

            if (g_Tile[nextY][nextX] == OPENLIST)
            {
                if (f < F_Tile[nextY][nextX])
                {
                    list<Node*>::iterator iter;
                    for (iter = openList.begin(); iter != openList.end(); ++iter)
                    {
                        if ((*iter)->_x == nextX && (*iter)->_y == nextY)
                        {
                            (*iter)->_G = g;
                            (*iter)->_H = h;
                            (*iter)->_F = f;

                            // 기본 방향
                            (*iter)->_dir = 0;
                            (*iter)->_dir = (*iter)->_dir | UU;

                            // 옵션 방향
                            if (TileMap[(*iter)->_y][(*iter)->_x - 1] == false && TileMap[(*iter)->_y - 1][(*iter)->_x - 1] == true)
                                (*iter)->_dir = (*iter)->_dir | LU;
                            if (TileMap[(*iter)->_y][(*iter)->_x + 1] == false && TileMap[(*iter)->_y - 1][(*iter)->_x + 1] == true)
                                (*iter)->_dir = (*iter)->_dir | RU;

                            (*iter)->parent = nowNode;

                            F_Tile[nextY][nextX] = f;

                            break;
                        }
                    }
                }
            }
            else
            {
                Node* newNode = new Node();

                newNode->_y = nextY;
                newNode->_x = nextX;

                newNode->_G = g;
                newNode->_H = h;
                newNode->_F = f;

                // 기본 방향
                newNode->_dir = newNode->_dir | UU;

                // 옵션 방향
                if (TileMap[newNode->_y][newNode->_x - 1] == false && TileMap[newNode->_y - 1][newNode->_x - 1] == true)
                    newNode->_dir = newNode->_dir | LU;
                if (TileMap[newNode->_y][newNode->_x + 1] == false && TileMap[newNode->_y - 1][newNode->_x + 1] == true)
                    newNode->_dir = newNode->_dir | RU;

                newNode->parent = nowNode;

                F_Tile[nextY][nextX] = newNode->_F;

                openList.push_back(newNode);
                g_Tile[newNode->_y][newNode->_x] = OPENLIST;
            }

            break;
        }
    }
    if ((nowDir & RU) == RU)
    {
        int nextY = nowNode->_y;
        int nextX = nowNode->_x;
        int cnt = 0;

        while (1)
        {
            cnt++;

            nextY = nextY - 1;
            nextX = nextX + 1;

            if (CanGo(nextY, nextX) == false)
                break;

            if (IsCorner(RU, nextY, nextX) || (nextX == endNodeYX.second && nextY == endNodeYX.first))
            {
                double g = nowNode->_G + 1.4 * cnt;
                double h = GetManhattan(nextY, nextX, endNodeYX.first, endNodeYX.second);
                double f = g + h;

                if (g_Tile[nextY][nextX] == OPENLIST)
                {
                    if (f < F_Tile[nextY][nextX])
                    {
                        list<Node*>::iterator iter;
                        for (iter = openList.begin(); iter != openList.end(); ++iter)
                        {
                            if ((*iter)->_x == nextX && (*iter)->_y == nextY)
                            {
                                (*iter)->_G = g;
                                (*iter)->_H = h;
                                (*iter)->_F = f;

                                // TODO: 방향설정
                                // 기본 방향
                                (*iter)->_dir = 0;
                                (*iter)->_dir = (*iter)->_dir | RU | RR | UU;

                                // 옵션 방향
                                if (TileMap[nextY + 1][nextX] == false && TileMap[nextY + 1][nextX + 1] == true)
                                    (*iter)->_dir = (*iter)->_dir | RD;
                                if (TileMap[nextY][nextX - 1] == false && TileMap[nextY - 1][nextX - 1] == true)
                                    (*iter)->_dir = (*iter)->_dir | LU;

                                (*iter)->parent = nowNode;

                                F_Tile[nextY][nextX] = f;

                                break;
                            }
                        }
                    }
                }
                else
                {
                    Node* newNode = new Node();

                    newNode->_y = nextY;
                    newNode->_x = nextX;

                    newNode->_G = g;
                    newNode->_H = h;
                    newNode->_F = f;

                    // 기본 방향
                    newNode->_dir = newNode->_dir | RU | RR | UU;

                    // 옵션 방향
                    if (TileMap[newNode->_y + 1][newNode->_x] == false && TileMap[newNode->_y + 1][newNode->_x + 1] == true)
                        newNode->_dir = newNode->_dir | RD;
                    if (TileMap[newNode->_y][newNode->_x - 1] == false && TileMap[newNode->_y - 1][newNode->_x - 1] == true)
                        newNode->_dir = newNode->_dir | LU;

                    newNode->parent = nowNode;

                    F_Tile[nextY][nextX] = newNode->_F;

                    openList.push_back(newNode);
                    g_Tile[newNode->_y][newNode->_x] = OPENLIST;
                }

                break;
            }

            // 수평 쭉 체크
            int horizontal_nextX = nextX;
            int horizontal_cnt = 0;
            while (1)
            {
                horizontal_cnt++;
                horizontal_nextX = horizontal_nextX + 1;

                if (CanGo(nextY, horizontal_nextX) == false)
                    break;

                if (IsCorner(RR, nextY, horizontal_nextX) || (horizontal_nextX == endNodeYX.second && nextY == endNodeYX.first))
                {
                    double g = nowNode->_G + 1.4 * cnt;
                    double h = GetManhattan(nextY, nextX, endNodeYX.first, endNodeYX.second);
                    double f = g + h;

                    if (g_Tile[nextY][nextX] == OPENLIST)
                    {
                        if (f < F_Tile[nextY][nextX])
                        {
                            list<Node*>::iterator iter;
                            for (iter = openList.begin(); iter != openList.end(); ++iter)
                            {
                                if ((*iter)->_x == nextX && (*iter)->_y == nextY)
                                {
                                    (*iter)->_G = g;
                                    (*iter)->_H = h;
                                    (*iter)->_F = f;

                                    // TODO: 방향설정
                                    (*iter)->_dir = 0;
                                    (*iter)->_dir = (*iter)->_dir | RU | RR | UU;

                                    (*iter)->parent = nowNode;

                                    F_Tile[nextY][nextX] = f;

                                    break;
                                }
                            }
                        }
                    }
                    else
                    {
                        Node* newNode = new Node();

                        newNode->_y = nextY;
                        newNode->_x = nextX;

                        newNode->_G = g;
                        newNode->_H = h;
                        newNode->_F = f;

                        // 기본 방향
                        newNode->_dir = newNode->_dir | RU | RR | UU;

                        newNode->parent = nowNode;

                        F_Tile[nextY][nextX] = newNode->_F;

                        openList.push_back(newNode);
                        g_Tile[newNode->_y][newNode->_x] = OPENLIST;
                    }

                    break;
                }
            }

            // 수직 쭉 체크
            int vertical_nextY = nextY;
            int vertical_cnt = 0;
            while (1)
            {
                vertical_cnt++;
                vertical_nextY = vertical_nextY - 1;

                if (CanGo(nextY, horizontal_nextX) == false)
                    break;

                if (IsCorner(UU, nextY, nextX) || (nextX == endNodeYX.second && nextY == endNodeYX.first))
                {
                    double g = nowNode->_G + 1.4 * cnt;
                    double h = GetManhattan(nextY, nextX, endNodeYX.first, endNodeYX.second);
                    double f = g + h;

                    if (g_Tile[nextY][nextX] == OPENLIST)
                    {
                        if (f < F_Tile[nextY][nextX])
                        {
                            list<Node*>::iterator iter;
                            for (iter = openList.begin(); iter != openList.end(); ++iter)
                            {
                                if ((*iter)->_x == nextX && (*iter)->_y == nextY)
                                {
                                    (*iter)->_G = g;
                                    (*iter)->_H = h;
                                    (*iter)->_F = f;

                                    // TODO: 방향설정
                                    (*iter)->_dir = 0;
                                    (*iter)->_dir = (*iter)->_dir | RU | RR | UU;

                                    (*iter)->parent = nowNode;

                                    F_Tile[nextY][nextX] = f;

                                    break;
                                }
                            }
                        }
                    }
                    else
                    {
                        Node* newNode = new Node();

                        newNode->_y = nextY;
                        newNode->_x = nextX;

                        newNode->_G = g;
                        newNode->_H = h;
                        newNode->_F = f;

                        // 기본 방향
                        newNode->_dir = newNode->_dir | RU | RR | UU;

                        newNode->parent = nowNode;

                        F_Tile[nextY][nextX] = newNode->_F;

                        openList.push_back(newNode);
                        g_Tile[newNode->_y][newNode->_x] = OPENLIST;
                    }

                    break;
                }
            }

        }

    }
    if ((nowDir & RR) == RR)
    {
        int nextY = nowNode->_y;
        int nextX = nowNode->_x;
        int cnt = 0;

        while (1)
        {
            cnt++;
            nextX = nextX + 1;

            if (CanGo(nextY, nextX) == false)
                break;

            if (IsCorner(RR, nextY, nextX) == false && (nextX == endNodeYX.second && nextY == endNodeYX.first) == false)
                continue;

            double g = nowNode->_G + 1 * cnt;
            double h = GetManhattan(nextY, nextX, endNodeYX.first, endNodeYX.second);
            double f = g + h;

            if (g_Tile[nextY][nextX] == OPENLIST)
            {
                if (f < F_Tile[nextY][nextX])
                {
                    list<Node*>::iterator iter;
                    for (iter = openList.begin(); iter != openList.end(); ++iter)
                    {
                        if ((*iter)->_x == nextX && (*iter)->_y == nextY)
                        {
                            (*iter)->_G = g;
                            (*iter)->_H = h;
                            (*iter)->_F = f;

                            // 기본 방향
                            (*iter)->_dir = 0;
                            (*iter)->_dir = (*iter)->_dir | RR;

                            // 옵션 방향
                            if (TileMap[(*iter)->_y + 1][(*iter)->_x] == false && TileMap[(*iter)->_y + 1][(*iter)->_x + 1] == true)
                                (*iter)->_dir = (*iter)->_dir | RD;
                            if (TileMap[(*iter)->_y - 1][(*iter)->_x] == false && TileMap[(*iter)->_y - 1][(*iter)->_x + 1] == true)
                                (*iter)->_dir = (*iter)->_dir | RU;

                            (*iter)->parent = nowNode;

                            F_Tile[nextY][nextX] = f;

                            break;
                        }
                    }
                }
            }
            else
            {
                Node* newNode = new Node();

                newNode->_y = nextY;
                newNode->_x = nextX;

                newNode->_G = g;
                newNode->_H = h;
                newNode->_F = f;

                // 기본 방향
                newNode->_dir = newNode->_dir | RR;

                // 옵션 방향
                if (TileMap[newNode->_y + 1][newNode->_x] == false && TileMap[newNode->_y + 1][newNode->_x + 1] == true)
                    newNode->_dir = newNode->_dir | RD;
                if (TileMap[newNode->_y - 1][newNode->_x] == false && TileMap[newNode->_y - 1][newNode->_x + 1] == true)
                    newNode->_dir = newNode->_dir | RU;

                newNode->parent = nowNode;

                F_Tile[nextY][nextX] = newNode->_F;

                openList.push_back(newNode);
                g_Tile[newNode->_y][newNode->_x] = OPENLIST;
            }

            break;
        }
    }
    if ((nowDir & RD) == RD)
    {
        int nextY = nowNode->_y;
        int nextX = nowNode->_x;
        int cnt = 0;

        while (1)
        {
            cnt++;

            nextY = nextY + 1;
            nextX = nextX + 1;

            if (CanGo(nextY, nextX) == false)
                break;

            if (IsCorner(RD, nextY, nextX) || (nextX == endNodeYX.second && nextY == endNodeYX.first))
            {
                double g = nowNode->_G + 1.4 * cnt;
                double h = GetManhattan(nextY, nextX, endNodeYX.first, endNodeYX.second);
                double f = g + h;

                if (g_Tile[nextY][nextX] == OPENLIST)
                {
                    if (f < F_Tile[nextY][nextX])
                    {
                        list<Node*>::iterator iter;
                        for (iter = openList.begin(); iter != openList.end(); ++iter)
                        {
                            if ((*iter)->_x == nextX && (*iter)->_y == nextY)
                            {
                                (*iter)->_G = g;
                                (*iter)->_H = h;
                                (*iter)->_F = f;

                                // TODO: 방향설정
                                // 기본 방향
                                (*iter)->_dir = 0;
                                (*iter)->_dir = (*iter)->_dir | RD | RR | DD;

                                // 옵션 방향
                                if (TileMap[nextY - 1][nextX] == false && TileMap[nextY - 1][nextX + 1] == true)
                                    (*iter)->_dir = (*iter)->_dir | RU;
                                if (TileMap[nextY][nextX - 1] == false && TileMap[nextY + 1][nextX - 1] == true)
                                    (*iter)->_dir = (*iter)->_dir | LD;

                                (*iter)->parent = nowNode;

                                F_Tile[nextY][nextX] = f;

                                break;
                            }
                        }
                    }
                }
                else
                {
                    Node* newNode = new Node();

                    newNode->_y = nextY;
                    newNode->_x = nextX;

                    newNode->_G = g;
                    newNode->_H = h;
                    newNode->_F = f;

                    // 기본 방향
                    newNode->_dir = newNode->_dir | RD | RR | DD;

                    // 옵션 방향
                    if (TileMap[newNode->_y - 1][newNode->_x] == false && TileMap[newNode->_y - 1][newNode->_x + 1] == true)
                        newNode->_dir = newNode->_dir | RU;
                    if (TileMap[newNode->_y][newNode->_x - 1] == false && TileMap[newNode->_y + 1][newNode->_x - 1] == true)
                        newNode->_dir = newNode->_dir | LD;

                    newNode->parent = nowNode;

                    F_Tile[nextY][nextX] = newNode->_F;

                    openList.push_back(newNode);
                    g_Tile[newNode->_y][newNode->_x] = OPENLIST;
                }

                break;
            }

            // 수평 쭉 체크
            int horizontal_nextX = nextX;
            int horizontal_cnt = 0;
            while (1)
            {
                horizontal_cnt++;
                horizontal_nextX = horizontal_nextX + 1;

                if (CanGo(nextY, horizontal_nextX) == false)
                    break;

                if (IsCorner(RR, nextY, horizontal_nextX) || (horizontal_nextX == endNodeYX.second && nextY == endNodeYX.first))
                {
                    double g = nowNode->_G + 1.4 * cnt;
                    double h = GetManhattan(nextY, nextX, endNodeYX.first, endNodeYX.second);
                    double f = g + h;

                    if (g_Tile[nextY][nextX] == OPENLIST)
                    {
                        if (f < F_Tile[nextY][nextX])
                        {
                            list<Node*>::iterator iter;
                            for (iter = openList.begin(); iter != openList.end(); ++iter)
                            {
                                if ((*iter)->_x == nextX && (*iter)->_y == nextY)
                                {
                                    (*iter)->_G = g;
                                    (*iter)->_H = h;
                                    (*iter)->_F = f;

                                    // TODO: 방향설정
                                    (*iter)->_dir = 0;
                                    (*iter)->_dir = (*iter)->_dir | RD | RR | DD;

                                    (*iter)->parent = nowNode;

                                    F_Tile[nextY][nextX] = f;

                                    break;
                                }
                            }
                        }
                    }
                    else
                    {
                        Node* newNode = new Node();

                        newNode->_y = nextY;
                        newNode->_x = nextX;

                        newNode->_G = g;
                        newNode->_H = h;
                        newNode->_F = f;

                        // 기본 방향
                        newNode->_dir = newNode->_dir | RD | RR | DD;

                        newNode->parent = nowNode;

                        F_Tile[nextY][nextX] = newNode->_F;

                        openList.push_back(newNode);
                        g_Tile[newNode->_y][newNode->_x] = OPENLIST;
                    }

                    break;
                }
            }

            // 수직 쭉 체크
            int vertical_nextY = nextY;
            int vertical_cnt = 0;
            while (1)
            {
                vertical_cnt++;
                vertical_nextY = vertical_nextY + 1;

                if (CanGo(nextY, horizontal_nextX) == false)
                    break;

                if (IsCorner(UU, nextY, nextX) || (nextX == endNodeYX.second && nextY == endNodeYX.first))
                {
                    double g = nowNode->_G + 1.4 * cnt;
                    double h = GetManhattan(nextY, nextX, endNodeYX.first, endNodeYX.second);
                    double f = g + h;

                    if (g_Tile[nextY][nextX] == OPENLIST)
                    {
                        if (f < F_Tile[nextY][nextX])
                        {
                            list<Node*>::iterator iter;
                            for (iter = openList.begin(); iter != openList.end(); ++iter)
                            {
                                if ((*iter)->_x == nextX && (*iter)->_y == nextY)
                                {
                                    (*iter)->_G = g;
                                    (*iter)->_H = h;
                                    (*iter)->_F = f;

                                    // TODO: 방향설정
                                    (*iter)->_dir = 0;
                                    (*iter)->_dir = (*iter)->_dir | RD | RR | DD;

                                    (*iter)->parent = nowNode;

                                    F_Tile[nextY][nextX] = f;

                                    break;
                                }
                            }
                        }
                    }
                    else
                    {
                        Node* newNode = new Node();

                        newNode->_y = nextY;
                        newNode->_x = nextX;

                        newNode->_G = g;
                        newNode->_H = h;
                        newNode->_F = f;

                        // 기본 방향
                        newNode->_dir = newNode->_dir | RD | RR | DD;

                        newNode->parent = nowNode;

                        F_Tile[nextY][nextX] = newNode->_F;

                        openList.push_back(newNode);
                        g_Tile[newNode->_y][newNode->_x] = OPENLIST;
                    }

                    break;
                }
            }

        }
    }
    if ((nowDir & DD) == DD)
    {
        int nextY = nowNode->_y;
        int nextX = nowNode->_x;
        int cnt = 0;

        while (1)
        {
            cnt++;
            nextY = nextY + 1;

            if (CanGo(nextY, nextX) == false)
                break;

            if (IsCorner(DD, nextY, nextX) == false && (nextX == endNodeYX.second && nextY == endNodeYX.first) == false)
                continue;

            double g = nowNode->_G + 1 * cnt;
            double h = GetManhattan(nextY, nextX, endNodeYX.first, endNodeYX.second);
            double f = g + h;

            if (g_Tile[nextY][nextX] == OPENLIST)
            {
                if (f < F_Tile[nextY][nextX])
                {
                    list<Node*>::iterator iter;
                    for (iter = openList.begin(); iter != openList.end(); ++iter)
                    {
                        if ((*iter)->_x == nextX && (*iter)->_y == nextY)
                        {
                            (*iter)->_G = g;
                            (*iter)->_H = h;
                            (*iter)->_F = f;

                            // 기본 방향
                            (*iter)->_dir = 0;
                            (*iter)->_dir = (*iter)->_dir | DD;

                            // 옵션 방향
                            if (TileMap[(*iter)->_y][(*iter)->_x - 1] == false && TileMap[(*iter)->_y + 1][(*iter)->_x - 1] == true)
                                (*iter)->_dir = (*iter)->_dir | RD;
                            if (TileMap[(*iter)->_y][(*iter)->_x + 1] == false && TileMap[(*iter)->_y + 1][(*iter)->_x + 1] == true)
                                (*iter)->_dir = (*iter)->_dir | LD;

                            (*iter)->parent = nowNode;

                            F_Tile[nextY][nextX] = f;

                            break;
                        }
                    }
                }
            }
            else
            {
                Node* newNode = new Node();

                newNode->_y = nextY;
                newNode->_x = nextX;

                newNode->_G = g;
                newNode->_H = h;
                newNode->_F = f;

                // 기본 방향
                newNode->_dir = newNode->_dir | DD;

                // 옵션 방향
                if (TileMap[newNode->_y][newNode->_x - 1] == false && TileMap[newNode->_y + 1][newNode->_x - 1] == true)
                    newNode->_dir = newNode->_dir | RD;
                if (TileMap[newNode->_y][newNode->_x + 1] == false && TileMap[newNode->_y + 1][newNode->_x + 1] == true)
                    newNode->_dir = newNode->_dir | LD;

                newNode->parent = nowNode;

                F_Tile[nextY][nextX] = newNode->_F;

                openList.push_back(newNode);
                g_Tile[newNode->_y][newNode->_x] = OPENLIST;
            }

            break;
        }
    }
    if ((nowDir & LD) == LD)
    {
        int nextY = nowNode->_y;
        int nextX = nowNode->_x;
        int cnt = 0;

        while (1)
        {
            cnt++;

            nextY = nextY + 1;
            nextX = nextX - 1;

            if (CanGo(nextY, nextX) == false)
                break;

            if (IsCorner(LD, nextY, nextX) || (nextX == endNodeYX.second && nextY == endNodeYX.first))
            {
                double g = nowNode->_G + 1.4 * cnt;
                double h = GetManhattan(nextY, nextX, endNodeYX.first, endNodeYX.second);
                double f = g + h;

                if (g_Tile[nextY][nextX] == OPENLIST)
                {
                    if (f < F_Tile[nextY][nextX])
                    {
                        list<Node*>::iterator iter;
                        for (iter = openList.begin(); iter != openList.end(); ++iter)
                        {
                            if ((*iter)->_x == nextX && (*iter)->_y == nextY)
                            {
                                (*iter)->_G = g;
                                (*iter)->_H = h;
                                (*iter)->_F = f;

                                // TODO: 방향설정
                                // 기본 방향
                                (*iter)->_dir = 0;
                                (*iter)->_dir = (*iter)->_dir | LD | LL | DD;

                                // 옵션 방향
                                if (TileMap[nextY - 1][nextX] == false && TileMap[nextY - 1][nextX - 1] == true)
                                    (*iter)->_dir = (*iter)->_dir | LU;
                                if (TileMap[nextY][nextX + 1] == false && TileMap[nextY + 1][nextX + 1] == true)
                                    (*iter)->_dir = (*iter)->_dir | RD;

                                (*iter)->parent = nowNode;

                                F_Tile[nextY][nextX] = f;

                                break;
                            }
                        }
                    }
                }
                else
                {
                    Node* newNode = new Node();

                    newNode->_y = nextY;
                    newNode->_x = nextX;

                    newNode->_G = g;
                    newNode->_H = h;
                    newNode->_F = f;

                    // 기본 방향
                    newNode->_dir = newNode->_dir | LD | LL | DD;

                    // 옵션 방향
                    if (TileMap[newNode->_y - 1][newNode->_x] == false && TileMap[newNode->_y - 1][newNode->_x - 1] == true)
                        newNode->_dir = newNode->_dir | RU;
                    if (TileMap[newNode->_y][newNode->_x + 1] == false && TileMap[newNode->_y + 1][newNode->_x + 1] == true)
                        newNode->_dir = newNode->_dir | LD;

                    newNode->parent = nowNode;

                    F_Tile[nextY][nextX] = newNode->_F;

                    openList.push_back(newNode);
                    g_Tile[newNode->_y][newNode->_x] = OPENLIST;
                }

                break;
            }

            // 수평 쭉 체크
            int horizontal_nextX = nextX;
            int horizontal_cnt = 0;
            while (1)
            {
                horizontal_cnt++;
                horizontal_nextX = horizontal_nextX - 1;

                if (CanGo(nextY, horizontal_nextX) == false)
                    break;

                if (IsCorner(LL, nextY, horizontal_nextX) || (horizontal_nextX == endNodeYX.second && nextY == endNodeYX.first))
                {
                    double g = nowNode->_G + 1.4 * cnt;
                    double h = GetManhattan(nextY, nextX, endNodeYX.first, endNodeYX.second);
                    double f = g + h;

                    if (g_Tile[nextY][nextX] == OPENLIST)
                    {
                        if (f < F_Tile[nextY][nextX])
                        {
                            list<Node*>::iterator iter;
                            for (iter = openList.begin(); iter != openList.end(); ++iter)
                            {
                                if ((*iter)->_x == nextX && (*iter)->_y == nextY)
                                {
                                    (*iter)->_G = g;
                                    (*iter)->_H = h;
                                    (*iter)->_F = f;

                                    // TODO: 방향설정
                                    (*iter)->_dir = 0;
                                    (*iter)->_dir = (*iter)->_dir | LD | LL | DD;

                                    (*iter)->parent = nowNode;

                                    F_Tile[nextY][nextX] = f;

                                    break;
                                }
                            }
                        }
                    }
                    else
                    {
                        Node* newNode = new Node();

                        newNode->_y = nextY;
                        newNode->_x = nextX;

                        newNode->_G = g;
                        newNode->_H = h;
                        newNode->_F = f;

                        // 기본 방향
                        newNode->_dir = newNode->_dir | LD | LL | DD;

                        newNode->parent = nowNode;

                        F_Tile[nextY][nextX] = newNode->_F;

                        openList.push_back(newNode);
                        g_Tile[newNode->_y][newNode->_x] = OPENLIST;
                    }

                    break;
                }
            }

            // 수직 쭉 체크
            int vertical_nextY = nextY;
            int vertical_cnt = 0;
            while (1)
            {
                vertical_cnt++;
                vertical_nextY = vertical_nextY + 1;

                if (CanGo(nextY, horizontal_nextX) == false)
                    break;

                if (IsCorner(DD, nextY, nextX) || (nextX == endNodeYX.second && nextY == endNodeYX.first))
                {
                    double g = nowNode->_G + 1.4 * cnt;
                    double h = GetManhattan(nextY, nextX, endNodeYX.first, endNodeYX.second);
                    double f = g + h;

                    if (g_Tile[nextY][nextX] == OPENLIST)
                    {
                        if (f < F_Tile[nextY][nextX])
                        {
                            list<Node*>::iterator iter;
                            for (iter = openList.begin(); iter != openList.end(); ++iter)
                            {
                                if ((*iter)->_x == nextX && (*iter)->_y == nextY)
                                {
                                    (*iter)->_G = g;
                                    (*iter)->_H = h;
                                    (*iter)->_F = f;

                                    // TODO: 방향설정
                                    (*iter)->_dir = 0;
                                    (*iter)->_dir = (*iter)->_dir | LD | LL | DD;

                                    (*iter)->parent = nowNode;

                                    F_Tile[nextY][nextX] = f;

                                    break;
                                }
                            }
                        }
                    }
                    else
                    {
                        Node* newNode = new Node();

                        newNode->_y = nextY;
                        newNode->_x = nextX;

                        newNode->_G = g;
                        newNode->_H = h;
                        newNode->_F = f;

                        // 기본 방향
                        newNode->_dir = newNode->_dir | LD | LL | DD;

                        newNode->parent = nowNode;

                        F_Tile[nextY][nextX] = newNode->_F;

                        openList.push_back(newNode);
                        g_Tile[newNode->_y][newNode->_x] = OPENLIST;
                    }

                    break;
                }
            }

        }
    }*/

}

bool CanGo(int y, int x)
{
    if (y < 0 || y >= GRID_HEIGHT || x < 0 || x >= GRID_WIDTH)
        return false;

    if (TileMap[y][x] == false)
        return false;

    return true;
}

bool IsCorner(unsigned int dir, int y, int x)
{
    if ((dir & LL) == LL)
    {
        if (TileMap[y + 1][x] == false && TileMap[y + 1][x - 1] == true ||
            TileMap[y - 1][x] == false && TileMap[y - 1][x - 1] == true)
            return true;
        else
            return false;
    }
    if ((dir & LU) == LU)
    {
        if (TileMap[y][x + 1] == false && TileMap[y - 1][x + 1] == true ||
            TileMap[y + 1][x] == false && TileMap[y + 1][x - 1] == true)
            return true;
        else
            return false;
    }
    if ((dir & UU) == UU)
    {
        if (TileMap[y][x - 1] == false && TileMap[y - 1][x - 1] == true ||
            TileMap[y][x + 1] == false && TileMap[y - 1][x + 1] == true)
            return true;
        else
            return false;
    }
    if ((dir & RU) == RU)
    {
        if (TileMap[y + 1][x] == false && TileMap[y + 1][x + 1] == true ||
            TileMap[y][x - 1] == false && TileMap[y - 1][x - 1] == true)
            return true;
        else
            return false;
    }
    if ((dir & RR) == RR)
    {
        if (TileMap[y + 1][x] == false && TileMap[y + 1][x + 1] == true ||
            TileMap[y - 1][x] == false && TileMap[y - 1][x + 1] == true)
            return true;
        else
            return false;
    }
    if ((dir & RD) == RD)
    {
        if (TileMap[y - 1][x] == false && TileMap[y - 1][x + 1] == true ||
            TileMap[y][x - 1] == false && TileMap[y + 1][x - 1] == true)
            return true;
        else
            return false;
    }
    if ((dir & DD) == DD)
    {
        if (TileMap[y][x - 1] == false && TileMap[y + 1][x - 1] == true ||
            TileMap[y][x + 1] == false && TileMap[y + 1][x + 1] == true)
            return true;
        else
            return false;
    }
    if ((dir & LD) == LD)
    {
        if (TileMap[y - 1][x] == false && TileMap[y - 1][x - 1] == true ||
            TileMap[y][x + 1] == false && TileMap[y + 1][x + 1] == true)
            return true;
        else
            return false;
    }

    return false;
}

void Init()
{
    completeRouteList.clear();

    while (!openList.empty())
    {
        Node* tmp = openList.front();
        openList.pop_front();

        if (g_Tile[tmp->_y][tmp->_x] == OPENLIST)
            g_Tile[tmp->_y][tmp->_x] = NONE;

        delete tmp;
    }

    while (!closeList.empty())
    {
        Node* tmp = closeList.front();
        closeList.pop_front();

        if (g_Tile[tmp->_y][tmp->_x] == CLOSELIST)
            g_Tile[tmp->_y][tmp->_x] = NONE;

        delete tmp;
    }

    for (int i = 0; i < GRID_HEIGHT; i++)
    {
        for (int j = 0; j < GRID_WIDTH; j++)
        {
            F_Tile[i][j] = 999999;
        }
    }

    for (int i = 0; i < GRID_HEIGHT; i++)
    {
        for (int j = 0; j < GRID_WIDTH; j++)
        {
            if (g_Tile[i][j] == OBSTACLE)
                TileMap[i][j] = false;
            else
                TileMap[i][j] = true;
        }
    }
}

void GetMinFNode()
{
    list<Node*>::iterator minIter;
    double minF = 999999;

    list<Node*>::iterator iter;
    for (iter = openList.begin(); iter != openList.end(); ++iter)
    {
        if ((*iter)->_F < minF)
        {
            minIter = iter;
            minF = (*iter)->_F;
        }
    }

    Node* tmp = *minIter;
    openList.erase(minIter);
    openList.push_front(tmp);

}

void Reset()
{
    completeRouteList.clear();

    while (!openList.empty())
    {
        Node* tmp = openList.front();
        openList.pop_front();

        g_Tile[tmp->_y][tmp->_x] = NONE;

        delete tmp;
    }

    while (!closeList.empty())
    {
        Node* tmp = closeList.front();
        closeList.pop_front();

        g_Tile[tmp->_y][tmp->_x] = NONE;

        delete tmp;
    }

    for (int i = 0; i < GRID_HEIGHT; i++)
    {
        for (int j = 0; j < GRID_WIDTH; j++)
        {
            F_Tile[i][j] = 999999;
        }
    }

    for (int i = 0; i < GRID_HEIGHT; i++)
    {
        for (int j = 0; j < GRID_WIDTH; j++)
        {
            g_Tile[i][j] = NONE;
        }
    }
}
