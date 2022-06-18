#pragma once

#include <thread>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <atomic>
#include <cstring>

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "IServerTimer.h"
#include "ObjectPool.h"
#include "Event.h"


//using atimer_t = boost::asio::steady_timer;
using atimer_t = boost::asio::high_resolution_timer;
using atimer_ptr = std::shared_ptr<atimer_t>;

using milliseconds = std::chrono::milliseconds;


class CAsioServerTimer final : public IServerTimer
{
	struct ServerTimerItem
	{
		timerid_t iTimerID{ 0 };
		milliseconds iElapse{ 0 };
		bool bShootOnce{ true };

		atimer_ptr t{ nullptr };

		mutable std::mutex _mutex;

		void cancel()
		{
			iTimerID = -1;
			if (t)t->cancel();
		}
	};
	using ServerTimerItemPtr = std::shared_ptr<ServerTimerItem>;
	using ServerTimerItemPtrMap = std::unordered_map<timerid_t, ServerTimerItemPtr>;
	using ServerTimerItemPool = ObjectPool<ServerTimerItem, std::shared_ptr<ServerTimerItem>>;

public:
	CAsioServerTimer();
	~CAsioServerTimer() final;

public:
	ServerTimerType GetType() const final
	{
		return ServerTimerType_Asio;
	}

	void RegisterListener(IServerTimerListener* pListener) final;

	void Start() final;

	void Stop() final;

	void SetTimer(timerid_t iTimerID, elapse_t iElapse, bool bShootOnce) final;

	void KillTimer(timerid_t iTimerID) final;

	void KillAllTimer() final;

protected:
	// 启动线程
	void startThread();
	// 停止线程
	void stopThread();

protected:
	// 是否存在这个定时器
	bool isExistTimer(timerid_t iTimerID) const;

protected:
	void onThread();

	void onTimeOut(boost::system::error_code ec, ServerTimerItemPtr pm);

protected:
	boost::asio::io_context _ioc;

	std::thread* _thread;
	mutable std::mutex _mutex;
	CEvent _evThreadStarted;

	ServerTimerItemPool _pool;
	ServerTimerItemPtrMap _items;

	IServerTimerListener* _listener;
};
