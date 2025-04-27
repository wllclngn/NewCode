#pragma once
#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <thread>
#include <mutex>

class Reducer {
public:
    void reduce(const std::vector<std::pair<std::string, int>>& mapped) {
        std::cout << "Reducing mapped data with cache-friendly chunking..." << std::endl;

        size_t numThreads = std::thread::hardware_concurrency();
        size_t chunkSize = 1024; // Adjust based on cache size
        std::vector<std::thread> threads;
        std::mutex mutex;

        for (size_t i = 0; i < numThreads; ++i) {
            threads.emplace_back([this, &mapped, i, chunkSize, &mutex]() {
                size_t startIdx = i * chunkSize;
                size_t endIdx = std::min(startIdx + chunkSize, mapped.size());

                std::map<std::string, int> localReduced;

                // Process key-value pairs in chunks
                for (size_t j = startIdx; j < endIdx; ++j) {
                    localReduced[mapped[j].first] += mapped[j].second;
                }

                // Merge local results into the global map
                std::lock_guard<std::mutex> lock(mutex);
                for (const auto& kv : localReduced) {
                    reduced[kv.first] += kv.second;
                }
            });
        }

        // Join all threads
        for (auto& thread : threads) {
            thread.join();
        }

        std::cout << "Reduction complete." << std::endl;
    }

    const std::map<std::string, int>& get_reduced_data() const {
        return reduced;
    }

private:
    std::map<std::string, int> reduced;
};