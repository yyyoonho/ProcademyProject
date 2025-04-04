#pragma once
#include <new.h>

namespace procademy
{
	template <typename DATA>
	class MemoryPool
	{
	private:

		class Node
		{
		public:
			Node();
			~Node();

			DATA* _pData;
			Node* _next;
		};

	public:
		MemoryPool(int iBlockNum, bool bPlacement = false);
		virtual ~MemoryPool();

		DATA* Alloc();
		bool Free(DATA* pData);

		int GetCapacity();
		int GetUseCount();

	private:
		int _capacity = 0;
		int _useCount = 0;

		Node* _pFreeNode = NULL;
	};
	

	template<typename DATA>
	inline MemoryPool<DATA>::MemoryPool(int iBlockNum, bool bPlacement)
	{
		for (int i = 0; i < iBlockNum; i++)
		{
			Node* tmpNode = new Node;

			tmpNode->_next = _pFreeNode;
			_pFreeNode = tmpNode;
		}

		_capacity = iBlockNum;
	}

	template<typename DATA>
	inline MemoryPool<DATA>::~MemoryPool()
	{
		if (_pFreeNode == NULL)
			return;

		Node* tmpNext;

		while (1)
		{
			tmpNext = _pFreeNode->_next;
			delete _pFreeNode;

			if (tmpNext == NULL)
				break;
		}
	}

	template<typename DATA>
	inline DATA* MemoryPool<DATA>::Alloc()
	{
		if (_pFreeNode == NULL)
		{
			_capacity++;
			_useCount++;

			Node* newNode = new Node;

			Node** tmp = (Node**)((BYTE*)(newNode->_pData) + sizeof(DATA));
			*tmp = newNode;

			return newNode->_pData;
		}
		else
		{
			_useCount++;

			Node* nowNode = _pFreeNode;
			_pFreeNode = nowNode->_next;

			Node** tmp = (Node**)((BYTE*)(nowNode->_pData) + sizeof(DATA));
			*tmp = nowNode;

			return nowNode->_pData;
		}
	}

	template<typename DATA>
	inline bool MemoryPool<DATA>::Free(DATA* pData)
	{
		_useCount--;

		Node** tmp = (Node**)((BYTE*)pData + sizeof(DATA));

		(*tmp)->_next = _pFreeNode;
		_pFreeNode = *tmp;

		return true;
	}

	template<typename DATA>
	inline int MemoryPool<DATA>::GetCapacity()
	{
		return _capacity;
	}

	template<typename DATA>
	inline int MemoryPool<DATA>::GetUseCount()
	{
		return _useCount;
	}

	template<typename DATA>
	inline MemoryPool<DATA>::Node::Node()
	{
		_pData = (DATA*)malloc(sizeof(DATA) + sizeof(Node*));
		_next = NULL;
	}

	template<typename DATA>
	inline MemoryPool<DATA>::Node::~Node()
	{
		free(_pData);
	}

}


