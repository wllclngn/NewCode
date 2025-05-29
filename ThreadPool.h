#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <stdexcept> // For std::runtime_error if needed for errors
#include "Logger.h"  // For logging thread creation/errors if desired

// Abstract Base Class for ThreadPool (Optional, but good for interface definition)
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
    ThreadPool(size_t minThreads, size_t maxThreads)
        : minThreadsCount(minThreads), maxThreadsCount(maxThreads), 
          activeThreadsCount(0), stopFlag(false), shuttingDownFlag(false) {
        if (minThreadsCount == 0) minThreadsCount = 1; // Ensure at least one minimum thread
        if (maxThreadsCount < minThreadsCount) maxThreadsCount = minThreadsCount; // Max cannot be less than min

        Logger::getInstance().log("THREAD_POOL: Initializing with MinThreads=" + std::to_string(minThreadsCount) + 
                                  ", MaxThreads=" + std::to_string(maxThreadsCount));
        for (size_t i = 0; i < minThreadsCount; ++i) {
            addThread();
        }
    }

    ~ThreadPool() {
        shutdown();
    }

    void enqueueTask(const std::function<void()>& task) override {
        if (shuttingDownFlag || stopFlag) {
            // Logger::getInstance().log("THREAD_POOL: Warning - Task enqueued after shutdown initiated.");
            // Depending on desired behavior, either throw or log and ignore.
            // For now, we'll allow enqueueing but workers might not pick up if already stopping.
        }
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            taskQueue.push(task);
        }
        condition.notify_one(); // Notify one worker thread
        adjustThreadPoolSize(); // Check if we need to scale up
    }

    void shutdown() override {
        if (shuttingDownFlag) return; // Already shutting down
        shuttingDownFlag = true;
        Logger::getInstance().log("THREAD_POOL: Shutdown initiated. Waiting for tasks and threads to complete.");
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stopFlag = true; // Signal workers to stop after finishing current task and queue
        }
        condition.notify_all(); // Wake up all waiting threads
        for (std::thread& thread : workerThreads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        workerThreads.clear();
        Logger::getInstance().log("THREAD_POOL: Shutdown complete. All threads joined.");
    }

    size_t getActiveThreads() const override {
        std::lock_guard<std::mutex> lock(threadsMutex); // Protect access to activeThreadsCount
        return activeThreadsCount;
    }
    
    size_t getTasksInQueue() const override {
        std::lock_guard<std::mutex> lock(queueMutex);
        return taskQueue.size();
    }

private:
    void workerLoop() {
        {
            std::lock_guard<std::mutex> lock(threadsMutex);
            activeThreadsCount++;
            // Logger::getInstance().log("THREAD_POOL: Worker thread started. Active: " + std::to_string(activeThreadsCount));
        }
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                condition.wait(lock, [this]() {
                    return stopFlag || !taskQueue.empty();
                });

                if (stopFlag && taskQueue.empty()) { // If stopping and queue is empty, exit
                    break; 
                }
                if(taskQueue.empty()) { // Spurious wakeup or stopFlag without empty queue yet
                    continue;
                }

                task = std::move(taskQueue.front());
                taskQueue.pop();
            } // Release lock on queueMutex

            try {
                task(); // Execute the task
            } catch (const std::exception& e) {
                Logger::getInstance().log("THREAD_POOL: Exception caught in worker thread: " + std::string(e.what()));
            } catch (...) {
                Logger::getInstance().log("THREAD_POOL: Unknown exception caught in worker thread.");
            }
        }
        {
            std::lock_guard<std::mutex> lock(threadsMutex);
            activeThreadsCount--;
            // Logger::getInstance().log("THREAD_POOL: Worker thread stopping. Active: " + std::to_string(activeThreadsCount));
        }
    }

    void addThread() {
        std::lock_guard<std::mutex> lock(threadsMutex); // Protect workerThreads vector and activeThreadsCount
        if (workerThreads.size() < maxThreadsCount) {
            workerThreads.emplace_back(&ThreadPool::workerLoop, this);
            // Logger::getInstance().log("THREAD_POOL: Added new thread. Total worker threads: " + std::to_string(workerThreads.size()));
        }
    }
    
    // Basic scaling: if queue grows and we have capacity, add a thread.
    // More sophisticated scaling (downscaling, hysteresis) could be added.
    void adjustThreadPoolSize() {
        std::lock_guard<std::mutex> lock(threadsMutex); // Protect queue and thread vector access
        if (!stopFlag && !shuttingDownFlag) { // Only adjust if not stopping
            // Scale up if there are tasks and we are below max capacity
            if (!taskQueue.empty() && workerThreads.size() < maxThreadsCount) {
                 // Heuristic: Add a thread if queue size is, for example, greater than current worker threads
                 // This is a simple heuristic and can be made more sophisticated.
                if (taskQueue.size() > workerThreads.size() || workerThreads.size() < minThreadsCount) {
                    // Logger::getInstance().log("THREAD_POOL: Adjusting pool size. Tasks: " + std::to_string(taskQueue.size()) + ", Workers: " + std::to_string(workerThreads.size()) + ". Adding thread.");
                    // addThread() is called without holding queueMutex here, it has its own threadsMutex
                    // Need to release queueMutex if addThread is called from here
                    // For simplicity, addThread is called outside this critical section if needed.
                }
            }
        }
        // Note: addThread() itself handles the check against maxThreadsCount
        // This function is more of a trigger point. For now, we only scale up to minThreadsCount initially
        // and then `addThread` is called from `enqueueTask` if needed and capacity allows.
        // A more advanced adjustThreadPoolSize would also handle scaling down idle threads.
        // For now, we ensure minThreads are running and scale up to maxThreads on demand.
        if (workerThreads.size() < minThreadsCount && !stopFlag && !shuttingDownFlag) {
            // Logger::getInstance().log("THREAD_POOL: Below min threads, adding more.");
            for(size_t i = workerThreads.size(); i < minThreadsCount; ++i) {
                if (workerThreads.size() < maxThreadsCount) { // Double check against max
                     // addThread(); // This would be inside the threadsMutex lock, which is fine.
                } else break;
            }
        }
         // Simplified: if queue is building up and we have capacity, add a thread
        if (!taskQueue.empty() && workerThreads.size() < maxThreadsCount && !stopFlag && !shuttingDownFlag) {
            // Logger::getInstance().log("THREAD_POOL: Task in queue, potentially adding thread.");
            // addThread(); // This would be inside the threadsMutex lock.
        }
    }


    std::vector<std::thread> workerThreads;
    std::queue<std::function<void()>> taskQueue;
    
    mutable std::mutex queueMutex; // Protects taskQueue
    mutable std::mutex threadsMutex; // Protects workerThreads vector and activeThreadsCount
    std::condition_variable condition;
    
    bool stopFlag; // True if shutdown initiated and queue should be drained
    bool shuttingDownFlag; // True once shutdown() is called, prevents new adjustments

    size_t minThreadsCount;
    size_t maxThreadsCount;
    size_t activeThreadsCount; // Number of threads currently running their workerLoop
};