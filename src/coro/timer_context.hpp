#pragma once

#include "awaitable.hpp"
#include <chrono>
#include <thread>
#include <condition_variable>
#include <queue>
#include "co_scheduler.hpp"

using time_point_t = std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds>;

class TimerAwaitable {

    time_point_t until_;
public:
    explicit TimerAwaitable(time_point_t until_);

    bool await_ready();

    void await_suspend(std::coroutine_handle<TaskPromise<void>> handle);

    void await_resume() {}
};

class TimerContext {

    struct TimerDesc {
        uint64_t id{};
        time_point_t until;

        bool operator<(const TimerDesc &rhs) const;
    };

    bool interrupted_{false};
    std::mutex timer_mutex_{};
    std::condition_variable timer_cv_;
    std::priority_queue<TimerDesc> queue_;
    Scheduler &sched_;
    std::thread thread_;

    friend class IOContext;

    void run();

public:

    explicit TimerContext(Scheduler &sched);

    void addTimer(uint64_t id, const time_point_t &until);

};

TimerAwaitable sleep_until(const time_point_t &until);

TimerAwaitable sleep_for(const std::chrono::nanoseconds time);


