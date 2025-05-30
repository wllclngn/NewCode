#ifdef _WIN32
    #include "..\include\Reducer_DLL_so.h"
    #include "..\include\ThreadPool.h"
    #include "..\include\ERROR_Handler.h"
    #include "..\include\Logger.h"
#elif defined(__unix__) || defined(__APPLE__) && defined(__MACH__)
    #include "../include/Reducer_DLL_so.h"
    #include "../include/ThreadPool.h"
    #include "../include/ERROR_Handler.h"
    #include "../include/Logger.h"
#else
    #error "Unsupported operating system. Please check your platform."
#endif

#include <mutex>
#include <thread>
#include <algorithm>
#include <map>
#include <vector>
#include <string>
#include <cstddef>

void ReducerDLLso::reduce(
    const std::vector<std::pair<std::string, int>>& mappedData,
    std::map<std::string, int>& reducedData,
    size_t minPoolThreadsConfig,
    size_t maxPoolThreadsConfig
) {
    process_reduce_internal(mappedData, reducedData, minPoolThreadsConfig, maxPoolThreadsConfig);
}

void ReducerDLLso::process_reduce_internal(
    const std::vector<std::pair<std::string, int>>& mappedData,
    std::map<std::string, int>& reducedData,
    size_t minThreads,
    size_t maxThreads
) {
    // Validate thread configurations
    size_t actualMinThreads = (minThreads == 0) ? std::thread::hardware_concurrency() : minThreads;
    if (actualMinThreads == 0) actualMinThreads = 2; // Fallback value

    size_t actualMaxThreads = (maxThreads == 0) ? actualMinThreads : maxThreads;
    if (actualMaxThreads < actualMinThreads) actualMaxThreads = actualMinThreads;

    Logger::getInstance().log("ReducerDLLso: Starting reduction with threads: " +
                              std::to_string(actualMinThreads) + " to " + std::to_string(actualMaxThreads));

    // Perform reduction logic here
    for (const auto& pair : mappedData) {
        reducedData[pair.first] += pair.second;
    }

    Logger::getInstance().log("ReducerDLLso: Reduction completed.");
}