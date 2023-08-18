cmake_minimum_required(VERSION 3.22)
project(RedisSimilairty)

# 设置C库的名称和源文件列表
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g -fPIC -lc -lm -std=gnu99  ")

set(LIB_NAME mros)
file(GLOB_RECURSE LIB_SOURCES RELATIVE ${CMAKE_SOURCE_DIR} "mros/*.c" "murmur2/*.c")

# 编译C库
add_library(${LIB_NAME} STATIC ${LIB_SOURCES})

# 添加头文件搜索路径，如果有的话
include_directories(${CMAKE_SOURCE_DIR}/mros)
include_directories(${CMAKE_SOURCE_DIR}/murmur2)
include_directories(${CMAKE_SOURCE_DIR}/redis-sdk)


# 设置主程序的源文件
set(REDIS_MAIN_SOURCES remros.c)

# 编译动态链接库.so
add_library(redis-similarity SHARED ${REDIS_MAIN_SOURCES})
target_link_libraries(redis-similarity PRIVATE ${LIB_NAME})