#pragma once

template<typename T>
class LockFreeQueue
{
public:
    LockFreeQueue();

private:
    struct Node
    {
        T data;
        Node* next;
    };

    Node* _head;
    Node* _tail;

    int _useSize;
    int maximumQSize = 100000;

    LONG _uniqueCode;

    DWORD64 _uniqueQueueCode;


public:
    inline static procademy::MemoryPool_TLS<Node> mp{ 0,false };

public:
    bool Enqueue(T data);
    bool Dequeue(T* data);

    int Size();
    void Clear();

};

template<typename T>
LockFreeQueue<T>::LockFreeQueue()
{
    _uniqueQueueCode = (DWORD64)&_uniqueQueueCode;
    _uniqueCode = 0;
    _useSize = 0;

    Node* initDummyNode = mp.Alloc();

    _head = initDummyNode;
    initDummyNode->next = (Node*)_uniqueQueueCode;
    _tail = initDummyNode;
}

template<typename T>
bool LockFreeQueue<T>::Enqueue(T data)
{
    if (_useSize >= maximumQSize)
        return false;


    Node* newNode = mp.Alloc();
    newNode->data = data;
    newNode->next = (Node*)_uniqueQueueCode;

    DWORD64 uID = (DWORD64)InterlockedIncrement(&_uniqueCode) % (USHRT_MAX + 1);
    newNode = (Node*)((DWORD64)newNode | (uID << 48));

    while (1)
    {
        Node* oldTail;
        while (1)
        {
            oldTail = _tail;

            if (((Node*)((DWORD64)oldTail & 0x0000ffffffffffff))->next == (Node*)_uniqueQueueCode)
                break;

            InterlockedCompareExchangePointer((PVOID*)&_tail, ((Node*)((DWORD64)oldTail & 0x0000ffffffffffff))->next, oldTail);
        }

        PVOID ret = InterlockedCompareExchangePointer((PVOID*)&((Node*)((DWORD64)oldTail & 0x0000ffffffffffff))->next, newNode, (Node*)_uniqueQueueCode);
        if (ret == (Node*)_uniqueQueueCode)
        {
            PVOID ret2 = InterlockedCompareExchangePointer((PVOID*)&_tail, newNode, oldTail);

            break;
        }
    }

    InterlockedIncrement((LONG*)&_useSize);

    return true;
}

template<typename T>
bool LockFreeQueue<T>::Dequeue(T* data)
{
    if (InterlockedDecrement((LONG*)&_useSize) < 0)
    {
        InterlockedIncrement((LONG*)&_useSize);
        return false;
    }

    while (1)
    {
        Node* oldTail;
        while (1)
        {
            oldTail = _tail;

            if (((Node*)((DWORD64)oldTail & 0x0000ffffffffffff))->next == (Node*)_uniqueQueueCode)
                break;

            InterlockedCompareExchangePointer((PVOID*)&_tail, ((Node*)((DWORD64)oldTail & 0x0000ffffffffffff))->next, oldTail);
        }

        Node* oldHead = _head;
        Node* newHead = ((Node*)((DWORD64)oldHead & 0x0000ffffffffffff))->next;
        if (newHead == (Node*)_uniqueQueueCode)
            continue;

        *data = ((Node*)((DWORD64)newHead & 0x0000ffffffffffff))->data;

        PVOID ret = InterlockedCompareExchangePointer((PVOID*)&_head, newHead, oldHead);
        if ((Node*)ret == oldHead)
        {
            oldHead = ((Node*)((DWORD64)oldHead & 0x0000ffffffffffff));
            mp.Free(oldHead);

            break;
        }
    }

    return true;
}

template<typename T>
int LockFreeQueue<T>::Size()
{
    return _useSize;
}

template<typename T>
inline void LockFreeQueue<T>::Clear()
{
    Node* oldTail;
    while (1)
    {
        oldTail = _tail;

        if (((Node*)((DWORD64)oldTail & 0x0000ffffffffffff))->next == (Node*)_uniqueQueueCode)
            break;

        InterlockedCompareExchangePointer((PVOID*)&_tail, ((Node*)((DWORD64)oldTail & 0x0000ffffffffffff))->next, oldTail);
    }

    while (1)
    {
        if (_useSize == 0)
            break;

        Node* oldHead = ((Node*)((DWORD64)_head & 0x0000ffffffffffff));
        _head = oldHead->next;

        mp.Free(oldHead);

        _useSize--;
    }

}