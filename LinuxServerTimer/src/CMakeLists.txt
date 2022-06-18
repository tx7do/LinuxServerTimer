cmake_minimum_required (VERSION 3.8)

# 项目名称
project(server_timer)

# 默认不导出所有符号
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN YES)

# 包含include目录，里面都是头文件
include_directories(../include)

aux_source_directory(. DIR_LIB_SRCS)
aux_source_directory(../include DIR_LIB_HEADERS)

# 构建生成动态库
add_library(
        ${PROJECT_NAME}
        SHARED
        ${DIR_LIB_HEADERS}
        ${DIR_LIB_SRCS}
)

target_include_directories(
        ${PROJECT_NAME}
        PUBLIC
        ../include
)

target_link_libraries(
        ${PROJECT_NAME}
        ${Boost_LIBRARIES}
        event
        uv
        event_pthreads
        pthread
        rt
        dl
)
