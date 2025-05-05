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
#include "ThreadPool.h"

class Reducer {
public:
    Reducer(size_t minThreads = 2, size_t maxThreads = 8)
        : threadPool(std::make_unique<ThreadPool>(minThreads, maxThreads)) {}

    void reduce(const std::vector<std::pair<std::string, int>>& mappedData, std::map<std::string, int>& reducedData) {
        std::mutex mutex;
        size_t chunkSize = calculate_dynamic_chunk_size(mappedData.size());

        for (size_t i = 0; i < mappedData.size(); i += chunkSize) {
            threadPool->enqueueTask([this, &mappedData, &reducedData, &mutex, i, chunkSize]() {
                size_t startIdx = i;
                size_t endIdx = std::min(startIdx + chunkSize, mappedData.size());
                std::map<std::string, int> localReduce;

                for (size_t j = startIdx; j < endIdx; ++j) {
                    localReduce[mappedData[j].first] += mappedData[j].second;
                }

                {
                    std::lock_guard<std::mutex> lock(mutex);
                    for (const auto& kv : localReduce) {
                        reducedData[kv.first] += kv.second;
                    }
                }
            });
        }

        threadPool->shutdown();
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

    std::unique_ptr<ThreadPoolBase> threadPool;
};
