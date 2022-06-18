#include <gtest/gtest.h>
#include <string>
#include "IServerTimer.h"

TEST(LibeventTimerTest, BasicAssertions)
{
	auto pITimer = CreateServerTimer(ServerTimerType_Libevent);
	EXPECT_EQ(pITimer->GetType(), ServerTimerType_Libevent);
}
