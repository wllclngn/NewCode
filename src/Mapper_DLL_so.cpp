#include "..\include\Mapper_DLL_so.h"
#include "..\include\Partitioner.h"
#include "..\include\Logger.h"
#include "..\include\ErrorHandler.h"

#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>

Mapper::Mapper(Logger& loggerRef, ErrorHandler& errorHandlerRef)
    : logger(loggerRef), errorHandler(errorHandlerRef) {
    this->logger.log("Mapper instance created.", Logger::Level::DEBUG);
}

Mapper::~Mapper() {
    this->logger.log("Mapper instance destroyed.", Logger::Level::DEBUG);
}

void Mapper::map(const std::string& documentId, const std::string& line, std::vector<std::pair<std::string, int>>& intermediateData) {
    if (line.empty()) return;

    std::istringstream iss(line);
    std::string word;

    while (iss >> word) {
        word.erase(std::remove_if(word.begin(), word.end(), [](unsigned char c) { return std::ispunct(c); }), word.end());
        std::transform(word.begin(), word.end(), word.begin(), [](unsigned char c) { return std::tolower(c); });
        if (!word.empty()) intermediateData.push_back({word, 1});
    }
}

bool Mapper::exportPartitionedData(const std::string& tempDir, const std::vector<std::pair<std::string, int>>& mappedData, int numReducers) {
    Partitioner partitioner(numReducers);
    std::unordered_map<int, std::ofstream> reducerFiles;

    // Open a file for each reducer
    for (int i = 0; i < numReducers; ++i) {
        std::string filePath = tempDir + "/partition_" + std::to_string(i) + ".txt";
        reducerFiles[i].open(filePath, std::ios::app);
        if (!reducerFiles[i].is_open()) {
            errorHandler.logError("Mapper: Could not open partition file for reducer " + std::to_string(i) + ": " + filePath);
            return false;
        }
    }

    // Write data to the appropriate partition file
    for (const auto& pair : mappedData) {
        int bucket = partitioner.getReducerBucket(pair.first);
        reducerFiles[bucket] << pair.first << "\t" << pair.second << "\n";
    }

    // Close all files
    for (auto& [_, outFile] : reducerFiles) {
        outFile.close();
        if (outFile.fail()) {
            errorHandler.logError("Mapper: Failed to properly close partition file.");
            return false;
        }
    }

    return true;
}