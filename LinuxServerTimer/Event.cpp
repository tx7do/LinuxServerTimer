#include "Event.h"
#include <time.h>
#include <sys/time.h>
#include <exception>
#include <stdexcept>

CEvent::CEvent(EventType type)
	: _auto(type == EVENT_AUTORESET)
	, _state(false)
{
	if (::pthread_mutex_init(&_mutex, NULL))
	{
		throw std::runtime_error("cannot create event (mutex)");
	}

	if (::pthread_cond_init(&_cond, NULL))
	{
		::pthread_mutex_destroy(&_mutex);
		throw std::runtime_error("cannot create event (condition)");
	}
}


CEvent::~CEvent()
{
	::pthread_cond_destroy(&_cond);
	::pthread_mutex_destroy(&_mutex);
}

void CEvent::set()
{
	if (::pthread_mutex_lock(&_mutex))
		throw std::runtime_error("cannot signal event (lock)");
	_state = true;
	if (::pthread_cond_broadcast(&_cond))
	{
		::pthread_mutex_unlock(&_mutex);
		throw std::runtime_error("cannot signal event");
	}
	::pthread_mutex_unlock(&_mutex);
}

void CEvent::wait()
{
	if (::pthread_mutex_lock(&_mutex))
		throw std::runtime_error("wait for event failed (lock)");
	while (!_state)
	{
		if (pthread_cond_wait(&_cond, &_mutex))
		{
			::pthread_mutex_unlock(&_mutex);
			throw std::runtime_error("wait for event failed");
		}
	}
	if (_auto)
		_state = false;
	::pthread_mutex_unlock(&_mutex);
}


void CEvent::wait(long milliseconds)
{
	if (!tryWait(milliseconds))
		throw std::runtime_error("event wait time out");
}

bool CEvent::tryWait(long milliseconds)
{
	int rc = 0;
	struct timespec abstime;

	::clock_gettime(CLOCK_MONOTONIC, &abstime);
	abstime.tv_sec += milliseconds / 1000;
	abstime.tv_nsec += (milliseconds % 1000) * 1000000;
	if (abstime.tv_nsec >= 1000000000)
	{
		abstime.tv_nsec -= 1000000000;
		abstime.tv_sec++;
	}

	if (::pthread_mutex_lock(&_mutex) != 0)
		throw std::runtime_error("wait for event failed (lock)");
	while (!_state)
	{
		if ((rc = ::pthread_cond_timedwait(&_cond, &_mutex, &abstime)))
		{
			if (rc == ETIMEDOUT) break;
			::pthread_mutex_unlock(&_mutex);
			throw std::runtime_error("cannot wait for event");
		}
	}
	if (rc == 0 && _auto) _state = false;
	::pthread_mutex_unlock(&_mutex);
	return rc == 0;
}


void CEvent::reset()
{
	if (::pthread_mutex_lock(&_mutex))
		throw std::runtime_error("cannot reset event");
	_state = false;
	::pthread_mutex_unlock(&_mutex);
}
