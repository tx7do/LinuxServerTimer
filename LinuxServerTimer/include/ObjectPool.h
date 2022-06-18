#pragma once

#include <vector>
#include <cctype>
#include <cassert>
#include <memory>
#include <mutex>
#include <atomic>
#include <iostream>

template<class C, class P = C*>
class PoolableObjectFactory
{
public:
	P createObject()
	{
		return new C;
	}

	bool validateObject(P pObject)
	{
		return pObject != nullptr;
	}

	void activateObject(P pObject)
	{
	}

	void deactivateObject(P pObject)
	{
	}

	void destroyObject(P pObject)
	{
		delete pObject;
	}
};

template<class C>
class PoolableObjectFactory<C, std::unique_ptr<C>>
{
public:
	std::unique_ptr<C> createObject()
	{
		return std::make_unique<C>();
	}

	bool validateObject(std::unique_ptr<C> pObject)
	{
		return pObject != nullptr;
	}

	void activateObject(std::unique_ptr<C> pObject)
	{
	}

	void deactivateObject(std::unique_ptr<C> pObject)
	{
	}

	void destroyObject(std::unique_ptr<C> pObject)
	{
	}
};

template<class C>
class PoolableObjectFactory<C, std::shared_ptr<C>>
{
public:
	std::shared_ptr<C> createObject()
	{
		return std::make_shared<C>();
	}

	bool validateObject(std::shared_ptr<C> pObject)
	{
		return pObject != nullptr;
	}

	void activateObject(std::shared_ptr<C> pObject)
	{
	}

	void deactivateObject(std::shared_ptr<C> pObject)
	{
	}

	void destroyObject(std::shared_ptr<C> pObject)
	{
	}
};

template<class C, class P = C*, class F = PoolableObjectFactory<C, P>>
class ObjectPool
{
public:
	explicit ObjectPool(std::size_t capacity)
		: _capacity(capacity), _size(0)
	{
		_pool.reserve(capacity);
	}

	ObjectPool(const F& factory, std::size_t capacity)
		: _factory(factory), _capacity(capacity), _size(0)
	{
	}

	~ObjectPool()
	{
		try
		{
			for (auto& p : _pool)
			{
				_factory.destroyObject(p);
			}
		}
		catch (...)
		{
		}
	}

	ObjectPool() = delete;
	ObjectPool(const ObjectPool&) = delete;
	ObjectPool& operator=(const ObjectPool&) = delete;

	P borrowObject()
	{
		std::lock_guard<std::mutex> lk(_mutex);

		if (!_pool.empty())
		{
			P pObject = _pool.back();
			_pool.pop_back();

			return activateObject(pObject);
		}

		P pObject = _factory.createObject();
		pObject = activateObject(pObject);
		_size++;

		return pObject;
	}

	void returnObject(P pObject)
	{
		std::lock_guard<std::mutex> lk(_mutex);

		if (_factory.validateObject(pObject))
		{
			_factory.deactivateObject(pObject);
			if (_pool.size() < _capacity)
			{
				try
				{
					_pool.push_back(pObject);
					return;
				}
				catch (...)
				{
				}
			}
		}
		_factory.destroyObject(pObject);
		_size--;
	}

	std::size_t capacity() const
	{
		return _capacity;
	}

	std::size_t size() const
	{
		std::lock_guard<std::mutex> lk(_mutex);

		return _size;
	}

	std::size_t available() const
	{
		std::lock_guard<std::mutex> lk(_mutex);

		return _pool.size() - _size;
	}

protected:
	P activateObject(P pObject)
	{
		try
		{
			_factory.activateObject(pObject);
		}
		catch (...)
		{
			_factory.destroyObject(pObject);
			throw;
		}
		return pObject;
	}

private:
	F _factory;
	std::size_t _capacity;
	std::size_t _size;
	std::vector<P> _pool;
	mutable std::mutex _mutex;
};
