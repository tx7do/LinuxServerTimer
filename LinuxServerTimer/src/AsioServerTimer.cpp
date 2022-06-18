
#include "AsioServerTimer.h"

using error_code = boost::system::error_code;

CAsioServerTimer::CAsioServerTimer()
	: _thread(nullptr), _listener(nullptr), _pool(100)
{
}

CAsioServerTimer::~CAsioServerTimer()
{
	stopThread();
	KillAllTimer();
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

	stopThread();

	KillAllTimer();
}

void CAsioServerTimer::SetTimer(timerid_t iTimerID, elapse_t iElapse, bool bShootOnce)
{
	if (isExistTimer(iTimerID))
	{
		return;
	}

	std::lock_guard<std::mutex> lk(_mutex);

	auto item = _pool.borrowObject();
	assert(item);

	if (item->t == nullptr)
	{
		item->t = std::make_shared<atimer_t>(_ioc);
	}

	item->t->expires_after(milliseconds(iElapse));

	item->iTimerID = iTimerID;
	item->iElapse = milliseconds(iElapse);
	item->bShootOnce = bShootOnce;
	_items[iTimerID] = item;

	item->t->async_wait([this, item](error_code ec)
	{
		onTimeOut(ec, item);
	});
}

void CAsioServerTimer::KillTimer(timerid_t iTimerID)
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

	_pool.returnObject(item);
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
		_pool.returnObject(item);
	}
	_items.clear();
}

void CAsioServerTimer::startThread()
{
	if (_thread == nullptr)
	{
		_thread = new std::thread(&CAsioServerTimer::onThread, this);
		assert(_thread);
		_evThreadStarted.wait();
	}
}

void CAsioServerTimer::stopThread()
{
	if (_thread != nullptr)
	{
		_thread->join();
		delete _thread;
		_thread = nullptr;
	}
}

bool CAsioServerTimer::isExistTimer(timerid_t iTimerID) const
{
	std::lock_guard<std::mutex> lk(_mutex);

	auto iter = _items.find(iTimerID);
	if (iter == _items.end())
	{
		return false;
	}

	return true;
}

void CAsioServerTimer::onThread()
{
	_evThreadStarted.set();
	auto work_guard = boost::asio::make_work_guard(_ioc);
	_ioc.run();
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
