#include "queue_executor.h"

void QueueExecutor::start(bool allowSameThreadExec) {
    std::lock_guard lck(initMutex_);
    if (running_)
        return;

    running_ = true;
    allowSameThreadExec_ = allowSameThreadExec;
    tasks_.clear();
    executor_ = std::thread{ &QueueExecutor::loop, this };
}

QueueExecutor::SyncToken QueueExecutor::sync(Fn fn) {
    auto task = std::make_shared<SyncTask>(std::move(fn));
    bool notify;
    {
        std::unique_lock<std::mutex> lck(mutex_);
        notify = tasks_.empty();
        if (allowSameThreadExec_ && tasks_.empty()) {
            task->steal();
        }
        tasks_.emplace_back(task);
    }

    if (notify)
        cv_.notify_one();

    return SyncToken{ std::move(task) };
}

void QueueExecutor::async(Fn fn) {
    auto task = std::make_shared<AsyncTask>(std::move(fn));
    bool notify;
    {
        std::unique_lock<std::mutex> lck(mutex_);
        notify = tasks_.empty();
        tasks_.emplace_back(std::move(task));
    }

    if (notify)
        cv_.notify_one();
}

void QueueExecutor::stop() {
    std::lock_guard lck(initMutex_);
    if (!running_)
        return;

    async([&]() {
        running_ = false;
    });
    executor_.join();
}

void QueueExecutor::loop() {
    std::unique_lock<std::mutex> lck(mutex_);
    while (running_) {
        if (tasks_.empty())
            cv_.wait(lck, [&] { return !tasks_.empty(); });

        auto task = std::move(tasks_.front());
        lck.unlock();

        task->process();

        lck.lock();
        tasks_.pop_front();
    }
}
