#pragma once
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <fstream>
#include <mutex>
#include <thread>
#include <iostream>
#include <cstdlib>
#include <queue>
#include <condition_variable>
#include <functional>
#include "ERROR_Handler.h"
#include "FileHandler.h"
#include "Mapper_DLL_so.h"
#include "Mapper.h"
#include "Reducer.h"

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

class Mapper {
public:
    Mapper(size_t minThreads = 2, size_t maxThreads = 8)
        : threadPool(minThreads, maxThreads) {}

    void map_words(const std::vector<std::string>& lines, const std::string& outputPath) {
        std::ofstream temp_out(outputPath);
        if (!temp_out) {
            ErrorHandler::reportError("Could not open " + outputPath + " for writing.");
            return;
        }

        std::mutex mutex;
        size_t chunkSize = calculate_dynamic_chunk_size(lines.size());

        for (size_t i = 0; i < lines.size(); i += chunkSize) {
            threadPool.enqueueTask([this, &lines, &temp_out, &mutex, i, chunkSize]() {
                size_t startIdx = i;
                size_t endIdx = std::min(startIdx + chunkSize, lines.size());
                std::map<std::string, int> localMap;

                for (size_t j = startIdx; j < endIdx; ++j) {
                    std::istringstream ss(lines[j]);
                    std::string word;
                    while (ss >> word) {
                        localMap[MapperDLLso::clean_word(word)]++;
                    }
                }

                std::lock_guard<std::mutex> lock(mutex);
                for (const auto& kv : localMap) {
                    temp_out << kv.first << ": " << kv.second << "\n";
                }
            });
        }

        threadPool.shutdown();
        temp_out.close();
    }

private:
    size_t calculate_dynamic_chunk_size(size_t totalSize) {
        size_t numThreads = std::thread::hardware_concurrency();
        size_t defaultChunkSize = 1024;

        if (numThreads == 0) {
            return defaultChunkSize;
        }

        size_t chunkSize = totalSize / numThreads;
        return chunkSize > defaultChunkSize ? chunkSize : defaultChunkSize;
    }

    ThreadPool threadPool;
};