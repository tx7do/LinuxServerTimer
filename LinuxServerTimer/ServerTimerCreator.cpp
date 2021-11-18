
#include "IServerTimer.h"

#include "LibeventServerTimer.h"
#include "EpollfdServerTimer.h"
#include "SleepServerTimer.h"
#include "AsioServerTimer.h"

IServerTimer* CreateServerTimer(ServerTimerType type)
{
	switch (type)
	{
	case ServerTimerType_Libevent: return new CLibeventServerTimer();  break;
	case ServerTimerType_Epollfd: return new CEpollfdServerTimer();  break;
	case ServerTimerType_Sleep: return new CSleepServerTimer();  break;
	case ServerTimerType_Asio: return new CAsioServerTimer();  break;
	}
	return nullptr;
}

void DestoryServerTimer(IServerTimer*& pIServerTimer)
{
	if (pIServerTimer)
	{
		delete pIServerTimer;
		pIServerTimer = nullptr;
	}
}
