#pragma once

#include <functional>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

// Abstract Base Class for ThreadPool
class ThreadPoolBase {
public:
    virtual ~ThreadPoolBase() {}
    virtual void enqueueTask(const std::function<void()>& task) = 0;
    virtual void shutdown() = 0;
    virtual size_t getActiveThreads() const = 0;
    virtual size_t getTasksInQueue() const = 0;
};

// Concrete Implementation of ThreadPool
class ThreadPool : public ThreadPoolBase {
public:
    ThreadPool(size_t minThreads, size_t maxThreads);
    ~ThreadPool();

    void enqueueTask(const std::function<void()>& task) override;
    void shutdown() override;

    size_t getActiveThreads() const override;
    size_t getTasksInQueue() const override;

private:
    void workerLoop();
    void addThread();
    void adjustThreadPoolSize();

    std::vector<std::thread> workerThreads;
    std::queue<std::function<void()>> taskQueue;

    mutable std::mutex queueMutex;
    mutable std::mutex threadsMutex;
    std::condition_variable condition;

    bool stopFlag;
    bool shuttingDownFlag;

    size_t minThreadsCount;
    size_t maxThreadsCount;
    size_t activeThreadsCount;
};