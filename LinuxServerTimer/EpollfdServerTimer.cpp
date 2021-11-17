
#include "EpollfdServerTimer.h"

#include <stdlib.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <stdint.h>
#include <algorithm>
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <sys/types.h>

#define MAX_EPOLL 10
#define EPOLL_TIMEOUT -1

static char* itimerspec_dump(struct itimerspec* ts)
{
	static char buf[1024];

	snprintf(buf, sizeof(buf),
		"itimer: [ interval=%lu s %lu ns, next expire=%lu s %lu ns ]",
		ts->it_interval.tv_sec,
		ts->it_interval.tv_nsec,
		ts->it_value.tv_sec,
		ts->it_value.tv_nsec
	);

	return (buf);
}

CEpollfdServerTimer::CEpollfdServerTimer()
	: _listener(nullptr)
	, _running(false)
	, _epoll_fd(-1)
{
}

CEpollfdServerTimer::~CEpollfdServerTimer()
{
	KillAllTimer();
	destroyEpoll();
	destroyThread();
	destroyTimerItemPool();
}

void CEpollfdServerTimer::RegisterListener(IServerTimerListener* pListener)
{
	_listener = pListener;
}

void CEpollfdServerTimer::Start()
{
	createThread();
}

void CEpollfdServerTimer::Stop()
{
	KillAllTimer();
	destroyThread();
}

void CEpollfdServerTimer::SetTimer(unsigned int iTimerID, unsigned int iElapse, bool bShootOnce)
{
	if (isExistTimer(iTimerID))
	{
		return;
	}

	int fd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
	assert(fd >= 0);
	if (fd < 0)
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

	item->iTimerFD = fd;
	item->iTimerID = iTimerID;
	item->iElapse = iElapse;
	item->bShootOnce = bShootOnce;
	_items[iTimerID] = item;

	struct timespec nw;
	if (::clock_gettime(CLOCK_MONOTONIC, &nw) != 0)
	{
		perror("clock_gettime() error");
		return;
	}

	struct itimerspec ts;
	ts.it_value.tv_sec = iElapse / 1000;
	ts.it_value.tv_nsec = (iElapse % 1000) * 1000000;
	ts.it_interval.tv_sec = bShootOnce ? 0 : (iElapse / 1000);
	ts.it_interval.tv_nsec = bShootOnce ? 0 : (iElapse % 1000) * 1000000;
	if (::timerfd_settime(fd, 0, &ts, nullptr) < 0)
	{
		printf("timerfd_settime() failed: errno=%d\n", errno);
		return;
	}

	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.ptr = item;
	if (::epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1)
	{
		printf("epoll_ctl(ADD) failed: errno=%d\n", errno);
		return;
	}
}

void CEpollfdServerTimer::KillTimer(unsigned int iTimerID)
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
	destroyTimerfd(_epoll_fd, item->iTimerFD);
	item->clear();

	_itemPool.push_back(item);
	_items.erase(iter);

	return;
}

void CEpollfdServerTimer::KillAllTimer()
{
	std::lock_guard<std::mutex> lk(_mutex);

	auto iter = _items.begin();
	auto iEnd = _items.end();
	for (; iter != iEnd; ++iter)
	{
		auto& item = iter->second;
		destroyTimerfd(_epoll_fd, item->iTimerFD);
		item->clear();
		_itemPool.push_back(item);
	}
	_items.clear();
}

bool CEpollfdServerTimer::isExistTimer(unsigned int iTimerID) const
{
	std::lock_guard<std::mutex> lk(_mutex);

	auto iter = _items.find(iTimerID);
	if (iter == _items.end())
	{
		return false;
	}

	return true;
}

void CEpollfdServerTimer::destroyTimerItemPool()
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

bool CEpollfdServerTimer::createEpoll()
{
	if (_epoll_fd < 0)
	{
		_epoll_fd = ::epoll_create1(EPOLL_CLOEXEC);
	}
	return _epoll_fd >= 0;
}

bool CEpollfdServerTimer::destroyEpoll()
{
	if (_epoll_fd >= 0)
	{
		::close(_epoll_fd);
		_epoll_fd = -1;
	}
	return true;
}

bool CEpollfdServerTimer::destroyTimerfd(int iEpollFD, int iTimerFD)
{
	bool bResult = true;
	int ret = ::epoll_ctl(iEpollFD, EPOLL_CTL_DEL, iTimerFD, nullptr);
	if (ret < 0)
	{
		printf("epoll_ctl() failed: errno=%d\n", errno);
		bResult = false;
	}

	struct itimerspec ts = { 0 };
	ret = ::timerfd_settime(iTimerFD, 0, &ts, nullptr);
	if (ret < 0)
	{
		printf("timerfd_settime() failed: errno=%d\n", errno);
		bResult = false;
	}

	ret = ::close(iTimerFD);
	if (ret < 0)
	{
		printf("close() failed: errno=%d\n", errno);
		bResult = false;
	}

	return bResult;
}

bool CEpollfdServerTimer::createThread()
{
	if (_thread == nullptr)
	{
		_running = true;

		_thread = new std::thread(&CEpollfdServerTimer::onThread, this);
		assert(_thread);

		_evThreadStarted.wait();
		return true;
	}
	return false;
}

bool CEpollfdServerTimer::destroyThread()
{
	_running = false;
	if (_thread != nullptr)
	{
		_thread->join();
		delete _thread;
		_thread = nullptr;
		return true;
	}
	return false;
}

void CEpollfdServerTimer::onThread()
{
	onThreadStartBefore();

	_evThreadStarted.set();

	int iTimerFD = 0;
	unsigned int iTimerID = 0;
	unsigned int iElapse = 0;
	int fireEvents = 0;
	ssize_t ret = 0;
	uint64_t exp = 0;
	struct epoll_event events[MAX_EPOLL] = { 0 };
	while (_running)
	{
		fireEvents = ::epoll_wait(_epoll_fd, events, MAX_EPOLL, EPOLL_TIMEOUT);
		if (fireEvents < 0)
		{
			printf("epoll_wait() failed: errno=%d\n", errno);
			break;
		}

		for (int i = 0; i < fireEvents; ++i)
		{
			struct ServerTimerItem* pm = (struct ServerTimerItem*)(events[i].data.ptr);
			//printf("timeout %d : %d\n", pm->iTimerID, pm->iElapse);

			iTimerFD = pm->iTimerFD;
			iTimerID = pm->iTimerID;
			iElapse = pm->iElapse;

			if (iTimerID <= 0) continue;

			if (pm->bShootOnce)
			{
				KillTimer(iTimerID);
				iTimerFD = -1;
			}

			if (_listener)
			{
				_listener->OnTimer(iTimerID, iElapse);
			}

			if (iTimerFD == -1) continue;

			ret = ::read(iTimerFD, &exp, sizeof(exp));
			if (ret != sizeof(exp))
			{
				printf("read() ret=%ld\n", ret);
			}
			//printf("read() returned %ld, res=%" PRIu64 "\n", ret, exp);
		}
	}

	onThreadStartEnd();
}

void CEpollfdServerTimer::onThreadStartBefore()
{
	if (!createEpoll())
	{
		destroyEpoll();
		return;
	}
}

void CEpollfdServerTimer::onThreadStartEnd()
{
	destroyEpoll();
}
