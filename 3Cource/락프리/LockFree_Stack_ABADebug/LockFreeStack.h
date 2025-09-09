#pragma once
DWORD64 pushAction = 0xdddddddddddddddd;
DWORD64 popAction = 0xffffffffffffffff;

struct DebugNote
{
    DWORD64 action;
    DWORD64 pushNodeAddr;
    DWORD64 oldTopNodeAddr;
    DWORD64 popNodeAddr;
    DWORD64 threadID;
};

LONG g_debugCount;
DebugNote* notes = new DebugNote[100000000];

DWORD Note(DWORD64 action, DWORD64 pushNodeAddr, DWORD64 oldTopNodeAddr, DWORD64 popNodeAddr, DWORD64 threadID)
{
    DWORD idx = InterlockedIncrement(&g_debugCount);

    notes[idx].action = action;
    notes[idx].pushNodeAddr = pushNodeAddr;
    notes[idx].oldTopNodeAddr = oldTopNodeAddr;
    notes[idx].popNodeAddr = popNodeAddr;
    notes[idx].threadID = threadID;

    return idx;
}

template <typename T>
struct Node
{
    T data;
    Node* next;
};

template <typename T>
class LockFreeStack
{
public:
    Node<T>* _top = NULL;

public:
    void Push(T data)
    {
        Node<T>* newNode = new Node<T>;
        newNode->data = data;

        while (1)
        {
            Node<T>* oldTop = _top;
            newNode->next = oldTop;

            LONG64 ret = InterlockedCompareExchange64((LONG64*)&_top, (LONG64)newNode, (LONG64)oldTop);
            if ((Node<T>*)ret == oldTop)
            {
                Note(pushAction, (DWORD64)newNode, (DWORD64)oldTop, NULL, GetCurrentThreadId());

                break;
            }
        }

        return;
    }

    void Pop(T* data)
    {
        while (1)
        {
            Node<T>* oldTop = _top;
            Node<T>* newTop = oldTop->next;

            LONG64 ret = InterlockedCompareExchange64((LONG64*)&_top, (LONG64)newTop, (LONG64)oldTop);
            if ((Node<T>*)ret == oldTop)
            {
                *data = oldTop->data;

                DWORD idx = Note(popAction, NULL, (DWORD64)newTop, (DWORD64)oldTop, GetCurrentThreadId());

                delete oldTop;

                break;
            }
        }

        return;
    }
};