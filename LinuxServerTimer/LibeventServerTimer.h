
#pragma once

#include "IServerTimer.h"
#include "Event.h"

#include <thread>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <atomic>

#include <event2/event.h>
#include <event2/event_struct.h>

struct event_base;
struct event;

class CLibeventServerTimer final : public IServerTimer
{
	struct ServerTimerItem
	{
		CLibeventServerTimer* pParent{};
		struct event* pEvent{};

		unsigned int iTimerID{};
		unsigned int iElapse{};
		bool bShootOnce{};

		mutable std::mutex _mutex;

		ServerTimerItem() = default;
		void clear()
		{
			pParent = nullptr;
			pEvent = nullptr;
			iTimerID = 0;
			iElapse = 0;
			bShootOnce = true;
		}
	};
	typedef ServerTimerItem* ServerTimerItemPtr;
	typedef std::unordered_map<unsigned int, ServerTimerItemPtr> ServerTimerItemPtrMap;
	typedef std::vector<ServerTimerItemPtr> ServerTimerItemPtrArray;

public:
	CLibeventServerTimer();
	~CLibeventServerTimer() final;

public:
	ServerTimerType GetType() const final
	{
		return ServerTimerType_Libevent;
	}

	void RegisterListener(IServerTimerListener* pListener) final;

	void Start() final;

	void Stop() final;

public:
	void SetTimer(unsigned int iTimerID, unsigned int iElapse, bool bShootOnce) final;

	void KillTimer(unsigned int iTimerID) final;

	void KillAllTimer() final;

protected:
	// 启动事件队列
	void startEvent();
	// 停止事件队列
	void stopEvent();

	// 启动定时器工作线程
	void startThread();
	// 停止定时器工作线程
	void stopThread();

protected:
	// 是否存在这个定时器
	bool isExistTimer(unsigned int iTimerID) const;

	// 销毁定时器池的项
	void destroyTimerItemPool();

protected:
	// 事件队列回调方法
	static void onEvent(evutil_socket_t fd, short event, void* arg);

	// 线程回调方法
	void onThread();

	// 定时器超时回调方法
	void onTimeOut(unsigned int iTimerID, unsigned int iElapse, bool bShootOnce);

private:
	struct event_base* _base;

	std::thread* _thread;
	mutable std::mutex _mutex;
	CEvent _evThreadStarted;
	std::atomic<bool> _running;

	ServerTimerItemPtrMap _items;
	ServerTimerItemPtrArray _itemPool;

	IServerTimerListener* _listener;
};
