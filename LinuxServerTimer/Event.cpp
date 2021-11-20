#include "Event.h"
#include <time.h>
#include <sys/time.h>
#include <errno.h>

CEvent::CEvent(EventType type)
	: _auto(type == EVENT_AUTORESET)
	, _state(false)
{
}


CEvent::~CEvent()
{
}

void CEvent::set()
{
	std::unique_lock<std::mutex> lock(_mutex);
	_cond.notify_all();
}

void CEvent::wait()
{
	std::unique_lock<std::mutex> lock(_mutex);
	while (!_state)
	{
		_cond.wait(lock);
	}
	//_cond.wait(lock, [this] {return _state; });

	if (_auto) _state = false;
}


void CEvent::wait(long milliseconds)
{
	if (!tryWait(milliseconds))
		throw std::runtime_error("wait for event failed");
}

bool CEvent::tryWait(long milliseconds)
{
	auto rc = std::cv_status::no_timeout;
	std::unique_lock<std::mutex> lock(_mutex);

	auto timeout_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds);
	while (!_state)
	{
		if (_cond.wait_until(lock, timeout_time) == std::cv_status::timeout)
		{
			break;
		}
	}

	if (rc == std::cv_status::no_timeout && _auto) _state = false;
	return rc == std::cv_status::no_timeout;
}


void CEvent::reset()
{
	std::unique_lock<std::mutex> lock(_mutex);
	_state = false;
}
