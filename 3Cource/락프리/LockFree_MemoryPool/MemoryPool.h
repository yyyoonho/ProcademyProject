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

		/*
		0x [0000][0000 .... 0000]
		Node 주소의 상위 2바이트 = 16비트를 id로 사용.
		*/
		unsigned short _uniqueCode = 0;
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
		DATA* ret;

		if (_pFreeNode == NULL)
		{
			Node* newNode = new Node;

			// prev 세팅
			newNode->_underflowGuard = (Node*)_poolId;

			// overflowGuard 세팅
			newNode->_pNextNode = (Node*)_poolId;

			_capacity++;
			_useCount++;

			ret = &(newNode->_data);
		}
		else
		{
			Node* oldTop = NULL;
			Node* newTop = NULL;

			while (1)
			{
				oldTop = _pFreeNode;
				newTop = ((Node*)((DWORD64)oldTop & 0x0000ffffffffffff))->_pNextNode;

				LONG64 ret = InterlockedCompareExchange64((LONG64*)&_pFreeNode, (LONG64)newTop, (LONG64)oldTop);
				if ((Node*)ret == oldTop)
				{
					oldTop = ((Node*)((DWORD64)oldTop & 0x0000ffffffffffff));

					break;
				}
			}

			// overflowGuard 세팅
			oldTop->_pNextNode = (Node*)_poolId;

			if (_bPlacement == true)
			{
				new(&(oldTop->_data)) DATA();
			}

			InterlockedIncrement((LONG*)&_useCount);

			ret = &(oldTop->_data);
		}

		return ret;
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

		// Alloc할때마다 생성자호출을 해야하기 때문에,
		// Free로 돌아올 때 소멸자 호출줘야한다.
		if (_bPlacement == true)
		{
			pData->~DATA();
		}

		Node* newNode = tmpNode;

		// ABA: 상위2바이트에 id심기
		DWORD64 uID = (DWORD64)InterlockedIncrement((LONG*)&_uniqueCode);
		newNode = (Node*)((DWORD64)newNode & 0x0000ffffffffffff);
		newNode = (Node*)((DWORD64)newNode | (uID << 48));

		while (1)
		{
			Node* oldTop = _pFreeNode;
			((Node*)((DWORD64)newNode & 0x0000ffffffffffff))->_pNextNode = oldTop;

			LONG64 ret = InterlockedCompareExchange64((LONG64*)&_pFreeNode, (LONG64)newNode, (LONG64)oldTop);
			if ((Node*)ret == oldTop)
			{
				break;
			}
		}

		InterlockedDecrement((LONG*)&_useCount);

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


