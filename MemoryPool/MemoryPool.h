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
		};

	public:
		MemoryPool(int iBlockNum, bool bPlacement = false);
		virtual ~MemoryPool();

		/* 테스트 */
		void Init();

		DATA* Alloc();
		bool Free(DATA* pData);

		int GetCapacity();
		int GetUseCount();

	private:
		int _capacity = 0;
		int _useCount = 0;

		Node* _pFreeNode = NULL;

		bool _bPlacement = false;

		__int64 _poolId;
	};
	

	template<typename DATA>
	inline MemoryPool<DATA>::MemoryPool(int iBlockNum, bool bPlacement)
	{
		Init();

		_bPlacement = bPlacement;

		// 지금 초기 세팅할때 생성자 호출하고, 이후로는 하지 않는다.ex) 직렬화버퍼 풀
		// 공간 확보 + 생성자 호출
		if (_bPlacement == false) 
		{
			for (int i = 0; i < iBlockNum; i++)
			{
				// malloc
				Node* newNode = (Node*)malloc(sizeof(Node*) + sizeof(Node) + sizeof(Node*));

				newNode = newNode + 1; // Free할때 -1 해서 Free 해야한다!!!

				// placement new
				new(newNode) DATA();

				// nextNode 세팅
				Node** ppNext = (Node**)((BYTE*)newNode + sizeof(Node));
				*ppNext = _pFreeNode;
				
				_pFreeNode = newNode;
				
				// prev 세팅
				Node** ppPrev = (Node**)((BYTE*)newNode - sizeof(Node*));
				*ppPrev = (Node*)_poolId;

			}
		}
		// 어차피 Alloc할때마다 생성자 호출할거니, 지금은 생성자 호출 하지않겠다.
		// 공간만 확보
		else
		{
			for (int i = 0; i < iBlockNum; i++)
			{
				Node* newNode = (Node*)malloc(sizeof(Node*) + sizeof(DATA) + sizeof(Node*));

			}
		}

		_capacity = iBlockNum;
	}

	template<typename DATA>
	inline MemoryPool<DATA>::~MemoryPool()
	{
		if (_bPlacement == true)
		{
			
		}
		else
		{

		}		
	}

	template<typename DATA>
	inline void MemoryPool<DATA>::Init()
	{
		// TODO: srand는 한번만 해도 되니 수정하자.
		srand(time(NULL));

		__int64 tmp = rand();

		for (int i = 1; i <= 4; i++)
		{
			_poolId = _poolId | (tmp << i * 16);
		}
	}

	template<typename DATA>
	inline DATA* MemoryPool<DATA>::Alloc()
	{
		/*if (_pFreeNode == NULL)
		{
			
		}
		else
		{

			_useCount++;

			return (DATA*)retNode;
		}*/

		return NULL;
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


