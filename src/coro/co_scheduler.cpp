#include <mutex>
#include <condition_variable>
#include "co_scheduler.hpp"
#include "spdlog/spdlog.h"
#include <iostream>

int Runner::id_cnt = 1;

Runner::Runner(Scheduler &sched) : id_{id_cnt++}, sched_(sched), thread_(&Runner::run, this) {}

Runner::~Runner() {
    interrupted = true;
    thread_.join();
}

void Runner::run() {
    uint64_t cnt = 0;
    while (!interrupted) {
        bool has_task = false;
        if (cnt % 61 == 0) {
            std::lock_guard<std::mutex> guard(sched_.sched_lock_);
            has_task = get_from_global_queue();
        }
        if (!has_task) {
            std::lock_guard guard(local_queue_lock_);
            has_task = !local_queue_.empty();
        }
        if (!has_task) {
            find_task();
        }


        auto &handle = local_queue_.front()->handle;
//        fmt::print("runner {} resume\n", id_);
        handle.resume();
        {
            std::lock_guard guard(local_queue_lock_);
            if (handle.done() || curr_awaiting_) {
                curr_awaiting_ = false;
                local_queue_.pop_front();
            } else {
                local_queue_.splice(local_queue_.cend(), local_queue_, local_queue_.begin());
            }
        }
    }
}

bool Runner::get_from_global_queue(bool once) {
    if (sched_.global_queue_.empty())
        return false;
    {
        std::lock_guard guard(local_queue_lock_);
        local_queue_.push_front(std::move(sched_.global_queue_.front()));
        local_queue_.front()->handle.promise().runner_ = this;
    }
    sched_.global_queue_.pop();
    if (!once) {
        size_t n = (sched_.global_queue_.size() / sched_.runners_.size()) + 1;
        n = std::min(std::min(n, 128UL), sched_.global_queue_.size());
        for (int i = 0; i < n; i++) {
            {
                std::lock_guard guard(local_queue_lock_);
                local_queue_.push_back(std::move(sched_.global_queue_.front()));
                local_queue_.back()->handle.promise().runner_ = this;
            }
            sched_.global_queue_.pop();
        }
    }
    return true;
}

void Runner::find_task() {
    while (true) {
        {
            std::lock_guard guard(local_queue_lock_);
            if (!local_queue_.empty())
                return;
        }
        {
            std::lock_guard<std::mutex> guard(sched_.sched_lock_);
            if (get_from_global_queue(false))
                return;
        }
        if (stole_task())
            return;

        std::unique_lock lock(sched_.sched_lock_);
        sched_.block_cv.wait(lock);
        if (get_from_global_queue(false))
            return;
    }
}

bool Runner::stole_task() {
    return false;
}

void Runner::await_once() {
    // Caller is always the first one at local queue
    {
        std::lock_guard guard(local_queue_lock_);
        auto &now = local_queue_.front();
        auto task_id = now->handle.promise().id;
        std::lock_guard guard1(sched_.blocking_map_lock);
//        spdlog::debug("runner task {} blocked", task_id);
        sched_.blocking_map_[task_id] = std::move(now);
    }
    curr_awaiting_ = true;
}

Scheduler::Scheduler()
//        : runners_(1) {
        : runners_(std::thread::hardware_concurrency()) {
    for (auto &p: runners_) {
        p = std::make_unique<Runner>(*this);
    }
}

void Scheduler::spawn(Task<void> &&task) {
    {
        std::lock_guard guard(sched_lock_);
        global_queue_.push(std::make_unique<Task<void>>(std::move(task))); // NOLINT(performance-move-const-arg)
    }
    block_cv.notify_one();
}

void Scheduler::await_once_ready(const uint64_t &task_id) {
//    spdlog::debug("sched task {} block stopped", task_id);
    std::lock_guard guard(blocking_map_lock);
    auto it = blocking_map_.find(task_id);
//    if (it == blocking_map_.end())
//        return;
    {
        std::lock_guard guard1(sched_lock_);
        global_queue_.push(std::move(it->second));
        block_cv.notify_one();
    }
    blocking_map_.erase(it);

}

void Scheduler::run() {
    using namespace std::chrono_literals;
    while (true) {
        std::this_thread::sleep_for(2.5s);
        std::lock_guard guard1(sched_lock_);
        std::lock_guard guard(blocking_map_lock);
        fmt::print("G: {}, B: {}, L:", global_queue_.size(), blocking_map_.size());
        size_t s = 0;
        for (auto &runner: runners_) {
            std::lock_guard g(runner->local_queue_lock_);
            fmt::print(" {}", runner->local_queue_.size());
            s += runner->local_queue_.size();
        }
        if (s == 0 && !global_queue_.empty()) {
//            block_cv.notify_one();
        }
        fmt::print("\n");
    }
}


