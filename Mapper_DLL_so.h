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
#include <functional> // For std::hash

#include "ThreadPool.h"    
#include "ErrorHandler.h"  // Ensuring this is included for ErrorHandler::reportError
#include "Logger.h"        

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
    // Default thread pool settings constants
    static constexpr size_t DEFAULT_MAP_MIN_THREADS = 0; 
    static constexpr size_t DEFAULT_MAP_MAX_THREADS = 0; 
    static constexpr size_t FALLBACK_MAP_THREAD_COUNT = 2;

    virtual ~MapperDLLso() {} 

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
        if (result.empty() || std::all_of(result.begin(), result.end(), ::isdigit)) {
            return ""; 
        }
        return result;
    }

    // Unified map_words function
    virtual void map_words(
        const std::vector<std::string>& lines,
        const std::string& outputPathOrDir, 
        bool isPartitionedOutput = false, 
        int mapperId = 0, 
        int numReducersForPartitioning = 1,
        size_t minPoolThreadsConfig = DEFAULT_MAP_MIN_THREADS,
        size_t maxPoolThreadsConfig = DEFAULT_MAP_MAX_THREADS
    ) {
        size_t actualMinThreads = minPoolThreadsConfig;
        if (actualMinThreads == 0) {
            actualMinThreads = std::thread::hardware_concurrency();
            if (actualMinThreads == 0) actualMinThreads = FALLBACK_MAP_THREAD_COUNT;
        }

        size_t actualMaxThreads = maxPoolThreadsConfig;
        if (actualMaxThreads == 0) {
            actualMaxThreads = actualMinThreads; 
        }
        if (actualMaxThreads < actualMinThreads) actualMaxThreads = actualMinThreads;

        std::string modeLogPrefix = isPartitionedOutput ? 
            "MapperDLLso (Partitioned Mode, ID " + std::to_string(mapperId) + ")" : 
            "MapperDLLso (Single File Mode)";
        
        Logger::getInstance().log(modeLogPrefix + ": Starting map_words. Pool: " +
                                  std::to_string(actualMinThreads) + "-" + std::to_string(actualMaxThreads) + " threads. Output: " + outputPathOrDir);

        if (isPartitionedOutput) {
            if (numReducersForPartitioning <= 0) {
                ErrorHandler::reportError(modeLogPrefix + ": Number of reducers for partitioning must be positive.");
                return;
            }
            const std::string& tempDir = outputPathOrDir; 
            if (!fs::exists(tempDir) || !fs::is_directory(tempDir)) {
                ErrorHandler::reportError(modeLogPrefix + ": Temporary directory " + tempDir + " is invalid.");
                return;
            }

            std::vector<std::ofstream> partition_outs(numReducersForPartitioning);
            std::vector<std::mutex> file_mutexes(numReducersForPartitioning);

            for (int p = 0; p < numReducersForPartitioning; ++p) {
                fs::path partition_file_path = fs::path(tempDir) / ("mapper" + std::to_string(mapperId) + "_partition" + std::to_string(p) + ".tmp");
                partition_outs[p].open(partition_file_path.string());
                if (!partition_outs[p]) {
                    ErrorHandler::reportError(modeLogPrefix + ": Could not open partition file " + partition_file_path.string() + " for writing.");
                    for (int k = 0; k < p; ++k) if(partition_outs[k].is_open()) partition_outs[k].close();
                    return;
                }
            }
            
            size_t chunkSize = calculate_dynamic_chunk_size(lines.size(), actualMaxThreads);
            ThreadPool pool(actualMinThreads, actualMaxThreads);

            for (size_t i = 0; i < lines.size(); i += chunkSize) {
                pool.enqueueTask([this, &lines, i, chunkSize, numReducersForPartitioning,
                                  &partition_outs, &file_mutexes, mapperId, modeLogPrefix]() { 
                    size_t startIdx = i;
                    size_t endIdx = std::min(startIdx + chunkSize, lines.size());
                    std::vector<std::map<std::string, int>> task_local_partition_maps(numReducersForPartitioning);

                    for (size_t j = startIdx; j < endIdx; ++j) {
                        std::istringstream ss(lines[j]);
                        std::string word;
                        while (ss >> word) {
                            std::string cleaned = this->clean_word(word); 
                            if (!cleaned.empty()) {
                                std::hash<std::string> hasher;
                                int partition_idx = hasher(cleaned) % numReducersForPartitioning;
                                task_local_partition_maps[partition_idx][cleaned]++;
                            }
                        }
                    }

                    for (int p = 0; p < numReducersForPartitioning; ++p) {
                        if (!task_local_partition_maps[p].empty()) {
                            std::lock_guard<std::mutex> lock(file_mutexes[p]);
                            if (partition_outs[p].is_open()) { 
                                for (const auto& kv : task_local_partition_maps[p]) {
                                    partition_outs[p] << kv.first << ": " << kv.second << "\n";
                                }
                            } else {
                                 // Log this occurrence, but ErrorHandler might be too severe if it exits
                                 Logger::getInstance().log(modeLogPrefix + ": CRITICAL - Found partition stream " + std::to_string(p) + " closed unexpectedly during write operation.");
                            }
                        }
                    }
                });
            }
            pool.shutdown();

            for (int p = 0; p < numReducersForPartitioning; ++p) {
                if (partition_outs[p].is_open()) partition_outs[p].close();
            }
            Logger::getInstance().log(modeLogPrefix + ": Finished processing.");

        } else {
            const std::string& tempFilePath = outputPathOrDir; 
            std::string fullDirPath;
            size_t last_slash_pos = tempFilePath.find_last_of("/\\");
            
            if (last_slash_pos != std::string::npos) {
                fullDirPath = tempFilePath.substr(0, last_slash_pos);
            } else {
                fullDirPath = "."; 
                Logger::getInstance().log(modeLogPrefix + ": No directory path in tempFilePath '" + tempFilePath + "', assuming current directory.");
            }

            if (!fs::exists(fullDirPath) || !fs::is_directory(fullDirPath)) {
                ErrorHandler::reportError(modeLogPrefix + ": Temporary folder '" + fullDirPath + "' derived from '" + tempFilePath + "' does not exist or is not a directory.");
                return;
            }

            std::ofstream temp_out(tempFilePath);
            if (!temp_out) {
                ErrorHandler::reportError(modeLogPrefix + ": Could not open " + tempFilePath + " for writing.");
                return;
            }
            Logger::getInstance().log(modeLogPrefix + ": Opened intermediate file for writing: " + tempFilePath);

            std::mutex file_mutex; 
            size_t chunkSize = calculate_dynamic_chunk_size(lines.size(), actualMaxThreads);
            ThreadPool pool(actualMinThreads, actualMaxThreads); 

            for (size_t i = 0; i < lines.size(); i += chunkSize) {
                pool.enqueueTask([this, &lines, &temp_out, &file_mutex, i, chunkSize]() { 
                    size_t startIdx = i;
                    size_t endIdx = std::min(startIdx + chunkSize, lines.size());
                    std::map<std::string, int> localMap; 

                    for (size_t j = startIdx; j < endIdx; ++j) {
                        std::istringstream ss(lines[j]);
                        std::string word;
                        while (ss >> word) {
                            std::string cleaned = this->clean_word(word); 
                            if (!cleaned.empty()) {
                                localMap[cleaned]++;
                            }
                        }
                    }

                    if (!localMap.empty()) {
                        std::lock_guard<std::mutex> lock(file_mutex);
                        for (const auto &kv : localMap) {
                            temp_out << kv.first << ": " << kv.second << "\n";
                        }
                    }
                });
            }
            pool.shutdown(); 
            temp_out.close();
            Logger::getInstance().log(modeLogPrefix + ": Finished writing to intermediate file: " + tempFilePath);
        }
    }

protected:
    // Removed local report_error, will use ErrorHandler::reportError directly

    size_t calculate_dynamic_chunk_size(size_t totalSize, size_t guideMaxThreads = 0) const {
        size_t numEffectiveThreads = guideMaxThreads > 0 ? guideMaxThreads : std::thread::hardware_concurrency();
        if (numEffectiveThreads == 0) numEffectiveThreads = FALLBACK_MAP_THREAD_COUNT; 
        
        const size_t minChunkSize = 256;  
        const size_t maxChunksPerThreadFactor = 4; 
        const size_t maxTotalChunks = numEffectiveThreads * maxChunksPerThreadFactor;

        if (totalSize == 0) return minChunkSize;

        size_t chunkSize = totalSize / numEffectiveThreads; 

        if (numEffectiveThreads > 0 && totalSize / chunkSize > maxTotalChunks && maxTotalChunks > 0) { 
            chunkSize = totalSize / maxTotalChunks;
        }
        
        return std::max(minChunkSize, chunkSize); 
    }
};

#endif // MAPPER_DLL_SO_H