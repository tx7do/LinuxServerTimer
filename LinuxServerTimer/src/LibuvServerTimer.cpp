
#include "LibuvServerTimer.h"

#include <thread>
#include <iostream>
#include <cassert>

// https://juejin.cn/post/7083030400389349413

template<typename T,
	typename std::enable_if<
		// these are the C-style 'subclasses' of uv_handle_t
		std::is_same<T, uv_async_t>::value ||
			std::is_same<T, uv_check_t>::value ||
			std::is_same<T, uv_fs_event_t>::value ||
			std::is_same<T, uv_fs_poll_t>::value ||
			std::is_same<T, uv_idle_t>::value ||
			std::is_same<T, uv_pipe_t>::value ||
			std::is_same<T, uv_poll_t>::value ||
			std::is_same<T, uv_prepare_t>::value ||
			std::is_same<T, uv_process_t>::value ||
			std::is_same<T, uv_signal_t>::value ||
			std::is_same<T, uv_stream_t>::value ||
			std::is_same<T, uv_tcp_t>::value ||
			std::is_same<T, uv_timer_t>::value ||
			std::is_same<T, uv_tty_t>::value ||
			std::is_same<T, uv_udp_t>::value>::type* = nullptr>
class UvHandle
{
public:
	UvHandle()
		: t_(new T)
	{
	}
	~UvHandle()
	{
		reset();
	}

	T* get()
	{
		return t_;
	}
	uv_handle_t* handle()
	{
		return reinterpret_cast<uv_handle_t*>(t_);
	}

	void reset()
	{
		auto* h = handle();
		if (h != nullptr)
		{
			if (uv_is_closing(h) == 0)
			{
				::uv_close(h, OnClosed);
			}
			t_ = nullptr;
		}
	}

private:
	static void OnClosed(uv_handle_t* handle)
	{
		delete reinterpret_cast<T*>(handle);
	}

	T* t_ = {};
};

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
//	stopEvent();

	_loop = new uv_loop_t{};

	::uv_loop_init(_loop);
	::uv_idle_init(_loop, &_idler);

	::uv_loop_configure(_loop, UV_LOOP_BLOCK_SIGNAL, SIGPROF);
}

void CLibuvServerTimer::stopEvent()
{
	::uv_idle_stop(&_idler);
	stop_and_close_uv_loop(_loop);
	::uv_loop_close(_loop);

	delete _loop;
	_loop = nullptr;
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

	::uv_idle_start(&_idler, [](uv_idle_t*)
	{
//		std::cout << "idle" << std::endl;
//		std::this_thread::sleep_for(std::chrono::milliseconds{ 50 });
	});

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
