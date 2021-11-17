#pragma once

#include <pthread.h>
#include <errno.h>


class CEvent
{
public:
	enum EventType
	{
		EVENT_MANUALRESET,
		EVENT_AUTORESET
	};

	explicit CEvent(EventType type = EVENT_AUTORESET);

	~CEvent();

	void set();

	void wait();

	void wait(long milliseconds);

	bool tryWait(long milliseconds);

	void reset();

private:
	CEvent(const CEvent&);
	CEvent& operator = (const CEvent&);

private:
	bool            _auto;
	volatile bool   _state;
	pthread_mutex_t _mutex;
	pthread_cond_t  _cond;
};

