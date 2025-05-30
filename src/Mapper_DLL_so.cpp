#include "Mapper_DLL_so.h"  // The header file that declares the Mapper class
#include "Logger.h"         // For logging functionality
#include "ErrorHandler.h"   // For error handling functionality

#include <fstream>          // For std::ofstream (file output)
#include <sstream>          // For std::istringstream (string parsing)
#include <string>           // For std::string manipulation
#include <vector>           // For std::vector
#include <utility>          // For std::pair
#include <algorithm>        // For std::transform, std::remove_if
#include <cctype>           // For ::ispunct, ::tolower

// Note: DLL_so_EXPORTS is NOT defined in this .cpp file.
// It is expected to be defined as a preprocessor directive by the build system
// (e.g., compiler flag /DDLL_so_EXPORTS or -DDLL_so_EXPORTS)
// when compiling this specific .cpp file into the MapperLib DLL/SO.
// This ensures that DLL_so_EXPORT in Mapper_DLL_so.h expands correctly
// to __declspec(dllexport) or __attribute__((visibility("default"))).

// Constructor Implementation
Mapper::Mapper(Logger& loggerRef, ErrorHandler& errorHandlerRef)
    : logger(loggerRef), errorHandler(errorHandlerRef) {
    // Use the 'this->' prefix if you prefer, or directly if no ambiguity
    // For references, they are initialized in the member initializer list above.
    this->logger.log("Mapper instance created.", Logger::Level::DEBUG);
}

// Destructor Implementation
Mapper::~Mapper() {
    this->logger.log("Mapper instance destroyed.", Logger::Level::DEBUG);
}

// map method implementation
void Mapper::map(const std::string& documentId, const std::string& line, std::vector<std::pair<std::string, int>>& intermediateData) {
    if (line.empty()) {
        // Optionally log or handle empty lines if necessary
        // logger.log("Mapper::map received an empty line from document: " + documentId, Logger::Level::TRACE);
        return;
    }

    std::istringstream iss(line);
    std::string word;

    // Tokenize the line by whitespace
    while (iss >> word) {
        // Basic sanitization:
        // 1. Remove punctuation from the beginning and end.
        // A more robust way might be to iterate and build a new string.
        // This example removes all punctuation.
        word.erase(std::remove_if(word.begin(), word.end(), 
            [](unsigned char c){ return std::ispunct(c); }), 
            word.end());

        // 2. Convert to lower case.
        std::transform(word.begin(), word.end(), word.begin(),
            [](unsigned char c){ return std::tolower(c); });

        if (!word.empty()) {
            intermediateData.push_back({word, 1}); // Emit (word, 1)
            // logger.log("Mapper: Emitted (\"" + word + "\", 1) from doc: " + documentId, Logger::Level::TRACE);
        }
    }
}

// exportMappedData method implementation
bool Mapper::exportMappedData(const std::string& filePath, const std::vector<std::pair<std::string, int>>& mappedData) {
    // Open the file in append mode, so multiple calls (e.g., from different map tasks)
    // can write to the same intermediate file if needed, or it can create a new file.
    std::ofstream outFile(filePath, std::ios_base::app);

    if (!outFile.is_open()) {
        errorHandler.logError("Mapper: Could not open intermediate file for writing: " + filePath);
        return false;
    }

    for (const auto& pair : mappedData) {
        // Output format: ("key",value) - ensure this is consistent with what the
        // shuffle/sort/reduce phase expects.
        outFile << "(\"" << pair.first << "\"," << pair.second << ")\n";
    }

    if (outFile.bad()) {
        errorHandler.logError("Mapper: Error occurred while writing to intermediate file: " + filePath);
        outFile.close(); // Attempt to close even on error
        return false;
    }
    
    outFile.close();

    if (outFile.fail() && !outFile.eof()) { // Check for errors after closing, excluding eof if it was just an empty write then close.
        errorHandler.logError("Mapper: Failed to properly close or write to intermediate file: " + filePath);
        return false;
    }

    // logger.log("Mapper: Successfully exported " + std::to_string(mappedData.size()) + " pairs to " + filePath, Logger::Level::DEBUG);
    return true;
}

// Implementation of any other private helper methods for the Mapper class would go here.
// For example:
// std::string Mapper::sanitizeWord(const std::string& rawWord) {
//     // ... implementation ...
// }