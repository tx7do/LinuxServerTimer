
#pragma once

#include "IServerTimer.h"

#include <thread>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <atomic>
#include <cstring>
#include "Event.h"


class CEpollfdServerTimer final : public IServerTimer
{
public:
	struct ServerTimerItem
	{
		int iTimerFD{};

		unsigned int iTimerID{};
		unsigned int iElapse{};
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
	typedef ServerTimerItem* ServerTimerItemPtr;
	typedef std::unordered_map<unsigned int, ServerTimerItemPtr> ServerTimerItemPtrMap;
	typedef std::vector<ServerTimerItemPtr> ServerTimerItemPtrArray;

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
	void SetTimer(unsigned int iTimerID, unsigned int iElapse, bool bShootOnce) final;

	void KillTimer(unsigned int iTimerID) final;

	void KillAllTimer() final;

protected:
	// 是否存在这个定时器
	bool isExistTimer(unsigned int iTimerID) const;

	// 销毁定时器池的项
	void destroyTimerItemPool();

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

	ServerTimerItemPtrMap _items;
	ServerTimerItemPtrArray _itemPool;

	std::thread* _thread{};
	mutable std::mutex _mutex;
	CEvent _evThreadStarted;
};
