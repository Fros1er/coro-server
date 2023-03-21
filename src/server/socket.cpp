#include "socket.hpp"
//#include <sys/socket.h>
//#include "utils/posix_call.hpp"
//#include "coro/io_context.hpp"
#include "coro/awaitable.hpp"


//T Socket::async_read_some(void *buf, size_t buf_len) {
//    ssize_t n;
//    co_await ctx_.async_wait(fd_);
//    POSIX_CALL_RETRY(n, recv(fd_, buf, buf_len, 0));
//    while (n == -1) {
//        if (errno != EAGAIN) {
//            break;
//        }
//        co_await ctx_.async_wait(fd_);
//        POSIX_CALL_RETRY(n, recv(fd_, buf, buf_len, 0));
//    }
//    return n;
//}
//
Socket::Socket(int fd, IOContext &ctx) : fd_{fd}, ctx_{ctx} {}
