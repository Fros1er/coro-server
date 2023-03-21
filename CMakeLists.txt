cmake_minimum_required(VERSION 3.24)
project(coro-server)

set(CMAKE_CXX_STANDARD 20)

include_directories(src)

find_package(fmt CONFIG REQUIRED)

add_executable(coro-server src/main.cpp src/coro/co_scheduler.cpp
        src/coro/epoll_context.cpp src/coro/io_context.cpp
        src/coro/timer_context.cpp src/coro/co_mutex.cpp
        src/server/socket.cpp)

target_link_libraries(coro-server pthread fmt::fmt)
