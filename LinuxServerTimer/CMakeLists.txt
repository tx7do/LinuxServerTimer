# CMakeList.txt: LinuxServerTimer 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

# 将源代码添加到此项目的可执行文件。
add_executable (LinuxServerTimer "main.cpp" "stdinc.h" "LibeventServerTimer.h" "LibeventServerTimer.cpp" "IServerTimer.h" "EpollfdServerTimer.h" "EpollfdServerTimer.cpp" "ServerTimerCreator.cpp" "SleepServerTimer.h" "SleepServerTimer.cpp" "Event.h" "Event.cpp" "AsioServerTimer.h" "AsioServerTimer.cpp" stopwatch.h)

target_link_libraries(LinuxServerTimer event pthread rt)
