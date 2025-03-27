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
		public:
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

		bool _bPlacement = false;
	};
	

	template<typename DATA>
	inline MemoryPool<DATA>::MemoryPool(int iBlockNum, bool bPlacement)
	{
		_bPlacement = bPlacement;

		// 지금 초기 세팅할때 생성자 호출하고, 이후로는 하지 않는다.ex) 직렬화버퍼 풀
		if (_bPlacement == false) 
		{
			for (int i = 0; i < iBlockNum; i++)
			{
				Node* newNode = (Node*)malloc(sizeof(DATA) + sizeof(Node*));
				
				// placement new
				new((DATA*)newNode) DATA();

				newNode->_pNext = _pFreeNode;
				_pFreeNode = newNode;
			}
		}
		// 어차피 Alloc할때마다 생성자 호출할거니, 지금은 생성자 호출 하지않겠다.
		else
		{
			for (int i = 0; i < iBlockNum; i++)
			{
				Node* newNode = (Node*)malloc(sizeof(DATA) + sizeof(Node*));

				newNode->_pNext = _pFreeNode;
				_pFreeNode = newNode;
			}
		}

		_capacity = iBlockNum;
	}

	template<typename DATA>
	inline MemoryPool<DATA>::~MemoryPool()
	{
		if (_bPlacement == true)
		{
			Node* tmpNext = _pFreeNode;
		
			while (tmpNext != NULL)
			{
				Node* targetNode = tmpNext;
				tmpNext = targetNode->_pNext;

				free(targetNode);
			}

		}
		else
		{
			Node* tmpNext = _pFreeNode;

			while (tmpNext != NULL)
			{
				Node* targetNode = tmpNext;
				tmpNext = targetNode->_pNext;

				DATA* tmpData = (DATA*)targetNode;
				tmpData->~DATA();

				free(targetNode);
			}
		}		
	}

	template<typename DATA>
	inline DATA* MemoryPool<DATA>::Alloc()
	{
		if (_pFreeNode == NULL)
		{
			Node* newNode = (Node*)malloc(sizeof(DATA) + sizeof(Node*));

			// placement new
			new((DATA*)newNode) DATA();

			_useCount++;
			_capacity++;

			return (DATA*)newNode;
		}
		else
		{
			Node* retNode = _pFreeNode;
			_pFreeNode = _pFreeNode->_pNext;

			// bPlacement == ture 시, Alloc할때마다 초기화 해주겠다.
			if (_bPlacement == true)
			{
				// placement new
				new((DATA*)retNode) DATA();
			}

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

		// Alloc할때마다 생성자호출을 해야하기 때문에,
		// Free로 돌아올 때 소멸자 호출줘야한다.
		if (_bPlacement == true)
		{
			DATA* tmpData = (DATA*)tmpNode;
			tmpData->~DATA();
		}

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


