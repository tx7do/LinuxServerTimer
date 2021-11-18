#pragma once

#include "IServerTimer.h"

#include <thread>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <atomic>
#include "Event.h"


class CSleepServerTimer : public IServerTimer
{
public:
	struct ServerTimerItem
	{
		unsigned int iTimerID;
		unsigned int iElapse;	// ��ʱ���������λ���룩
		long long iStartTime;	// ��ʼʱ�䣨��λ���룩
		bool bShootOnce;
		ServerTimerItem()
		{
			clear();
		}
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
	virtual ~CSleepServerTimer();

	virtual ServerTimerType GetType() const { return ServerTimerType_Sleep; }

	virtual void RegisterListener(IServerTimerListener* pListener);

	virtual void Start();

	virtual void Stop();

	virtual void SetTimer(unsigned int iTimerID, unsigned int iElapse, bool bShootOnce = true);

	virtual void KillTimer(unsigned int iTimerID);

	virtual void KillAllTimer();

protected:
	void startThread();
	void stopThread();

protected:
	// �Ƿ���������ʱ��
	bool isExistTimer(unsigned int iTimerID) const;

	// ���ٶ�ʱ���ص���
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
	static unsigned int				s_iResolution;		///< ��ʱ�ľ��ȣ���λ������
};
