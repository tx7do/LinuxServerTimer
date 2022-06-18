
#include "IServerTimer.h"

#include "LibeventServerTimer.h"
#include "EpollfdServerTimer.h"
#include "SleepServerTimer.h"
#include "AsioServerTimer.h"
#include "LibuvServerTimer.h"

IServerTimer* CreateServerTimer(ServerTimerType type)
{
	switch (type)
	{
	case ServerTimerType_Libevent:
		return new CLibeventServerTimer();
		break;
	case ServerTimerType_Epollfd:
		return new CEpollfdServerTimer();
		break;
	case ServerTimerType_Sleep:
		return new CSleepServerTimer();
		break;
	case ServerTimerType_Asio:
		return new CAsioServerTimer();
		break;
	case ServerTimerType_Libuv:
		return new CLibuvServerTimer();
		break;
	}
	return nullptr;
}

void DestroyServerTimer(IServerTimer*& pIServerTimer)
{
	if (pIServerTimer)
	{
		delete pIServerTimer;
		pIServerTimer = nullptr;
	}
}
