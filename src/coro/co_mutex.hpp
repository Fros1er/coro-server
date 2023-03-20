#pragma once

#include "awaitable.hpp"
#include "co_scheduler.hpp"
#include <atomic>
#include <queue>

class IOContext;

class MutexAwaitable {
    bool block_;
    uint64_t mutex_id_;
public:

    MutexAwaitable(bool block, uint64_t mutex_id);

    bool await_ready(); // NOLINT(modernize-use-nodiscard)

    void await_suspend(std::coroutine_handle<TaskPromise<void>> handle);

    void await_resume() {}
};

class CoMutex : Uncopyable {
    static uint64_t id_inc;

    uint64_t id;
    std::atomic_int locked_{0};

    IOContext &ctx_;

public:
    explicit CoMutex(IOContext &ctx);

    MutexAwaitable lock();

    void unlock();
};

class MutexHandler {
    std::mutex handler_mutex;
    std::unordered_map<uint64_t, std::queue<uint64_t>> map_;
    Scheduler &sched_;
public:
    explicit MutexHandler(Scheduler &sched);

    void register_mutex(const uint64_t &id, const uint64_t &mutex_id);

    void notify_mutex(const uint64_t &mutex_id);
};
