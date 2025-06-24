#pragma comment(lib, "winmm.lib")

#include <iostream>
#include <list>
#include <Windows.h>
#include <thread>
#include <unordered_set>

#include <DbgHelp.h>
#include "CrashDump.h"

using namespace std;

template <typename T>
struct Node
{
	T data;
	Node* next;
};

// 디버깅 전역변수
unordered_set<Node<int>*> deleteSet;

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
			// 디버깅 - 4번 씬
			printf("ThreadID:%5d Push() | oldTop(= newNode->next): %p | newTop(= newNode): %p\n", GetCurrentThreadId(), oldTop, newNode);

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
			
			// 디버깅 - 4번 씬
			printf("ThreadID:%5d Pop() | oldTop:%p | newTop(= oldTop->Next):%p\n", GetCurrentThreadId(), oldTop, newTop);

			// 디버깅 - 1번 씬
			if (deleteSet.find(oldTop) == deleteSet.end())
			{
				deleteSet.insert(oldTop);
			}
			else
			{
				// 디버깅 - 2번 씬, 3번 씬
				printf("중복 delete 발생! %p\n", oldTop);
				DebugBreak();
			}

			delete oldTop;

			break;
		}
	}

	return;
}

/*******************************************************************************************/

MyStack<int> g_Stack;


HANDLE thread1;
HANDLE thread2;
HANDLE thread3;

void ThreadFunc()
{
	while (1)
	{
		for (int i = 0; i < 100; i++)
		{
			g_Stack.Push(i);
		}

		for (int i = 0; i < 100; i++)
		{
			int tmp;
			g_Stack.Pop(&tmp);
		}
	}
}

void ThreadFunc2()
{
	int tmp;
	while (1)
	{
		g_Stack.Pop(&tmp);
	}
}

procademy::CrashDump crashDump();

int main()
{
	timeBeginPeriod(1);

	thread1 = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&ThreadFunc, NULL, NULL, NULL);
	thread2 = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&ThreadFunc, NULL, NULL, NULL);

	WaitForSingleObject(thread1, INFINITE);
	WaitForSingleObject(thread2, INFINITE);
	

	//for (int i = 0; i < 10000; i++)
	//{
	//	g_Stack.Push(i);
	//}

	//thread1 = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&ThreadFunc2, NULL, NULL, NULL);
	//thread2 = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&ThreadFunc2, NULL, NULL, NULL);

	//WaitForSingleObject(thread1, INFINITE);
	//WaitForSingleObject(thread2, INFINITE);

	return 0;
}