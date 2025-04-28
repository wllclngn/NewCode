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
#include "utility.h"

class Mapper {
public:
    void map_words(const std::vector<std::string>& lines, const std::string& outputPath) {
        std::ofstream temp_out(outputPath);
        if (!temp_out) {
            ErrorHandler::reportError("Could not open " + outputPath + " for writing.");
            return;
        }

        std::mutex mutex;
        size_t chunkSize = calculate_dynamic_chunk_size(lines.size());
        size_t numThreads = std::thread::hardware_concurrency();
        std::vector<std::thread> threads;

        for (size_t i = 0; i < numThreads; ++i) {
            threads.emplace_back([this, &lines, &temp_out, &mutex, i, chunkSize]() {
                size_t startIdx = i * chunkSize;
                size_t endIdx = std::min(startIdx + chunkSize, lines.size());
                std::map<std::string, int> localMap;

                for (size_t j = startIdx; j < endIdx; ++j) {
                    std::istringstream ss(lines[j]);
                    std::string word;
                    while (ss >> word) {
                        localMap[Utility::clean_word(word)]++;
                    }
                }

                std::lock_guard<std::mutex> lock(mutex);
                for (const auto& kv : localMap) {
                    temp_out << kv.first << ": " << kv.second << "\n";
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

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
};