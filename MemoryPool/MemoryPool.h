#pragma once
#include <new.h>

namespace procademy
{
	template <typename DATA>
	class MemoryPool
	{
	private:

		struct Node
		{
			DATA _data;
			Node* _pNext = NULL;
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
			Node* newNode = new Node;

			newNode->_pNext = _pFreeNode;
			_pFreeNode = newNode;
		}

		_capacity = iBlockNum;
	}

	template<typename DATA>
	inline MemoryPool<DATA>::~MemoryPool()
	{
		Node* nextNode = _pFreeNode;

		while (nextNode != NULL)
		{
			Node* targetNode = nextNode;
			nextNode = nextNode->_pNext;

			delete targetNode;
		}
	}

	template<typename DATA>
	inline DATA* MemoryPool<DATA>::Alloc()
	{
		if (_pFreeNode == NULL)
		{
			Node* newNode = new Node;

			_useCount++;
			_capacity++;

			return (DATA*)newNode;
		}
		else
		{
			Node* retNode = _pFreeNode;
			_pFreeNode = _pFreeNode->_pNext;

			_useCount++;

			return (DATA*)retNode;
		}
	}

	template<typename DATA>
	inline bool MemoryPool<DATA>::Free(DATA* pData)
	{
		
		Node* tmpNode = (Node*)pData;
		tmpNode->_pNext = _pFreeNode;
		_pFreeNode = tmpNode;

		_useCount--;

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


}


