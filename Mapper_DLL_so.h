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
#include "ErrorHandler.h"  
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

    // Original map_words for interactive mode (2 arguments)
    virtual void map_words(const std::vector<std::string> &lines, const std::string &tempFilePath) {
        std::string fullDirPath;
        size_t last_slash_pos_win = tempFilePath.find_last_of('\\');
        size_t last_slash_pos_unix = tempFilePath.find_last_of('/');
        size_t last_slash_pos = std::string::npos;

        if (last_slash_pos_win != std::string::npos && last_slash_pos_unix != std::string::npos) {
            last_slash_pos = std::max(last_slash_pos_win, last_slash_pos_unix);
        } else if (last_slash_pos_win != std::string::npos) {
            last_slash_pos = last_slash_pos_win;
        } else {
            last_slash_pos = last_slash_pos_unix;
        }
        
        if (last_slash_pos != std::string::npos) {
            fullDirPath = tempFilePath.substr(0, last_slash_pos);
        } else {
            fullDirPath = "."; 
            Logger::getInstance().log("MAPPER_DLL_SO (2-arg): No directory path in tempFilePath '" + tempFilePath + "', assuming current directory.");
        }

        if (!fs::exists(fullDirPath)) {
            report_error("MAPPER_DLL_SO (2-arg): Temporary folder does not exist: " + fullDirPath);
            return;
        }
        if (!fs::is_directory(fullDirPath)) {
            report_error("MAPPER_DLL_SO (2-arg): Path for temporary file is not a directory: " + fullDirPath);
            return;
        }

        std::ofstream temp_out(tempFilePath);
        if (!temp_out) {
            report_error("MAPPER_DLL_SO (2-arg): Could not open " + tempFilePath + " for writing.");
            return;
        }
        Logger::getInstance().log("MAPPER_DLL_SO (2-arg): Opened intermediate file for writing: " + tempFilePath);

        std::mutex file_mutex; 
        size_t chunkSize = calculate_dynamic_chunk_size(lines.size());
        
        // For interactive mode, use default hardware concurrency for thread pool
        size_t defaultMinThreads = std::thread::hardware_concurrency();
        if (defaultMinThreads == 0) defaultMinThreads = 2; // Fallback
        size_t defaultMaxThreads = defaultMinThreads; // Keep it simple for interactive: min=max

        ThreadPool pool(defaultMinThreads, defaultMaxThreads); 

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
        Logger::getInstance().log("MAPPER_DLL_SO (2-arg): Finished writing to intermediate file: " + tempFilePath);
    }

    // Updated map_words for command-line mapper mode (6 arguments)
    virtual void map_words(const std::vector<std::string>& lines, 
                           const std::string& tempDir, 
                           int mapperId, 
                           int numReducers,
                           size_t minPoolThreads, // New parameter
                           size_t maxPoolThreads) // New parameter
                           {
        if (numReducers <= 0) {
            report_error("MAPPER_DLL_SO (6-arg): Number of reducers must be positive.");
            return;
        }
        if (!fs::exists(tempDir) || !fs::is_directory(tempDir)) {
            report_error("MAPPER_DLL_SO (6-arg): Temporary directory " + tempDir + " is invalid.");
            return;
        }
        // Validate thread pool sizes
        if (minPoolThreads == 0) minPoolThreads = 1; // Ensure at least one thread
        if (maxPoolThreads < minPoolThreads) maxPoolThreads = minPoolThreads;

        Logger::getInstance().log("MAPPER_DLL_SO (6-arg): Mapper " + std::to_string(mapperId) + " starting. Output to " + tempDir + 
                                  " for " + std::to_string(numReducers) + " reducers. Pool: " + 
                                  std::to_string(minPoolThreads) + "-" + std::to_string(maxPoolThreads) + " threads.");

        std::vector<std::ofstream> partition_outs(numReducers);
        std::vector<std::mutex> file_mutexes(numReducers);

        for (int p = 0; p < numReducers; ++p) {
            fs::path partition_file_path = fs::path(tempDir) / ("mapper" + std::to_string(mapperId) + "_partition" + std::to_string(p) + ".tmp");
            partition_outs[p].open(partition_file_path.string());
            if (!partition_outs[p]) {
                report_error("MAPPER_DLL_SO (6-arg): Could not open partition file " + partition_file_path.string() + " for writing.");
                for (int k = 0; k < p; ++k) if(partition_outs[k].is_open()) partition_outs[k].close();
                return;
            }
        }

        size_t chunkSize = calculate_dynamic_chunk_size(lines.size(), maxPoolThreads); // Pass maxPoolThreads to chunk calculation

        ThreadPool pool(minPoolThreads, maxPoolThreads); // Use provided thread counts

        for (size_t i = 0; i < lines.size(); i += chunkSize) {
            pool.enqueueTask([this, &lines, i, chunkSize, numReducers, 
                              &partition_outs, &file_mutexes, mapperId]() { 
                size_t startIdx = i;
                size_t endIdx = std::min(startIdx + chunkSize, lines.size());
                std::vector<std::map<std::string, int>> task_local_partition_maps(numReducers);

                for (size_t j = startIdx; j < endIdx; ++j) {
                    std::istringstream ss(lines[j]);
                    std::string word;
                    while (ss >> word) {
                        std::string cleaned = this->clean_word(word); 
                        if (!cleaned.empty()) {
                            std::hash<std::string> hasher;
                            int partition_idx = hasher(cleaned) % numReducers;
                            task_local_partition_maps[partition_idx][cleaned]++;
                        }
                    }
                }

                for (int p = 0; p < numReducers; ++p) {
                    if (!task_local_partition_maps[p].empty()) {
                        std::lock_guard<std::mutex> lock(file_mutexes[p]);
                        if (partition_outs[p].is_open()) { // Check if stream is still open
                            for (const auto& kv : task_local_partition_maps[p]) {
                                partition_outs[p] << kv.first << ": " << kv.second << "\n";
                            }
                        } else {
                             Logger::getInstance().log("MAPPER_DLL_SO (6-arg): Error - Mapper " + std::to_string(mapperId) + " found partition stream " + std::to_string(p) + " closed unexpectedly.");
                        }
                    }
                }
            });
        }
        pool.shutdown();

        for (int p = 0; p < numReducers; ++p) {
            if (partition_outs[p].is_open()) {
                partition_outs[p].close();
            }
        }
        Logger::getInstance().log("MAPPER_DLL_SO (6-arg): Mapper " + std::to_string(mapperId) + " finished processing.");
    }

protected:
    virtual void report_error(const std::string &error_message) const {
        Logger::getInstance().log("ERROR (MapperDLLso): " + error_message);
    }

    // Updated calculate_dynamic_chunk_size to potentially use maxThreads for guidance
    size_t calculate_dynamic_chunk_size(size_t totalSize, size_t guideMaxThreads = 0) const {
        size_t numEffectiveThreads = guideMaxThreads > 0 ? guideMaxThreads : std::thread::hardware_concurrency();
        if (numEffectiveThreads == 0) numEffectiveThreads = 4; 
        
        const size_t minChunkSize = 256;  
        const size_t maxChunksPerThreadFactor = 4; // Aim for up to this many chunks per effective thread
        const size_t maxTotalChunks = numEffectiveThreads * maxChunksPerThreadFactor;

        if (totalSize == 0) return minChunkSize; // Or some other default for empty input

        size_t chunkSize = totalSize / numEffectiveThreads; // Initial ideal distribution

        if (numEffectiveThreads > 0 && totalSize / chunkSize > maxTotalChunks && maxTotalChunks > 0) { 
            chunkSize = totalSize / maxTotalChunks;
        }
        
        return std::max(minChunkSize, chunkSize); 
    }
};

#endif // MAPPER_DLL_SO_H