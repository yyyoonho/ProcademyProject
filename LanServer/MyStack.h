#pragma once

class MyStack
{
public:
    MyStack()
    {
        point = -1;
        capacity = 1000;
    }
    ~MyStack()
    {
        delete[] arr;
    }

    void ReSize(int reSize)
    {
        if (reSize <= capacity)
            return;

        int* newArr = new int[reSize];
        memcpy_s(newArr, reSize, arr, capacity);

        delete[] arr;
        arr = newArr;

        capacity = reSize;

        return;
    }

    int Top()
    {
        if (point >= 0)
            return arr[point];
        else
            return NULL;
    }

    void Pop()
    {
        if (point < 0)
            return;

        point--;

        return;
    }

    void Push(int data)
    {
        if (point + 1 >= capacity)
        {
            ReSize(capacity * 2);
        }

        point++;
        arr[point] = data;

        return;
    }

    bool Empty()
    {
        if (point >= 0)
            return false;

        else
            return true;
    }

    void Clear()
    {
        point = -1;
    }

    int Size()
    {
        return point + 1;
    }

private:
    int* arr = new int[1000];

    int point;
    int capacity;
};