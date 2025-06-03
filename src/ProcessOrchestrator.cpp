#ifdef _WIN32
    #include "..\include\ProcessOrchestrator.h"
    #include "..\include\Logger.h"
    #include "..\include\ERROR_Handler.h"
    #include "..\include\ThreadPool.h"
    #include "..\include\FileHandler.h"
    #include "..\include\Mapper_DLL_so.h"
    #include "..\include\Reducer_DLL_so.h"
#elif defined(__unix__) || defined(__APPLE__) && defined(__MACH__)
    #include "../include/ProcessOrchestrator.h"
    #include "../include/Logger.h"
    #include "../include/ERROR_Handler.h"
    #include "../include/ThreadPool.h"
    #include "../include/FileHandler.h"
    #include "../include/Mapper_DLL_so.h"
    #include "../include/Reducer_DLL_so.h"
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
    Logger& logger = Logger::getInstance();
    logger.log("Starting final reduction...");

    // Ensure output directory exists
    if (!fs::exists(outputDir)) {
        try {
            fs::create_directories(outputDir);
        } catch (const fs::filesystem_error&) {
            logger.log("Failed to create output directory", Logger::Level::ERROR);
            return;
        }
    }

    // Find all reducer output files and perform final aggregation
    std::map<std::string, int> finalResults;
    std::map<std::string, std::vector<int>> finalVectorResults;

    try {
        for (const auto& entry : fs::directory_iterator(outputDir)) {
            if (entry.is_regular_file() && entry.path().filename().string().find("reducer_") == 0) {
                std::ifstream inFile(entry.path());
                std::string line;
                while (std::getline(inFile, line)) {
                    size_t colonPos = line.find(": ");
                    if (colonPos != std::string::npos) {
                        std::string key = line.substr(0, colonPos);
                        int value = std::stoi(line.substr(colonPos + 2));
                        finalResults[key] += value;
                        finalVectorResults[key].push_back(value);
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        logger.log("Error in final aggregation: " + std::string(e.what()), Logger::Level::ERROR);
    }

    // Write outputs
    FileHandler::write_output(outputDir + "/output.txt", finalResults);
    FileHandler::write_summed_output(outputDir + "/output_summed.txt", finalVectorResults);

    logger.log("Final reduction completed.");
}

// Function to run the mapper
bool ProcessOrchestratorDLL::runMapper(const std::string& tempDir,
                                      int mapperId,
                                      int numReducers,
                                      const std::vector<std::string>& inputFilePaths,
                                      size_t minPoolThreads,
                                      size_t maxPoolThreads) {
    Logger& logger = Logger::getInstance();
    logger.log("Running mapper " + std::to_string(mapperId) + " with " + std::to_string(inputFilePaths.size()) + " files");
    
    // Ensure temp directory exists
    if (!fs::exists(tempDir) && !fs::create_directories(tempDir)) {
        logger.log("Failed to create temp directory", Logger::Level::ERROR);
        return false;
    }

    // Initialize and process
    ErrorHandler errorHandler;
    Mapper mapper(logger, errorHandler);
    std::vector<std::pair<std::string, int>> mappedData;
    
    // Process all input files
    for (const auto& filePath : inputFilePaths) {
        std::vector<std::string> lines;
        if (FileHandler::read_file(filePath, lines)) {
            for (const auto& line : lines) {
                mapper.map(filePath, line, mappedData);
            }
        }
    }
    
    // Export partitioned data
    bool success = mapper.exportPartitionedData(tempDir, mappedData, numReducers, "partition_", ".txt");
    logger.log(success ? "Mapper completed successfully" : "Mapper failed to export data", 
              success ? Logger::Level::INFO : Logger::Level::ERROR);
    
    return success;
}

// Function to run the reducer
bool ProcessOrchestratorDLL::runReducer(const std::string& outputDir,
                                       const std::string& tempDir,
                                       int reducerId,
                                       size_t minPoolThreads,
                                       size_t maxPoolThreads) {
    Logger& logger = Logger::getInstance();
    logger.log("Running reducer " + std::to_string(reducerId));
    
    // Ensure output directory exists
    if (!fs::exists(outputDir) && !fs::create_directories(outputDir)) {
        logger.log("Failed to create output directory", Logger::Level::ERROR);
        return false;
    }

    // Find all partition files for this reducer
    std::vector<std::string> partitionFiles;
    std::vector<std::pair<std::string, int>> allMappedData;
    
    try {
        for (const auto& entry : fs::directory_iterator(tempDir)) {
            if (entry.is_regular_file()) {
                std::string fname = entry.path().filename().string();
                if (fname.find("_" + std::to_string(reducerId) + ".") != std::string::npos) {
                    std::vector<std::pair<std::string, int>> mappedData;
                    FileHandler::read_mapped_data(entry.path().string(), mappedData);
                    allMappedData.insert(allMappedData.end(), mappedData.begin(), mappedData.end());
                }
            }
        }
    } catch (const fs::filesystem_error&) {
        logger.log("Error scanning temp directory", Logger::Level::ERROR);
        return false;
    }

    if (allMappedData.empty()) {
        logger.log("No data found for reducer " + std::to_string(reducerId), Logger::Level::WARNING);
        return true;
    }

    // Perform reduction
    std::map<std::string, int> reducedData;
    ReducerDLLso reducer;
    reducer.reduce(allMappedData, reducedData, minPoolThreads, maxPoolThreads);

    // Write output
    std::string outputPath = (fs::path(outputDir) / ("reducer_" + std::to_string(reducerId) + ".txt")).string();
    bool success = FileHandler::write_output(outputPath, reducedData);
    
    logger.log(success ? "Reducer completed successfully" : "Failed to write reducer output", 
              success ? Logger::Level::INFO : Logger::Level::ERROR);
    
    return success;
}

