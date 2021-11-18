#pragma once


#include "IServerTimer.h"
#include <thread>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <atomic>
#include <string.h>
#include "Event.h"

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>  


class CAsioServerTimer : public IServerTimer
{
	struct ServerTimerItem
	{
		unsigned int iTimerID;
		unsigned int iElapse;
		bool bShootOnce;

		boost::asio::steady_timer* t;

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
	virtual ~CAsioServerTimer();

	virtual ServerTimerType GetType() const { return ServerTimerType_Asio; }

	virtual void RegisterListener(IServerTimerListener* pListener);

	virtual void Start();

	virtual void Stop();

	virtual void SetTimer(unsigned int iTimerID, unsigned int iElapse, bool bShootOnce = true);

	virtual void KillTimer(unsigned int iTimerID);

	virtual void KillAllTimer();

protected:
	// ������ʱ�������߳�
	void startThread();
	// ֹͣ��ʱ�������߳�
	void stopThread();

protected:
	// �Ƿ���������ʱ��
	bool isExistTimer(unsigned int iTimerID) const;

	// ���ٶ�ʱ���ص���
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
