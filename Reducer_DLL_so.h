#ifndef REDUCER_DLL_SO_H
#define REDUCER_DLL_SO_H

#include <map>
#include <vector>
#include <string>
#include <mutex>
#include <thread>
#include <algorithm> // For std::min, std::max
// #include <iostream> // Removed if not used directly by this class for errors (prefer ErrorHandler)
// #include <filesystem> // Removed as not used in the provided class logic

#include "ThreadPool.h"
#include "ErrorHandler.h" // Include for centralized error handling
#include "Logger.h"       // Include for centralized logging

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
    virtual ~ReducerDLLso() {}

    // The reduce function takes data that has already been read and parsed by the Reducer Process.
    // Sorting of mappedData (by key) should ideally happen in the Reducer Process before calling this.
    virtual void reduce(const std::vector<std::pair<std::string, int>> &mappedData, std::map<std::string, int> &reducedData) {
        // If mappedData is empty, reducedData will remain empty. This is valid.
        // Logger::getInstance().log("ReducerDLLso: Starting reduce operation. Mapped data size: " + std::to_string(mappedData.size()));

        std::mutex mutex; // Mutex to protect access to reducedData map
        size_t chunkSize = calculate_dynamic_chunk_size(mappedData.size());
        size_t numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) numThreads = 4; // fallback

        ThreadPool pool(numThreads, numThreads); // Assuming ThreadPool is suitable

        for (size_t i = 0; i < mappedData.size(); i += chunkSize) {
            pool.enqueueTask([&, i, &mappedData, &reducedData, &mutex, chunkSize]() { // Explicitly capture all needed variables
                size_t startIdx = i;
                size_t endIdx = std::min(startIdx + chunkSize, mappedData.size());
                
                // Local aggregation for this thread's chunk
                std::map<std::string, int> localReduce; 
                for (size_t j = startIdx; j < endIdx; ++j) {
                    // Basic check, though loop conditions and std::min should prevent out-of-bounds
                    if (j < mappedData.size()) { 
                        localReduce[mappedData[j].first] += mappedData[j].second;
                    }
                }
                
                // Merge localReduce into the global reducedData under a lock
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    for (const auto &kv : localReduce) {
                        reducedData[kv.first] += kv.second;
                    }
                }
            });
        }
        pool.shutdown(); // Wait for all tasks to complete
        // Logger::getInstance().log("ReducerDLLso: Reduce operation finished. Reduced data size: " + std::to_string(reducedData.size()));
    }

protected:
    // Standardized dynamic chunk size calculation
    virtual size_t calculate_dynamic_chunk_size(size_t totalSize) const {
        size_t numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) numThreads = 1; // Ensure at least 1 thread for calculation
        size_t defaultChunkSize = 1024; // Minimum items per chunk
        
        if (totalSize == 0) return defaultChunkSize; // If totalSize is 0, loop for tasks won't run based on i < mappedData.size().

        size_t chunkSize = totalSize / numThreads;
        if (totalSize % numThreads != 0 && totalSize > numThreads) { // ensure chunkSize increases only if it makes sense
             chunkSize++;
        }
        if (chunkSize == 0 && totalSize > 0) { // handles totalSize < numThreads
            chunkSize = totalSize; // process all in one chunk if less than numThreads or defaultChunkSize allows
        }

        return std::max(defaultChunkSize, chunkSize);
    }
};

#endif // REDUCER_DLL_SO_H