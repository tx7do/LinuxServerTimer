#pragma once


template<class M>
class ScopedUnlock
{
public:
	inline explicit ScopedUnlock(M& mutex, bool unlockNow = true)
		: _mutex(mutex)
	{
		if (unlockNow)
		{
			_mutex.unlock();
		}
	}
	inline ~ScopedUnlock()
	{
		try
		{
			_mutex.lock();
		}
		catch (...)
		{
		}
	}

	ScopedUnlock() = delete;
	ScopedUnlock(const ScopedUnlock&) = delete;
	ScopedUnlock& operator=(const ScopedUnlock&) = delete;

private:
	M& _mutex;
};
