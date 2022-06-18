
#pragma once

#include <thread>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <atomic>

#include <event2/event.h>
#include <event2/event_struct.h>

#include "IServerTimer.h"
#include "Event.h"
#include "ObjectPool.h"


class CLibeventServerTimer final : public IServerTimer
{
	struct ServerTimerItem
	{
		CLibeventServerTimer* pParent{};
		struct event* pEvent{};

		timerid_t iTimerID{};
		elapse_t iElapse{};
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
	using ServerTimerItemPtr = std::shared_ptr<ServerTimerItem>;
	using ServerTimerItemPtrMap = std::unordered_map<timerid_t, ServerTimerItemPtr>;
	using ServerTimerItemPool = ObjectPool<ServerTimerItem, std::shared_ptr<ServerTimerItem>>;

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
	void SetTimer(timerid_t iTimerID, elapse_t iElapse, bool bShootOnce) final;

	void KillTimer(timerid_t iTimerID) final;

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
	bool isExistTimer(timerid_t iTimerID) const;

protected:
	// 事件队列回调方法
	static void onEvent(evutil_socket_t fd, short event, void* arg);

	// 线程回调方法
	void onThread();

	// 定时器超时回调方法
	void onTimeOut(timerid_t iTimerID, elapse_t iElapse, bool bShootOnce);

private:
	struct event_base* _base;

	std::thread* _thread;
	mutable std::mutex _mutex;
	CEvent _evThreadStarted;
	std::atomic<bool> _running;

	ServerTimerItemPool _pool;
	ServerTimerItemPtrMap _items;

	IServerTimerListener* _listener;
};
