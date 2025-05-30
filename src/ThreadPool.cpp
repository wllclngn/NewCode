#ifdef _WIN32
    #include "..\include\ThreadPool.h"
    #include "..\include\Logger.h"
#elif defined(__unix__) || defined(__APPLE__) && defined(__MACH__)
    #include "../include/ThreadPool.h"
    #include "../include/Logger.h"
#else
    #error "Unsupported operating system. Please check your platform."
#endif

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <string>
#include <atomic>

ThreadPool::ThreadPool(size_t minThreads, size_t maxThreads)
    : minThreadsCount(minThreads), maxThreadsCount(maxThreads),
      stopFlag(false), shuttingDownFlag(false), activeThreadsCount(0) {
    if (minThreadsCount == 0) minThreadsCount = 1;
    if (maxThreadsCount < minThreadsCount) maxThreadsCount = minThreadsCount;

    Logger::getInstance().log("THREAD_POOL: Initializing with MinThreads=" + std::to_string(minThreadsCount) +
                              ", MaxThreads=" + std::to_string(maxThreadsCount));
    for (size_t i = 0; i < minThreadsCount; ++i) {
        addThread();
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::addThread() {
    std::lock_guard<std::mutex> lock(threadsMutex);
    if (workerThreads.size() < maxThreadsCount) {
        workerThreads.emplace_back(&ThreadPool::workerLoop, this);
    }
}

void ThreadPool::adjustThreadPoolSize() {
    std::lock_guard<std::mutex> lock(threadsMutex);
    if (!stopFlag && !shuttingDownFlag) {
        if (!taskQueue.empty() && workerThreads.size() < maxThreadsCount) {
            addThread();
        }
    }
}

void ThreadPool::enqueueTask(const std::function<void()>& task) {
    if (shuttingDownFlag || stopFlag) return;

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        taskQueue.push(task);
    }
    condition.notify_one();
    adjustThreadPoolSize();
}

void ThreadPool::shutdown() {
    if (shuttingDownFlag) return;
    shuttingDownFlag = true;

    Logger::getInstance().log("THREAD_POOL: Shutdown initiated. Waiting for tasks and threads to complete.");
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        stopFlag = true;
    }
    condition.notify_all();

    for (std::thread& thread : workerThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    workerThreads.clear();
    Logger::getInstance().log("THREAD_POOL: Shutdown complete. All threads joined.");
}

size_t ThreadPool::getActiveThreads() const {
    std::lock_guard<std::mutex> lock(threadsMutex); // Ensure threadsMutex is non-const
    return activeThreadsCount;
}

size_t ThreadPool::getTasksInQueue() const {
    std::lock_guard<std::mutex> lock(queueMutex); // Ensure queueMutex is non-const
    return taskQueue.size();
}