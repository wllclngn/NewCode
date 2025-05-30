#include "..\include\ThreadPool.h"
#include "..\include\Logger.h"

#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <string>
#include <atomic>

class ThreadPool {
public:
    // Constructor: Initializes the thread pool with min and max thread counts
    ThreadPool(size_t minThreads, size_t maxThreads);

    // Destructor: Gracefully shuts down the thread pool
    ~ThreadPool();

    // Enqueue a task into the thread pool
    void enqueueTask(const std::function<void()>& task);

    // Shutdown the thread pool and wait for all threads to complete
    void shutdown();

    // Get the count of currently active threads
    size_t getActiveThreads() const;

    // Get the number of tasks remaining in the queue
    size_t getTasksInQueue() const;

private:
    // Worker function for each thread in the pool
    void workerLoop();

    // Dynamically add a thread to the pool if needed
    void addThread();

    // Adjust the thread pool size based on workload
    void adjustThreadPoolSize();

    // Mutex for synchronizing access to the task queue
    mutable std::mutex queueMutex;

    // Mutex for synchronizing access to the thread pool
    mutable std::mutex threadsMutex;

    // Condition variable for task scheduling
    std::condition_variable condition;

    // Task queue to hold pending tasks
    std::queue<std::function<void()>> taskQueue;

    // Vector to store worker threads
    std::vector<std::thread> workerThreads;

    // Minimum and maximum thread counts
    size_t minThreadsCount;
    size_t maxThreadsCount;

    // Active thread count
    std::atomic<size_t> activeThreadsCount;

    // Flags for stopping and shutting down the thread pool
    std::atomic<bool> stopFlag;
    std::atomic<bool> shuttingDownFlag;
};