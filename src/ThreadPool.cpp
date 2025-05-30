#include "..\include\ThreadPool.h"
#include "..\include\Logger.h"

ThreadPool::ThreadPool(size_t minThreads, size_t maxThreads)
    : minThreadsCount(minThreads), maxThreadsCount(maxThreads),
      activeThreadsCount(0), stopFlag(false), shuttingDownFlag(false) {
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

void ThreadPool::enqueueTask(const std::function<void()>& task) {
    if (shuttingDownFlag || stopFlag) return;

    {
        std::unique_lock<std::mutex> lock(queueMutex);
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
        std::unique_lock<std::mutex> lock(queueMutex);
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
    std::lock_guard<std::mutex> lock(threadsMutex);
    return activeThreadsCount;
}

size_t ThreadPool::getTasksInQueue() const {
    std::lock_guard<std::mutex> lock(queueMutex);
    return taskQueue.size();
}

void ThreadPool::workerLoop() {
    {
        std::lock_guard<std::mutex> lock(threadsMutex);
        activeThreadsCount++;
    }

    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [this]() {
                return stopFlag || !taskQueue.empty();
            });

            if (stopFlag && taskQueue.empty()) break;
            if (taskQueue.empty()) continue;

            task = std::move(taskQueue.front());
            taskQueue.pop();
        }

        try {
            task();
        } catch (const std::exception& e) {
            Logger::getInstance().log("THREAD_POOL: Exception caught in worker thread: " + std::string(e.what()));
        } catch (...) {
            Logger::getInstance().log("THREAD_POOL: Unknown exception caught in worker thread.");
        }
    }

    {
        std::lock_guard<std::mutex> lock(threadsMutex);
        activeThreadsCount--;
    }
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
            if (taskQueue.size() > workerThreads.size() || workerThreads.size() < minThreadsCount) {
                addThread();
            }
        }
    }
    if (workerThreads.size() < minThreadsCount && !stopFlag && !shuttingDownFlag) {
        for (size_t i = workerThreads.size(); i < minThreadsCount; ++i) {
            if (workerThreads.size() < maxThreadsCount) {
                addThread();
            } else break;
        }
    }
}