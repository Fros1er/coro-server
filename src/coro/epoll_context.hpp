#pragma once

#include <unordered_map>
#include <sys/epoll.h>
#include "co_scheduler.hpp"
#include <atomic>
#include "unidef.hpp"

class EpollContext : Uncopyable {

    int epoll_fd_;
    bool interrupted_{false};
    std::thread thread_;
    std::unordered_map<int, epoll_event> events_;
    std::unordered_map<int, epoll_handler> handlers_;
    std::atomic<epoll_handler> next_handler_{1};
    Scheduler scheduler_;

    void run();

public:
    EpollContext();

    ~EpollContext();

    epoll_handler register_runner();

    void register_fd(epoll_handler handler, int fd, const epoll_event &event);
};