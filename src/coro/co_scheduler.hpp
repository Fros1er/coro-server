#pragma once

#include "awaitable.hpp"
#include <queue>
#include <thread>
#include <list>
#include "unidef.hpp"
#include <unordered_map>
#include "utils/Uncopyable.hpp"
#include <condition_variable>
#include "spdlog/spdlog.h"

class Scheduler;

using task_ptr_t = std::unique_ptr<Task<void>>;

class Runner : Uncopyable {
    static int id_cnt;
    int id_;
    // TODO: lock_free queue？
    std::mutex local_queue_lock_;
    std::list<task_ptr_t> local_queue_;
    bool interrupted{false}, curr_awaiting_{false};
    Scheduler &sched_;
    std::thread thread_;

    bool get_from_global_queue(bool once = true);

    void find_task();

    bool stole_task();

    void run();

    friend class Scheduler;

public:
    explicit Runner(Scheduler &sched);

    ~Runner();

    void await_once();
};

class Scheduler : Uncopyable {
    std::mutex sched_lock_{}, blocking_map_lock{};
    std::queue<std::unique_ptr<Task<void>>> global_queue_;
    std::vector<std::unique_ptr<Runner>> runners_;
    std::unordered_map<uint64_t, task_ptr_t> blocking_map_; // place task in map when waiting
    std::condition_variable block_cv;

    std::thread thread_;

    friend class Runner;

public:

    void run();

    Scheduler();

    Scheduler(Scheduler &) = delete;

    void spawn(Task<void> &&task);

    void await_once_ready(const uint64_t &task_id);
};
