#ifndef PROCESS_ORCHESTRATOR_DLL_H
#define PROCESS_ORCHESTRATOR_DLL_H

#include <string>
#include <vector>
#include <map>
#include <filesystem>

#include "FileHandler.h"   
#include "Logger.h"        
#include "ERROR_Handler.h"  
#include "Mapper_DLL_so.h" 
#include "Reducer_DLL_so.h"

#if defined(_WIN32) || defined(_WIN64)
#define ORCH_DLL_EXPORT __declspec(dllexport)
#elif defined(__linux__) || defined(__unix__)
#define ORCH_DLL_EXPORT __attribute__((visibility("default")))
#else
#define ORCH_DLL_EXPORT
#endif

namespace fs = std::filesystem;

class ORCH_DLL_EXPORT ProcessOrchestratorDLL {
public:
    static constexpr size_t DEFAULT_MIN_THREADS = 0; 
    static constexpr size_t DEFAULT_MAX_THREADS = 0; 
    static constexpr size_t FALLBACK_THREAD_COUNT = 2; 

    virtual ~ProcessOrchestratorDLL();
    virtual int launchProcess(const std::string& command);
    virtual bool runController(const std::string& executablePath,
                               const std::string& inputDir,
                               const std::string& outputDir,
                               const std::string& tempDir,
                               int numMappers,
                               int numReducers,
                               size_t mapperMinPoolThreads = DEFAULT_MIN_THREADS,
                               size_t mapperMaxPoolThreads = DEFAULT_MAX_THREADS,
                               size_t reducerMinPoolThreads = DEFAULT_MIN_THREADS,
                               size_t reducerMaxPoolThreads = DEFAULT_MAX_THREADS);
    virtual bool runMapper(const std::string& tempDir,
                           int mapperId,
                           int numReducers,
                           const std::vector<std::string>& inputFilePaths,
                           size_t minPoolThreads = DEFAULT_MIN_THREADS,
                           size_t maxPoolThreads = DEFAULT_MAX_THREADS);
    virtual bool runReducer(const std::string& outputDir,
                            const std::string& tempDir,
                            int reducerId,
                            size_t minPoolThreads = DEFAULT_MIN_THREADS,
                            size_t maxPoolThreads = DEFAULT_MAX_THREADS);

    // Moved runFinalReducer to public section
    void runFinalReducer(const std::string& outputDir, const std::string& tempDir);

private:
    size_t resolveDefaultThreads() const;
    std::string formatThreadCount(size_t count) const;
    std::string buildCommandString_impl(const std::string& executablePath, 
                                        const std::string& mode, 
                                        const std::vector<std::string>& args,
                                        const std::vector<std::string>& fileArgs = {}) const;
    bool setupDirectories_impl(const std::string& inputDir, const std::string& outputDir, const std::string& tempDir) const;
    bool distributeInputFiles_impl(const std::string& inputDir, int numMappers, std::vector<std::vector<std::string>>& mapperFileAssignments) const;
    bool launchMapperProcesses_impl(const std::string& executablePath, 
                                    const std::string& tempDir, 
                                    int numMappers, 
                                    int numReducers,
                                    const std::vector<std::vector<std::string>>& mapperFileAssignments,
                                    size_t minPoolThreads, 
                                    size_t maxPoolThreads);
    bool launchReducerProcesses_impl(const std::string& executablePath,
                                     const std::string& outputDir,
                                     const std::string& tempDir,
                                     int numReducers,
                                     size_t minPoolThreads,
                                     size_t maxPoolThreads);
    bool performFinalAggregation_impl(const std::string& outputDir, int numReducers, const std::string& finalOutputFilename) const;
};

#endif // PROCESS_ORCHESTRATOR_DLL_H