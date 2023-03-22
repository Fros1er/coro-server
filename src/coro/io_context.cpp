#include "io_context.hpp"

void IOContext::spawn(Task<void> &&task) {
    task.handle.promise().ctx_ = this;
    task.handle.promise().id = ctx_id_++;
    sched_.spawn(std::move(task)); // NOLINT(performance-move-const-arg)
}

void IOContext::register_timer(const uint64_t &id, const time_point_t &until) {
    timer_ctx_.addTimer(id, until);
}

void IOContext::register_mutex(const uint64_t &id, const uint64_t &mutex_id) {
    mutex_handler_.register_mutex(id, mutex_id);
}

void IOContext::notify_mutex(const uint64_t &mutex_id) {
    mutex_handler_.notify_mutex(mutex_id);
}

EpollAwaitable IOContext::async_wait(int fd, EpollWaitType type) {
    return epoll_ctx_.async_wait(fd, type);
}

void IOContext::run() {
    sched_.run();
}
