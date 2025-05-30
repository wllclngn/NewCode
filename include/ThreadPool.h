#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

class ThreadPool {
public:
    ThreadPool(size_t minThreads, size_t maxThreads);
    ~ThreadPool();

    void enqueueTask(const std::function<void()>& task);
    void shutdown();
    size_t getActiveThreads() const;
    size_t getTasksInQueue() const;

private:
    void addThread();
    void adjustThreadPoolSize();
    void workerLoop();

    size_t minThreadsCount;
    size_t maxThreadsCount;

    std::vector<std::thread> workerThreads;
    std::queue<std::function<void()>> taskQueue;

    mutable std::mutex queueMutex;
    mutable std::mutex threadsMutex;

    std::condition_variable condition;

    std::atomic<bool> stopFlag;
    std::atomic<bool> shuttingDownFlag;
    std::atomic<size_t> activeThreadsCount;
};

#endif // THREAD_POOL_H