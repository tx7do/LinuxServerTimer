#include "Condition.h"

void Condition::signal()
{
	std::lock_guard<std::mutex> lk(_mutex);

	if (!_waitQueue.empty())
	{
		_waitQueue.front()->set();
		dequeue();
	}
}

void Condition::broadcast()
{
	std::lock_guard<std::mutex> lk(_mutex);

	for (auto p : _waitQueue)
	{
		p->set();
	}
	_waitQueue.clear();
}

void Condition::enqueue(CEvent& event)
{
	_waitQueue.push_back(&event);
}

void Condition::dequeue()
{
	_waitQueue.pop_front();
}

void Condition::dequeue(CEvent& event)
{
	for (auto it = _waitQueue.begin(); it != _waitQueue.end(); ++it)
	{
		if (*it == &event)
		{
			_waitQueue.erase(it);
			break;
		}
	}
}
