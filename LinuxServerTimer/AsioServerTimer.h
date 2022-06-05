#pragma once


#include "IServerTimer.h"
#include <thread>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <atomic>
#include <cstring>
#include "Event.h"

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


class CAsioServerTimer final : public IServerTimer
{
	struct ServerTimerItem
	{
		unsigned int iTimerID{};
		unsigned int iElapse{};
		bool bShootOnce{};

		boost::asio::steady_timer* t{};

		mutable std::mutex _mutex;

		ServerTimerItem()
		{
			clear();
		}
		void clear()
		{
			iTimerID = 0;
			iElapse = 0;
			bShootOnce = true;
			t = nullptr;
		}
	};
	typedef ServerTimerItem* ServerTimerItemPtr;
	typedef std::unordered_map<unsigned int, ServerTimerItemPtr> ServerTimerItemPtrMap;
	typedef std::vector<ServerTimerItemPtr> ServerTimerItemPtrArray;

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

	void SetTimer(unsigned int iTimerID, unsigned int iElapse, bool bShootOnce) final;

	void KillTimer(unsigned int iTimerID) final;

	void KillAllTimer() final;

protected:
	// 启动线程
	void startThread();
	// 停止线程
	void stopThread();

protected:
	// 是否存在这个定时器
	bool isExistTimer(unsigned int iTimerID) const;

	// 销毁定时器池的项
	void destroyTimerItemPool();

protected:
	void onThread();

	void onTimeOut(boost::system::error_code ec, ServerTimerItemPtr pm);

protected:
	boost::asio::io_context _ioc;

	std::thread* _thread;
	mutable std::mutex _mutex;
	CEvent _evThreadStarted;
	std::atomic<bool> _running;

	ServerTimerItemPtrMap _items;
	ServerTimerItemPtrArray _itemPool;

	IServerTimerListener* _listener;
};
