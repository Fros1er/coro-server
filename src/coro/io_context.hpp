#pragma once
#include "co_scheduler.hpp"
#include "epoll_context.hpp"
#include "timer_context.hpp"
#include "co_mutex.hpp"

class IOContext {
    Scheduler sched_;
//    EpollContext epoll_ctx_;
    TimerContext timer_ctx_{sched_};
    MutexHandler mutex_handler_{sched_};

    uint64_t ctx_id_{0};

public:
    void spawn(Task<void> &&task);

    void register_timer(const uint64_t &id, const time_point_t &until);
    void register_mutex(const uint64_t &id, const uint64_t &mutex_id);
    void notify_mutex(const uint64_t &mutex_id);
};
