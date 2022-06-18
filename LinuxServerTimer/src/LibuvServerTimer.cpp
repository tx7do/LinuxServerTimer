
#include "LibuvServerTimer.h"

#include <thread>
#include <iostream>
#include <cassert>


void stop_and_close_uv_loop(uv_loop_t* loop)
{
	::uv_stop(loop);

	auto const ensure_closing = [](uv_handle_t* handle, void*)
	{
		if (!::uv_is_closing(handle))
		{
			::uv_close(handle, nullptr);
		}
	};

	::uv_walk(loop, ensure_closing, nullptr);

	for (;;)
	{
		if (::uv_run(loop, UV_RUN_DEFAULT) == 0)
		{
			break;
		}
	}
}

CLibuvServerTimer::CLibuvServerTimer()
	: _running(false), _listener(nullptr), _pool(100), _thread(nullptr)
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

	::uv_timer_init(_loop, &item->uv_timer);

	item->uv_timer.data = item.get();

	::uv_timer_start(&item->uv_timer, CLibuvServerTimer::onEvent, iElapse, iElapse);

	item->parent = this;
	item->iTimerID = iTimerID;
	item->iElapse = iElapse;
	item->bShootOnce = bShootOnce;

	std::lock_guard<std::mutex> lk(_mutex);
	_items[iTimerID] = item;

	::uv_async_send(call_next_tick_async_.get());
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
	stopEvent();

	_loop = new uv_loop_t{};

	::uv_loop_init(_loop);

	::uv_loop_configure(_loop, UV_LOOP_BLOCK_SIGNAL, SIGPROF);

	::uv_async_init(_loop, call_next_tick_async_.get(), [](uv_async_t* handle){
		std::cout << "async:" << std::this_thread::get_id() << std::endl;
	});

}

void CLibuvServerTimer::stopEvent()
{
	if (_loop == nullptr) return;

	stop_and_close_uv_loop(_loop);
	::uv_loop_close(_loop);

	delete _loop;
	_loop = nullptr;

	std::cout << "libuv finished." << std::endl;
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

//	::uv_idle_start(&_idler, [](uv_idle_t*)	{	});

	::uv_run(_loop, UV_RUN_DEFAULT);

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
