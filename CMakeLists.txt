cmake_minimum_required(VERSION 3.10)
project(gitlite)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置可执行文件输出路径为build目录
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})

# 包含头文件目录
include_directories(include)

# 收集源文件
file(GLOB SOURCES 
    "src/*.cpp"
    "main.cpp"
)

# 生成可执行文件
add_executable(gitlite ${SOURCES})

# 链接必要的库（文件系统操作可能需要）
target_link_libraries(gitlite stdc++fs)

# 确保编译时包含所有必要的定义
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG)
endif()