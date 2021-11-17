
#include "SleepServerTimer.h"
#include <unistd.h>
#include <sys/time.h>
#include <assert.h>

long long GetSysMilliseconds()
{
	struct timeval tv;
	::gettimeofday(&tv, NULL);

	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

unsigned int CSleepServerTimer::s_iResolution = 100;

CSleepServerTimer::CSleepServerTimer()
	: _thread(nullptr)
	, _listener(nullptr)
	, _running(false)
{}

CSleepServerTimer::~CSleepServerTimer()
{
	stopThread();
	KillAllTimer();
	destroyTimerItemPool();
}

void CSleepServerTimer::RegisterListener(IServerTimerListener* pListener)
{
	_listener = pListener;
}

void CSleepServerTimer::Start()
{
	startThread();
}

void CSleepServerTimer::Stop()
{
	stopThread();
}

void CSleepServerTimer::SetTimer(unsigned int iTimerID, unsigned int iElapse, bool bShootOnce)
{
	if (iElapse < s_iResolution)
	{
		iElapse = s_iResolution;
	}

	struct timeval tv;
	::gettimeofday(&tv, nullptr);
	long long iMS = tv.tv_sec * 1000 + tv.tv_usec / 1000;

	ServerTimerItemPtr item = nullptr;
	{
		std::lock_guard<std::mutex> lk(_mutex);
		if (_itemPool.empty())
		{
			item = new ServerTimerItem();
			_items[iTimerID] = item;
		}
		else
		{
			item = _itemPool.back();
			_itemPool.pop_back();
		}
		assert(item);
	}

	item->iTimerID = iTimerID;
	item->iElapse = iElapse / s_iResolution * s_iResolution;
	item->iStartTime = iMS / s_iResolution * s_iResolution + iElapse;
	item->bShootOnce = bShootOnce;

	_evThreadWait.set();
}

void CSleepServerTimer::KillTimer(unsigned int iTimerID)
{
	if (!isExistTimer(iTimerID))
	{
		return;
	}

	std::lock_guard<std::mutex> lk(_mutex);

	auto iter = _items.find(iTimerID);
	if (iter == _items.end())
	{
		return;
	}

	auto& item = iter->second;
	_itemPool.push_back(item);
	_items.erase(iter);

	return;
}

void CSleepServerTimer::KillAllTimer()
{
	std::lock_guard<std::mutex> lk(_mutex);
	auto iter = _items.begin();
	auto iEnd = _items.end();
	for (; iter != iEnd; ++iter)
	{
		auto& item = iter->second;
		item->clear();
		_itemPool.push_back(item);
	}
	_items.clear();
}

void CSleepServerTimer::startThread()
{
	_running = true;
	if (_thread == nullptr)
	{
		_thread = new std::thread(&CSleepServerTimer::onThread, this);
		_evThreadStarted.wait();
	}
}

void CSleepServerTimer::stopThread()
{
	_running = false;
	if (_thread != nullptr)
	{
		_thread->join();
		delete _thread;
		_thread = nullptr;
	}
}

inline bool CSleepServerTimer::isExistTimer(unsigned int iTimerID) const
{
	std::lock_guard<std::mutex> lk(_mutex);

	auto iter = _items.find(iTimerID);
	if (iter == _items.end())
	{
		return false;
	}

	return true;
}

void CSleepServerTimer::destroyTimerItemPool()
{
	auto iter = _itemPool.begin();
	auto iEnd = _itemPool.end();
	for (; iter != iEnd; ++iter)
	{
		auto& item = *iter;
		delete item;
	}
	_itemPool.clear();
}

void CSleepServerTimer::onThread()
{
	const int micro_seconds = s_iResolution * 1000;

	ServerTimerItemPtrArray temps;
	long long currTime = 0;

	_evThreadStarted.set();

	while (_running)
	{
		bool bIsEmpty = false;
		{
			std::lock_guard<std::mutex> lk(_mutex);
			bIsEmpty = _items.empty();
		}

		if (bIsEmpty)
		{
			_evThreadWait.wait();
			continue;
		}

		::usleep(micro_seconds);

		temps.clear();
		currTime = GetSysMilliseconds() / s_iResolution * s_iResolution;

		{
			std::lock_guard<std::mutex> lk(_mutex);

			for (auto iter = _items.begin(); iter != _items.end();)
			{
				auto& item = iter->second;
				if ((currTime >= item->iStartTime) &&
					(currTime - item->iStartTime) % item->iElapse == 0)
				{
					temps.push_back(item);

					if (item->bShootOnce)
					{
						_items.erase(iter++);
						continue;
					}
				}
				iter++;
			}
		}

		if (!temps.empty())
		{
			auto iter = temps.begin();
			auto iEnd = temps.end();
			for (; iter != iEnd; ++iter)
			{
				auto& item = *iter;
				if (_listener != nullptr)
				{
					_listener->OnTimer(item->iTimerID, item->iElapse);
				}
			}
		}

	}
}
