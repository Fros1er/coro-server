#pragma once

#include <unordered_map>
#include <sys/epoll.h>
#include "co_scheduler.hpp"
#include <atomic>
#include "unidef.hpp"

enum EpollWaitType {
    READ,
    WRITE
};

class EpollAwaitable {
    bool block_;
    int fd_;
    EpollWaitType type_;

public:
    explicit EpollAwaitable(bool block, int fd, EpollWaitType type)
            : block_{block}, fd_{fd}, type_{type} {}

    bool await_ready() { return block_; } // NOLINT

    void await_suspend(std::coroutine_handle<TaskPromise<void>> handle);

    void await_resume() {}
};

class EpollContext : Uncopyable {

    int epoll_fd_;
    bool interrupted_{false};
    Scheduler &sched_;
    std::mutex reader_lock_, writer_lock_;
    std::unordered_map<int, bool> reader_map_, writer_map_;
    std::unordered_map<int, uint64_t> reader_task_map_, writer_task_map_;
    std::thread thread_;


    void run();

public:


    explicit EpollContext(Scheduler &sched);

    ~EpollContext();

//    epoll_handler register_runner();

    void register_fd(int fd, epoll_event event) const;

    EpollAwaitable async_wait(int fd, EpollWaitType type);

    void register_epoll_event(const uint64_t &id, int fd, EpollWaitType type);
};