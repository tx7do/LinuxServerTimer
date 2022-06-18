#pragma once

#include <thread>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <atomic>

#include "IServerTimer.h"
#include "Event.h"
#include "ObjectPool.h"


class CSleepServerTimer final : public IServerTimer
{
public:
	struct ServerTimerItem
	{
		timerid_t iTimerID{};
		elapse_t iElapse{};    // 定时器间隔（单位毫秒）
		time_t iStartTime{};    // 起始时间（单位毫秒）
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
	using ServerTimerItemPtr = std::shared_ptr<ServerTimerItem>;
	using ServerTimerItemPtrMap = std::unordered_map<timerid_t, ServerTimerItemPtr>;
	using ServerTimerItemPtrArray = std::vector<ServerTimerItemPtr>;
	using ServerTimerItemPool = ObjectPool<ServerTimerItem, std::shared_ptr<ServerTimerItem>>;

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

	void SetTimer(timerid_t iTimerID, elapse_t iElapse, bool bShootOnce) final;

	void KillTimer(timerid_t iTimerID) final;

	void KillAllTimer() final;

protected:
	void startThread();
	void stopThread();

protected:
	// 是否存在这个定时器
	bool isExistTimer(timerid_t iTimerID) const;

protected:
	void onThread();

private:
	std::thread* _thread;
	mutable std::mutex _mutex;
	IServerTimerListener* _listener;
	std::atomic<bool> _running;
	CEvent _evThreadWait;
	CEvent _evThreadStarted;

	ServerTimerItemPool _pool;
	ServerTimerItemPtrMap _items;

private:
	static time_t s_iResolution;        ///< 延时的精度，单位：毫秒
};
