#include <gtest/gtest.h>
#include <string>
#include <memory>
#include <mutex>
#include <chrono>
#include <atomic>
#include <thread>
#include "Condition.h"

namespace
{
	class WaitRunnable
	{
	public:
		WaitRunnable(Condition& cond, std::mutex& mutex)
			:
			_ran(false),
			_cond(cond),
			_mutex(mutex)
		{
		}

		void run()
		{
			_mutex.lock();
			_cond.wait(_mutex);
			_mutex.unlock();
			_ran = true;
		}

		bool ran() const
		{
			return _ran;
		}

	private:
		bool _ran;
		Condition& _cond;
		std::mutex& _mutex;
	};

	class TryWaitRunnable
	{
	public:
		TryWaitRunnable(Condition& cond, std::mutex& mutex)
			:
			_ran(false),
			_cond(cond),
			_mutex(mutex)
		{
		}

		void run()
		{
			_mutex.lock();
			if (_cond.tryWait(_mutex, 10000))
			{
				_ran = true;
			}
			_mutex.unlock();
		}

		bool ran() const
		{
			return _ran;
		}

	private:
		bool _ran;
		Condition& _cond;
		std::mutex& _mutex;
	};

}

TEST(ConditionSignalTest, BasicAssertions)
{
	Condition cond;
	std::mutex mtx;
	WaitRunnable r1(cond, mtx);
	WaitRunnable r2(cond, mtx);

	std::thread t1([&r1]()
	{
		r1.run();
	});
	std::this_thread::sleep_for(std::chrono::microseconds{ 200 });
	std::thread t2([&r2]()
	{
		r2.run();
	});

	EXPECT_TRUE (!r1.ran());
	EXPECT_TRUE (!r2.ran());

	cond.signal();

	t1.join();
	EXPECT_TRUE (r1.ran());

	cond.signal();

	t2.join();

	EXPECT_TRUE (r2.ran());
}

TEST(ConditionBroadcastTest, BasicAssertions)
{
	Condition cond;
	std::mutex mtx;
	WaitRunnable r1(cond, mtx);
	WaitRunnable r2(cond, mtx);
	TryWaitRunnable r3(cond, mtx);


	std::thread t1([&r1]()
	{
		r1.run();
	});
	std::this_thread::sleep_for(std::chrono::microseconds{ 200 });
	std::thread t2([&r2]()
	{
		r2.run();
	});
	std::this_thread::sleep_for(std::chrono::microseconds{ 200 });
	std::thread t3([&r3]()
	{
		r3.run();
	});

	EXPECT_TRUE (!r1.ran());
	EXPECT_TRUE (!r2.ran());
	EXPECT_TRUE (!r3.ran());

	cond.signal();
	t1.join();

	EXPECT_TRUE (r1.ran());

	cond.broadcast();

	t2.join();
	t3.join();

	EXPECT_TRUE (r2.ran());
	EXPECT_TRUE (r3.ran());
}
