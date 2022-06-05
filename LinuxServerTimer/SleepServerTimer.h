#pragma once

#include "IServerTimer.h"

#include <thread>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <atomic>
#include "Event.h"


class CSleepServerTimer final : public IServerTimer
{
public:
	struct ServerTimerItem
	{
		unsigned int iTimerID{};
		unsigned int iElapse{};    // 定时器间隔（单位毫秒）
		long long iStartTime{};    // 起始时间（单位毫秒）
		bool bShootOnce{};

		ServerTimerItem() = default;
		void clear()
		{
			iTimerID = 0;
			iElapse = 0;
			iStartTime = 0;
			bShootOnce = true;
		}
	};
	typedef ServerTimerItem* ServerTimerItemPtr;
	typedef std::unordered_map<unsigned int, ServerTimerItemPtr> ServerTimerItemPtrMap;
	typedef std::vector<ServerTimerItemPtr> ServerTimerItemPtrArray;

public:
	CSleepServerTimer();
	~CSleepServerTimer() final;

public:
	ServerTimerType GetType() const final
	{
		return ServerTimerType_Sleep;
	}

	void RegisterListener(IServerTimerListener* pListener) final;

	void Start() final;

	void Stop() final;

	void SetTimer(unsigned int iTimerID, unsigned int iElapse, bool bShootOnce) final;

	void KillTimer(unsigned int iTimerID) final;

	void KillAllTimer() final;

protected:
	void startThread();
	void stopThread();

protected:
	// 是否存在这个定时器
	bool isExistTimer(unsigned int iTimerID) const;

	// 销毁定时器池的项
	void destroyTimerItemPool();

protected:
	void onThread();

private:
	std::thread* _thread;
	mutable std::mutex _mutex;
	IServerTimerListener* _listener;
	std::atomic<bool> _running;
	CEvent _evThreadWait;
	CEvent _evThreadStarted;

	ServerTimerItemPtrMap _items;
	ServerTimerItemPtrArray _itemPool;

private:
	static time_t s_iResolution;        ///< 延时的精度，单位：毫秒
};
