#ifndef PROCESS_ORCHESTRATOR_H
#define PROCESS_ORCHESTRATOR_H

#include <string>
#include <vector>

class ProcessOrchestratorDLL {
public:
    static constexpr size_t DEFAULT_MIN_THREADS = 0;
    static constexpr size_t DEFAULT_MAX_THREADS = 0;

    // Function to run the final reducer
    void runFinalReducer(const std::string& outputDir, const std::string& tempDir);

    // Function to run the mapper
    bool runMapper(const std::string& tempDir,
                   int mapperId,
                   int numReducers,
                   const std::vector<std::string>& inputFilePaths,
                   size_t minPoolThreads = DEFAULT_MIN_THREADS,
                   size_t maxPoolThreads = DEFAULT_MAX_THREADS);

    // Function to run the reducer
    bool runReducer(const std::string& outputDir,
                    const std::string& tempDir,
                    int reducerId,
                    size_t minPoolThreads = DEFAULT_MIN_THREADS,
                    size_t maxPoolThreads = DEFAULT_MAX_THREADS);

private:
    // Private helper functions
    size_t resolveDefaultThreads() const;
    std::string formatThreadCount(size_t count) const;

    std::string buildCommandString_impl(const std::string& command,
                                        const std::vector<std::string>& args) const;

    void setupDirectories_impl(const std::string& tempDir);
    void distributeInputFiles_impl(const std::string& inputDir,
                                    const std::string& tempDir,
                                    int numMappers);

    void performFinalAggregation_impl(const std::string& tempDir,
                                      const std::string& outputDir);
};

#endif // PROCESS_ORCHESTRATOR_H