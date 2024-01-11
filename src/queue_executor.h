#pragma once

// This is an elaborate implementation of macOS dispatch queue
// It always persists the 'thread' that is executing
// That means that 'sync' is always executing on either created thread or in current thread
// Similarly to macOS impl 'async' always ex

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>

class QueueExecutor {
  public:
    using Fn = std::function<void()>;
    QueueExecutor() = default;

    class Task {
      public:
        virtual void process() = 0;
    };

    class SyncTask final : public Task {
      public:
        SyncTask(Fn fn) : task_(fn) {
        }

        virtual void process() final override {
            if (!stolen_) {
                run();
            } else {
                completed_.wait(false);
            }
        }

        void tokenDtor() {
            if (!stolen_) {
                completed_.wait(false);
            } else {
                run();
            }
        }

        void run() {
            task_();
            completed_ = true;
            completed_.notify_one();
        }
        void steal() {
            stolen_ = true;
        }

      private:
        // Flag that notifies that task was 'stolen' from 'executor' and it needs to wait for it to finish
        bool stolen_ = false;

        // Packaged task to be executed. Future is used for syncing purposes when necessary
        // If task is 'stolen', executor will use future to ensure caller thread is done
        // If task is not 'stolen', sync caller will check is 'executor' is done
        std::atomic_bool completed_ = false;

        Fn task_;
    };

    class SyncToken
    {
    public:
        SyncToken() = default;
        explicit SyncToken(std::shared_ptr<SyncTask>&& task)
            : task_(std::forward<std::shared_ptr<SyncTask>>(task)) {
        }

        SyncToken(const SyncToken&) = delete;
        SyncToken& operator=(const SyncToken&) = delete;

        SyncToken(SyncToken&& other) : task_(std::move(other.task_)) {
        }

        SyncToken& operator=(SyncToken&& other) {
            if (task_)
                task_->tokenDtor();

            task_ = std::move(other.task_);
            return *this;
        }

        ~SyncToken() {
            if (task_)
                task_->tokenDtor();
        }

    private:
        std::shared_ptr<SyncTask> task_;
    };

    void start(bool allowSameThreadExec);
    SyncToken sync(Fn);
    void async(Fn);
    void stop();

  private:
    class AsyncTask final : public Task {
      public:
        AsyncTask(Fn fn) : task_(fn) {
        }

        virtual void process() final override {
            task_();
        }

      private:
        Fn task_;
    };

    using TaskPtr = std::shared_ptr<Task>;

    // OpenGL will be unhappy if different thread will attempt to execute the code related to it
    // This flag will force execution on 'executor' even if current thread can be used instead
    bool allowSameThreadExec_ = false;

    // Lots of shimes needed for the tasks queue, built around condvar + deque
    std::condition_variable cv_;
    std::mutex mutex_;
    std::deque<TaskPtr> tasks_;
    bool running_ = false;

    // The Executor as it goes
    std::thread executor_;

    // Sync for 'start' and 'stop' to avoid weird edge cases
    std::mutex initMutex_;

    void loop();
};
