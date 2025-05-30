#ifndef REDUCER_DLL_SO_H
#define REDUCER_DLL_SO_H

#include <map>
#include <vector>
#include <string>

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
    );

protected:
    // Internal processing logic for reduce
    void process_reduce_internal(
        const std::vector<std::pair<std::string, int>>& mappedData, 
        std::map<std::string, int>& reducedData,
        size_t minThreads, 
        size_t maxThreads
    );

    // Dynamic chunk size calculation
    size_t calculate_dynamic_chunk_size(size_t totalSize, size_t guideMaxThreads = 0) const;
};

#endif // REDUCER_DLL_SO_H