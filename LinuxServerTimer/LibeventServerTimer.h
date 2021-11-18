
#pragma once

#include "IServerTimer.h"
#include "Event.h"

#include <thread>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <atomic>

#include <event2/event.h>
#include <event2/event_struct.h>

struct event_base;
struct event;


class CLibeventServerTimer : public IServerTimer
{
	struct ServerTimerItem
	{
		CLibeventServerTimer* pParent;
		struct event* pEvent;

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
			pParent = nullptr;
			pEvent = nullptr;
			iTimerID = 0;
			iElapse = 0;
			bShootOnce = true;
		}
	};
	typedef ServerTimerItem* ServerTimerItemPtr;
	typedef std::unordered_map<unsigned int, ServerTimerItemPtr> ServerTimerItemPtrMap;
	typedef std::vector<ServerTimerItemPtr> ServerTimerItemPtrArray;

public:
	CLibeventServerTimer();
	virtual ~CLibeventServerTimer();

public:
	virtual ServerTimerType GetType() const { return ServerTimerType_Libevent; }

	virtual void RegisterListener(IServerTimerListener* pListener);

	virtual void Start();

	virtual void Stop();

public:
	virtual void SetTimer(unsigned int iTimerID, unsigned int iElapse, bool bShootOnce = true);

	virtual void KillTimer(unsigned int iTimerID);

	virtual void KillAllTimer();

protected:
	// �����¼�����
	void startEvent();
	// ֹͣ�¼�����
	void stopEvent();

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
	// �¼����лص�����
	static void onEvent(evutil_socket_t fd, short event, void* arg);

	// �̻߳ص�����
	void onThread();

	// ��ʱ����ʱ�ص�����
	void onTimeOut(unsigned int iTimerID, unsigned int iElapse, bool bShootOnce);

private:
	struct event_base* _base;

	std::thread* _thread;
	mutable std::mutex _mutex;
	CEvent _evThreadStarted;
	std::atomic<bool> _running;

	ServerTimerItemPtrMap _items;
	ServerTimerItemPtrArray _itemPool;

	IServerTimerListener* _listener;
};
