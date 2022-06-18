#include <gtest/gtest.h>
#include <string>
#include <thread>
#include "ObjectPool.h"

TEST(ObjectPoolTest, BasicAssertions)
{
	using string_t = std::string;
	using string_ptr = std::shared_ptr<string_t>;

	ObjectPool<string_t, string_ptr> pool(3);

	EXPECT_TRUE (pool.capacity() == 3);
	EXPECT_TRUE (pool.size() == 0);
	EXPECT_TRUE (pool.available() == 4);

	auto pStr1 = pool.borrowObject();
	pStr1->assign("first");
	EXPECT_TRUE (pool.size() == 1);
	EXPECT_TRUE (pool.available() == 3);

	auto pStr2 = pool.borrowObject();
	pStr2->assign("second");
	EXPECT_TRUE (pool.size() == 2);
	EXPECT_TRUE (pool.available() == 2);

	auto pStr3 = pool.borrowObject();
	pStr3->assign("third");
	EXPECT_TRUE (pool.size() == 3);
	EXPECT_TRUE (pool.available() == 1);

	auto pStr4 = pool.borrowObject();
	pStr4->assign("fourth");
	EXPECT_TRUE (pool.size() == 4);
	EXPECT_TRUE (pool.available() == 0);

	auto pStr5 = pool.borrowObject();
	EXPECT_TRUE (pStr5 == nullptr);

	pool.returnObject(pStr4);
	EXPECT_TRUE (pool.size() == 4);
	EXPECT_TRUE (pool.available() == 1);

	pool.returnObject(pStr3);
	EXPECT_TRUE (pool.size() == 4);
	EXPECT_TRUE (pool.available() == 2);

	pStr3 = pool.borrowObject();
	EXPECT_TRUE (*pStr3 == "third");
	EXPECT_TRUE (pool.available() == 1);

	pool.returnObject(pStr3);
	pool.returnObject(pStr2);
	pool.returnObject(pStr1);

	EXPECT_TRUE (pool.size() == 3);
	EXPECT_TRUE (pool.available() == 4);

	pStr1 = pool.borrowObject();
	EXPECT_TRUE (*pStr1 == "second");
	EXPECT_TRUE (pool.available() == 3);

	pool.returnObject(pStr1);
	EXPECT_TRUE (pool.available() == 4);
}

TEST(ObjectPoolWaitOnBorrowObjectTest, BasicAssertions)
{
	using string_t = std::string;
	using string_ptr = std::shared_ptr<string_t>;

	ObjectPool<string_t, string_ptr> pool(1);

	auto objectToReturnDuringBorrow = pool.borrowObject();
	std::cout << "get object: " << objectToReturnDuringBorrow.use_count() << std::endl;

	std::thread threadToReturnObject(
		[&pool, &objectToReturnDuringBorrow]()
		{
			pool.returnObject(objectToReturnDuringBorrow);
		}
	);

	auto object = pool.borrowObject();

	threadToReturnObject.join();
	EXPECT_FALSE(object == nullptr);
}
