# CMake 最低版本号要求
cmake_minimum_required(VERSION 3.0)

# 子目录下需要构建另一个工程
project(test)

# 查找当前目录下的所有源文件
# 并将名称保存到 DIR_SRCS 变量中
aux_source_directory(. SRC_FILE)

# 包含include目录，里面都是头文件
include_directories(../include)

# 构建生成单元测试可执行文件
add_executable(
        ${PROJECT_NAME}
        ${SRC_FILE}
)

# 指定目标链接的库
target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
        server_timer
        uv
        dl
        rt
        pthread
)
