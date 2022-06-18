
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
	UvHandle<uv_async_t> call_next_tick_async_;

	std::thread* _thread = { nullptr };
	mutable std::mutex _mutex;
	CEvent _evThreadStarted;

	ServerTimerItemPool _pool;
	ServerTimerItemPtrMap _items;

	IServerTimerListener* _listener = { nullptr };
};
