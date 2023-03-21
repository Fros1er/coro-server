#pragma once

#include "co_scheduler.hpp"
#include "epoll_context.hpp"
#include "timer_context.hpp"
#include "co_mutex.hpp"
#include "server/socket.hpp"
#include "server/socket.hpp"

class IOContext {
    Scheduler sched_;
    EpollContext epoll_ctx_{sched_};
    TimerContext timer_ctx_{sched_};
    MutexHandler mutex_handler_{sched_};

    uint64_t ctx_id_{0};

public:
    void spawn(Task<void> &&task);

    void register_timer(const uint64_t &id, const time_point_t &until);

    void register_mutex(const uint64_t &id, const uint64_t &mutex_id);

    void notify_mutex(const uint64_t &mutex_id);

    template<class Session>
    void start_handle(int fd, Session &session) {
        epoll_ctx_.register_fd(fd, epoll_event{EPOLLET | EPOLLIN, {.fd=fd}});
        epoll_ctx_.register_fd(fd, epoll_event{EPOLLET | EPOLLOUT, {.fd=fd}});

        session.start();
        // TODO: session stop
    }

    EpollAwaitable async_wait(int fd, EpollWaitType type);

    void register_epoll_event(const uint64_t &id, int fd, EpollWaitType type);
};
