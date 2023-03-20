#include "co_mutex.hpp"
#include "co_scheduler.hpp"
#include "io_context.hpp"

bool MutexAwaitable::await_ready() {
    return block_;
}

void MutexAwaitable::await_suspend(std::coroutine_handle<TaskPromise<void>> handle) {
    auto &promise = handle.promise();
    promise.runner_->await_once();
    promise.ctx_->register_mutex(promise.id, mutex_id_);
}

MutexAwaitable::MutexAwaitable(bool block, uint64_t mutex_id) : block_{block}, mutex_id_{mutex_id} {}

MutexAwaitable CoMutex::lock() {
    if (locked_.fetch_add(1) == 0) {
        return {true, id};
    }
    return {false, id};
}

void CoMutex::unlock() {
    if (locked_.fetch_sub(1) != 1) {
        ctx_.notify_mutex(id);
    }
}

uint64_t CoMutex::id_inc = 0;

CoMutex::CoMutex(IOContext &ctx) : id{id_inc++}, ctx_{ctx} {}

MutexHandler::MutexHandler(Scheduler &sched) : sched_{sched} {}

void MutexHandler::register_mutex(const uint64_t &id, const uint64_t &mutex_id) {
    std::lock_guard guard(handler_mutex);
    map_[mutex_id].push(id);
}

void MutexHandler::notify_mutex(const uint64_t &mutex_id) {
    uint64_t id;
    {
        std::lock_guard guard(handler_mutex);
        id = map_[mutex_id].front();
        map_[mutex_id].pop();
    }
    sched_.await_once_ready(id);
}
