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
    // Constructor with minimum and maximum thread limits
    ThreadPool(size_t minThreads, size_t maxThreads);

    // Destructor
    ~ThreadPool();

    // Enqueue a new task into the thread pool
    void enqueueTask(const std::function<void()>& task);

    // Shutdown the thread pool and wait for all tasks to complete
    void shutdown();

    // Get the current number of active threads
    size_t getActiveThreads() const;

    // Get the current number of tasks in the queue
    size_t getTasksInQueue() const;

private:
    // Add a new thread to the worker pool
    void addThread();

    // Adjust the thread pool size dynamically based on workload
    void adjustThreadPoolSize();

    // Worker loop for threads
    void workerLoop();

    // Minimum and maximum number of threads allowed
    size_t minThreadsCount;
    size_t maxThreadsCount;

    // Worker threads
    std::vector<std::thread> workerThreads;

    // Task queue
    std::queue<std::function<void()>> taskQueue;

    // Mutexes for thread safety
    std::mutex queueMutex;
    std::mutex threadsMutex;

    // Condition variable for task synchronization
    std::condition_variable condition;

    // Flags for stopping and shutting down the thread pool
    std::atomic<bool> stopFlag;
    std::atomic<bool> shuttingDownFlag;

    // Counter for active threads
    std::atomic<size_t> activeThreadsCount;
};

#endif // THREAD_POOL_H