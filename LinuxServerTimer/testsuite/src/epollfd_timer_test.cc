#include <gtest/gtest.h>
#include <string>
#include "IServerTimer.h"

TEST(EpollFdTimerTest, BasicAssertions)
{
	auto pITimer = CreateServerTimer(ServerTimerType_Epollfd);
	EXPECT_EQ(pITimer->GetType(), ServerTimerType_Epollfd);
}
