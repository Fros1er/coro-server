#include <coroutine>
#include <iostream>
#include <stdexcept>
#include <thread>

#include "coro/awaitable.hpp"
#include "coro/co_scheduler.hpp"
#include "coro/epoll_context.hpp"
#include "coro/io_context.hpp"

using namespace std::chrono_literals;

Task<void> test1(int x, CoMutex &mtx) {
    std::cout << "a task " << x << std::endl;
    co_await mtx.lock();
    co_await sleep_for(5s);
    std::cout << "b task " << x << std::endl;
    mtx.unlock();
    co_return;
}

Task<void> test2(int x, CoMutex &mtx) {
    std::cout << "a task " << x << std::endl;
    co_await sleep_for(300ms);
    co_await mtx.lock();
    std::cout << "b task " << x << std::endl;
    mtx.unlock();
    co_return;
}

int main(int argc, char *argv[]) {
    IOContext ctx;
    CoMutex mtx{ctx};
    ctx.spawn(test1(1, mtx));
    ctx.spawn(test2(2, mtx));

//    for (int i = 0; i < 5; i++) {
//        ctx.spawn(test(i));
//        std::this_thread::sleep_for(0.2s);
//    }

    sleep(10000);
    return 0;
}
