#pragma once

#include <map>
#include <vector>
#include <string>
#include <mutex>
#include <thread>
#include <future>
#include <iostream>
#include <cstdlib>
#include <functional>

class Reducer {
public:
    Reducer() {}

    void reduce(const std::vector<std::pair<std::string, int>>& mappedData, std::map<std::string, int>& reducedData) {
        std::mutex mutex;
        size_t chunkSize = calculate_dynamic_chunk_size(mappedData.size());

        // Launch threads for parallel reduction
        std::vector<std::future<void>> futures;
        for (size_t i = 0; i < mappedData.size(); i += chunkSize) {
            futures.push_back(std::async(std::launch::async, [this, &mappedData, &reducedData, &mutex, i, chunkSize]() {
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
            }));
        }

        // Wait for all threads to finish
        for (auto& future : futures) {
            future.get();
        }
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
};