#pragma once
#include <map>
#include <vector>
#include <string>
#include <mutex>
#include <thread>
#include <iostream>

class Reducer {
public:
    void reduce(const std::vector<std::pair<std::string, int>>& mappedData, std::map<std::string, int>& reducedData) {
        std::mutex mutex;
        size_t chunkSize = 1024; // Adjust based on system cache
        size_t numThreads = std::thread::hardware_concurrency();
        std::vector<std::thread> threads;

        for (size_t i = 0; i < numThreads; ++i) {
            threads.emplace_back([this, &mappedData, &reducedData, &mutex, i, chunkSize]() {
                size_t startIdx = i * chunkSize;
                size_t endIdx = std::min(startIdx + chunkSize, mappedData.size());
                std::map<std::string, int> localReduce;

                for (size_t j = startIdx; j < endIdx; ++j) {
                    localReduce[mappedData[j].first] += mappedData[j].second;
                }

                std::lock_guard<std::mutex> lock(mutex);
                for (const auto& kv : localReduce) {
                    reducedData[kv.first] += kv.second;
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }
    }
};