#pragma once
#include "logger.h"
#include "error_handler.h"
#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <mutex>
#include <thread>

class Reducer {
public:
    void reduce(const std::vector<std::pair<std::string, int>>& mapped) {
        Logger::getInstance().log("Starting reducing phase...");

        size_t numThreads = std::thread::hardware_concurrency();
        size_t chunkSize = mapped.size() / numThreads + (mapped.size() % numThreads != 0);
        std::vector<std::thread> threads;
        std::mutex mutex;

        for (size_t i = 0; i < numThreads; ++i) {
            size_t startIdx = i * chunkSize;
            size_t endIdx = std::min(startIdx + chunkSize, mapped.size());

            threads.emplace_back([this, &mapped, startIdx, endIdx, &mutex]() {
                std::map<std::string, int> localReduced;
                for (size_t j = startIdx; j < endIdx; ++j) {
                    localReduced[mapped[j].first] += mapped[j].second;
                }

                std::lock_guard<std::mutex> lock(mutex);
                for (const auto& kv : localReduced) {
                    reduced_[kv.first] += kv.second;
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        Logger::getInstance().log("Reducing phase completed.");
    }

    const std::map<std::string, int>& get_reduced_data() const {
        return reduced_;
    }

private:
    std::map<std::string, int> reduced_;
};