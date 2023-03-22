#pragma once

#include <thread>
#include <netinet/in.h>
#include <cstring>
#include <arpa/inet.h>
#include <fmt/core.h>
#include "coro/io_context.hpp"
#include "server/socket.hpp"
#include "utils/posix_call.hpp"
#include <list>
#include <fcntl.h>

template<typename Session>
class TCPAcceptor {

    bool interrupted{false};
    int listen_fd;
    IOContext &ctx_;
    std::list<Session> sessions_;
    std::thread thread_;

    void run() {
        while (!interrupted) {
            sockaddr_in in_addr{0};
            socklen_t in_size{0};
            int fd;
            POSIX_CALL_RETRY(fd, accept(listen_fd, reinterpret_cast<sockaddr *>(&in_addr), &in_size));
            int flags = fcntl(fd, F_GETFL);
            flags |= O_NONBLOCK;
            fcntl(fd, F_SETFL, flags);
            spdlog::debug("socket accepted\n");
            sessions_.emplace_back(Socket{fd, ctx_});
            ctx_.start_handle<Session>(fd, sessions_.back());
        }
    }

public:
    TCPAcceptor(IOContext &ctx, const std::string_view &addr, uint16_t port)
            : listen_fd{socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)}, ctx_{ctx} {
        sockaddr_in sock_addr{AF_INET, htons(port), inet_addr(addr.data())};
        POSIX_CALL_EXCEPTION(bind(listen_fd, reinterpret_cast<sockaddr *>(&sock_addr), sizeof(sock_addr)));
        POSIX_CALL_EXCEPTION(listen(listen_fd, 32));
        thread_ = std::thread(&TCPAcceptor::run, this);
    }

    ~TCPAcceptor() {
        interrupted = true;
        thread_.join();
    }
};
