#include <gtest/gtest.h>
#include <string>
#include "IServerTimer.h"

TEST(SleepTimerTest, BasicAssertions)
{
	auto pITimer = CreateServerTimer(ServerTimerType_Sleep);
	EXPECT_EQ(pITimer->GetType(), ServerTimerType_Sleep);
}
