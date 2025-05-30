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
    static constexpr size_t FALLBACK_REDUCE_THREAD_COUNT = 2;
    virtual ~ReducerDLLso() {}

    virtual void reduce(
        const std::vector<std::pair<std::string, int>>& mappedData,
        std::map<std::string, int>& reducedData,
        size_t minPoolThreadsConfig = 0,
        size_t maxPoolThreadsConfig = 0
    );

    void process_reduce_internal(
    const std::vector<std::pair<std::string, int>>& mappedData,
    std::map<std::string, int>& reducedData,
    size_t minThreads,
    size_t maxThreads);

protected:protected:
    void process_reduce_internal(
        const std::vector<std::pair<std::string, int>>& mappedData,
        std::map<std::string, int>& reducedData,
        size_t minThreads,
        size_t maxThreads
    );

    size_t calculate_dynamic_chunk_size(size_t totalSize, size_t guideMaxThreads = 0) const;
};
#endif // REDUCER_DLL_SO_H