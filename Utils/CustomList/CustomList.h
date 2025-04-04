#pragma once

template <typename T>
class MyList
{
public:
	struct Node
	{
		T _data;
		Node* _prev;
		Node* _next;
	};

	class iterator
	{
	private:
		Node* _node;

	public:
		iterator(Node* node = nullptr)
		{
			_node = node;
		}

		const iterator operator++(int)
		{
			iterator tmp = _node;
			_node = _node->_next;

			return tmp;
		}

		iterator& operator++()
		{
			_node = _node->_next;

			return *this;
		}

		const iterator operator--(int)
		{
			iterator tmp = _node;
			_node = _node->_prev;

			return tmp;
		}

		iterator& operator--()
		{
			_node = _node->_prev;

			return *this;
		}

		T& operator*()
		{
			return _node->_data;
		}

		bool operator ==(const iterator& other)
		{
			if (_node == other._node)
				return true;

			return false;
		}

		bool operator !=(const iterator& other)
		{
			if (_node != other._node)
				return true;

			return false;
		}

		iterator erase()
		{
			iterator ret(_node->_next);

			_node->_prev->_next = _node->_next;
			_node->_next->_prev = _node->_prev;
			
			delete(_node);

			_node = ret._node;

			return ret;
		}
	};

public:
	MyList()
	{
		_head._next = &_tail;
		_tail._prev = &_head;
	}
	~MyList()
	{
		Node* now = _head._next;

		while (now != &_tail)
		{
			Node* next = now->_next;
			delete now;

			now = next;
		}
	}

	iterator begin()
	{
		iterator tmp = _head._next;

		return tmp;
	}

	iterator end()
	{
		iterator tmp = &_tail;

		return tmp;
	}

	void push_front(T data)
	{
		Node* newNode = new Node;
		newNode->_data = data;

		newNode->_prev = &_head;
		newNode->_next = _head._next;

		_head._next->_prev = newNode;
		_head._next = newNode;

		_size++;
	}
	void push_back(T data)
	{
		Node* newNode = new Node;
		newNode->_data = data;

		newNode->_next = &_tail;
		newNode->_prev = _tail._prev;

		_tail._prev->_next = newNode;
		_tail._prev = newNode;

		_size++;
	}
	bool pop_front()
	{
		if (_head._next == &_tail)
			return false;

		Node* tmp = _head._next;

		_head._next = tmp->_next;
		tmp->_next->_prev = &_head;

		delete tmp;

		_size--;

		return true;
	}
	bool pop_back()
	{
		if (_tail._prev == &_head)
			return false;

		Node* tmp = _tail._prev;

		_tail._prev = tmp->_prev;
		tmp->_prev->_next = &_tail;

		delete tmp;

		_size--;

		return true;
	}
	void clear()
	{
		if (_size <= 0)
			return;

		Node* now = _head._next;

		while (now != &_tail)
		{
			Node* next = now->_next;
			delete now;

			now = next;
		}

		_head._next = &_tail;
		_head._prev = nullptr;
		_tail._prev = &_head;
		_tail._prev = nullptr;

		_size = 0;
	}
	int size()
	{
		return _size;
	}
	bool empty()
	{
		if (_size <= 0)
			return true;
	}

	iterator erase(iterator iter)
	{
		iterator nextIt = iter.erase();

		return nextIt;
	}

	void remove(T Data)
	{
		MyList<T>::iterator iter;
		for (iter = begin(); iter != end(); ++iter)
		{
			if (*iter == Data)
				erase(iter);
		}
	}

private:
	int _size = 0;
	Node _head;
	Node _tail;
};