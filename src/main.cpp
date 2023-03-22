#include <stdexcept>

#include "coro/awaitable.hpp"
#include "coro/epoll_context.hpp"
#include "coro/io_context.hpp"
#include "server/tcp_acceptor.hpp"
#include "spdlog/spdlog.h"

using namespace std::chrono_literals;

class Session {
private:
    Socket socket_;
    IOContext &ctx_;
    int fd_;

    Task<void> reader() {
        constexpr size_t BUF_SIZE = 2048;
        std::vector<char> buf(BUF_SIZE);
        int n;
        while (true) {
            // async_read_some
//            co_await ctx_.async_wait(fd_, READ);
            spdlog::debug("reader awake");
            POSIX_CALL_RETRY(n, recv(fd_, buf.data(), BUF_SIZE, 0));
            while (n == -1) {
                if (errno != EAGAIN) {
                    SPDLOG_ERROR("error on read: {}", strerror(errno));
                    co_return;
                }
                spdlog::debug("block at read");
                co_await ctx_.async_wait(fd_, READ);
                POSIX_CALL_RETRY(n, recv(fd_, buf.data(), BUF_SIZE, 0));
            }
//            fmt::print("{}\n", n);

            // async_write_some
//            size_t pos = 0;
////            co_await ctx_.async_wait(fd_, WRITE);
//            do {
//                ssize_t ret;
//                POSIX_CALL_RETRY(ret, send(fd_, buf.data(), n, MSG_DONTWAIT));
////                fmt::print("send {} {}\n", ret, n);
//                if (-1 == ret) {
//                    if (errno != EAGAIN) {
//                        SPDLOG_ERROR("error on write: {}", strerror(errno));
//                        co_return;
//                    }
//                    spdlog::debug("block at write");
//                    co_await ctx_.async_wait(fd_, WRITE);
//                    continue;
//                }
//                pos += ret;
//            } while (pos < n);
        }
    }

public:
    explicit Session(Socket socket) : socket_(socket), ctx_{socket.ctx()}, fd_(socket.fd()) {}


    void start() {
        ctx_.spawn(reader());
    }

    void stop() {

    }
};

int main(int argc, char *argv[]) {
    spdlog::set_level(spdlog::level::err);
//    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%H:%M:%S %z] [%n] [%^---%L---%$] [thread %t] %v");

    IOContext ctx;
    TCPAcceptor<Session> acceptor{ctx, "127.0.0.1", 6668};

    ctx.run();
    return 0;
}
