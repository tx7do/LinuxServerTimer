
#pragma once

#include "IServerTimer.h"

#include <thread>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <string.h>
#include "Event.h"


class CEpollfdServerTimer : public IServerTimer
{
public:
	struct ServerTimerItem
	{
		int iTimerFD;

		unsigned int iTimerID;
		unsigned int iElapse;
		bool bShootOnce;

		mutable std::mutex _mutex;

		ServerTimerItem()
		{
			clear();
		}
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
	virtual ~CEpollfdServerTimer();

public:
	virtual ServerTimerType GetType() const { return ServerTimerType_Epollfd; }

	virtual void RegisterListener(IServerTimerListener* pListener);

	virtual void Start();

	virtual void Stop();

public:
	virtual void SetTimer(unsigned int iTimerID, unsigned int iElapse, bool bShootOnce = true);

	virtual void KillTimer(unsigned int iTimerID);

	virtual void KillAllTimer();

protected:
	// 是否存在这个定时器
	bool isExistTimer(unsigned int iTimerID) const;

	// 销毁定时器池的项
	void destroyTimerItemPool();

private:
	bool createEpoll();
	bool destroyEpoll();

	bool destroyTimerfd(int iEpollFD, int iTimerFD);

	bool createThread();
	bool destroyThread();

protected:
	void onThread();
	void onThreadStartBefore();
	void onThreadStartEnd();

private:
	IServerTimerListener* _listener;
	bool _running;

	int _epoll_fd;

	ServerTimerItemPtrMap _items;
	ServerTimerItemPtrArray _itemPool;

	std::thread* _thread;
	mutable std::mutex _mutex;
	CEvent _evThreadStarted;
};
