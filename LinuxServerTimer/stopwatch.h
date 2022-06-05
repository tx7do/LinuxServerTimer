#pragma once

#include <chrono>
#include <iostream>

template<typename Clock = std::chrono::steady_clock>
class StopWatch
{
	typename Clock::time_point last_;

public:
	StopWatch()
		: last_(Clock::now())
	{
	}

	void reset()
	{
		*this = StopWatch();
	}

	typename Clock::duration elapsed() const
	{
		return Clock::now() - last_;
	}

	typename Clock::duration tick()
	{
		auto now = Clock::now();
		auto elapsed = now - last_;
		last_ = now;
		return elapsed;
	}
};

template<typename T, typename Rep, typename Period>
T duration_cast(const std::chrono::duration<Rep, Period>& duration)
{
	return duration.count() * static_cast<T>(Period::num) / static_cast<T>(Period::den);
}
