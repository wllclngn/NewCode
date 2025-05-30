#include "Mapper_DLL_so.h"
#include "Logger.h"
#include "ErrorHandler.h"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>
#include <cctype>

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

bool Mapper::exportMappedData(const std::string& filePath, const std::vector<std::pair<std::string, int>>& mappedData) {
    std::ofstream outFile(filePath, std::ios_base::app);

    if (!outFile.is_open()) {
        errorHandler.logError("Mapper: Could not open intermediate file for writing: " + filePath);
        return false;
    }

    for (const auto& pair : mappedData) {
        outFile << "(\"" << pair.first << "\"," << pair.second << ")\n";
    }

    if (outFile.bad()) {
        errorHandler.logError("Mapper: Error occurred while writing to intermediate file: " + filePath);
        outFile.close();
        return false;
    }

    outFile.close();

    if (outFile.fail() && !outFile.eof()) {
        errorHandler.logError("Mapper: Failed to properly close or write to intermediate file: " + filePath);
        return false;
    }

    return true;
}