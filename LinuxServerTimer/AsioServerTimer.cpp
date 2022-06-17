
#include "AsioServerTimer.h"

using error_code = boost::system::error_code;

CAsioServerTimer::CAsioServerTimer()
	: _thread(nullptr), _running(false), _listener(nullptr)
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

	if (item->t == nullptr)
	{
		item->t = std::make_shared<atimer_t>(_ioc);
	}

	item->t->expires_after(elapse_t(iElapse));

	item->iTimerID = iTimerID;
	item->iElapse = elapse_t(iElapse);
	item->bShootOnce = bShootOnce;
	_items[iTimerID] = item;

	item->t->async_wait([this, item](error_code ec)
	{
		onTimeOut(ec, item);
	});
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
	item->cancel();

	_itemPool.push_back(item);
	_items.erase(iter);
}

void CAsioServerTimer::KillAllTimer()
{
	std::lock_guard<std::mutex> lk(_mutex);

	auto iter = _items.begin();
	auto iEnd = _items.end();
	for (; iter != iEnd; ++iter)
	{
		auto& item = iter->second;
		item->cancel();
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

	if (ec == boost::asio::error::operation_aborted)
	{
		std::cout << "The timer is cancelled" << std::endl;
		return;
	}

	const auto iTimerID = pm->iTimerID;
	const auto iElapse = pm->iElapse;
	if (pm->bShootOnce)
	{
		KillTimer(iTimerID);
	}
	else
	{
		pm->t->expires_after(iElapse);
		pm->t->async_wait([this, pm](error_code ec)
		{
			onTimeOut(ec, pm);
		});
	}

	if (_listener != nullptr)
	{
		_listener->OnTimer(iTimerID, iElapse.count());
	}
}
