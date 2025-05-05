#pragma once

#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <fstream>
#include <mutex>
#include <thread>
#include <future>
#include <iostream>
#include <cstdlib>
#include <functional>
#include "ERROR_Handler.h"
#include "FileHandler.h"
#include "Mapper_DLL_so.h"

class Mapper {
public:
    Mapper() {}

    void map_words(const std::vector<std::string>& lines, const std::string& tempFilePath) {

        // Build out tempFilePath from user input for blank, "", user input directories.
        std::string fullDirPath;
        size_t last_slash_pos = tempFilePath.find_last_of('\\');

        // Check if a backslash was found
        if (last_slash_pos != std::string::npos) {
            // Dynamically build out tempFilePath for blank outPutFolder and tempFolder
            fullDirPath = tempFilePath.substr(0, last_slash_pos);
            std::cout << "MAPPER: " << fullDirPath<< std::endl;
            std::cout << "MAPPER FILE: " << tempFilePath<< std::endl;
        } else {
            last_slash_pos = tempFilePath.find_last_of('/');
            // Dynamically build out tempFilePath for blank outPutFolder and tempFolder
            fullDirPath = tempFilePath.substr(0, last_slash_pos);
            std::cout << "MAPPER: " << fullDirPath<< std::endl;
            std::cout << "MAPPER FILE: " << tempFilePath<< std::endl;
        }
        
        // Ensure the temporary folder path is valid
        if (!fs::exists(fullDirPath)) {
            ErrorHandler::reportError("Temporary folder does not exist: " + tempFilePath);
            return;
        }
        if (!fs::is_directory(fullDirPath)) {
            ErrorHandler::reportError("Temporary folder is not a directory: " + tempFilePath);
            return;
        }

        // Open the output file for writing
        std::ofstream temp_out(tempFilePath);
        if (!temp_out) {
            ErrorHandler::reportError("Could not open " + tempFilePath + " for writing.");
            return;
        }
      
        // Mutex for synchronizing file writes
        std::mutex file_mutex;

        // Calculate the optimal chunk size for processing
        size_t chunkSize = calculate_dynamic_chunk_size(lines.size());

        // Launch threads for parallel processing
        std::vector<std::future<void>> futures;
        for (size_t i = 0; i < lines.size(); i += chunkSize) {
            futures.push_back(std::async(std::launch::async, [this, &lines, &temp_out, &file_mutex, i, chunkSize]() {
                size_t startIdx = i;
                size_t endIdx = std::min(startIdx + chunkSize, lines.size());
                std::map<std::string, int> localMap;

                // Perform local mapping
                for (size_t j = startIdx; j < endIdx; ++j) {
                    std::istringstream ss(lines[j]);
                    std::string word;
                    while (ss >> word) {
                        MapperDLLso mapperDLL;
                        localMap[mapperDLL.clean_word(word)]++;
                    }
                }

                // Write the local map to the output file with synchronization
                {
                    std::lock_guard<std::mutex> lock(file_mutex);
                    for (const auto& kv : localMap) {
                        temp_out << kv.first << ": " << kv.second << "\n";
                    }
                }
            }));
        }

        // Wait for all threads to finish
        for (auto& future : futures) {
            future.get();
        }

        // Close the output file
        temp_out.close();
    }

private:
    size_t calculate_dynamic_chunk_size(size_t totalSize) {
        size_t numThreads = std::thread::hardware_concurrency();
        size_t defaultChunkSize = 500;

        if (numThreads == 0) {
            return defaultChunkSize;
        }

        size_t chunkSize = totalSize / numThreads;
        return chunkSize > defaultChunkSize ? chunkSize : defaultChunkSize;
    }
};