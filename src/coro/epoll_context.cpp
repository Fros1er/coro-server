#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <thread>
#include <iostream>
#include "epoll_context.hpp"

#define POSIX_CALL_RETRY(res, call) \
    while((res = call) == EINTR) {}

constexpr int MAX_RECEIVED_EVENTS = 32;

void test() {
    int listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in addr{AF_INET, htons(9989), inet_addr("127.0.0.1")};
    bind(listen_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
    listen(listen_fd, 5);
    int epoll_fd = epoll_create(5);

    std::unordered_map<int, epoll_event> events;

    std::thread{[epoll_fd]() {
        char *buf = new char[2048];
        while (true) {
            epoll_event events[10];
            int n = epoll_wait(epoll_fd, events, 10, -1);
            if (n == -1) {
                perror("epoll: ");
                continue;
            }
            for (int i = 0; i < n; i++) {
                if (events[i].events & EPOLLIN) {
                    recv(events[i].data.fd, buf, 2048, 0);
                    std::cout << buf << std::endl;
                }
            }
        }
    }}.detach();

    while (true) {
        sockaddr_in in_addr{0};
        socklen_t in_size{0};
        int fd = accept(listen_fd, reinterpret_cast<sockaddr *>(&in_addr), &in_size);
        events[fd] = {EPOLLIN | EPOLLET | EPOLLRDHUP, {.fd=fd}};
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &events[fd]);
    }
}

epoll_handler EpollContext::register_runner() {
    return next_handler_.fetch_add(1); // TODO: memory order?
}

void EpollContext::register_fd(epoll_handler handler, int fd, const epoll_event &event) {
    handlers_[fd] = handler;
    events_[fd] = event;
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &events_[fd]);
}

void EpollContext::run() {
    int n;
    std::vector<epoll_event> received_events(MAX_RECEIVED_EVENTS);
    while (!interrupted_) {
        POSIX_CALL_RETRY(n, epoll_wait(epoll_fd_, received_events.data(), MAX_RECEIVED_EVENTS, -1));
        for (int i = 0; i < n; i++) {
            auto &event = received_events[i];
            if (events_[event.data.fd].events & event.events) {
//                scheduler_.awaitable_ready(handlers_[event.data.fd]);
            }
        }
    }
}

EpollContext::EpollContext()
        : epoll_fd_{epoll_create(16)}, thread_(&EpollContext::run, this) {}

EpollContext::~EpollContext() {
    interrupted_ = true;
    thread_.join();
}
