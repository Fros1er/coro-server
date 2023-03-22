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

class EpollContext;

class EpollAwaitable {
    int fd_;
    EpollWaitType type_;
    EpollContext *epoll_ctx_;

public:
    explicit EpollAwaitable(int fd, EpollWaitType type, EpollContext *epoll_ctx)
            : fd_{fd}, type_{type}, epoll_ctx_{epoll_ctx} {}

    bool await_ready() { return false; } // NOLINT

    bool await_suspend(std::coroutine_handle<TaskPromise<void>> handle);

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

    friend class EpollAwaitable;

public:


    explicit EpollContext(Scheduler &sched);

    ~EpollContext();

    void register_fd(int fd, epoll_event event) const;

    EpollAwaitable async_wait(int fd, EpollWaitType type);
};