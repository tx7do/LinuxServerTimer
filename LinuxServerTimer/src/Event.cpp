#include "Event.h"

CEvent::CEvent(EventType type)
	: _auto(type == EVENT_AUTORESET), _ready(false)
{
}

void CEvent::set()
{
	std::unique_lock<std::mutex> lock(_mutex);
	_ready = true;
	_cond.notify_all();
}

void CEvent::wait()
{
	std::unique_lock<std::mutex> lock(_mutex);
	_cond.wait(lock, [this]() {return this->_ready; });
	if (_auto) _ready = false;
}

void CEvent::wait(long milliseconds)
{
	if (!tryWait(milliseconds))
	{
		throw std::runtime_error("wait for event failed");
	}
}

bool CEvent::tryWait(long milliseconds)
{
	auto rc = std::cv_status::no_timeout;
	std::unique_lock<std::mutex> lock(_mutex);

	auto timeout_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds);
	while (!_ready)
	{
		if ((rc = _cond.wait_until(lock, timeout_time)) == std::cv_status::timeout)
		{
			break;
		}
	}

	if (rc == std::cv_status::no_timeout && _auto) _ready = false;
	return rc == std::cv_status::no_timeout;
}

void CEvent::reset()
{
	std::unique_lock<std::mutex> lock(_mutex);
	_ready = false;
}
