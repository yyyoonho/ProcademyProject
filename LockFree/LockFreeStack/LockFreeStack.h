#pragma once

template <typename T>
struct Node
{
	T* data;
	Node* next;
};

template <typename T>
class MyStack
{
public:
	Node<T>* top = NULL;

	void Push(T data);
	void Pop(T* data);

private:

	/*
		0x [0000][0000 .... 0000]
		Node 주소의 상위 2바이트 = 16비트를 id로 사용.
	*/
	unsigned short _id = 0;
};

template<typename T>
void MyStack<T>::Push(T data)
{
	Node<T>* newNode = new Node<T>;

	newNode->data = new T;
	*(newNode->data) = data; 

	DWORD64 ret = (DWORD64)InterlockedIncrement((LONG*)&_id);
	newNode = (Node<T>*)((DWORD64)newNode | (ret << 48));

	while (1)
	{
		Node<T>* oldTop = top;
		((Node<T>*)((DWORD64)newNode & (0x0000ffffffffffff)))->next = oldTop;

		LONG64 ret = InterlockedCompareExchange64((LONG64*)&top, (LONG64)newNode, (LONG64)oldTop);
		if ((Node<T>*)ret == oldTop)
		{
			break;
		}
	}
}

template<typename T>
void MyStack<T>::Pop(T* data)
{
	while (1)
	{
		Node<T>* oldTop = top;
		Node<T>* newTop = ((Node<T>*)((DWORD64)oldTop & (0x0000ffffffffffff)))->next;

		LONG64 ret = InterlockedCompareExchange64((LONG64*)&top, (LONG64)newTop, (LONG64)oldTop);
		if ((Node<T>*)ret == oldTop)
		{
			oldTop = (Node<T>*)((DWORD64)(oldTop) & (0x0000ffffffffffff));

			*data = *(oldTop->data);
			delete (oldTop->data);

			delete oldTop;
			break;
		}
	}

	return;
}
