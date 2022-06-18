
#pragma once

#include <thread>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <atomic>

#include <uv.h>

#include "IServerTimer.h"
#include "Event.h"
#include "ObjectPool.h"


class CLibuvServerTimer final : public IServerTimer
{
	struct ServerTimerItem
	{
		CLibuvServerTimer* parent = nullptr;

		uv_timer_t uv_timer{};

		timerid_t iTimerID{};
		elapse_t iElapse{};
		bool bShootOnce{};

		mutable std::mutex _mutex;

		ServerTimerItem() = default;
		void clear()
		{
			iTimerID = 0;
			iElapse = 0;
			bShootOnce = true;
			//::uv_close((uv_handle_t*) &uv_timer, nullptr);
		}
	};
	using ServerTimerItemPtr = std::shared_ptr<ServerTimerItem>;
	using ServerTimerItemPtrMap = std::unordered_map<timerid_t, ServerTimerItemPtr>;
	using ServerTimerItemPool = ObjectPool<ServerTimerItem, std::shared_ptr<ServerTimerItem>>;

public:
	CLibuvServerTimer();
	~CLibuvServerTimer() final;

public:
	ServerTimerType GetType() const final
	{
		return ServerTimerType_Libuv;
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
	static void onEvent(uv_timer_t* handle);

	// 线程回调方法
	void onThread();

	// 定时器超时回调方法
	void onTimeOut(timerid_t iTimerID, elapse_t iElapse, bool bShootOnce);

private:
	uv_loop_t* _loop = nullptr;
	uv_idle_t _idler = {};

	std::thread* _thread = { nullptr };
	mutable std::mutex _mutex;
	CEvent _evThreadStarted;
	std::atomic<bool> _running = { false };

	ServerTimerItemPool _pool;
	ServerTimerItemPtrMap _items;

	IServerTimerListener* _listener = { nullptr };
};
