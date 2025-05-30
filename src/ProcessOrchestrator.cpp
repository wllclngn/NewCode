#ifdef _WIN32
    #include "..\\include\\ProcessOrchestrator.h"
    #include "..\\include\\Logger.h"
    #include "..\\include\\ERROR_Handler.h"
    #include "..\\include\\ThreadPool.h"
#elif defined(__unix__) || defined(__APPLE__) && defined(__MACH__)
    #include "../include/ProcessOrchestrator.h"
    #include "../include/Logger.h"
    #include "../include/ERROR_Handler.h"
    #include "../include/ThreadPool.h"
#else
    #error "Unsupported operating system. Please utilize Windows, MacOS or any Linux distribution to operate this C++ program."
#endif

#include <fstream>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// Implementation of the ProcessOrchestratorDLL class
void ProcessOrchestratorDLL::runFinalReducer(const std::string& outputDir, const std::string& tempDir) {
    Logger::getInstance().log("Starting final reduction...");

    // Example logic for final reduction
    std::map<std::string, int> finalResults;

    // Read intermediate reducer outputs
    for (const auto& entry : fs::directory_iterator(tempDir)) {
        if (entry.path().extension() == ".txt") {
            std::ifstream inFile(entry.path());
            std::string key;
            int value;

            while (inFile >> key >> value) {
                finalResults[key] += value;
            }
        }
    }

    // Write final results to output directory
    std::ofstream outFile(outputDir + "/final_results.txt");
    if (!outFile.is_open()) {
        Logger::getInstance().log("ERROR: Could not open final_results.txt for writing.", Logger::Level::ERROR);
        return;
    }

    for (const auto& [key, value] : finalResults) {
        outFile << key << "\t" << value << "\n";
    }

    Logger::getInstance().log("Final reduction completed.");
}

// Example of a function with unused parameters — updated to use [[maybe_unused]]
void ProcessOrchestratorDLL::runMapper([[maybe_unused]] const std::string& tempDir,
                                       [[maybe_unused]] int mapperId,
                                       [[maybe_unused]] int numReducers,
                                       [[maybe_unused]] const std::vector<std::string>& inputFilePaths,
                                       [[maybe_unused]] size_t minPoolThreads,
                                       [[maybe_unused]] size_t maxPoolThreads) {
    Logger::getInstance().log("Running mapper (simulation)...");
}

// Another example of a function with unused parameters — updated to use [[maybe_unused]]
void ProcessOrchestratorDLL::runReducer([[maybe_unused]] const std::string& outputDir,
                                        [[maybe_unused]] const std::string& tempDir,
                                        [[maybe_unused]] int reducerId,
                                        [[maybe_unused]] size_t minPoolThreads,
                                        [[maybe_unused]] size_t maxPoolThreads) {
    Logger::getInstance().log("Running reducer (simulation)...");
}