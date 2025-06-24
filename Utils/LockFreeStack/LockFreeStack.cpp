#pragma comment(lib, "winmm.lib")

#include <iostream>
#include <list>
#include <Windows.h>

using namespace std;

template <typename T>
struct Node
{
	T data;
	Node* next;
};

template <typename T>
class MyStack
{
public:
	Node<T>* top = NULL;

	void Push(T data);
	void Pop(T* data);
};

template<typename T>
void MyStack<T>::Push(T data)
{
	Node<T>* newNode = new Node<T>;
	newNode->data = data;

	while (1)
	{
		Node<T>* oldTop = top;
		newNode->next = oldTop;

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
		Node<T>* newTop = oldTop->next;

		LONG64 ret = InterlockedCompareExchange64((LONG64*)&top, (LONG64)newTop, (LONG64)oldTop);
		if ((Node<T>*)ret == oldTop)
		{
			*data = oldTop->data;
			delete oldTop;
			break;
		}
	}

	return;
}

MyStack<int> g_Stack;

int main()
{
	timeBeginPeriod(1);



	return 0;
}
