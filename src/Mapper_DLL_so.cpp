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

    std::istringstream iss(line);
    std::string word;

    while (iss >> word) {
        // Remove punctuation and convert to lowercase
        word.erase(std::remove_if(word.begin(), word.end(), [](unsigned char c) {
            return std::ispunct(c);
        }), word.end());
        std::transform(word.begin(), word.end(), word.begin(), [](unsigned char c) {
            return std::tolower(c);
        });

        if (!word.empty()) {
            intermediateData.push_back({word, 1});
        }
    }
}

// Export mapped data to a file
bool Mapper::exportMappedData(const std::string& filePath, const std::vector<std::pair<std::string, int>>& mappedData) {
    std::ofstream outFile(filePath, std::ios::trunc);
    if (!outFile.is_open()) {
        errorHandler.reportError("Failed to open file for exporting mapped data: " + filePath, false);
        return false;
    }

    for (const auto& pair : mappedData) {
        outFile << pair.first << "\t" << pair.second << "\n";
    }

    outFile.close();
    if (outFile.fail()) {
        errorHandler.reportError("Failed to properly close file: " + filePath, false);
        return false;
    }

    return true;
}

// Export partitioned data into temporary files for reducers
bool Mapper::exportPartitionedData(const std::string& tempDir, const std::vector<std::pair<std::string, int>>& mappedData, int numReducers) {
    Partitioner partitioner(numReducers);
    std::unordered_map<int, std::ofstream> reducerFiles;

    // Open a file for each reducer
    for (int i = 0; i < numReducers; ++i) {
        std::string filePath = tempDir + "/partition_" + std::to_string(i) + ".txt";
        reducerFiles[i].open(filePath, std::ios::app);

        if (!reducerFiles[i].is_open()) {
            errorHandler.reportError("Mapper: Could not open partition file for reducer " + std::to_string(i) + ": " + filePath, false);
            return false;
        }
    }

    // Write mapped data to the appropriate partition file
    for (const auto& pair : mappedData) {
        int bucket = partitioner.getReducerBucket(pair.first);
        reducerFiles[bucket] << pair.first << "\t" << pair.second << "\n";
    }

    // Close all files
    for (auto& [_, outFile] : reducerFiles) {
        outFile.close();
        if (outFile.fail()) {
            errorHandler.reportError("Mapper: Failed to properly close partition file.", false);
            return false;
        }
    }

    return true;
}