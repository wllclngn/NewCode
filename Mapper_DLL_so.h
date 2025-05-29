#ifndef MAPPER_DLL_SO_H
#define MAPPER_DLL_SO_H

#include <string>
#include <vector>
#include <utility>
#include <fstream>
#include <sstream>
#include <cctype>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>
#include <algorithm>
#include <filesystem>
#include <functional> // Required for std::hash
#include "ThreadPool.h" // Assuming ThreadPool.h is accessible
#include "ErrorHandler.h" // Include ErrorHandler for centralized error reporting
#include "Logger.h"     // Assuming Logger.h is accessible for potential logging


// Export macro for cross-platform compatibility
#if defined(_WIN32) || defined(_WIN64)
#define DLL_so_EXPORT __declspec(dllexport)
#elif defined(__linux__) || defined(__unix__)
#define DLL_so_EXPORT __attribute__((visibility("default")))
#else
#define DLL_so_EXPORT
#endif

namespace fs = std::filesystem;

class DLL_so_EXPORT MapperDLLso {
public:
    virtual ~MapperDLLso() {} // Virtual destructor for polymorphism

    // Virtual function for character validation
    virtual bool is_valid_char(char c) const {
        return std::isalnum(static_cast<unsigned char>(c));
    }

    virtual std::string clean_word(const std::string &word) const {
        std::string result;
        for (char c : word) {
            if (is_valid_char(c)) {
                result += std::tolower(static_cast<unsigned char>(c));
            }
        }
        // Filter out words that are purely numeric or become empty after cleaning
        if (result.empty() || std::all_of(result.begin(), result.end(), ::isdigit)) {
            return "";
        }
        return result;
    }

    // Updated map_words for partitioning
    virtual void map_words(
        const std::vector<std::string> &lines, 
        const std::string &tempDirectoryPath, 
        int mapperId, 
        int numReducers) 
    {
        if (numReducers <= 0) {
            ErrorHandler::reportError("MapperDLLso: Number of reducers must be positive.", true); // Critical error
            return;
        }

        if (!fs::exists(tempDirectoryPath)) {
            // ErrorHandler::reportError("MapperDLLso: Temporary directory does not exist: " + tempDirectoryPath);
            try {
                if (fs::create_directories(tempDirectoryPath)) {
                    Logger::getInstance().log("MapperDLLso: Created temporary directory: " + tempDirectoryPath);
                } else {
                    // This case might indicate a permissions issue or an invalid path segment.
                    ErrorHandler::reportError("MapperDLLso: Failed to create temporary directory: " + tempDirectoryPath + ". Check permissions and path validity.", true);
                    return;
                }
            } catch (const fs::filesystem_error& e) {
                ErrorHandler::reportError("MapperDLLso: Filesystem error creating temporary directory " + tempDirectoryPath + ": " + e.what(), true);
                return;
            }
        }
        if (!fs::is_directory(tempDirectoryPath)) {
            ErrorHandler::reportError("MapperDLLso: Temporary path is not a directory: " + tempDirectoryPath, true);
            return;
        }

        std::vector<std::ofstream> partition_files(numReducers);
        for (int i = 0; i < numReducers; ++i) {
            std::string partition_file_path = tempDirectoryPath + "/mapper" + std::to_string(mapperId) + "_partition" + std::to_string(i) + ".tmp";
            partition_files[i].open(partition_file_path, std::ios::out | std::ios::app);
            if (!partition_files[i]) {
                ErrorHandler::reportError("MapperDLLso: Could not open partition file " + partition_file_path + " for writing.", true);
                // Close already opened files before returning
                for(int j=0; j<i; ++j) if(partition_files[j].is_open()) partition_files[j].close();
                return;
            }
        }

        std::mutex file_mutex; 
        size_t chunkSize = calculate_dynamic_chunk_size(lines.size());
        size_t numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) numThreads = 4; 

        ThreadPool pool(numThreads, numThreads); 

        for (size_t i = 0; i < lines.size(); i += chunkSize) {
            pool.enqueueTask([&, i, numReducers, &partition_files, &file_mutex, this]() { 
                size_t startIdx = i;
                size_t endIdx = std::min(startIdx + chunkSize, lines.size());
                std::map<std::string, int> localMap; 

                for (size_t j = startIdx; j < endIdx; ++j) {
                    std::istringstream ss(lines[j]);
                    std::string word;
                    while (ss >> word) {
                        std::string cleaned = clean_word(word); 
                        if (!cleaned.empty()) {
                            localMap[cleaned]++;
                        }
                    }
                }
                
                std::lock_guard<std::mutex> lock(file_mutex);
                std::hash<std::string> str_hash; 

                for (const auto &kv : localMap) {
                    if (numReducers > 0) { 
                        int partition_idx = str_hash(kv.first) % numReducers;
                        if (partition_idx < 0) partition_idx += numReducers; 

                        if (partition_idx >=0 && partition_idx < numReducers && partition_files[partition_idx].is_open()) {
                            partition_files[partition_idx] << kv.first << ": " << kv.second << "\n";
                        } else {
                            // This is a critical internal error if reached, as files should be open and index valid.
                            std::string errMsg = "MapperDLLso: CRITICAL - Partition file for index " + std::to_string(partition_idx) + 
                                                 " (key: " + kv.first + ") is not open or index is out of bounds during write. NumReducers: " + std::to_string(numReducers);
                            ErrorHandler::reportError(errMsg, true); // Make this critical as it indicates a logic flaw or severe issue.
                            // Consider how to handle this - perhaps stop all processing. For now, it exits.
                        }
                    }
                }
            });
        }
        
        pool.shutdown(); 

        for (int i = 0; i < numReducers; ++i) {
            if (partition_files[i].is_open()) {
                partition_files[i].close();
            }
        }
    }

protected:
    // report_error method is now removed from MapperDLLso

    size_t calculate_dynamic_chunk_size(size_t totalSize) const {
        size_t numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) numThreads = 1; 
        size_t defaultChunkSize = 1024; 
        
        if (totalSize == 0) return defaultChunkSize;

        size_t chunkSize = totalSize / numThreads;
        if (totalSize % numThreads != 0) { 
             chunkSize++;
        }
        return std::max(defaultChunkSize, chunkSize); 
    }
};

#endif // MAPPER_DLL_SO_H