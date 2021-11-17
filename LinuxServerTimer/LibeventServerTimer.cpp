
#include "LibeventServerTimer.h"

#include <sys/types.h>
#include <event2/event-config.h>
#include <sys/stat.h>
#include <time.h>
#ifdef EVENT__HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <thread>
#include <iostream>
#include <assert.h>

#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/util.h>


CLibeventServerTimer::CLibeventServerTimer()
	: _running(false)
	, _listener(nullptr)
	, _base(nullptr)
	, _thread(nullptr)
{
}

CLibeventServerTimer::~CLibeventServerTimer()
{
	stopEvent();
	stopThread();
	KillAllTimer();
	destroyTimerItemPool();
}

void CLibeventServerTimer::RegisterListener(IServerTimerListener* pListener)
{
	_listener = pListener;
}

void CLibeventServerTimer::Start()
{
	startThread();
}

void CLibeventServerTimer::Stop()
{
	_running = false;

	stopThread();

	KillAllTimer();
}

void CLibeventServerTimer::SetTimer(unsigned int iTimerID, unsigned int iElapse, bool bShootOnce)
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

	assert(_base);
	struct event* ev = ::event_new(_base, -1, bShootOnce ? 0 : EV_PERSIST, CLibeventServerTimer::onEvent, item);

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = iElapse * 1000;
	::event_add(ev, &tv);

	item->pParent = this;
	item->pEvent = ev;
	item->iTimerID = iTimerID;
	item->iElapse = iElapse;
	item->bShootOnce = bShootOnce;
	_items[iTimerID] = item;
}

void CLibeventServerTimer::KillTimer(unsigned int iTimerID)
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
	::event_free(item->pEvent);
	item->clear();

	_itemPool.push_back(item);
	_items.erase(iter);

	return;
}

void CLibeventServerTimer::KillAllTimer()
{
	std::lock_guard<std::mutex> lk(_mutex);

	auto iter = _items.begin();
	auto iEnd = _items.end();
	for (; iter != iEnd; ++iter)
	{
		auto& item = iter->second;
		::event_free(item->pEvent);
		item->clear();
		_itemPool.push_back(item);
	}
	_items.clear();
}

bool CLibeventServerTimer::isExistTimer(unsigned int iTimerID) const
{
	std::lock_guard<std::mutex> lk(_mutex);

	auto iter = _items.find(iTimerID);
	if (iter == _items.end())
	{
		return false;
	}

	return true;
}

inline void CLibeventServerTimer::destroyTimerItemPool()
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

void CLibeventServerTimer::startEvent()
{
	if (_base != nullptr)
	{
		stopEvent();
	}

	_base = event_base_new();
	assert(_base);
	//std::cout << "Event Base:" << _base << std::endl;
}

void CLibeventServerTimer::stopEvent()
{
	if (_base != nullptr)
	{
		::event_base_free(_base);
		_base = nullptr;
	}
}

void CLibeventServerTimer::startThread()
{
	if (_thread == nullptr)
	{
		_running = true;

		_thread = new std::thread(&CLibeventServerTimer::onThread, (CLibeventServerTimer*)this);
		assert(_thread);
		_evThreadStarted.wait();
	}
}

void CLibeventServerTimer::stopThread()
{
	_running = false;
	if (_thread != nullptr)
	{
		_thread->join();
		delete _thread;
		_thread = nullptr;
	}
}

void CLibeventServerTimer::onEvent(evutil_socket_t fd, short event, void* arg)
{
	struct CLibeventServerTimer::ServerTimerItem* pParam = (struct CLibeventServerTimer::ServerTimerItem*)arg;
	assert(pParam);
	assert(pParam->pParent);

	//std::cout << "Event Thread ID:" << std::this_thread::get_id() << std::endl;

	if (pParam->pParent != nullptr)
	{
		pParam->pParent->onTimeOut(pParam->iTimerID, pParam->iElapse, pParam->bShootOnce);
	}
}

void CLibeventServerTimer::onThread()
{
	std::cout << "Timer Thread ID:" << std::this_thread::get_id() << std::endl;

	startEvent();

	_evThreadStarted.set();

	while (_running)
	{
		event_base_dispatch(_base);
	}

	stopEvent();
}

void CLibeventServerTimer::onTimeOut(unsigned int iTimerID, unsigned int iElapse, bool bShootOnce)
{
	if (!_running)
	{
		::event_base_loopbreak(_base);
	}

	if (bShootOnce)
	{
		KillTimer(iTimerID);
	}

	if (_listener != nullptr)
	{
		_listener->OnTimer(iTimerID, iElapse);
	}
}
