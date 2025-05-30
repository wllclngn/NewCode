#ifndef REDUCER_DLL_SO_H
#define REDUCER_DLL_SO_H

#include "ExportDefinitions.h"
#include <map>
#include <vector>
#include <string>

class DLL_so_EXPORT ReducerDLLso {
public:
    static constexpr size_t FALLBACK_REDUCE_THREAD_COUNT = 2;

    virtual ~ReducerDLLso() {}

    virtual void reduce(
        const std::vector<std::pair<std::string, int>>& mappedData,
        std::map<std::string, int>& reducedData,
        size_t minPoolThreadsConfig = 0,
        size_t maxPoolThreadsConfig = 0
    );

protected:
    void process_reduce_internal(
        const std::vector<std::pair<std::string, int>>& mappedData,
        std::map<std::string, int>& reducedData,
        size_t minThreads,
        size_t maxThreads
    );

    size_t calculate_dynamic_chunk_size(size_t totalSize, size_t guideMaxThreads = 0) const;
};

#endif // REDUCER_DLL_SO_H