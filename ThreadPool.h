#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

// Abstract Base Class for ThreadPool
class ThreadPoolBase {
public:
    virtual ~ThreadPoolBase() {}

    // Virtual methods to be implemented by derived classes
    virtual void enqueueTask(const std::function<void()>& task) = 0;
    virtual void shutdown() = 0;
};

// Concrete Implementation of ThreadPool
class ThreadPool : public ThreadPoolBase {
public:
    ThreadPool(size_t minThreads, size_t maxThreads)
        : minThreads(minThreads), maxThreads(maxThreads), stopFlag(false) {
        for (size_t i = 0; i < minThreads; ++i) {
            addThread();
        }
    }

    ~ThreadPool() {
        shutdown();
    }

    void enqueueTask(const std::function<void()>& task) override {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            taskQueue.push(task);
        }
        condition.notify_one();
        adjustThreadPool();
    }

    void shutdown() override {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stopFlag = true;
        }
        condition.notify_all();
        for (std::thread& thread : threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

private:
    void addThread() {
        threads.emplace_back([this]() {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(queueMutex);
                    condition.wait(lock, [this]() {
                        return stopFlag || !taskQueue.empty();
                    });
                    if (stopFlag && taskQueue.empty()) {
                        return;
                    }
                    task = std::move(taskQueue.front());
                    taskQueue.pop();
                }
                task();
            }
        });
    }

    void adjustThreadPool() {
        if (taskQueue.size() > threads.size() && threads.size() < maxThreads) {
            addThread();
        }
    }

    std::vector<std::thread> threads;
    std::queue<std::function<void()>> taskQueue;
    std::mutex queueMutex;
    std::condition_variable condition;
    bool stopFlag;
    size_t minThreads;
    size_t maxThreads;
};
