#pragma once

#include <coroutine>
#include <algorithm>

class IOContext;
//class Scheduler;
class Runner;

template<typename T>
struct TaskPromise;

template<typename T>
struct Task {
    using promise_type = TaskPromise<T>;
    using handle_type = std::coroutine_handle<TaskPromise<T>>;

    handle_type handle;

    explicit Task(const handle_type &handle) : handle(handle) {}
};

template<typename T>
struct TaskPromise {
    T return_value_;

    auto yield_value(T &&v) {
        return_value_ = std::move(v);
        return std::suspend_always{};
    }

    void return_value(T &&v) {
        return_value_ = v;
    }

    Task<T> get_return_object() { return Task{Task<T>::handle_type::from_promise(*this)}; }

    std::suspend_always initial_suspend() noexcept { return {}; }

    std::suspend_always final_suspend() noexcept { return {}; }

    void unhandled_exception() {}
};

template<>
struct TaskPromise<void> {

    IOContext *ctx_;
    Runner *runner_;
//    Scheduler *sched_;
    uint64_t id;

    Task<void> get_return_object() { return Task{Task<void>::handle_type::from_promise(*this)}; }

    std::suspend_always initial_suspend() noexcept { return {}; }

    std::suspend_always final_suspend() noexcept {
        if (ctx_ != nullptr) {
            // TODO: unregister epoll
        }
        return {};
    }

    void return_void() {}

    void unhandled_exception() {}
};

struct Awaitable {
    bool await_ready() { return false; }

    void await_suspend(std::coroutine_handle<TaskPromise<void>> handle) {
    }

    void await_resume() {}
};
