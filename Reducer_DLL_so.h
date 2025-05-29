#ifndef REDUCER_DLL_SO_H
#define REDUCER_DLL_SO_H

#include <map>
#include <vector>
#include <string>
#include <mutex>
#include <thread>
#include <future>
#include <iostream>
#include <cstdlib>
#include <functional>

// Export macro for cross-platform compatibility
#if defined(_WIN32) || defined(_WIN64)
#define DLL_so_EXPORT __declspec(dllexport)
#elif defined(__linux__) || defined(__unix__)
#define DLL_so_EXPORT __attribute__((visibility("default")))
#else
#define DLL_so_EXPORT
#endif

class DLL_so_EXPORT ReducerDLLso {
public:
    virtual ~ReducerDLLso() {} // Virtual destructor for polymorphism

    virtual void reduce(const std::vector<std::pair<std::string, int>>& mappedData, std::map<std::string, int>& reducedData) {
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

protected:
    // Protected member to allow overriding in derived classes
    virtual size_t calculate_dynamic_chunk_size(size_t totalSize) const {
        size_t numThreads = std::thread::hardware_concurrency();
        size_t defaultChunkSize = 1024;

        if (numThreads == 0) {
            return defaultChunkSize;
        }

        size_t chunkSize = totalSize / numThreads;
        return chunkSize > defaultChunkSize ? chunkSize : defaultChunkSize;
    }
};

#endif // REDUCER_DLL_SO_H