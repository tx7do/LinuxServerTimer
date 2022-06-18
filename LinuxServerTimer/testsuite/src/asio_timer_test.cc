#include <gtest/gtest.h>
#include <string>
#include "IServerTimer.h"

TEST(AsioTimerTest, BasicAssertions)
{
	auto pITimer = CreateServerTimer(ServerTimerType_Asio);
	EXPECT_EQ(pITimer->GetType(), ServerTimerType_Asio);
}
