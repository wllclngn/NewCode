#include "..\\include\\Reducer_DLL_so.h"
#include "..\\include\\ThreadPool.h"
#include "..\\include\\ERROR_Handler.h"
#include "..\\include\\Logger.h"

#include <mutex>
#include <thread>
#include <algorithm>
#include <map>
#include <vector>
#include <string>
#include <cstddef>

void ReducerDLLso::process_reduce_internal(
    const std::vector<std::pair<std::string, int>>& mappedData,
    std::map<std::string, int>& reducedData,
    size_t minThreads,
    size_t maxThreads) {    const std::vector<std::pair<std::string, int>>& mappedData,
    std::map<std::string, int>& reducedData,
    size_t minPoolThreadsConfig,
    size_t maxPoolThreadsConfig) {
    size_t actualMinThreads = minPoolThreadsConfig == 0 ? std::thread::hardware_concurrency() : minPoolThreadsConfig;
    if (actualMinThreads == 0) actualMinThreads = FALLBACK_REDUCE_THREAD_COUNT;

    size_t actualMaxThreads = maxPoolThreadsConfig == 0 ? actualMinThreads : maxPoolThreadsConfig;
    if (actualMaxThreads < actualMinThreads) actualMaxThreads = actualMinThreads;

    Logger::getInstance().log("ReducerDLLso: Starting reduction.");

    process_reduce_internal(mappedData, reducedData, actualMinThreads, actualMaxThreads);
    Logger::getInstance().log("ReducerDLLso: Finished reduction.");
}

void ReducerDLLso::process_reduce_internal(
    const std::vector<std::pair<std::string, int>>& mappedData,
    std::map<std::string, int>& reducedData,
    size_t minThreads,
    size_t maxThreads) {
}