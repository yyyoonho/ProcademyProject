#pragma once

template <typename T>
class LockFreeStack
{
public:
    LockFreeStack();

public:
    struct Node
    {
        T data;
        Node* next;
    };

    Node* _top = NULL;

    void Push(T data);
    void Pop(T* data);

    int Size();

public:
    procademy::MemoryPool_TLS<Node> mp;

private:
    /*
        0x [0000][0000 .... 0000]
        Node 주소의 상위 2바이트 = 16비트를 id로 사용.
    */
    LONG _uniqueCode = 0;

    int _useSize = 0;
};

template<typename T>
inline LockFreeStack<T>::LockFreeStack() : mp(0,false)
{
    return;
}

template<typename T>
inline void LockFreeStack<T>::Push(T data)
{
    Node* newNode = mp.Alloc();
    newNode->data = data;

    DWORD64 uID = (DWORD64)InterlockedIncrement(&_uniqueCode) % (USHRT_MAX + 1);
    newNode = (Node*)((DWORD64)newNode | (uID << 48));

    while (1)
    {
        Node* oldTop = _top;
        ((Node*)((DWORD64)newNode & 0x0000ffffffffffff))->next = oldTop;

        LONG64 ret = InterlockedCompareExchange64((LONG64*)&_top, (LONG64)newNode, (LONG64)oldTop);
        if ((Node*)ret == oldTop)
        {
            break;
        }
    }

    InterlockedIncrement((LONG*)&_useSize);

    return;
}

template<typename T>
inline void LockFreeStack<T>::Pop(T* data)
{
    while (1)
    {
        Node* oldTop = _top;
        Node* newTop = ((Node*)((DWORD64)oldTop & 0x0000ffffffffffff))->next;

        LONG64 ret = InterlockedCompareExchange64((LONG64*)&_top, (LONG64)newTop, (LONG64)oldTop);
        if ((Node*)ret == oldTop)
        {
            oldTop = ((Node*)((DWORD64)oldTop & 0x0000ffffffffffff));

            *data = oldTop->data;
            bool ret = mp.Free(oldTop);

            break;
        }
    }

    InterlockedDecrement((LONG*)&_useSize);

    return;
}

template<typename T>
inline int LockFreeStack<T>::Size()
{
    return _useSize;
}
