#include <gtest/gtest.h>
#include <string>
#include "IServerTimer.h"

TEST(LibuvTimerTest, BasicAssertions)
{
	auto pITimer = CreateServerTimer(ServerTimerType_Libuv);
	EXPECT_EQ(pITimer->GetType(), ServerTimerType_Libuv);
}
