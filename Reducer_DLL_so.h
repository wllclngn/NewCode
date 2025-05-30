#ifndef REDUCER_DLL_SO_H
#define REDUCER_DLL_SO_H

#include <map>
#include <vector>
#include <string>
#include <mutex>
#include <thread>
#include <algorithm> 
#include <iostream>  
#include <filesystem>

#include "ThreadPool.h"   
#include "ErrorHandler.h" // Ensuring this is included for ErrorHandler::reportError
#include "Logger.h"       

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
    // Default thread pool settings constants
    static constexpr size_t DEFAULT_REDUCE_MIN_THREADS = 0; 
    static constexpr size_t DEFAULT_REDUCE_MAX_THREADS = 0; 
    static constexpr size_t FALLBACK_REDUCE_THREAD_COUNT = 2;

    virtual ~ReducerDLLso() {}

    // Unified reduce method
    virtual void reduce(
        const std::vector<std::pair<std::string, int>>& mappedData, 
        std::map<std::string, int>& reducedData,
        size_t minPoolThreadsConfig = DEFAULT_REDUCE_MIN_THREADS,
        size_t maxPoolThreadsConfig = DEFAULT_REDUCE_MAX_THREADS
    ) {
        size_t actualMinThreads = minPoolThreadsConfig;
        if (actualMinThreads == 0) {
            actualMinThreads = std::thread::hardware_concurrency();
            if (actualMinThreads == 0) actualMinThreads = FALLBACK_REDUCE_THREAD_COUNT;
        }

        size_t actualMaxThreads = maxPoolThreadsConfig;
        if (actualMaxThreads == 0) {
            actualMaxThreads = actualMinThreads; 
        }
        if (actualMaxThreads < actualMinThreads) actualMaxThreads = actualMinThreads;

        Logger::getInstance().log("ReducerDLLso: Starting reduction. Pool: " + 
                                  std::to_string(actualMinThreads) + "-" + std::to_string(actualMaxThreads) + " threads.");
        
        process_reduce_internal(mappedData, reducedData, actualMinThreads, actualMaxThreads);
        
        Logger::getInstance().log("ReducerDLLso: Finished reduction.");
    }

protected:
    // Internal processing logic for reduce, using configured thread pool sizes
    void process_reduce_internal(
        const std::vector<std::pair<std::string, int>>& mappedData, 
        std::map<std::string, int>& reducedData,
        size_t minThreads, 
        size_t maxThreads
    ) {
        std::mutex reduce_mutex; 
        
        size_t chunkSize = calculate_dynamic_chunk_size(mappedData.size(), maxThreads);
        ThreadPool pool(minThreads, maxThreads);

        for (size_t i = 0; i < mappedData.size(); i += chunkSize) {
            pool.enqueueTask([this, &mappedData, &reducedData, &reduce_mutex, i, chunkSize]() { 
                size_t startIdx = i;
                size_t endIdx = std::min(startIdx + chunkSize, mappedData.size());
                std::map<std::string, int> localReduceMap; 

                for (size_t j = startIdx; j < endIdx; ++j) {
                    localReduceMap[mappedData[j].first] += mappedData[j].second;
                }

                if (!localReduceMap.empty()) {
                    std::lock_guard<std::mutex> lock(reduce_mutex);
                    for (const auto &kv : localReduceMap) {
                        reducedData[kv.first] += kv.second;
                    }
                }
            });
        }
        pool.shutdown(); 
    }

    // Removed local report_error, will use ErrorHandler::reportError directly

    size_t calculate_dynamic_chunk_size(size_t totalSize, size_t guideMaxThreads = 0) const {
        size_t numEffectiveThreads = guideMaxThreads > 0 ? guideMaxThreads : std::thread::hardware_concurrency();
        if (numEffectiveThreads == 0) numEffectiveThreads = FALLBACK_REDUCE_THREAD_COUNT;
        
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