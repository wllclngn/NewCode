#ifdef _WIN32
    #include "..\include\ProcessOrchestrator.h"
    #include "..\include\Logger.h"
    #include "..\include\ERROR_Handler.h"
    #include "..\include\ThreadPool.h"
#elif defined(__unix__) || defined(__APPLE__) && defined(__MACH__)
    #include "../include/ProcessOrchestrator.h"
    #include "../include/Logger.h"
    #include "../include/ERROR_Handler.h"
    #include "../include/ThreadPool.h"
#else
    #error "Unsupported operating system. Please utilize Windows, MacOS, or any Linux distribution to operate this C++ program."
#endif

#include <fstream>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// Implementation of ProcessOrchestratorDLL class
void ProcessOrchestratorDLL::runFinalReducer(const std::string& outputDir, const std::string& tempDir) {
    Logger::getInstance().log("Starting final reduction...");

    std::map<std::string, int> finalResults;

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

// Function to start the orchestration process
void ProcessOrchestratorDLL::start([[maybe_unused]] const std::string& tempDir, 
                                   [[maybe_unused]] size_t minPoolThreads, 
                                   [[maybe_unused]] size_t maxPoolThreads) {
    // Your implementation
    Logger::getInstance().log("Starting process orchestration...");
}

// Function to run the mapper
bool ProcessOrchestratorDLL::runMapper([[maybe_unused]] const std::string& tempDir,
                                       [[maybe_unused]] int mapperId,
                                       [[maybe_unused]] int numReducers,
                                       [[maybe_unused]] const std::vector<std::string>& inputFilePaths,
                                       [[maybe_unused]] size_t minPoolThreads,
                                       [[maybe_unused]] size_t maxPoolThreads) {
    Logger::getInstance().log("Running mapper (simulation)...");
    return true; // Temporary return value to satisfy the compiler
}

// Function to run the reducer
bool ProcessOrchestratorDLL::runReducer([[maybe_unused]] const std::string& outputDir,
                                        [[maybe_unused]] const std::string& tempDir,
                                        [[maybe_unused]] int reducerId,
                                        [[maybe_unused]] size_t minPoolThreads,
                                        [[maybe_unused]] size_t maxPoolThreads) {
    Logger::getInstance().log("Running reducer (simulation)...");
    return true; // Temporary return value to satisfy the compiler
}