#pragma once
#include <new.h>

/* 메모리풀 클래스 사용법 

procademy::MemoryPool mp(int size, bool placementNew)

1.	size 인자만큼 초기 할당을 진행합니다.
	0을 넣었다면 FreeList로 진행합니다.

2. placementNew false -> Alloc으로 메모리를 할당받을 때, 생성자를 호출하지 않습니다. 최초 1회만 호출합니다.
				true -> Alloc으로 메모리를 할당받을 때, 매번 생성자를 호출합니다.


*/

namespace procademy
{
	template <typename DATA>
	class MemoryPool
	{
	private:
		struct Node
		{
			Node* _underflowGuard = NULL; // 겸 poolId 체크용
			DATA _data;
			Node* _pNextNode = NULL; // 겸 _overflowGuard
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
				Node* newNode = new Node;

				// nextNode 세팅
				newNode->_pNextNode = _pFreeNode;
				_pFreeNode = newNode;

				// prev 세팅
				newNode->_underflowGuard = (Node*)_poolId;

			}
		}
		// 어차피 Alloc할때마다 생성자 호출할거니, 지금은 생성자 호출 하지않겠다.
		// 공간만 확보
		else
		{
			for (int i = 0; i < iBlockNum; i++)
			{
				// malloc
				Node* newNode = (Node*)malloc(sizeof(Node));
				if (newNode == NULL)
				{
					//cout << "Error malloc" << endl;
					return;
				}

				// nextNode 세팅
				newNode->_pNextNode = _pFreeNode;
				_pFreeNode = newNode;

				// prev 세팅
				newNode->_underflowGuard = (Node*)_poolId;
			}
		}

		_capacity = iBlockNum;
	}

	template<typename DATA>
	inline MemoryPool<DATA>::~MemoryPool()
	{
		if (_bPlacement == true)
		{
			Node* next = NULL;
			next = _pFreeNode;

			while (next != NULL)
			{
				Node* targetNode = next;
				next = targetNode->_pNextNode;

				free(targetNode);
			}
		}
		else
		{
			Node* next = NULL;
			next = _pFreeNode;

			while (next != NULL)
			{
				Node* targetNode = next;
				next = targetNode->_pNextNode;

				delete targetNode;
			}
		}
	}

	template<typename DATA>
	inline void MemoryPool<DATA>::Init()
	{
		// TODO: srand는 한번만 해도 되니 수정하자.
		srand((unsigned int)time(NULL));

		__int64 tmp = (__int64)rand();

		for (int i = 1; i <= 4; i++)
		{
			_poolId = _poolId | (tmp << i * 16);
		}
	}

	template<typename DATA>
	inline DATA* MemoryPool<DATA>::Alloc()
	{
		if (_pFreeNode == NULL)
		{
			Node* newNode = new Node;

			// prev 세팅
			newNode->_underflowGuard = (Node*)_poolId;

			// overflowGuard 세팅
			newNode->_pNextNode = (Node*)_poolId;

			_capacity++;
			_useCount++;

			return &(newNode->_data);
		}
		else
		{
			Node* newNode = _pFreeNode;
			_pFreeNode = newNode->_pNextNode;

			// overflowGuard 세팅
			newNode->_pNextNode = (Node*)_poolId;

			if (_bPlacement == true)
			{
				new(&(newNode->_data)) DATA();
			}

			_useCount++;

			return &(newNode->_data);
		}
	}

	template<typename DATA>
	inline bool MemoryPool<DATA>::Free(DATA* pData)
	{
		Node* tmpNode = (Node*)((BYTE*)pData - sizeof(Node*));

		// 내 풀인지 아닌지 체크 + 언더플로우 체크
		Node* underflowGuard = tmpNode->_underflowGuard;
		if (underflowGuard != (Node*)_poolId)
		{
			return false;
		}

		// 오버플로우 체크
		Node* overflowGuard = tmpNode->_pNextNode;
		if (overflowGuard != (Node*)_poolId)
		{
			return false;
		}

		tmpNode->_pNextNode = _pFreeNode;
		_pFreeNode = tmpNode;

		// Alloc할때마다 생성자호출을 해야하기 때문에,
		// Free로 돌아올 때 소멸자 호출줘야한다.
		if (_bPlacement == true)
		{
			pData->~DATA();
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


