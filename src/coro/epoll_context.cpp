#include <sys/epoll.h>
#include <iostream>
#include "epoll_context.hpp"
#include "io_context.hpp"
#include "utils/posix_call.hpp"
#include "spdlog/spdlog.h"

constexpr int MAX_RECEIVED_EVENTS = 32;

void EpollContext::register_fd(int fd, epoll_event event) const {
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event);
}

void EpollContext::run() {
    int n;
    std::vector<epoll_event> events(MAX_RECEIVED_EVENTS);
    while (!interrupted_) {
        POSIX_CALL_RETRY(n, epoll_wait(epoll_fd_, events.data(), MAX_RECEIVED_EVENTS, -1));
        if (n == -1) {
            SPDLOG_CRITICAL("epoll listener fatal: {}", strerror(errno));
            break;
        }
        for (int i = 0; i < n; i++) {
            auto &event = events[i];
            int fd = event.data.fd;
            if (event.events & EPOLLIN) {
                std::lock_guard guard(reader_lock_);
                if (reader_task_map_.contains(fd)) {
//                    spdlog::debug("epoll_awake read");
                    sched_.await_once_ready(reader_task_map_[fd]);
                    reader_task_map_.erase(fd);

                } else if (reader_map_.contains(fd)) {
//                    spdlog::debug("epoll_awake read set map");
                    reader_map_[fd] = true;
                }
            } else if (event.events & EPOLLOUT) {
                std::lock_guard guard(writer_lock_);
                spdlog::debug("epoll_awake write");
                if (writer_task_map_.contains(fd)) {
                    sched_.await_once_ready(writer_task_map_[fd]);
                    writer_task_map_.erase(fd);
                } else if (writer_map_.contains(fd)) {
                    spdlog::debug("epoll_awake write set map");
                    writer_map_[fd] = true;
                }
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
            reader_map_[fd] = false;
            break;
        }
        case WRITE: {
            std::lock_guard guard(writer_lock_);
            writer_map_[fd] = false;
            break;
        }
    }
    return EpollAwaitable(fd, type, this);
}

bool EpollAwaitable::await_suspend(std::coroutine_handle<TaskPromise<void>> handle) {
    auto &promise = handle.promise();
    std::mutex *lock;
    std::unordered_map<int, bool> *tmp_map;
    std::unordered_map<int, uint64_t> *task_map;
    switch (type_) {
        case READ: {
            lock = &epoll_ctx_->reader_lock_;
            tmp_map = &epoll_ctx_->reader_map_;
            task_map = &epoll_ctx_->reader_task_map_;
            break;
        }
        case WRITE: {
            lock = &epoll_ctx_->writer_lock_;
            tmp_map = &epoll_ctx_->writer_map_;
            task_map = &epoll_ctx_->writer_task_map_;
            break;
        }
    }
    std::lock_guard guard(*lock);
    auto it = tmp_map->find(fd_);
    bool unblock = it->second;
    tmp_map->erase(it);
    if (unblock) {
        spdlog::debug("unblock");
        return false;
    }
    promise.runner_->await_once();
    (*task_map)[fd_] = promise.id;
    return true;
}


