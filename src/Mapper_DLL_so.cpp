#ifdef _WIN32
    #include "..\include\Mapper_DLL_so.h"
    #include "..\include\Partitioner.h"
    #include "..\include\Logger.h"
    #include "..\include\ERROR_Handler.h"
#elif defined(__unix__) || defined(__APPLE__) && defined(__MACH__)
    #include "../include/Mapper_DLL_so.h"
    #include "../include/Partitioner.h"
    #include "../include/Logger.h"
    #include "../include/ERROR_Handler.h"
#else
    #error "Unsupported operating system. Please utilize Windows, MacOS, or any Linux distribution to operate this C++ program."
#endif

#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <filesystem> // Required for creating directories if tempDir doesn't exist

namespace fs = std::filesystem;

// Constructor for the Mapper class
Mapper::Mapper(Logger& loggerRef, ErrorHandler& errorHandlerRef)
    : logger(loggerRef), errorHandler(errorHandlerRef) {
    this->logger.log("[DEBUG] Mapper instance created.");
}

// Destructor for the Mapper class
Mapper::~Mapper() {
    this->logger.log("[DEBUG] Mapper instance destroyed.");
}

// Perform the map operation on a single line of input
void Mapper::map(const std::string& documentId, const std::string& line, std::vector<std::pair<std::string, int>>& intermediateData) {
    if (line.empty()) return;
    
    // Use documentId to avoid unused parameter warning
    if(documentId.empty() && line.empty()) return;

    std::istringstream iss(line);
    std::string word;

    while (iss >> word) {
        // Remove punctuation and convert to lowercase
        word.erase(std::remove_if(word.begin(), word.end(), [](unsigned char c) {
            return std::ispunct(c);
        }), word.end());
        
        // Fix for warning: conversion from int to char
        std::transform(word.begin(), word.end(), word.begin(), [](unsigned char c) -> unsigned char {
            return static_cast<unsigned char>(std::tolower(static_cast<unsigned int>(c)));
        });

        if (!word.empty()) {
            intermediateData.push_back({word, 1});
        }
    }
}

// Export mapped data to a file
bool Mapper::exportMappedData(const std::string& filePath, const std::vector<std::pair<std::string, int>>& mappedData) {
    // Ensure directory for filePath exists
    fs::path p(filePath);
    if (p.has_parent_path()) {
        fs::path dir = p.parent_path();
        if (!fs::exists(dir)) {
            if (!fs::create_directories(dir)) {
                errorHandler.reportError("Failed to create directory for exporting mapped data: " + dir.string(), false);
                return false;
            }
        }
    }

    std::ofstream outFile(filePath, std::ios::trunc); // Changed to trunc to overwrite if file exists
    if (!outFile.is_open()) {
        errorHandler.reportError("Failed to open file for exporting mapped data: " + filePath, false);
        return false;
    }

    for (const auto& pair : mappedData) {
        outFile << pair.first << "\t" << pair.second << "\n";
    }

    outFile.close();
    if (outFile.fail()) {
        errorHandler.reportError("Failed to properly close file after writing mapped data: " + filePath, false);
        return false;
    }
    logger.log("Successfully exported mapped data to: " + filePath);
    return true;
}

// Export partitioned data into temporary files for reducers
bool Mapper::exportPartitionedData(const std::string& tempDir, 
                                  const std::vector<std::pair<std::string, int>>& mappedData, 
                                  int numReducers,
                                  const std::string& partitionFilePrefix,
                                  const std::string& partitionFileSuffix) {
    if (numReducers <= 0) {
        errorHandler.reportError("Mapper: Number of reducers must be positive. Got: " + std::to_string(numReducers), true);
        return false;
    }
    
    // Ensure tempDir exists
    if (!fs::exists(tempDir)) {
        if (!fs::create_directories(tempDir)) {
            errorHandler.reportError("Mapper: Could not create temporary directory: " + tempDir, false);
            return false;
        }
    }

    Partitioner partitioner(numReducers);
    std::vector<std::ofstream> reducerFiles(numReducers); // Use vector instead of map for direct indexing

    // Open a file for each reducer
    for (int i = 0; i < numReducers; ++i) {
        // Use fs::path for robust path construction
        fs::path partitionFilePath = fs::path(tempDir) / (partitionFilePrefix + std::to_string(i) + partitionFileSuffix);
        reducerFiles[i].open(partitionFilePath, std::ios::app); // Open in append mode

        if (!reducerFiles[i].is_open()) {
            errorHandler.reportError("Mapper: Could not open partition file " + partitionFilePath.string() + " for reducer " + std::to_string(i), false);
            // Close already opened files before returning
            for (int j = 0; j < i; ++j) {
                if (reducerFiles[j].is_open()) {
                    reducerFiles[j].close();
                }
            }
            return false;
        }
    }

    // Write mapped data to the appropriate partition file
    for (const auto& pair : mappedData) {
        int bucket = partitioner.getReducerBucket(pair.first);
        if (bucket >= 0 && bucket < numReducers) {
            reducerFiles[bucket] << pair.first << "\t" << pair.second << "\n";
        } else {
            // This case should ideally not happen if Partitioner is correct
            errorHandler.reportError("Mapper: Invalid bucket " + std::to_string(bucket) + " for key '" + pair.first + "'", false);
        }
    }

    // Close all files
    bool allClosedSuccessfully = true;
    for (int i = 0; i < numReducers; ++i) {
        if (reducerFiles[i].is_open()) {
            reducerFiles[i].close();
            if (reducerFiles[i].fail()) {
                fs::path partitionFilePath = fs::path(tempDir) / (partitionFilePrefix + std::to_string(i) + partitionFileSuffix);
                errorHandler.reportError("Mapper: Failed to properly close partition file: " + partitionFilePath.string(), false);
                allClosedSuccessfully = false; 
            }
        }
    }
    
    if(allClosedSuccessfully) {
        logger.log("Successfully exported partitioned data to " + tempDir);
    }
    return allClosedSuccessfully;
}