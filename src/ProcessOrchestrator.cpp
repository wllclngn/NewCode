#include "..\include\ProcessOrchestrator.h"
#include "..\include\Logger.h"
#include "..\include\ERROR_Handler.h"
#include "..\include\ThreadPool.h"

#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

// Function to dynamically monitor mapper progress and start reducers
bool ProcessOrchestratorDLL::runController(const std::string& executablePath,
                                           const std::string& inputDir,
                                           const std::string& outputDir,
                                           const std::string& tempDir,
                                           int numMappers,
                                           int numReducers,
                                           size_t mapperMinPoolThreads,
                                           size_t mapperMaxPoolThreads,
                                           size_t reducerMinPoolThreads,
                                           size_t reducerMaxPoolThreads) {
    Logger::getInstance().log("[CONTROLLER] Starting controller process...");

    // Step 1: Distribute input files among mappers
    std::vector<std::vector<std::string>> mapperFileAssignments(numMappers);
    if (!distributeInputFiles_impl(inputDir, numMappers, mapperFileAssignments)) {
        Logger::getInstance().log("[CONTROLLER] Failed to distribute input files among mappers.");
        return false;
    }

    // Step 2: Launch mappers and monitor their progress
    std::vector<std::atomic<bool>> mapperCompletionFlags(numMappers);
    std::fill(mapperCompletionFlags.begin(), mapperCompletionFlags.end(), false);

    std::mutex reducerStartMutex;
    std::condition_variable reducerStartCV;
    std::atomic<int> completedMappers(0);

    // Launch mappers
    std::vector<std::thread> mapperThreads;
    for (int i = 0; i < numMappers; ++i) {
        mapperThreads.emplace_back([&, i]() {
            Logger::getInstance().log("[CONTROLLER] Launching Mapper " + std::to_string(i));
            if (launchMapperProcesses_impl(executablePath, tempDir, i, numReducers, mapperFileAssignments[i], mapperMinPoolThreads, mapperMaxPoolThreads)) {
                mapperCompletionFlags[i] = true;
                completedMappers.fetch_add(1);
                reducerStartCV.notify_all();
            } else {
                Logger::getInstance().log("[CONTROLLER] Mapper " + std::to_string(i) + " failed.");
            }
        });
    }

    // Step 3: Dynamically start reducers as mappers complete
    std::vector<std::atomic<bool>> reducerCompletionFlags(numReducers);
    std::fill(reducerCompletionFlags.begin(), reducerCompletionFlags.end(), false);

    std::vector<std::thread> reducerThreads;
    for (int r = 0; r < numReducers; ++r) {
        reducerThreads.emplace_back([&, r]() {
            std::unique_lock<std::mutex> lock(reducerStartMutex);
            reducerStartCV.wait(lock, [&]() {
                // Start reducer only if its partition is ready (at least one mapper has completed)
                for (int m = 0; m < numMappers; ++m) {
                    if (mapperCompletionFlags[m]) return true;
                }
                return false;
            });

            Logger::getInstance().log("[CONTROLLER] Launching Reducer " + std::to_string(r));
            if (launchReducerProcesses_impl(executablePath, outputDir, tempDir, r, reducerMinPoolThreads, reducerMaxPoolThreads)) {
                reducerCompletionFlags[r] = true;
                Logger::getInstance().log("[CONTROLLER] Reducer " + std::to_string(r) + " completed successfully.");
            } else {
                Logger::getInstance().log("[CONTROLLER] Reducer " + std::to_string(r) + " failed.");
            }
        });
    }

    // Step 4: Wait for all mappers to complete
    for (auto& thread : mapperThreads) {
        if (thread.joinable()) thread.join();
    }
    Logger::getInstance().log("[CONTROLLER] All mappers have completed.");

    // Step 5: Wait for all reducers to complete
    for (auto& thread : reducerThreads) {
        if (thread.joinable()) thread.join();
    }
    Logger::getInstance().log("[CONTROLLER] All reducers have completed.");

    return true;
}