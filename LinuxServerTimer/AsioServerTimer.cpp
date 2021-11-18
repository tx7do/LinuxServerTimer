
#include "AsioServerTimer.h"

CAsioServerTimer::CAsioServerTimer()
	: _thread(nullptr)
	, _running(false)
	, _listener(nullptr)
{
}

CAsioServerTimer::~CAsioServerTimer()
{
	stopThread();
	KillAllTimer();
	destroyTimerItemPool();
}

void CAsioServerTimer::RegisterListener(IServerTimerListener* pListener)
{
	_listener = pListener;
}

void CAsioServerTimer::Start()
{
	_ioc.reset();
	startThread();
}

void CAsioServerTimer::Stop()
{
	_ioc.stop();

	_running = false;

	stopThread();

	KillAllTimer();
}

void CAsioServerTimer::SetTimer(unsigned int iTimerID, unsigned int iElapse, bool bShootOnce)
{
	if (isExistTimer(iTimerID))
	{
		return;
	}

	std::lock_guard<std::mutex> lk(_mutex);

	ServerTimerItemPtr item = nullptr;
	if (_itemPool.empty())
	{
		item = new ServerTimerItem();
	}
	else
	{
		item = _itemPool.back();
		_itemPool.pop_back();
	}
	assert(item);

	//item->t = new boost::asio::deadline_timer(_ioc, boost::posix_time::milliseconds(iElapse));
	item->t = new boost::asio::steady_timer(_ioc, std::chrono::milliseconds(iElapse));
	item->iTimerID = iTimerID;
	item->iElapse = iElapse;
	item->bShootOnce = bShootOnce;
	_items[iTimerID] = item;

	item->t->async_wait(boost::bind(&CAsioServerTimer::onTimeOut, this, boost::asio::placeholders::error, item));
}

void CAsioServerTimer::KillTimer(unsigned int iTimerID)
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
	delete item->t;
	item->clear();

	_itemPool.push_back(item);
	_items.erase(iter);

	return;
}

void CAsioServerTimer::KillAllTimer()
{
	std::lock_guard<std::mutex> lk(_mutex);

	auto iter = _items.begin();
	auto iEnd = _items.end();
	for (; iter != iEnd; ++iter)
	{
		auto& item = iter->second;
		delete item->t;
		item->clear();
		_itemPool.push_back(item);
	}
	_items.clear();
}

void CAsioServerTimer::startThread()
{
	if (_thread == nullptr)
	{
		_running = true;

		_thread = new std::thread(&CAsioServerTimer::onThread, this);
		assert(_thread);
		_evThreadStarted.wait();
	}
}

void CAsioServerTimer::stopThread()
{
	_running = false;
	if (_thread != nullptr)
	{
		_thread->join();
		delete _thread;
		_thread = nullptr;
	}
}

bool CAsioServerTimer::isExistTimer(unsigned int iTimerID) const
{
	std::lock_guard<std::mutex> lk(_mutex);

	auto iter = _items.find(iTimerID);
	if (iter == _items.end())
	{
		return false;
	}

	return true;
}

void CAsioServerTimer::destroyTimerItemPool()
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

void CAsioServerTimer::onThread()
{
	_evThreadStarted.set();
	auto work_guard = boost::asio::make_work_guard(_ioc);
	while (_running)
	{
		_ioc.run();
	}
}

void CAsioServerTimer::onTimeOut(boost::system::error_code ec, ServerTimerItemPtr pm)
{
	if (pm == nullptr) return;

	const unsigned int iTimerID = pm->iTimerID;
	const unsigned int iElapse = pm->iElapse;
	if (pm->bShootOnce)
	{
		KillTimer(iTimerID);
	}
	else
	{
		pm->t->expires_from_now(std::chrono::milliseconds(iElapse));
		pm->t->async_wait(boost::bind(&CAsioServerTimer::onTimeOut, this, boost::asio::placeholders::error, pm));
	}

	if (_listener != nullptr)
	{
		_listener->OnTimer(iTimerID, iElapse);
	}
}
