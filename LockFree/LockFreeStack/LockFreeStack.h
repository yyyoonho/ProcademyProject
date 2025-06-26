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
	void PushID(Node<T>** ppNewNode);
	void PopID(Node<T>** ppOldNode);
	Node<T>* GetTmpNodeOriginPtr(Node<T>* pNode);

	unsigned short _id = 0;
};

template<typename T>
void MyStack<T>::Push(T data)
{
	Node<T>* newNode = new Node<T>;

	newNode->data = new T;
	*(newNode->data) = data;

	PushID(&newNode);

	while (1)
	{
		Node<T>* oldTop = top;
		//newNode->next = oldTop;
		GetTmpNodeOriginPtr(newNode)->next = oldTop;

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
		Node<T>* newTop = GetTmpNodeOriginPtr(oldTop)->next;

		LONG64 ret = InterlockedCompareExchange64((LONG64*)&top, (LONG64)newTop, (LONG64)oldTop);
		if ((Node<T>*)ret == oldTop)
		{
			PopID(&oldTop);

			*data = *(oldTop->data);
			delete (oldTop->data);

			delete oldTop;
			break;
		}
	}

	return;
}

template<typename T>
inline void MyStack<T>::PushID(Node<T>** ppNewNode)
{
	DWORD64 ret = (DWORD64)InterlockedIncrement((LONG*)&_id);

	DWORD64 tmp = (DWORD64)*ppNewNode;
	tmp = tmp | (ret << 48);

	*ppNewNode = (Node<T>*)tmp;
}

template<typename T>
inline void MyStack<T>::PopID(Node<T>** ppOldNode)
{
	DWORD64 tmp = (DWORD64)*ppOldNode;
	tmp = tmp & (0x0000ffffffffffff);

	*ppOldNode = (Node<T>*)tmp;
}

template<typename T>
inline Node<T>* MyStack<T>::GetTmpNodeOriginPtr(Node<T>* pNode)
{
	DWORD64 tmp = (DWORD64)pNode;
	tmp = tmp & (0x0000ffffffffffff);

	return (Node<T>*)tmp;
}
