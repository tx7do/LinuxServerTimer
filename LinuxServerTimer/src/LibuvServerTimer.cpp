
#include "LibuvServerTimer.h"

#include <thread>
#include <iostream>
#include <cassert>

CLibuvServerTimer::CLibuvServerTimer()
	: _running(false), _listener(nullptr), _pool(100), _thread(nullptr), _loop(nullptr)
{
}

CLibuvServerTimer::~CLibuvServerTimer()
{
	stopEvent();
	stopThread();
	KillAllTimer();
}

void CLibuvServerTimer::RegisterListener(IServerTimerListener* pListener)
{
	_listener = pListener;
}

void CLibuvServerTimer::Start()
{
	startThread();
}

void CLibuvServerTimer::Stop()
{
	_running = false;

	stopThread();

	KillAllTimer();
}

void CLibuvServerTimer::SetTimer(timerid_t iTimerID, elapse_t iElapse, bool bShootOnce)
{
	if (isExistTimer(iTimerID))
	{
		return;
	}

	auto item = _pool.borrowObject();

	assert(_loop);

	::uv_timer_init(_loop, &item->uv_timer);

	item->uv_timer.data = item.get();

	::uv_timer_start(&item->uv_timer, CLibuvServerTimer::onEvent, iElapse, iElapse);

	item->parent = this;
	item->iTimerID = iTimerID;
	item->iElapse = iElapse;
	item->bShootOnce = bShootOnce;

	std::lock_guard<std::mutex> lk(_mutex);
	_items[iTimerID] = item;
}

void CLibuvServerTimer::KillTimer(timerid_t iTimerID)
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
	::uv_timer_stop(&item->uv_timer);
	item->clear();

	_pool.returnObject(item);
	_items.erase(iter);
}

void CLibuvServerTimer::KillAllTimer()
{
	std::lock_guard<std::mutex> lk(_mutex);

	auto iter = _items.begin();
	auto iEnd = _items.end();
	for (; iter != iEnd; ++iter)
	{
		auto& item = iter->second;
		::uv_timer_stop(&item->uv_timer);
		item->clear();
		_pool.returnObject(item);
	}
	_items.clear();
}

bool CLibuvServerTimer::isExistTimer(timerid_t iTimerID) const
{
	std::lock_guard<std::mutex> lk(_mutex);

	auto iter = _items.find(iTimerID);
	if (iter == _items.end())
	{
		return false;
	}

	return true;
}

void CLibuvServerTimer::startEvent()
{
	if (_loop != nullptr)
	{
		stopEvent();
	}

	_loop = ::uv_default_loop();
//	::uv_idle_init(_loop, &_idler);
}

void CLibuvServerTimer::stopEvent()
{
	if (_loop != nullptr)
	{
		::uv_loop_close(_loop);
		_loop = nullptr;
	}
}

void CLibuvServerTimer::startThread()
{
	if (_thread == nullptr)
	{
		_running = true;

		_thread = new std::thread(&CLibuvServerTimer::onThread, (CLibuvServerTimer*)this);
		assert(_thread);
		_evThreadStarted.wait();
	}
}

void CLibuvServerTimer::stopThread()
{
	_running = false;
	if (_thread != nullptr)
	{
		_thread->join();
		delete _thread;
		_thread = nullptr;
	}
}

void CLibuvServerTimer::onEvent(uv_timer_t* handle)
{
	auto* pParam = (struct CLibuvServerTimer::ServerTimerItem*)handle->data;
	assert(pParam);
	assert(pParam->parent);

	//std::cout << "Event Thread ID:" << std::this_thread::get_id() << std::endl;

	if (pParam->parent != nullptr)
	{
		pParam->parent->onTimeOut(pParam->iTimerID, pParam->iElapse, pParam->bShootOnce);
	}
}

void CLibuvServerTimer::onThread()
{
	std::cout << "Timer Thread ID:" << std::this_thread::get_id() << std::endl;

	startEvent();

	_evThreadStarted.set();

//	::uv_idle_start(&_idler, wait_for_a_while);
	do
	{
		::uv_run(_loop, UV_RUN_DEFAULT);
	} while (_running);

	stopEvent();
}

void CLibuvServerTimer::onTimeOut(timerid_t iTimerID, elapse_t iElapse, bool bShootOnce)
{
	if (bShootOnce)
	{
		KillTimer(iTimerID);
	}

	if (_listener != nullptr)
	{
		_listener->OnTimer(iTimerID, iElapse);
	}
}
