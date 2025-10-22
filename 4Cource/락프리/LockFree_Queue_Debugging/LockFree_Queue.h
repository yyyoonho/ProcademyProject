#pragma once

struct stNote
{
    DWORD64 _action;

    DWORD64 _deQueueNode;
    DWORD64 _newHead;

    DWORD64 _oldTail;
    DWORD64 _enQueueNode;

    DWORD64 _threadID;
};

int noteIdx = -1;
stNote* debugNotes = new stNote[10000000];

void Note(int idx, DWORD64 action, DWORD64 deQueueNode, DWORD64 newHead, DWORD64 oldTail, DWORD64 enQueueNode, DWORD64 threadID)
{
    debugNotes[idx]._action = action;
    debugNotes[idx]._deQueueNode = deQueueNode;
    debugNotes[idx]._newHead = newHead;
    debugNotes[idx]._oldTail = oldTail;
    debugNotes[idx]._enQueueNode = enQueueNode;
    debugNotes[idx]._threadID = threadID;
}


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
    int maximumQSize = 1000000;

    LONG _uniqueCode;

private:
    procademy::MemoryPool<Node> mp;

public:
    bool Enqueue(T data);
    bool Dequeue(T* data);

    int Size();
};

template<typename T>
LockFreeQueue<T>::LockFreeQueue() : mp(0, false)
{
    _uniqueCode = 0;
    _useSize = 0;

    Node* initDummyNode = mp.Alloc();

    _head = initDummyNode;
    initDummyNode->next = NULL;
    _tail = initDummyNode;
}

template<typename T>
bool LockFreeQueue<T>::Enqueue(T data)
{
    if (_useSize >= maximumQSize)
        return false;


    Node* newNode = mp.Alloc();
    newNode->data = data;
    newNode->next = NULL;

    DWORD64 uID = (DWORD64)InterlockedIncrement(&_uniqueCode) % (USHRT_MAX + 1);
    newNode = (Node*)((DWORD64)newNode | (uID << 48));

    while (1)
    {
        Node* oldTail;
        while (1)
        {
            oldTail = _tail;

            if (((Node*)((DWORD64)oldTail & 0x0000ffffffffffff))->next == NULL)
                break;

            InterlockedCompareExchangePointer((PVOID*)&_tail, ((Node*)((DWORD64)oldTail & 0x0000ffffffffffff))->next, oldTail);
        }

        PVOID ret = InterlockedCompareExchangePointer((PVOID*)&((Node*)((DWORD64)oldTail & 0x0000ffffffffffff))->next, newNode, NULL);
        if (ret == NULL)
        {
            // µð¹ö±ë
            int idx = (int)InterlockedIncrement((LONG*)&noteIdx);
            Note(idx, 0xBBBBBBBBBBBBBBBB, NULL, NULL, (DWORD64)oldTail, (DWORD64)newNode, GetCurrentThreadId());

            PVOID ret2 = InterlockedCompareExchangePointer((PVOID*)&_tail, newNode, oldTail);
            if (ret2 != oldTail)
            {
                //DebugBreak();
            }

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
        Node* oldHead = _head;
        Node* newHead = ((Node*)((DWORD64)oldHead & 0x0000ffffffffffff))->next;
        if (newHead == NULL)
            continue;

        *data = ((Node*)((DWORD64)newHead & 0x0000ffffffffffff))->data;

        PVOID ret = InterlockedCompareExchangePointer((PVOID*)&_head, newHead, oldHead);
        if ((Node*)ret == oldHead)
        {
            // µð¹ö±ë
            int idx = (int)InterlockedIncrement((LONG*)&noteIdx);
            Note(idx, 0xAAAAAAAAAAAAAAAA, (DWORD64)oldHead, (DWORD64)newHead, NULL, NULL, GetCurrentThreadId());

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