#include "..\include\ThreadPool.h"
#include "..\include\Logger.h"

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <string>
#include <atomic>

// Constructor
ThreadPool::ThreadPool(size_t minThreads, size_t maxThreads)
    : minThreadsCount(minThreads), maxThreadsCount(maxThreads), stopFlag(false), shuttingDownFlag(false), activeThreadsCount(0) {
    // Initialize threads
    for (size_t i = 0; i < minThreadsCount; ++i) {
        addThread();
    }
}

// Destructor
ThreadPool::~ThreadPool() {
    shutdown();
}

// Enqueue a task
void ThreadPool::enqueueTask(const std::function<void()>& task) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        taskQueue.push(task);
    }
    condition.notify_one();
}

// Shutdown the thread pool
void ThreadPool::shutdown() {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        stopFlag = true;
    }
    condition.notify_all();

    for (std::thread& worker : workerThreads) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workerThreads.clear();
}

// Get the count of active threads
size_t ThreadPool::getActiveThreads() const {
    return activeThreadsCount;
}

// Get the number of tasks in the queue
size_t ThreadPool::getTasksInQueue() const {
    std::lock_guard<std::mutex> lock(queueMutex);
    return taskQueue.size();
}

// Worker loop for each thread
void ThreadPool::workerLoop() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [this] { return stopFlag || !taskQueue.empty(); });

            if (stopFlag && taskQueue.empty()) {
                return;
            }

            task = std::move(taskQueue.front());
            taskQueue.pop();
        }

        ++activeThreadsCount;
        task();
        --activeThreadsCount;
    }
}

// Add a thread to the pool
void ThreadPool::addThread() {
    workerThreads.emplace_back(&ThreadPool::workerLoop, this);
}

// Adjust the thread pool size based on workload
void ThreadPool::adjustThreadPoolSize() {
    // Implementation for dynamic resizing (optional)
}