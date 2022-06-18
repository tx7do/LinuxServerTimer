
#pragma once

#include <thread>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <atomic>
#include <cstring>

#include "IServerTimer.h"
#include "Event.h"
#include "ObjectPool.h"


class CEpollfdServerTimer final : public IServerTimer
{
public:
	struct ServerTimerItem
	{
		int iTimerFD{};

		timerid_t iTimerID{};
		elapse_t iElapse{};
		bool bShootOnce{};

		mutable std::mutex _mutex;

		ServerTimerItem() = default;
		void clear()
		{
			iTimerFD = -1;
			iTimerID = 0;
			iElapse = 0;
			bShootOnce = true;
		}
	};
	using ServerTimerItemPtr = std::shared_ptr<ServerTimerItem>;
	using ServerTimerItemPtrMap = std::unordered_map<timerid_t, ServerTimerItemPtr>;
	using ServerTimerItemPool = ObjectPool<ServerTimerItem, std::shared_ptr<ServerTimerItem>>;

public:
	CEpollfdServerTimer();
	~CEpollfdServerTimer() final;

public:
	ServerTimerType GetType() const final
	{
		return ServerTimerType_Epollfd;
	}

	void RegisterListener(IServerTimerListener* pListener) final;

	void Start() final;

	void Stop() final;

public:
	void SetTimer(timerid_t iTimerID, elapse_t iElapse, bool bShootOnce) final;

	void KillTimer(timerid_t iTimerID) final;

	void KillAllTimer() final;

protected:
	// 是否存在这个定时器
	bool isExistTimer(timerid_t iTimerID) const;

private:
	bool createEpoll();
	bool destroyEpoll();

	static bool destroyTimerfd(int iEpollFD, int iTimerFD);

	bool createThread();
	bool destroyThread();

protected:
	void onThread();
	void onThreadStartBefore();
	void onThreadStartEnd();

private:
	IServerTimerListener* _listener;
	std::atomic<bool> _running;

	int _epoll_fd;

	ServerTimerItemPool _pool;
	ServerTimerItemPtrMap _items;

	std::thread* _thread{};
	mutable std::mutex _mutex;
	CEvent _evThreadStarted;
};
