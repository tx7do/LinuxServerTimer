
#include "SleepServerTimer.h"
#include <ctime>
#include <cassert>
#include <chrono>

time_t GetSysMilliseconds()
{
	using std::chrono::duration_cast;
	using std::chrono::milliseconds;
	using std::chrono::seconds;
	using std::chrono::system_clock;
	return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

time_t CSleepServerTimer::s_iResolution = 100;

CSleepServerTimer::CSleepServerTimer()
	: _thread(nullptr), _listener(nullptr), _running(false), _pool(100)
{
}

CSleepServerTimer::~CSleepServerTimer()
{
	stopThread();
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

void CSleepServerTimer::SetTimer(timerid_t iTimerID, elapse_t iElapse, bool bShootOnce)
{
	if (iElapse < s_iResolution)
	{
		iElapse = s_iResolution;
	}

	auto iMS = GetSysMilliseconds();

	auto item = _pool.borrowObject();

	item->iTimerID = iTimerID;
	item->iElapse = iElapse / s_iResolution * s_iResolution;
	item->iStartTime = iMS / s_iResolution * s_iResolution + iElapse;
	item->bShootOnce = bShootOnce;

	std::lock_guard<std::mutex> lk(_mutex);
	_items[iTimerID] = item;

	_evThreadWait.set();
}

void CSleepServerTimer::KillTimer(timerid_t iTimerID)
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
	_pool.returnObject(item);
	_items.erase(iter);
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
		_pool.returnObject(item);
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

inline bool CSleepServerTimer::isExistTimer(timerid_t iTimerID) const
{
	std::lock_guard<std::mutex> lk(_mutex);

	auto iter = _items.find(iTimerID);
	if (iter == _items.end())
	{
		return false;
	}

	return true;
}

void CSleepServerTimer::onThread()
{
	const time_t micro_seconds = s_iResolution * 1000;

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

		std::this_thread::sleep_for(std::chrono::microseconds{ micro_seconds });

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
			for (auto& item : temps)
			{
				if (_listener != nullptr)
				{
					_listener->OnTimer(item->iTimerID, item->iElapse);
				}
			}
		}

	}
}
