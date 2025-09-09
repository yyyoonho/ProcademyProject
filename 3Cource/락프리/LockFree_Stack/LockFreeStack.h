#pragma once

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

    void Push(T data);
    void Pop(T* data);

private:
    /*
        0x [0000][0000 .... 0000]
        Node 주소의 상위 2바이트 = 16비트를 id로 사용.
    */
    unsigned short _uniqueCode = 0;
};

template<typename T>
inline void LockFreeStack<T>::Push(T data)
{
    Node<T>* newNode = new Node<T>;
    newNode->data = data;

    DWORD64 uID = (DWORD64)InterlockedIncrement((LONG*)&_uniqueCode);
    newNode = (Node<T>*)((DWORD64)newNode | (uID << 48));

    while (1)
    {
        Node<T>* oldTop = _top;
        ((Node<T>*)((DWORD64)newNode & 0x0000ffffffffffff))->next = oldTop;

        LONG64 ret = InterlockedCompareExchange64((LONG64*)&_top, (LONG64)newNode, (LONG64)oldTop);
        if ((Node<T>*)ret == oldTop)
        {
            break;
        }
    }

    return;
}

template<typename T>
inline void LockFreeStack<T>::Pop(T* data)
{
    while (1)
    {
        Node<T>* oldTop = _top;
        Node<T>* newTop = ((Node<T>*)((DWORD64)oldTop & 0x0000ffffffffffff))->next;

        LONG64 ret = InterlockedCompareExchange64((LONG64*)&_top, (LONG64)newTop, (LONG64)oldTop);
        if ((Node<T>*)ret == oldTop)
        {
            oldTop = ((Node<T>*)((DWORD64)oldTop & 0x0000ffffffffffff));
            *data = oldTop->data;

            delete oldTop;

            break;
        }
    }

    return;
}
