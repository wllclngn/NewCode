#pragma once

#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <fstream>
#include <mutex>
#include <thread>
#include <iostream>
#include <cstdlib>
#include <queue>
#include <condition_variable>
#include <functional>
#include "ERROR_Handler.h"
#include "FileHandler.h"
#include "Mapper_DLL_so.h"
#include "ThreadPool.h"

class Mapper {
public:
    Mapper(size_t minThreads = 2, size_t maxThreads = 8)
        : threadPool(std::make_unique<ThreadPool>(minThreads, maxThreads)) {}

    void map_words(const std::vector<std::string>& lines, const std::string& tempFolderPath) {
        // Ensure the temporary folder path is valid
        if (!fs::exists(tempFolderPath)) {
            ErrorHandler::reportError("Temporary folder does not exist: " + tempFolderPath);
            return;
        }
        if (!fs::is_directory(tempFolderPath)) {
            ErrorHandler::reportError("Temporary folder is not a directory: " + tempFolderPath);
            return;
        }

        // Construct the output path for mapped_temp.txt
        std::string outputPath = tempFolderPath + "/mapped_temp.txt";

        // Open the output file for writing
        std::ofstream temp_out(outputPath);
        if (!temp_out) {
            ErrorHandler::reportError("Could not open " + outputPath + " for writing.");
            return;
        }

        // Mutex for synchronizing file writes
        std::mutex file_mutex;

        // Calculate the optimal chunk size for processing
        size_t chunkSize = calculate_dynamic_chunk_size(lines.size());

        // Process lines in parallel using the thread pool
        for (size_t i = 0; i < lines.size(); i += chunkSize) {
            threadPool->enqueueTask([this, &lines, &temp_out, &file_mutex, i, chunkSize]() {
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
            });
        }

        // Wait for all threads to finish
        threadPool->shutdown();

        // Close the output file
        temp_out.close();
    }

private:
    size_t calculate_dynamic_chunk_size(size_t totalSize) {
        size_t numThreads = std::thread::hardware_concurrency();
        size_t defaultChunkSize = 1024;

        if (numThreads == 0) {
            return defaultChunkSize;
        }

        size_t chunkSize = totalSize / numThreads;
        return chunkSize > defaultChunkSize ? chunkSize : defaultChunkSize;
    }

    std::unique_ptr<ThreadPoolBase> threadPool;
};
