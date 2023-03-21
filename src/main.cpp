#include <stdexcept>
#include <thread>

#include "coro/awaitable.hpp"
#include "coro/epoll_context.hpp"
#include "coro/io_context.hpp"
#include "server/tcp_acceptor.hpp"

using namespace std::chrono_literals;

class Session {
private:
    Socket socket_;
    IOContext &ctx_;
    int fd_;

    Task<void> reader() {
        constexpr size_t BUF_SIZE = 2048;
        std::vector<char> buf(BUF_SIZE);
        while (true) {
            // async_read_some
            co_await ctx_.async_wait(fd_, READ);
            fmt::print("reader awake\n");
            int n;
            POSIX_CALL_RETRY(n, recv(fd_, buf.data(), BUF_SIZE, 0));
            while (n == -1) {
                if (errno != EAGAIN) {
                    std::perror("error on read: ");
                    co_return;
                }
                co_await ctx_.async_wait(fd_, READ);
                POSIX_CALL_RETRY(n, recv(fd_, buf.data(), BUF_SIZE, 0));
            }

            // async_write_some
            size_t pos = 0;
            do {
                ssize_t ret;
                POSIX_CALL_RETRY(ret, send(fd_, buf.data(), n, 0));
                if (-1 == ret) {
                    if (errno != EAGAIN) {
                        std::perror("error on write: ");
                        co_return;
                    }
                    co_await ctx_.async_wait(fd_, WRITE);
                    continue;
                }
                pos += ret;
            } while (pos < n);
        }
    }

public:
    explicit Session(Socket socket) : socket_(socket), ctx_{socket_.ctx()}, fd_(socket.fd()) {}


    void start() {
        ctx_.spawn(reader());
    }

    void stop() {

    }
};

int main(int argc, char *argv[]) {
    IOContext ctx;
    TCPAcceptor<Session> acceptor{ctx, "127.0.0.1", 6667};

    sleep(10000);
    return 0;
}
