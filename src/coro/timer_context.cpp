#include "timer_context.hpp"
#include "io_context.hpp"

TimerAwaitable::TimerAwaitable(time_point_t until_)
        : until_{until_} {}

bool TimerAwaitable::await_ready() {
    return until_ <= std::chrono::steady_clock::now();
}

void TimerAwaitable::await_suspend(std::coroutine_handle<TaskPromise<void>> handle) {
    auto &promise = handle.promise();
    promise.runner_->await_once();
    promise.ctx_->register_timer(promise.id, until_);
}

bool TimerContext::TimerDesc::operator<(const TimerContext::TimerDesc &rhs) const {
    return until > rhs.until;
}

void TimerContext::run() {
    while (!interrupted_) {
        std::unique_lock lk(timer_mutex_);
        bool empty = queue_.empty();
        if (empty) {
            timer_cv_.wait(lk);
        }
        while (!queue_.empty() && queue_.top().until <= std::chrono::steady_clock::now()) {
            sched_.await_once_ready(queue_.top().id);
            queue_.pop();
        }
        if (queue_.empty())
            continue;
        auto &until = queue_.top().until;
        lk.unlock();
        std::this_thread::sleep_until(until);
    }
}

void TimerContext::addTimer(uint64_t id, const time_point_t &until) {
    {
        std::unique_lock lk(timer_mutex_);
        queue_.push({id, until});
    }
    timer_cv_.notify_one();
}

TimerContext::TimerContext(Scheduler &sched) : sched_{sched}, thread_{&TimerContext::run, this} {}


TimerAwaitable sleep_until(const time_point_t &until) {
    return TimerAwaitable{until};
}

TimerAwaitable sleep_for(const std::chrono::nanoseconds &time) {
    return sleep_until(std::chrono::steady_clock::now() + time);
}
