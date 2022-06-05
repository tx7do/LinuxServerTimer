#pragma once

#include <condition_variable>
#include <mutex>
#include <atomic>


class CEvent final
{
public:
	enum EventType
	{
		EVENT_MANUALRESET,
		EVENT_AUTORESET
	};

	explicit CEvent(EventType type = EVENT_AUTORESET);

public:
	void set();

	void wait();

	void wait(long milliseconds);

	bool tryWait(long milliseconds);

	void reset();

private:
	CEvent(const CEvent&);
	CEvent& operator=(const CEvent&);

private:
	bool _auto;
	volatile bool _ready;

	std::mutex _mutex;
	std::condition_variable _cond;
};

