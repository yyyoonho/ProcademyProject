#pragma once
#include <new.h>

#define CHUNKSIZE 10

using namespace std;

int testCount = 0;

namespace procademy
{
	template <typename DATA>
	class MemoryPool
	{
	private:
		struct Node
		{
			Node*	_underflowGuard = NULL; // 겸 poolId 체크용
			DATA	_data;
			Node*	_pNextNode = NULL; // 겸 _overflowGuard
		};

		struct Chunk
		{
			Node*	_pFreeNode = NULL;
			Chunk*	_pNextChunk = NULL;
		};

		class TLS_MemoryPool
		{
		public:
			TLS_MemoryPool(DWORD64 poolId, bool bPlacement)
			{
				_tlsPoolId = poolId;
				_bPlacement = bPlacement;
			}

		public:
			// 메인 스택
			Chunk*			tlsMain_Chunk = NULL;
			int				tlsMain_Count = 0;

			// 서브 스택
			Chunk*			tlsSub_Chunk = NULL;
			int				tlsSub_Count = 0;

		private:
			bool			_bPlacement;
			DWORD64			_tlsPoolId;

		public:
			DATA*			MyTLSAlloc();
			bool			MyTLSFree(DATA* pData);
		};

	public:
		MemoryPool(int iChunkNum, bool bPlacement = false);
		virtual ~MemoryPool();

		void			Init();

		DATA* Alloc();
		bool			Free(DATA* pData);

	private:
		Chunk*			_pFullChunk = NULL;
		Chunk*			_pEmptyChunk = NULL;

		bool			_bPlacement = false;

		DWORD64			_poolId;
		LONG			_uniqueCode_FullStack = 0;
		LONG			_uniqueCode_EmptyStack = 0;

	private:
		inline static int	_tlsIdx = TLS_OUT_OF_INDEXES;
	private:
		LONG			_fullChunkUseCount = 0;
	public:
		LONG			GetFullChunkUseCount();
	};


	template<typename DATA>
	inline MemoryPool<DATA>::MemoryPool(int iChunkNum, bool bPlacement)
	{
		if (_tlsIdx == TLS_OUT_OF_INDEXES)
		{
			_tlsIdx = TlsAlloc();
		}

		Init();

		_bPlacement = bPlacement;

		// 지금 초기 세팅할때 생성자 호출하고, 이후로는 하지 않는다.
		// 공간 확보 + 생성자 호출
		if (_bPlacement == false)
		{
			for (int i = 0; i < iChunkNum; i++)
			{
				Chunk* pNewChunk = new Chunk;

				// 로깅
				//InterlockedIncrement(&_fullChunkUseCount);
				cout << "2번" << endl;

				// chunk에 삽입
				for (int j = 0; j < CHUNKSIZE; j++)
				{
					Node* newNode = new Node;

					// nextNode 세팅
					newNode->_pNextNode = pNewChunk->_pFreeNode;
					pNewChunk->_pFreeNode = newNode;

					// prev 세팅
					newNode->_underflowGuard = (Node*)_poolId;
				}

				// chunk 끼리 연결
				pNewChunk->_pNextChunk = _pFullChunk;
				_pFullChunk = pNewChunk;

			}
		}
		// 어차피 Alloc할때마다 생성자 호출할거니, 지금은 생성자 호출 하지않겠다.
		// 공간만 확보
		else
		{
			for (int i = 0; i < iChunkNum; i++)
			{
				Chunk* pNewChunk = new Chunk;

				// 로깅
				//InterlockedIncrement(&_fullChunkUseCount);
				cout << "3번" << endl;

				// chunk에 삽입
				for (int j = 0; j < CHUNKSIZE; j++)
				{
					// malloc
					Node* newNode = (Node*)malloc(sizeof(Node));
					if (newNode == NULL)
					{
						return;
					}

					// nextNode 세팅
					newNode->_pNextNode = pNewChunk->_pFreeNode;
					pNewChunk->_pFreeNode = newNode;

					// prev 세팅
					newNode->_underflowGuard = (Node*)_poolId;
				}

				// chunk 끼리 연결
				pNewChunk->_pNextChunk = _pFullChunk;
				_pFullChunk = pNewChunk;
			}
		}
	}

	template<typename DATA>
	inline MemoryPool<DATA>::~MemoryPool()
	{

	}

	template<typename DATA>
	inline void MemoryPool<DATA>::Init()
	{
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
		// TLS 세팅 및 얻기
		if (TlsGetValue(_tlsIdx) == 0)
		{
			TLS_MemoryPool* pTLS_MemoryPool = new TLS_MemoryPool(_poolId, _bPlacement);
			TlsSetValue(_tlsIdx, (LPVOID)pTLS_MemoryPool);

			pTLS_MemoryPool->tlsMain_Chunk = new Chunk;
			pTLS_MemoryPool->tlsSub_Chunk = new Chunk;
		}

		TLS_MemoryPool* pTLS_MemoryPool = (TLS_MemoryPool*)TlsGetValue(_tlsIdx);
		DATA* pData = NULL;


		// 1. TLS 메인스택에서 꺼내기
		if (pTLS_MemoryPool->tlsMain_Count > 0)
		{
			pData = pTLS_MemoryPool->MyTLSAlloc();
		}

		// 2. TLS 서브스택과 스왑 후 꺼내기
		else
		{
			if (pTLS_MemoryPool->tlsSub_Count > 0)
			{
				Chunk* tmp = pTLS_MemoryPool->tlsMain_Chunk;
				int tmpSize = pTLS_MemoryPool->tlsMain_Count;

				pTLS_MemoryPool->tlsMain_Chunk = pTLS_MemoryPool->tlsSub_Chunk;
				pTLS_MemoryPool->tlsMain_Count = pTLS_MemoryPool->tlsSub_Count;

				pTLS_MemoryPool->tlsSub_Chunk = tmp;
				pTLS_MemoryPool->tlsSub_Count = tmpSize;

				pData = pTLS_MemoryPool->MyTLSAlloc();
			}
		}

		if (pData != NULL)
			return pData;


		// 3. TLS에서 해결불가. [메인 메모리풀 -> 청크 -> TLS 메인풀]
		// FullStack 에서 청크하나 꺼내오기.
		Chunk* oldTop = NULL;
		Chunk* newTop = NULL;
		while (1)
		{
			oldTop = _pFullChunk;

			// 3-1. 메인 메모리풀의 락프리스택에도 여유 청크가 없는 경우.
			if (oldTop == NULL)
			{
				Chunk* pNewChunk = new Chunk;

				// 로깅
				//InterlockedIncrement(&_fullChunkUseCount);
				cout << "1번" << endl;

				// chunk에 삽입
				for (int j = 0; j < CHUNKSIZE; j++)
				{
					Node* newNode = NULL;

					if (_bPlacement == true)
					{
						newNode = (Node*)malloc(sizeof(Node));
					}
					else
					{
						newNode = new Node;
					}

					if (newNode == NULL)
						return nullptr;

					// nextNode 세팅
					newNode->_pNextNode = pNewChunk->_pFreeNode;
					pNewChunk->_pFreeNode = newNode;

					// prev 세팅
					newNode->_underflowGuard = (Node*)_poolId;
				}

				oldTop = pNewChunk;
				break;
			}

			// 3-2. 메인 메모리풀의 락프리 스택에 여유 청크가 있는 경우.
			newTop = ((Chunk*)((DWORD64)oldTop & 0x0000ffffffffffff))->_pNextChunk;

			LONG64 ret = InterlockedCompareExchange64((LONG64*)&_pFullChunk, (LONG64)newTop, (LONG64)oldTop);
			if ((Chunk*)ret == oldTop)
			{
				oldTop = ((Chunk*)((DWORD64)oldTop & 0x0000ffffffffffff));

				break;
			}
		}

		// 현재 oldTop에 FullChunk가 있는 상태

		// EmptyStack 으로 청크 하나 반납하기.
		DWORD64 uID = (DWORD64)InterlockedIncrement(&_uniqueCode_EmptyStack) % (USHRT_MAX + 1);
		pTLS_MemoryPool->tlsMain_Chunk = (Chunk*)((DWORD64)(pTLS_MemoryPool->tlsMain_Chunk) & 0x0000ffffffffffff);
		pTLS_MemoryPool->tlsMain_Chunk = (Chunk*)((DWORD64)(pTLS_MemoryPool->tlsMain_Chunk) | (uID << 48));

		while (1)
		{
			Chunk* oldTop = _pEmptyChunk;
			((Chunk*)((DWORD64)pTLS_MemoryPool->tlsMain_Chunk & 0x0000ffffffffffff))->_pNextChunk = oldTop;

			LONG64 ret = InterlockedCompareExchange64((LONG64*)&_pEmptyChunk, (LONG64)pTLS_MemoryPool->tlsMain_Chunk, (LONG64)oldTop);
			if ((Chunk*)ret == oldTop)
			{
				break;
			}
		}

		pTLS_MemoryPool->tlsMain_Chunk = oldTop;
		pTLS_MemoryPool->tlsMain_Count = CHUNKSIZE;


		// 4. 장전 끝났으면 다시 TLS에서 Alloc
		pData = pTLS_MemoryPool->MyTLSAlloc();

		return pData;
	}

	template<typename DATA>
	inline bool MemoryPool<DATA>::Free(DATA* pData)
	{
		// TLS 세팅 및 얻기
		if (TlsGetValue(_tlsIdx) == 0)
		{
			TLS_MemoryPool* pTLS_MemoryPool = new TLS_MemoryPool(_poolId, _bPlacement);
			TlsSetValue(_tlsIdx, (LPVOID)pTLS_MemoryPool);

			pTLS_MemoryPool->tlsMain_Chunk = new Chunk;
			pTLS_MemoryPool->tlsSub_Chunk = new Chunk;
		}

		TLS_MemoryPool* pTLS_MemoryPool = (TLS_MemoryPool*)TlsGetValue(_tlsIdx);

		// 1. TLS 메인 스택으로 반납
		// 2. TLS 메인 스택이 다 찼으면 TLS 서브 스택으로 반납
		pTLS_MemoryPool->MyTLSFree(pData);


		// 3. 서브 스택이 꽉찼으면 서브 청크를 메인 메모리풀로 반납
		if (pTLS_MemoryPool->tlsSub_Count >= CHUNKSIZE)
		{
			Chunk* newChunk = pTLS_MemoryPool->tlsSub_Chunk;

			// ABA: 상위2바이트에 id심기
			DWORD64 uID = (DWORD64)InterlockedIncrement((LONG*)&_uniqueCode_FullStack) % (USHRT_MAX+1);
			newChunk = (Chunk*)((DWORD64)newChunk & 0x0000ffffffffffff);
			newChunk = (Chunk*)((DWORD64)newChunk | (uID << 48));

			while (1)
			{
				Chunk* oldTop = _pFullChunk;
				((Chunk*)((DWORD64)newChunk & 0x0000ffffffffffff))->_pNextChunk = oldTop;

				LONG64 ret = InterlockedCompareExchange64((LONG64*)&_pFullChunk, (LONG64)newChunk, (LONG64)oldTop);
				if ((Chunk*)ret == oldTop)
				{
					break;
				}
			}

			Chunk* oldTop = NULL;
			Chunk* newTop = NULL;
			while (1)
			{
				oldTop = _pEmptyChunk;

				// 3-1. 메인 메모리풀의 Empty 락프리스택에도 여유 청크가 없는 경우.
				if (oldTop == NULL)
				{
					Chunk* pNewChunk = new Chunk;
					pNewChunk->_pFreeNode = NULL;
					pNewChunk->_pNextChunk = NULL;

					oldTop = pNewChunk;
					break;
				}

				// 3-2. 메인 메모리풀의 Empty 락프리 스택에 여유 청크가 있는 경우.
				newTop = ((Chunk*)((DWORD64)oldTop & 0x0000ffffffffffff))->_pNextChunk;

				LONG64 ret = InterlockedCompareExchange64((LONG64*)&_pEmptyChunk, (LONG64)newTop, (LONG64)oldTop);
				if ((Chunk*)ret == oldTop)
				{
					oldTop = ((Chunk*)((DWORD64)oldTop & 0x0000ffffffffffff));
					break;
				}
			}

			pTLS_MemoryPool->tlsSub_Chunk = oldTop;
			pTLS_MemoryPool->tlsSub_Chunk->_pNextChunk = NULL;
			pTLS_MemoryPool->tlsSub_Count = 0;
		}

		return true;
	}

	template<typename DATA>
	inline LONG MemoryPool<DATA>::GetFullChunkUseCount()
	{
		return _fullChunkUseCount;
	}

	template<typename DATA>
	DATA* MemoryPool<DATA>::TLS_MemoryPool::MyTLSAlloc()
	{
		Node* allocNode = tlsMain_Chunk->_pFreeNode;
		tlsMain_Count--;

		tlsMain_Chunk->_pFreeNode = tlsMain_Chunk->_pFreeNode->_pNextNode;

		// underflowGuard 세팅
		allocNode->_underflowGuard = (Node*)_tlsPoolId;

		// overflowGuard 세팅
		allocNode->_pNextNode = (Node*)_tlsPoolId;

		// 생성자 호출해줘야 하면 생성자 호출
		if (_bPlacement == true)
		{
			new(&(allocNode->_data)) DATA();
		}

		return &(allocNode->_data);
	}

	template<typename DATA>
	bool MemoryPool<DATA>::TLS_MemoryPool::MyTLSFree(DATA* pData)
	{
		Node* tmpNode = (Node*)((BYTE*)pData - sizeof(Node*));

		// 내 풀인지 아닌지 체크 + 언더플로우 체크
		Node* underflowGuard = tmpNode->_underflowGuard;
		if (underflowGuard != (Node*)_tlsPoolId)
		{
			return false;
		}

		// 오버플로우 체크
		Node* overflowGuard = tmpNode->_pNextNode;
		if (overflowGuard != (Node*)_tlsPoolId)
		{
			return false;
		}

		// Alloc할때마다 생성자호출을 해야하기 때문에,
		// Free로 돌아올 때 소멸자 호출줘야한다.
		if (_bPlacement == true)
		{
			pData->~DATA();
		}

		if (tlsMain_Count < CHUNKSIZE)
		{
			tmpNode->_pNextNode = tlsMain_Chunk->_pFreeNode;
			tlsMain_Chunk->_pFreeNode = tmpNode;

			tlsMain_Count++;
		}
		else
		{
			tmpNode->_pNextNode = tlsSub_Chunk->_pFreeNode;
			tlsSub_Chunk->_pFreeNode = tmpNode;

			tlsSub_Count++;
		}


		return true;
	}

}