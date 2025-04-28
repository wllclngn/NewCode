#pragma once
#include <map>
#include <vector>
#include <string>
#include <mutex>
#include <thread>
#include <iostream>
#include <cstdlib>
#include <queue>
#include <condition_variable>
#include <functional>

class ThreadPool {
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

    void enqueueTask(const std::function<void()>& task) {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            taskQueue.push(task);
        }
        condition.notify_one();
        adjustThreadPool();
    }

    void shutdown() {
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

class Reducer {
public:
    Reducer(size_t minThreads = 2, size_t maxThreads = 8)
        : threadPool(minThreads, maxThreads) {}

    void reduce(const std::vector<std::pair<std::string, int>>& mappedData, std::map<std::string, int>& reducedData) {
        std::mutex mutex;
        size_t chunkSize = calculate_dynamic_chunk_size(mappedData.size());

        for (size_t i = 0; i < mappedData.size(); i += chunkSize) {
            threadPool.enqueueTask([this, &mappedData, &reducedData, &mutex, i, chunkSize]() {
                size_t startIdx = i;
                size_t endIdx = std::min(startIdx + chunkSize, mappedData.size());
                std::map<std::string, int> localReduce;

                for (size_t j = startIdx; j < endIdx; ++j) {
                    localReduce[mappedData[j].first] += mappedData[j].second;
                }

                {
                    std::lock_guard<std::mutex> lock(mutex); // Ensure thread-safe access to shared data
                    for (const auto& kv : localReduce) {
                        reducedData[kv.first] += kv.second;
                    }
                }
            });
        }

        threadPool.shutdown();
    }

private:
    size_t calculate_dynamic_chunk_size(size_t totalSize) {
        size_t numThreads = std::thread::hardware_concurrency();
        size_t defaultChunkSize = 1024;

        if (numThreads == 0) {
            return defaultChunkSize; // Fallback in case hardware_concurrency is not supported
        }

        size_t chunkSize = totalSize / numThreads;
        return chunkSize > defaultChunkSize ? chunkSize : defaultChunkSize;
    }

    ThreadPool threadPool;
};