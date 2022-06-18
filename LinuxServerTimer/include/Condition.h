#pragma once

#include "linux_server_timer.h"
#include <deque>
#include <stdexcept>
#include "Event.h"
#include "ScopedUnlock.h"

class LinuxServerTimer_API Condition
{
public:
	Condition() = default;
	~Condition() = default;
	Condition(const Condition&) = delete;
	Condition& operator=(const Condition&) = delete;

	template<class Mtx>
	void wait(Mtx& mutex)
	{
		ScopedUnlock<Mtx> unlock(mutex, false);

		std::unique_lock<std::mutex> lock1(mutex, std::defer_lock);

		CEvent event;
		{
			std::lock_guard<std::mutex> lk(_mutex);
			mutex.unlock();
			enqueue(event);
		}
		event.wait();
	}

	template<class Mtx>
	void wait(Mtx& mutex, long milliseconds)
	{
		if (!tryWait(mutex, milliseconds))
			throw std::runtime_error("wait timeout");
	}

	template<class Mtx>
	bool tryWait(Mtx& mutex, long milliseconds)
	{
		ScopedUnlock<Mtx> unlock(mutex, false);
		CEvent event;
		{
			std::lock_guard<std::mutex> lk(_mutex);
			mutex.unlock();
			enqueue(event);
		}
		if (!event.tryWait(milliseconds))
		{
			std::lock_guard<std::mutex> lk(_mutex);
			dequeue(event);
			return false;
		}
		return true;
	}

	void signal();

	void broadcast();

protected:
	void enqueue(CEvent& event);
	void dequeue();
	void dequeue(CEvent& event);

private:
	typedef std::deque<CEvent*> WaitQueue;

	std::mutex _mutex;
	WaitQueue _waitQueue;
};
