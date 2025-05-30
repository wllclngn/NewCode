#include "..\include\Reducer_DLL_so.h"
#include "..\include\ThreadPool.h"
#include "..\include\ErrorHandler.h"
#include "..\include\Logger.h"

#include <mutex>
#include <thread>
#include <algorithm>
#include <map>
#include <vector>
#include <string>

// Reduce method implementation
void ReducerDLLso::reduce(
    const std::vector<std::pair<std::string, int>>& mappedData,
    std::map<std::string, int>& reducedData,
    size_t minPoolThreadsConfig,
    size_t maxPoolThreadsConfig) {
    size_t actualMinThreads = minPoolThreadsConfig == 0 ? std::thread::hardware_concurrency() : minPoolThreadsConfig;
    if (actualMinThreads == 0) actualMinThreads = FALLBACK_REDUCE_THREAD_COUNT;

    size_t actualMaxThreads = maxPoolThreadsConfig == 0 ? actualMinThreads : maxPoolThreadsConfig;
    if (actualMaxThreads < actualMinThreads) actualMaxThreads = actualMinThreads;

    Logger::getInstance().log("ReducerDLLso: Starting reduction. Pool: " +
                              std::to_string(actualMinThreads) + "-" + std::to_string(actualMaxThreads) + " threads.");

    process_reduce_internal(mappedData, reducedData, actualMinThreads, actualMaxThreads);

    Logger::getInstance().log("ReducerDLLso: Finished reduction.");
}

// Internal processing logic for reduce
void ReducerDLLso::process_reduce_internal(
    const std::vector<std::pair<std::string, int>>& mappedData,
    std::map<std::string, int>& reducedData,
    size_t minThreads,
    size_t maxThreads) {
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
                for (const auto& kv : localReduceMap) {
                    reducedData[kv.first] += kv.second;
                }
            }
        });
    }
    pool.shutdown();
}

// Dynamic chunk size calculation
size_t ReducerDLLso::calculate_dynamic_chunk_size(size_t totalSize, size_t guideMaxThreads) const {
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