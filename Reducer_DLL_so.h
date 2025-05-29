#ifndef REDUCER_DLL_SO_H
#define REDUCER_DLL_SO_H

#include <map>
#include <vector>
#include <string>
#include <mutex>
#include <thread>
#include <algorithm> // For std::min
#include <iostream>  // For std::cerr (if ErrorHandler isn't used directly)
#include <filesystem>

#include "ThreadPool.h"   // Assuming ThreadPool.h is accessible
#include "ErrorHandler.h" // Assuming ErrorHandler.h is accessible
#include "Logger.h"       // Assuming Logger.h is accessible


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

    // Original reduce method (used by interactive mode, which prepares a single mapped_data vector)
    // This version will use default thread pool sizes.
    virtual void reduce(const std::vector<std::pair<std::string, int>>& mappedData, 
                        std::map<std::string, int>& reducedData) {
        
        size_t defaultMinThreads = std::thread::hardware_concurrency();
        if (defaultMinThreads == 0) defaultMinThreads = 2;
        size_t defaultMaxThreads = defaultMinThreads; // Keep min=max for simplicity here

        Logger::getInstance().log("REDUCER_DLL_SO (2-arg): Starting reduction. Pool: " + 
                                  std::to_string(defaultMinThreads) + "-" + std::to_string(defaultMaxThreads) + " threads.");
        
        process_reduce(mappedData, reducedData, defaultMinThreads, defaultMaxThreads);
        Logger::getInstance().log("REDUCER_DLL_SO (2-arg): Finished reduction.");
    }

    // New reduce method for command-line driven reducer mode (4 arguments)
    // Accepts min/max pool threads.
    virtual void reduce(const std::vector<std::pair<std::string, int>>& mappedData, 
                        std::map<std::string, int>& reducedData,
                        size_t minPoolThreads, // New parameter
                        size_t maxPoolThreads) // New parameter
                        {
        // Validate thread pool sizes
        if (minPoolThreads == 0) minPoolThreads = 1;
        if (maxPoolThreads < minPoolThreads) maxPoolThreads = minPoolThreads;

        Logger::getInstance().log("REDUCER_DLL_SO (4-arg): Starting reduction. Pool: " + 
                                  std::to_string(minPoolThreads) + "-" + std::to_string(maxPoolThreads) + " threads.");
        
        process_reduce(mappedData, reducedData, minPoolThreads, maxPoolThreads);
        Logger::getInstance().log("REDUCER_DLL_SO (4-arg): Finished reduction.");
    }


protected:
    // Common processing logic for reduce, now taking thread pool sizes
    void process_reduce(const std::vector<std::pair<std::string, int>>& mappedData, 
                        std::map<std::string, int>& reducedData,
                        size_t minThreads, 
                        size_t maxThreads) {
        std::mutex reduce_mutex; // Mutex to protect writing to the shared reducedData map
        
        // Sort data by key to group identical keys for parallel processing if desired,
        // or rely on map's ordering and atomic operations if applicable.
        // For simplicity with ThreadPool and merging local maps, sorting isn't strictly
        // necessary if each thread processes a chunk and merges into a global map.
        // The current approach: each thread processes a chunk of the vector and aggregates into local map,
        // then merges local map into the global reducedData map under a lock.

        size_t chunkSize = calculate_dynamic_chunk_size(mappedData.size(), maxThreads);
        
        ThreadPool pool(minThreads, maxThreads);

        for (size_t i = 0; i < mappedData.size(); i += chunkSize) {
            pool.enqueueTask([this, &mappedData, &reducedData, &reduce_mutex, i, chunkSize]() { // Capture 'this'
                size_t startIdx = i;
                size_t endIdx = std::min(startIdx + chunkSize, mappedData.size());
                std::map<std::string, int> localReduceMap; // Each thread accumulates locally

                for (size_t j = startIdx; j < endIdx; ++j) {
                    // mappedData should already contain <word, count> pairs from mappers.
                    // If multiple mappers produced the same word, they'd be separate entries here.
                    localReduceMap[mappedData[j].first] += mappedData[j].second;
                }

                // Lock and merge the local map into the global reducedData
                if (!localReduceMap.empty()) {
                    std::lock_guard<std::mutex> lock(reduce_mutex);
                    for (const auto &kv : localReduceMap) {
                        reducedData[kv.first] += kv.second;
                    }
                }
            });
        }
        pool.shutdown(); // Wait for all tasks to complete
    }


    virtual void report_error(const std::string &error_message) const {
         Logger::getInstance().log("ERROR (ReducerDLLso): " + error_message);
    }

    // Calculate dynamic chunk size, similar to Mapper's
    size_t calculate_dynamic_chunk_size(size_t totalSize, size_t guideMaxThreads = 0) const {
        size_t numEffectiveThreads = guideMaxThreads > 0 ? guideMaxThreads : std::thread::hardware_concurrency();
        if (numEffectiveThreads == 0) numEffectiveThreads = 4;
        
        const size_t minChunkSize = 256; 
        const size_t maxChunksPerThreadFactor = 4;
        const size_t maxTotalChunks = numEffectiveThreads * maxChunksPerThreadFactor;

        if (totalSize == 0) return minChunkSize;

        size_t chunkSize = totalSize / numEffectiveThreads;

        if (numEffectiveThreads > 0 && totalSize / chunkSize > maxTotalChunks && maxTotalChunks > 0) {
            chunkSize = totalSize / maxTotalChunks;
        }
        
        return std::max(minChunkSize, chunkSize);
    }
};

#endif // REDUCER_DLL_SO_H