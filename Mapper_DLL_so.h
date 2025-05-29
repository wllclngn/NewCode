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
#include "ThreadPool.h"

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
        if (result.empty() || std::all_of(result.begin(), result.end(), ::isdigit)) {
            return "";
        }
        return result;
    }

    // Now using ThreadPool for parallel mapping
    virtual void map_words(const std::vector<std::string> &lines, const std::string &tempFilePath) {
        std::string fullDirPath;
        size_t last_slash_pos = tempFilePath.find_last_of('\\');
        if (last_slash_pos != std::string::npos)
            fullDirPath = tempFilePath.substr(0, last_slash_pos);
        else {
            last_slash_pos = tempFilePath.find_last_of('/');
            fullDirPath = tempFilePath.substr(0, last_slash_pos);
        }

        if (!fs::exists(fullDirPath)) {
            report_error("Temporary folder does not exist: " + tempFilePath);
            return;
        }
        if (!fs::is_directory(fullDirPath)) {
            report_error("Temporary folder is not a directory: " + tempFilePath);
            return;
        }

        std::ofstream temp_out(tempFilePath);
        if (!temp_out) {
            report_error("Could not open " + tempFilePath + " for writing.");
            return;
        }

        std::mutex file_mutex;
        size_t chunkSize = calculate_dynamic_chunk_size(lines.size());
        size_t numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) numThreads = 4; // fallback

        ThreadPool pool(numThreads, numThreads);

        for (size_t i = 0; i < lines.size(); i += chunkSize) {
            pool.enqueueTask([&, i]() {
                size_t startIdx = i;
                size_t endIdx = std::min(startIdx + chunkSize, lines.size());
                std::map<std::string, int> localMap;
                for (size_t j = startIdx; j < endIdx; ++j) {
                    std::istringstream ss(lines[j]);
                    std::string word;
                    while (ss >> word) {
                        std::string cleaned = clean_word(word);
                        if (!cleaned.empty()) localMap[cleaned]++;
                    }
                }
                {
                    std::lock_guard<std::mutex> lock(file_mutex);
                    for (const auto &kv : localMap) {
                        temp_out << kv.first << ": " << kv.second << "\n";
                    }
                }
            });
        }
        pool.shutdown();
        temp_out.close();
    }

protected:
    virtual void report_error(const std::string &error_message) const {
        std::cerr << "Error: " << error_message << std::endl;
    }

    size_t calculate_dynamic_chunk_size(size_t totalSize) const {
        size_t numThreads = std::thread::hardware_concurrency();
        size_t defaultChunkSize = 1024;
        if (numThreads == 0) return defaultChunkSize;
        size_t chunkSize = totalSize / numThreads;
        return chunkSize > defaultChunkSize ? chunkSize : defaultChunkSize;
    }
};

#endif