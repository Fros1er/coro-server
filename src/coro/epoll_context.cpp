#include <sys/epoll.h>
#include <iostream>
#include <fmt/core.h>
#include "epoll_context.hpp"
#include "io_context.hpp"
#include "utils/posix_call.hpp"

constexpr int MAX_RECEIVED_EVENTS = 32;

void EpollContext::register_fd(int fd, epoll_event event) const {
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event);
}

void EpollContext::run() {
    int n;
    std::vector<epoll_event> events(MAX_RECEIVED_EVENTS);
    while (!interrupted_) {
        POSIX_CALL_RETRY(n, epoll_wait(epoll_fd_, events.data(), MAX_RECEIVED_EVENTS, -1));
        fmt::print("epoll_awake\n");
        if (n == -1) {
            perror("epoll listener fatal: ");
            break;
        }
        for (int i = 0; i < n; i++) {
            auto &event = events[i];
            int fd = event.data.fd;
            if (event.events & EPOLLIN) {
                std::lock_guard guard(reader_lock_);
                if (reader_task_map_.contains(fd)) {
                    sched_.await_once_ready(reader_task_map_[fd]);
                    continue;
                }
                reader_map_[fd] = true;
            } else if (event.events & EPOLLOUT) {
                std::lock_guard guard(writer_lock_);
                if (writer_map_.contains(fd)) {
                    sched_.await_once_ready(reader_task_map_[fd]);
                    continue;
                }
                writer_map_[fd] = true;
            }
        }
    }
}

EpollContext::EpollContext(Scheduler &sched)
        : sched_{sched}, epoll_fd_{epoll_create(16)}, thread_(&EpollContext::run, this) {}

EpollContext::~EpollContext() {
    interrupted_ = true;
    thread_.join();
}

EpollAwaitable EpollContext::async_wait(int fd, EpollWaitType type) {
    switch (type) {
        case READ: {
            std::lock_guard guard(reader_lock_);
            if (reader_map_[fd]) {
                return EpollAwaitable{true, fd, type};
            }
            break;
        }
        case WRITE: {
            std::lock_guard guard(writer_lock_);
            if (writer_map_[fd]) {
                return EpollAwaitable{true, fd, type};
            }
            break;
        }
    }
    return EpollAwaitable(false, fd, type);
}

void EpollContext::register_epoll_event(const uint64_t &id, int fd, EpollWaitType type) {
    switch (type) {
        case READ: {
            std::lock_guard guard(reader_lock_);
            reader_task_map_[fd] = id;
            break;
        }
        case WRITE: {
            std::lock_guard guard(writer_lock_);
            writer_task_map_[fd] = id;
            break;
        }
    }
}

void EpollAwaitable::await_suspend(std::coroutine_handle<TaskPromise<void>> handle) {
    auto &promise = handle.promise();
    promise.runner_->await_once();
    promise.ctx_->register_epoll_event(promise.id, fd_, type_);
}


